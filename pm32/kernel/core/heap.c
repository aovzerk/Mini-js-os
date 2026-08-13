typedef struct HeapBlock {
    u32 size;
    u32 used;
} HeapBlock;

static void zero_bytes(u8 *memory, u32 size)
{
    while (size--) *memory++ = 0;
}

static void heap_reset(unsigned pid)
{
    u8 *physical = (u8 *)(PROCESS_MEMORY_BASE + pid * USER_SIZE +
                          (USER_HEAP_BASE - USER_BASE));
    HeapBlock *first;

    zero_bytes(physical, USER_HEAP_SIZE);
    first = (HeapBlock *)physical;
    first->size = USER_HEAP_SIZE - sizeof(HeapBlock);
    first->used = 0;
}

static void *heap_allocate(u32 size)
{
    u8 *position = (u8 *)USER_HEAP_BASE;
    u8 *end = position + USER_HEAP_SIZE;

    if (!size) return 0;
    size = (size + 3) & ~3UL;
    while (position + sizeof(HeapBlock) <= end) {
        HeapBlock *block = (HeapBlock *)position;
        u32 remaining;

        if (block->size > (u32)(end - position - sizeof(HeapBlock))) return 0;
        if (!block->used && block->size >= size) {
            remaining = block->size - size;
            if (remaining > sizeof(HeapBlock) + 4) {
                HeapBlock *next = (HeapBlock *)(position + sizeof(HeapBlock) + size);
                next->size = remaining - sizeof(HeapBlock);
                next->used = 0;
                block->size = size;
            }
            block->used = 1;
            zero_bytes(position + sizeof(HeapBlock), block->size);
            return position + sizeof(HeapBlock);
        }
        position += sizeof(HeapBlock) + block->size;
    }
    return 0;
}

static int heap_release(void *pointer)
{
    u8 *position = (u8 *)USER_HEAP_BASE;
    u8 *end = position + USER_HEAP_SIZE;
    HeapBlock *previous = 0;

    while (position + sizeof(HeapBlock) <= end) {
        HeapBlock *block = (HeapBlock *)position;
        u8 *next_position;

        if (block->size > (u32)(end - position - sizeof(HeapBlock))) return -1;
        next_position = position + sizeof(HeapBlock) + block->size;
        if (position + sizeof(HeapBlock) == (u8 *)pointer && block->used) {
            block->used = 0;
            zero_bytes(position + sizeof(HeapBlock), block->size);
            if (next_position + sizeof(HeapBlock) <= end) {
                HeapBlock *next = (HeapBlock *)next_position;
                if (!next->used &&
                    next->size <= (u32)(end - next_position - sizeof(HeapBlock)))
                    block->size += sizeof(HeapBlock) + next->size;
            }
            if (previous && !previous->used)
                previous->size += sizeof(HeapBlock) + block->size;
            return 0;
        }
        previous = block;
        position = next_position;
    }
    return -1;
}
