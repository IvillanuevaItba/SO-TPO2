//
// Created by ignac on 18-Apr-20.
//

#ifndef MMFREELIST_H
#define MMFREELIST_H

// TODO: Should this struct be public?
typedef struct _FREE_LIST {
    size_t total_memory;
    size_t used_memory;
    size_t blocks_count;
    size_t spacing; // In order to make the struct 16 Bytes
}freelist_t;

typedef freelist_t * FreeList;

/*Initializes FreeList Memory Manager.
 * Returns a FreeList instance.
 * If not enough memory is allocated, returns NULL.
 * */
FreeList init_freelist(void* memory_start, int memory_size);

/* Allocate contiguous memory.
 * Returns void* to the start of the memory block.
 * If no memory is available, it returns null*/
void* alloc(FreeList freeList, size_t alloc_size);

// TODO: If the block pointer is not aligned it will break everything. Should this happen?
/*Free allocated memory.
 * The block pointer must be the same returned from alloc().
 * Freeing any other pointer may produce unwanted behaviour.
 * */
void ffree(FreeList freeList, void* block);

int test_mm(FreeList freeList);

#endif //MMFREELIST_H
