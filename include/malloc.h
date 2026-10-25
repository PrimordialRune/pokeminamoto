#ifndef GUARD_ALLOC_H
#define GUARD_ALLOC_H


#define FREE_AND_SET_NULL(ptr)          \
{                                       \
    Free(ptr);                          \
    ptr = NULL;                         \
}

#define TRY_FREE_AND_SET_NULL(ptr) if (ptr != NULL) FREE_AND_SET_NULL(ptr)

// 88 KB. Highest measured use is ~76 KB (double battle); tools/regress
// scene_check.py and run.py report the peak per screen.
#define HEAP_SIZE 0x16000
extern u8 gHeap[HEAP_SIZE];

// How close the heap came to full since power-on, in every build (8 bytes)
struct HeapStats
{
    u32 peak;     // highest offset into the heap any block reached
    u32 failures; // allocations that found no room
};
extern struct HeapStats gHeapStats;

// What stopped the game, in debug and regress builds (see HeapFatal)
enum
{
    HEAP_FATAL_ALLOC = 1,   // Alloc or AllocZeroed found no room
    HEAP_FATAL_BAD_FREE,    // Free of a pointer that isn't an allocated block (or was freed and merged)
    HEAP_FATAL_DOUBLE_FREE, // Free of a block that is already free
};

// In debug and regress builds these stop the game with a red screen when the
// heap is full (Free when the pointer is bad); release builds return NULL.
void *Alloc(u32 size);
void *AllocZeroed(u32 size);
void Free(void *pointer);
// For callers with their own fallback: NULL when the heap is full, never fatal
void *TryAlloc(u32 size);
void *TryAllocZeroed(u32 size);
void InitHeap(void *heapStart, u32 heapSize);
u32 GetHeapUsed(void);

#endif // GUARD_ALLOC_H
