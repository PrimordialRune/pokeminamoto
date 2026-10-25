#include "global.h"
#include "malloc.h"
#include "regress.h"
#include "constants/rgb.h"

// Debug and regress builds stop with a red screen when the heap runs out or
// Free gets a pointer the heap never handed out (see HeapFatal). Release
// builds keep Emerald's behaviour: Alloc returns NULL and Free trusts its
// argument, and gHeapStats records how close the heap came to full.
#if !defined(RELEASE) || defined(REGRESS_HOOKS)
#define HEAP_CHECKS
#endif

static void *sHeapStart;
static u32 sHeapSize;

ALIGNED(4) EWRAM_DATA u8 gHeap[HEAP_SIZE] = {0};
EWRAM_DATA struct HeapStats gHeapStats = {0};

#define MALLOC_SYSTEM_ID 0xA3A3

struct MemBlock {
    // Whether this block is currently allocated.
    bool16 flag;

    // Magic number used for error checking. Should equal MALLOC_SYSTEM_ID.
    u16 magic;

    // Size of the block (not including this header struct).
    u32 size;

    // Previous block pointer. Equals sHeapStart if this is the first block.
    struct MemBlock *prev;

    // Next block pointer. Equals sHeapStart if this is the last block.
    struct MemBlock *next;

    // Data in the memory block. (Arrays of length 0 are a GNU extension.)
    u8 data[0];
};

void PutMemBlockHeader(void *block, struct MemBlock *prev, struct MemBlock *next, u32 size)
{
    struct MemBlock *header = (struct MemBlock *)block;

    header->flag = FALSE;
    header->magic = MALLOC_SYSTEM_ID;
    header->size = size;
    header->prev = prev;
    header->next = next;
}

void PutFirstMemBlockHeader(void *block, u32 size)
{
    PutMemBlockHeader(block, (struct MemBlock *)block, (struct MemBlock *)block, size - sizeof(struct MemBlock));
}

void *AllocInternal(void *heapStart, u32 size)
{
    struct MemBlock *pos = (struct MemBlock *)heapStart;
    struct MemBlock *head = pos;
    struct MemBlock *splitBlock;
    u32 foundBlockSize;

    // Alignment
    if (size & 3)
        size = 4 * ((size / 4) + 1);

    for (;;)
    {
        // Loop through the blocks looking for unused block that's big enough.

        if (!pos->flag)
        {
            foundBlockSize = pos->size;

            if (foundBlockSize >= size)
            {
                if (foundBlockSize - size < 2 * sizeof(struct MemBlock))
                {
                    // The block isn't much bigger than the requested size,
                    // so just use it.
                    pos->flag = TRUE;
                }
                else
                {
                    // The block is significantly bigger than the requested
                    // size, so split the rest into a separate block.
                    foundBlockSize -= sizeof(struct MemBlock);
                    foundBlockSize -= size;

                    splitBlock = (struct MemBlock *)(pos->data + size);

                    pos->flag = TRUE;
                    pos->size = size;

                    PutMemBlockHeader(splitBlock, pos, pos->next, foundBlockSize);

                    pos->next = splitBlock;

                    if (splitBlock->next != head)
                        splitBlock->next->prev = splitBlock;
                }

                if ((u8 *)pos->data + size - (u8 *)heapStart > gHeapStats.peak)
                    gHeapStats.peak = (u8 *)pos->data + size - (u8 *)heapStart;
                REGRESS_NOTE_ALLOC(heapStart, pos->data, size);
                return pos->data;
            }
        }

        if (pos->next == head)
        {
            gHeapStats.failures++;
            REGRESS_NOTE_ALLOC(heapStart, NULL, size);
            return NULL;
        }

        pos = pos->next;
    }
}

void FreeInternal(void *heapStart, void *pointer)
{
    if (pointer)
    {
        struct MemBlock *head = (struct MemBlock *)heapStart;
        struct MemBlock *block = (struct MemBlock *)((u8 *)pointer - sizeof(struct MemBlock));
        block->flag = FALSE;

        // If the freed block isn't the last one, merge with the next block
        // if it's not in use.
        if (block->next != head)
        {
            if (!block->next->flag)
            {
                block->size += sizeof(struct MemBlock) + block->next->size;
                block->next->magic = 0;
                block->next = block->next->next;
                if (block->next != head)
                    block->next->prev = block;
            }
        }

        // If the freed block isn't the first one, merge with the previous block
        // if it's not in use.
        if (block != head)
        {
            if (!block->prev->flag)
            {
                block->prev->next = block->next;

                if (block->next != head)
                    block->next->prev = block->prev;

                block->magic = 0;
                block->prev->size += sizeof(struct MemBlock) + block->size;
            }
        }
    }
}

void *AllocZeroedInternal(void *heapStart, u32 size)
{
    void *mem = AllocInternal(heapStart, size);

    if (mem != NULL)
    {
        if (size & 3)
            size = 4 * ((size / 4) + 1);

        CpuFill32(0, mem, size);
    }

    return mem;
}

bool32 CheckMemBlockInternal(void *heapStart, void *pointer)
{
    struct MemBlock *head = (struct MemBlock *)heapStart;
    struct MemBlock *block = (struct MemBlock *)((u8 *)pointer - sizeof(struct MemBlock));

    if (block->magic != MALLOC_SYSTEM_ID)
        return FALSE;

    if (block->next->magic != MALLOC_SYSTEM_ID)
        return FALSE;

    if (block->next != head && block->next->prev != block)
        return FALSE;

    if (block->prev->magic != MALLOC_SYSTEM_ID)
        return FALSE;

    if (block->prev != head && block->prev->next != block)
        return FALSE;

    if (block->next != head && block->next != (struct MemBlock *)(block->data + block->size))
        return FALSE;

    return TRUE;
}

void InitHeap(void *heapStart, u32 heapSize)
{
    sHeapStart = heapStart;
    sHeapSize = heapSize;
    PutFirstMemBlockHeader(heapStart, heapSize);
}

#ifdef HEAP_CHECKS

// A bitmap screen that needs nothing but VRAM, so it works with the heap,
// the stack of tasks and the palettes all in any state. 3x5 pixel glyphs,
// drawn 3x, rows top to bottom, 3 bits per row with the left pixel highest.
#define FATAL_SCALE 3
#define FATAL_COLOR_BG RGB(20, 0, 0)
#define FATAL_COLOR_TEXT RGB(31, 31, 31)

static const u16 sFatalGlyphs[36] =
{
    075557, 026227, 071747, 071717, 055711, 074717, 074757, 071122, 075757, 075717, // 0-9
    025755, 065656, 034443, 065556, 074647, 074644, 034553, 055755, 072227, 011152, // A-J
    055655, 044447, 057755, 065555, 025552, 065644, 025563, 065655, 034216, 072222, // K-T
    055557, 055552, 055775, 055255, 055222, 071247,                                 // U-Z
};

static void FatalDrawChar(u32 x, u32 y, char c)
{
    u16 *vram = (u16 *)VRAM;
    u32 glyph, row, col, i, j;

    if (c >= '0' && c <= '9')
        glyph = sFatalGlyphs[c - '0'];
    else if (c >= 'A' && c <= 'Z')
        glyph = sFatalGlyphs[c - 'A' + 10];
    else
        return;

    for (row = 0; row < 5; row++)
    {
        for (col = 0; col < 3; col++)
        {
            if (!(glyph & (1 << (14 - row * 3 - col))))
                continue;
            for (i = 0; i < FATAL_SCALE; i++)
                for (j = 0; j < FATAL_SCALE; j++)
                    vram[(y + row * FATAL_SCALE + i) * DISPLAY_WIDTH + x + col * FATAL_SCALE + j] = FATAL_COLOR_TEXT;
        }
    }
}

static void FatalDrawLine(u32 line, const char *text, u32 value, bool32 showValue)
{
    u32 x = 12;
    u32 y = 12 + line * 24;
    s32 i;

    for (; *text; text++, x += 4 * FATAL_SCALE)
        FatalDrawChar(x, y, *text);
    if (!showValue)
        return;
    for (i = 28; i >= 0; i -= 4, x += 4 * FATAL_SCALE)
        FatalDrawChar(x, y, "0123456789ABCDEF"[(value >> i) & 0xF]);
}

static const char *const sHeapFatalTitles[] =
{
    [HEAP_FATAL_ALLOC]       = "HEAP FULL",
    [HEAP_FATAL_BAD_FREE]    = "FREE BAD POINTER",
    [HEAP_FATAL_DOUBLE_FREE] = "FREE TWICE",
};

// Stops the game on a heap error: a red screen with what happened, the size
// or pointer, and the address of the call (look it up with addr2line or the
// .map). Never returns.
static void __attribute__((noreturn)) HeapFatal(u32 kind, u32 value, u32 caller)
{
    u32 i;

    REG_IME = 0;
    REG_DMA0CNT_H = 0;
    REG_DMA1CNT_H = 0;
    REG_DMA2CNT_H = 0;
    REG_DMA3CNT_H = 0;
    REG_SOUNDCNT_X = 0;

    REG_DISPCNT = DISPCNT_MODE_3 | DISPCNT_BG2_ON;
    REG_BLDCNT = 0;
    REG_MOSAIC = 0;
    REG_BG2CNT = 0;
    REG_BG2PA = 0x100;
    REG_BG2PB = 0;
    REG_BG2PC = 0;
    REG_BG2PD = 0x100;
    REG_BG2X = 0;
    REG_BG2Y = 0;
    for (i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++)
        ((u16 *)VRAM)[i] = FATAL_COLOR_BG;

    FatalDrawLine(0, sHeapFatalTitles[kind], 0, FALSE);
    FatalDrawLine(1, kind == HEAP_FATAL_ALLOC ? "SIZE " : "PTR ", value, TRUE);
    FatalDrawLine(2, "CALL ", caller, TRUE);
    FatalDrawLine(3, "USED ", GetHeapUsed(), TRUE);
    FatalDrawLine(4, "PEAK ", gHeapStats.peak, TRUE);

    for (;;)
        ;
}

// The address of the BL that called the function this is used in
#define CALLER_ADDRESS() (((u32)__builtin_return_address(0) & ~1) - 4)

static void CheckFree(void *pointer, u32 caller)
{
    struct MemBlock *block = (struct MemBlock *)((u8 *)pointer - sizeof(struct MemBlock));

    if ((u8 *)pointer < (u8 *)sHeapStart + sizeof(struct MemBlock)
     || (u8 *)pointer >= (u8 *)sHeapStart + sHeapSize
     || ((u32)pointer & 3)
     || block->magic != MALLOC_SYSTEM_ID) // also a block that was freed and merged
        HeapFatal(HEAP_FATAL_BAD_FREE, (u32)pointer, caller);
    if (!block->flag)
        HeapFatal(HEAP_FATAL_DOUBLE_FREE, (u32)pointer, caller);
}

void *Alloc(u32 size)
{
    void *mem = AllocInternal(sHeapStart, size);
    if (mem == NULL)
        HeapFatal(HEAP_FATAL_ALLOC, size, CALLER_ADDRESS());
    return mem;
}

void *AllocZeroed(u32 size)
{
    void *mem = AllocZeroedInternal(sHeapStart, size);
    if (mem == NULL)
        HeapFatal(HEAP_FATAL_ALLOC, size, CALLER_ADDRESS());
    return mem;
}

void Free(void *pointer)
{
    if (pointer != NULL)
        CheckFree(pointer, CALLER_ADDRESS());
    FreeInternal(sHeapStart, pointer);
}

#else

void *Alloc(u32 size)
{
    return AllocInternal(sHeapStart, size);
}

void *AllocZeroed(u32 size)
{
    return AllocZeroedInternal(sHeapStart, size);
}

void Free(void *pointer)
{
    FreeInternal(sHeapStart, pointer);
}

#endif // HEAP_CHECKS

// For callers that handle running out of heap themselves: never fatal
void *TryAlloc(u32 size)
{
    return AllocInternal(sHeapStart, size);
}

void *TryAllocZeroed(u32 size)
{
    return AllocZeroedInternal(sHeapStart, size);
}

// Bytes in allocated blocks, headers included
u32 GetHeapUsed(void)
{
    struct MemBlock *pos = (struct MemBlock *)sHeapStart;
    u32 used = 0;

    do {
        if (pos->flag)
            used += sizeof(struct MemBlock) + pos->size;
        pos = pos->next;
    } while (pos != (struct MemBlock *)sHeapStart);

    return used;
}

bool32 CheckMemBlock(void *pointer)
{
    return CheckMemBlockInternal(sHeapStart, pointer);
}

bool32 CheckHeap()
{
    struct MemBlock *pos = (struct MemBlock *)sHeapStart;

    do {
        if (!CheckMemBlockInternal(sHeapStart, pos->data))
            return FALSE;
        pos = pos->next;
    } while (pos != (struct MemBlock *)sHeapStart);

    return TRUE;
}
