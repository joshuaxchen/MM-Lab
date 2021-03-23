#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Joshua Chen jc89873" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;
// heap_head is a dummy node that marks the beginning of the heap, including
// allocated and unallocated memory blocks
memory_block_t *heap_head;

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
 * Initializes the size and allocated fields, along with NUlling out the next 
 * field.
 */
void put_block(memory_block_t *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;

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
bool is_allocated_f(memory_footer *block) {
    assert(block != NULL);
    return block->block_size_alloc & 0x1;
}

void allocate_f(memory_footer *block) {
    assert(block != NULL);
    block->block_size_alloc |= 0x1;
}

void deallocate_f(memory_footer *block) {
    assert(block != NULL);
    block->block_size_alloc &= ~0x1;
}

size_t get_size_f(memory_footer *block) {
    assert(block != NULL);
    return block->block_size_alloc & ~(ALIGNMENT-1);
}

void put_block_f(memory_footer *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
}

void *get_payload_f(memory_footer *block) {
    assert(block != NULL);
    return (void*)(block - get_size_f(block));
}

memory_footer *get_footer(void *payload) {
    assert(payload != NULL);
    return ((memory_footer *)payload) + get_size(get_block(payload));
}

memory_footer *header_to_footer(memory_footer *block) {
    assert(block != NULL);
    return ((memory_footer *)block) + sizeof(memory_block_t) + get_size(block);
}
*/

void free_list_add(memory_block_t *block) {

}

void free_list_delete(memory_block_t *block) {
    assert(is_allocated(block));
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
    //user requests a block of memory larger than heap
    if (ALIGN(size) > 16 * PAGESIZE) {
        return NULL;
    }
    //node used to iterate through linked list
    memory_block_t *traverse = free_head;
    //iterates until first block that is free and larger than requested size is found
    while (traverse != NULL) {
        if (get_size(traverse) >= ALIGN(size) && !is_allocated(traverse))
            return traverse;
        else
            traverse = traverse->next;
    }
    //no fit found
    if(traverse == NULL) {
        return NULL;
    }
    
    return NULL;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    //call csbrk and add to free list?
    memory_block_t *new_head = csbrk(size);
    put_block(new_head, size - sizeof(memory_block_t), false);
    
    new_head->next = free_head;
    new_head->prev = NULL;
    free_head->prev = new_head;
    free_head = new_head;
    return new_head;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    assert(get_size(block) >= ALIGN(size));
    assert(!is_allocated(block));
    if (get_size(block) >= (2 * ALIGNMENT) + ALIGN(size)) {
        memory_block_t *new_node = (memory_block_t *)((void*)block + get_size(block) - ALIGN(size));
        put_block(new_node, ALIGN(size), true);
        new_node->next = NULL;
        new_node->prev = NULL;
        put_block(block, get_size(block) - ALIGN(size) - sizeof(memory_block_t), false);
        return new_node;
    } else {
        //printf("2");
        allocate(block);
        memory_block_t *previous = block->prev;
        memory_block_t *next = block->next;
        if (previous == NULL) {
            free_head = next;
        } else {
            previous->next = next;
        }
        next->prev = block->prev;
        return block;
    }
    
    //return new_node;
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
    free_head = csbrk(16 * PAGESIZE);
    if (free_head == NULL) {
        return -1;
    }
    put_block(free_head, PAGESIZE * 16 - sizeof(memory_block_t), false);
    // do this in put_block
    free_head->next = NULL;
    free_head->prev = NULL;
    //memory_footer *temp = free_head + PAGESIZE * 4 - sizeof(memory_footer);
    //put_block_f(temp, PAGESIZE * 4 - sizeof(memory_block_t) - sizeof(memory_footer), false);
    return 0;
}

// need padding for footer? go from head to foot?
/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    memory_block_t *traverse = find(size);
    //printf("0");
    if (traverse == NULL) {
        //printf("1");
        extend(16 * PAGESIZE);
        return umalloc(size);
    }
    // traverse is the pointer to first address
    // change traverse size, when splitting create new node with footer/header
    memory_block_t *block = split(traverse, size);
    
    return get_payload(block);
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    //printf("5");
    memory_block_t *block = get_block(ptr);
    if(is_allocated(block)) {
        deallocate(block);
        block->next = free_head;
        block->prev = NULL;
        free_head->prev = block;
        free_head = block;
    }
}
// if (block->prev == NULL) {
//         free_head = block->next;
//     } else {
//         block->prev->next = block->next;
//     }
//     block->next->prev = block->prev;