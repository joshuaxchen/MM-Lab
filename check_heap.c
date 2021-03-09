
#include "umalloc.h"

// Place any variables needed here from umalloc.c as an extern.
extern memory_block_t *free_head;
// heap_head is a dummy node that marks the beginning of the heap, including
// allocated and unallocated memory blocks
extern memory_block_t *heap_head;

/*
 * check_heap -  used to check that the heap is still in a consistent state.
 * Required to be completed for checkpoint 1.
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap()
{
    // Checks if every block in the free list is marked as free
    memory_block_t *tempnode = free_head;
    while (tempnode != NULL) {
        if (is_allocated(tempnode)) {
            return 1;
        }
        tempnode = get_next(tempnode);
    }

    // Iterates through all memory blocks in the heap
    memory_block_t *heapnode = heap_head;
    // nextnode is to help check if current heapnode is contiguous and escaped
    // coalescing
    memory_block_t *nextnode = free_head;
    nextnode = get_next(nextnode);
    while (heapnode != NULL) {
        // If the heapnode is a free block, then iterate through free list to
        // check if it is in the list
        if (!is_allocated(heapnode)) {
            memory_block_t *temp2 = free_head;
            bool found = false;
            while (temp2 != NULL && !found) {
                if (temp2 != heapnode) {
                    temp2 = get_next(temp2);
                }
                else {
                    found = true;
                }
            }
            if (!found)
                return 2;
        }
        // Checks if the block is aligned to 16 bytes
        if ((size_t)get_payload(heapnode) % ALIGNMENT != 0) {
            return 3;
        }
        // Checks if the pointer of nextnode is contiguous to heapnode by
        // matching the pointer arithmetic of heapnode to nextnode's address
        if (nextnode != NULL && (char *)heapnode + get_size(heapnode) +
            sizeof(memory_block_t) == nextnode) {
            return 9;
        }
        heapnode = get_next(heapnode);
        // Gets the next node except when nextnode is NULL to avoid seg fault
        if (nextnode != NULL) {
            nextnode = get_next(nextnode);
        }
    }
    return 0;
}