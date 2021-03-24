#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Joshua Chen jc89873" ANSI_RESET;
const int mem_size = 8 * PAGESIZE;

/*
 * Free and allocated blocks are usually contiguous unless there are multiple
 * calls of csbrk. All blocks contain block_size_alloc which keeps track of
 * whether the block is allocated and its size. Memory blocks also contain
 * pointers to next and previous memory blocks within the free list to iterate
 * through blocks faster. There are 8 bytes of padding in the structure to make
 * it easier to align while coding. The allocator adds free blocks to the front
 * of the list and can remove them anywhere in the list. The free list is not
 * contiguous and contains only free blocks.
 */

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;

/*
 * is_allocated - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc |= 0x1;
}

/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_size(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & ~(ALIGNMENT-1);
}

/*
 * get_next - gets the next block.
 */
memory_block_t *get_next(memory_block_t *block) {
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields
 */
void put_block(memory_block_t *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
    // deleted next being set to NULL
}

/*
 * get_payload - gets the payload of the block.
 */
void *get_payload(memory_block_t *block) {
    assert(block != NULL);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t *get_block(void *payload) {
    assert(payload != NULL);
    return ((memory_block_t *)payload) - 1;
}

/*
 * free_list_add - adds a block of memory to the front of the free list
 */
void free_list_add(memory_block_t *block) {
    assert(!is_allocated(block));
    block->next = free_head;
    block->prev = NULL;
    free_head->prev = block;
    free_head = block;
}

/*
 * free_list_delete - deletes a block of memory in the free list
 */
void free_list_delete(memory_block_t *block) {
    assert(is_allocated(block));
    // if block is the free_head set to next
    if (block->prev == NULL) {
        free_head = block->next;
    } else {
        block->prev->next = block->next;
    }
    block->next->prev = block->prev;
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required. 
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) {
    // node used to iterate through linked list
    memory_block_t *traverse = free_head;
    // iterates until it finds the first block that is free and larger than 
    // requested size
    while (traverse != NULL) {
        if (get_size(traverse) >= ALIGN(size) && !is_allocated(traverse))
            return traverse;
        else
            traverse = get_next(traverse);
    }
    // no fit found
    return NULL;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    //calls csbrk and adds a free block to free list
    memory_block_t *new_head = csbrk(size);
    put_block(new_head, size - sizeof(memory_block_t), false);
    free_list_add(new_head);
    return new_head;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    // block size must be larger than size and free
    assert(get_size(block) >= ALIGN(size));
    assert(!is_allocated(block));
    // if size of block is greater than size of header + ALIGNMENT + ALIGN(size)
    // then there is enough space to split
    if (get_size(block) >= sizeof(memory_block_t) + ALIGNMENT + ALIGN(size)) {
        // memory arithmetic to find address for allocated node
        memory_block_t *allocated_node = (memory_block_t *)((void*)block + get_size(block) - ALIGN(size));
        put_block(allocated_node, ALIGN(size), true);
        allocated_node->next = NULL;
        allocated_node->prev = NULL;
        // kept original block unchanged except size
        put_block(block, get_size(block) - ALIGN(size) - sizeof(memory_block_t), false);
        return allocated_node;
    } else {
        // else just allocate and remove from free list
        allocate(block);
        free_list_delete(block);
        return block;
    }
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block) {
    return NULL;
}


/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    // free_head calls csbrk to allocate initial memory
    free_head = csbrk(mem_size);
    if (free_head == NULL) {
        return -1;
    }
    put_block(free_head, mem_size - sizeof(memory_block_t), false);
    free_head->next = NULL;
    free_head->prev = NULL;
    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    // iterates through free list to find block large enough for size
    memory_block_t *traverse = find(size);
    //if there is no block big enough, extend the heap and recall umalloc
    if (traverse == NULL) {
        extend(mem_size * 2);
        return umalloc(size);
    }
    // block is the pointer to address of allocated block after split
    memory_block_t *block = split(traverse, size);
    
    return get_payload(block);
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    // deallocate memory block and add to free list
    memory_block_t *block = get_block(ptr);
    if(is_allocated(block)) {
        deallocate(block);
        free_list_add(block);
    }
}