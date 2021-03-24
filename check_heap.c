
#include "umalloc.h"

// Place any variables needed here from umalloc.c as an extern.
extern memory_block_t *free_head;

/*
 * check_heap -  used to check that the heap is still in a consistent state.
 * Required to be completed for checkpoint 1.
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap()
{
    // Iterates through the free list of memory blocks
    memory_block_t *tempnode = free_head;
    memory_block_t *iternode = NULL;
    while (tempnode != NULL) {
        // Checks if every block in the free list is marked as free
        if (is_allocated(tempnode)) {
            return 1;
        }
        // Checks if free blocks are aligned correctly
        if ((size_t)get_payload(tempnode) % ALIGNMENT != 0) {
            return 2;
        }
        // Checks if pointers on the free list point to free blocks
        if (get_block(get_payload(tempnode)) != tempnode) {
            return 3;
        }
        // iternode gets the last free block in free list
        if (get_next(tempnode) == NULL) {
            iternode = tempnode;
        }
        tempnode = get_next(tempnode);
    }
    // Checks if doubly linked list is assigned correctly by going backwards
    // through the list
    if (iternode != NULL) {
        while (iternode->prev != NULL) {
            iternode = iternode->prev;
        }
        if (iternode != free_head) {
            return 4;
        }
    }
    
    return 0;
}