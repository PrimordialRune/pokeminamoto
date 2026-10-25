#include "global.h"
#include "dma3.h"
#include "regress.h"

// tools/regress measured at most 9 queued requests (battles); 64 leaves wide headroom
#define MAX_DMA_REQUESTS 64

#define DMA_REQUEST_COPY32 1
#define DMA_REQUEST_FILL32 2
#define DMA_REQUEST_COPY16 3
#define DMA_REQUEST_FILL16 4

struct Dma3Request
{
    const u8 *src;
    u8 *dest;
    u16 size;
    u16 mode;
    u32 value;
};

static struct Dma3Request sDma3Requests[MAX_DMA_REQUESTS];

static vbool8 sDma3ManagerLocked;
static u8 sDma3RequestCursor;

void ClearDma3Requests(void)
{
    int i;

    sDma3ManagerLocked = TRUE;
    sDma3RequestCursor = 0;

    for (i = 0; i < MAX_DMA_REQUESTS; i++)
    {
        sDma3Requests[i].size = 0;
        sDma3Requests[i].src = NULL;
        sDma3Requests[i].dest = NULL;
    }

    sDma3ManagerLocked = FALSE;
}

// VBlank time budget. The queue is processed from the VBlank interrupt (after the
// vblank callback, before the sound mixer), so every request has to be paid for out of
// the lines that are left before the picture starts again. A request is never dropped:
// one that doesn't fit is left at the head of the queue (order is kept), and a request
// that is bigger than what is left is shrunk in place and finished next frame. Its slot
// stays busy until the last chunk is copied, so CheckForSpaceForDma3Request and
// IsDma3ManagerBusyWithBgCopy keep meaning "everything has landed".
#define DMA_VBLANK_LAST_LINE    226     // VBlank is lines 160-227; stop early to leave a line of slack
#define DMA_CYCLES_PER_LINE     1232    // 960 draw + 272 hblank
#define DMA_REQUEST_OVERHEAD    80      // register setup and the loop around one request (cycles)
// Cycles per 16-bit unit, kept above the real figures (ROM at 3/1 waitstates to VRAM is about 3,
// EWRAM to EWRAM about 6 per word) because running late costs a tearing frame and the
// only price of being conservative is a frame of extra latency on a big upload.
#define DMA_CYCLES_PER_HALFWORD_COPY 4
#define DMA_CYCLES_PER_HALFWORD_FILL 3
#define DMA_MIN_CHUNK           128     // never start a piece smaller than this unless it finishes the request

static void DoDma3Request(struct Dma3Request *req, u32 size)
{
    switch (req->mode)
    {
    case DMA_REQUEST_COPY32: // regular 32-bit copy
        Dma3CopyLarge32_(req->src, req->dest, size);
        break;
    case DMA_REQUEST_FILL32: // repeat a single 32-bit value across RAM
        Dma3FillLarge32_(req->value, req->dest, size);
        break;
    case DMA_REQUEST_COPY16: // regular 16-bit copy
        Dma3CopyLarge16_(req->src, req->dest, size);
        break;
    case DMA_REQUEST_FILL16: // repeat a single 16-bit value across RAM
        Dma3FillLarge16_(req->value, req->dest, size);
        break;
    }
}

void ProcessDma3Requests(void)
{
    u32 budget;
    u32 line;

    if (sDma3ManagerLocked)
        return;

#ifdef REGRESS_HOOKS
    {
        u32 pending = 0, cursor = sDma3RequestCursor;
        while (pending < MAX_DMA_REQUESTS && sDma3Requests[cursor].size != 0)
        {
            pending++;
            cursor = (cursor + 1) % MAX_DMA_REQUESTS;
        }
        REGRESS_NOTE_DMA_QUEUE(pending);
    }
#endif

    // The interrupt can only be late into the next frame's picture, never early, so a line
    // below 160 means "no time at all": try again next VBlank.
    line = *(vu8 *)REG_ADDR_VCOUNT;
    if (line < DISPLAY_HEIGHT || line >= DMA_VBLANK_LAST_LINE)
        return;
    budget = (DMA_VBLANK_LAST_LINE - line) * DMA_CYCLES_PER_LINE;

    // as long as there are DMA requests to process (unless the time budget is used up), do not exit
    while (sDma3Requests[sDma3RequestCursor].size != 0)
    {
        struct Dma3Request *req = &sDma3Requests[sDma3RequestCursor];
        u32 size = req->size;
        u32 perUnit = (req->mode == DMA_REQUEST_COPY32 || req->mode == DMA_REQUEST_COPY16)
                    ? DMA_CYCLES_PER_HALFWORD_COPY : DMA_CYCLES_PER_HALFWORD_FILL;
        u32 cost = DMA_REQUEST_OVERHEAD + (size / 2) * perUnit;

        if (cost > budget)
        {
            u32 chunk;

            if (budget <= DMA_REQUEST_OVERHEAD)
                return;
            // largest piece that fits, a multiple of 4 bytes so 32-bit requests stay aligned
            chunk = ((budget - DMA_REQUEST_OVERHEAD) / perUnit) * 2 & ~3;
            if (chunk < DMA_MIN_CHUNK)
                return; // not worth starting; the head of the queue waits for next frame
            DoDma3Request(req, chunk);
            if (req->mode == DMA_REQUEST_COPY32 || req->mode == DMA_REQUEST_COPY16)
                req->src += chunk;
            req->dest += chunk;
            req->size -= chunk;
            return; // the budget is spent
        }

        budget -= cost;
        DoDma3Request(req, size);

        // Free the request
        req->src = NULL;
        req->dest = NULL;
        req->size = 0;
        req->mode = 0;
        req->value = 0;
        sDma3RequestCursor++;

        if (sDma3RequestCursor >= MAX_DMA_REQUESTS) // loop back to the first DMA request
            sDma3RequestCursor = 0;
    }
}

s16 RequestDma3Copy(const void *src, void *dest, u16 size, u8 mode)
{
    int cursor;
    int i = 0;

    sDma3ManagerLocked = TRUE;
    cursor = sDma3RequestCursor;

    while (i < MAX_DMA_REQUESTS)
    {
        if (sDma3Requests[cursor].size == 0) // an empty request was found.
        {
            sDma3Requests[cursor].src = src;
            sDma3Requests[cursor].dest = dest;
            sDma3Requests[cursor].size = size;

            if (mode == 1)
                sDma3Requests[cursor].mode = DMA_REQUEST_COPY32;
            else
                sDma3Requests[cursor].mode = DMA_REQUEST_COPY16;

            sDma3ManagerLocked = FALSE;
            return cursor;
        }
        if (++cursor >= MAX_DMA_REQUESTS) // loop back to start.
            cursor = 0;
        i++;
    }
    sDma3ManagerLocked = FALSE;
    REGRESS_NOTE_DMA_FULL();
    return -1;  // no free DMA request was found
}

s16 RequestDma3Fill(s32 value, void *dest, u16 size, u8 mode)
{
    int cursor;
    int i = 0;

    cursor = sDma3RequestCursor;
    sDma3ManagerLocked = TRUE;

    while (i < MAX_DMA_REQUESTS)
    {
        if (sDma3Requests[cursor].size == 0) // an empty request was found.
        {
            sDma3Requests[cursor].dest = dest;
            sDma3Requests[cursor].size = size;
            sDma3Requests[cursor].mode = mode;
            sDma3Requests[cursor].value = value;

            if(mode == 1)
                sDma3Requests[cursor].mode = DMA_REQUEST_FILL32;
            else
                sDma3Requests[cursor].mode = DMA_REQUEST_FILL16;

            sDma3ManagerLocked = FALSE;
            return cursor;
        }
        if (++cursor >= MAX_DMA_REQUESTS) // loop back to start.
            cursor = 0;
        i++;
    }
    sDma3ManagerLocked = FALSE;
    REGRESS_NOTE_DMA_FULL();
    return -1;  // no free DMA request was found
}

s16 CheckForSpaceForDma3Request(s16 index)
{
    int i = 0;

    if (index == -1)  // check if all requests are free
    {
        while (i < MAX_DMA_REQUESTS)
        {
            if (sDma3Requests[i].size != 0)
                return -1;
            i++;
        }
        return 0;
    }
    else  // check the specified request
    {
        if (sDma3Requests[index].size != 0)
            return -1;
        return 0;
    }
}
