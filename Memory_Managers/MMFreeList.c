#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "MMFreeList.h"

/*TODO:
 * Replace printf with debug print
 * Adapt to OS*/

typedef struct _BLOCK_HEADER {
    struct blockheader_t* next;
    struct blockheader_t* prev;
    //struct blockheader_t* next_free;
    size_t blockSize;
    int free;
} blockheader_t;

typedef blockheader_t * BlockHeader;



#define MIN_BLOCK_SIZE 32 // Avoid insignificantly small sized blocks (external fragmentation)
#define MIN_PARTITION_SIZE sizeof(blockheader_t) + MIN_BLOCK_SIZE

#define CAN_PARTITION(block_size, partition_size) block_size - partition_size <= MIN_PARTITION_SIZE
#define HAS_ENOUGH_SPACE(block_size, requested_size) block_size >= requested_size

FreeList init_freelist(void *memory_start, int memory_size) {
    if(memory_size < sizeof(freelist_t) + MIN_PARTITION_SIZE)
        return NULL;        // Not enough memory

    // Initialize FreeList instance
    FreeList freeList = memory_start;
    freeList -> total_memory = memory_size;
    freeList -> used_memory = sizeof(freelist_t) + sizeof(blockheader_t);
    freeList -> blocks_count = 1;

    // Initialize first BlockHeader Instance
    BlockHeader first_block = (BlockHeader)(freeList + 1);
    first_block->blockSize = memory_size - sizeof(blockheader_t) - sizeof(freelist_t);
    first_block->next = NULL;
    first_block->prev = NULL;
    first_block->free = 1;

    return freeList;
}

void * get_memory(BlockHeader blockHeader){
    return (void*)(blockHeader + 1);
}

int partition_block(FreeList freeList, BlockHeader primary_partition, size_t partition_size) {
    if(CAN_PARTITION(primary_partition->blockSize,partition_size)){
        return 0;   // Secondary/remaining partition is too small
    }

    // Create secondary/remaining partition
    BlockHeader secondary_partition = (BlockHeader)((char*)get_memory(primary_partition) + partition_size);
    secondary_partition -> blockSize = primary_partition->blockSize - partition_size - sizeof(blockheader_t);
    secondary_partition -> prev = (struct blockheader_t*)primary_partition;
    secondary_partition -> next = (struct blockheader_t*)primary_partition -> next;

    // Update next block header
    if(secondary_partition -> next != NULL){
        ((BlockHeader)secondary_partition ->  next) -> prev = (struct blockheader_t*)secondary_partition;
    }

    //Update primary partition
    primary_partition -> blockSize = partition_size;
    primary_partition -> next = (struct blockheader_t*)secondary_partition;

    freeList->blocks_count++;
    freeList->used_memory += sizeof(blockheader_t);

    return 1;
}

BlockHeader join_partitions(FreeList freeList, BlockHeader first_block, BlockHeader second_block){
    printf("Joining: 0x%" PRIXPTR " - 0x%" PRIXPTR "\t Memory added: %d\n", (unsigned int) first_block, (unsigned int) second_block, second_block -> blockSize + sizeof(blockheader_t));

    first_block -> next = (struct blockheader_t*)second_block->next;

    if(second_block -> next != NULL)
        ((BlockHeader)second_block -> next) -> prev = (struct blockheader_t*)first_block;
    first_block -> blockSize += second_block -> blockSize + sizeof(blockheader_t);

    // Update FreeList
    freeList->blocks_count--;
    freeList->used_memory -= sizeof(blockheader_t);

    return first_block;
}

void *alloc(FreeList freeList, size_t alloc_size) {
    BlockHeader block = (BlockHeader)(freeList + 1);

    // Search for a large enough, free block.
    while (block != NULL){
        if(block->free && HAS_ENOUGH_SPACE(block->blockSize,alloc_size)) {
            partition_block(freeList, block, alloc_size);
            block->free = 0;
            freeList->used_memory += alloc_size;
            return get_memory(block);
        }

        block = (BlockHeader)block -> next;
    }
    return NULL;    // No memory available
}

void ffree(FreeList freeList, void *block) {
    BlockHeader blockHeader = (BlockHeader)block - 1;
    printf("Freeing Memory: 0x%" PRIXPTR "\t Block Header: 0x%" PRIXPTR "\t Block Size: %d\n", (unsigned int)block, (unsigned int) blockHeader, blockHeader->blockSize) ;

    blockHeader -> free = 1;
    freeList->used_memory -= blockHeader->blockSize;

    // Check if blocks can be merged
    if(blockHeader -> prev != NULL && ((BlockHeader)blockHeader -> prev) -> free){
        blockHeader = join_partitions(freeList, (BlockHeader)blockHeader -> prev, blockHeader);
    }
    if(blockHeader -> next != NULL &&((BlockHeader)blockHeader -> next) -> free){
        join_partitions(freeList, blockHeader, (BlockHeader)blockHeader -> next);
    }
}

void printHeader(BlockHeader blockHeader){
    printf("---- ----\n Header: 0x%" PRIXPTR "\t Mem Start: 0x%" PRIXPTR "\n Size: %d\t Free: %s\n Prev: 0x%" PRIXPTR "\t Next:0x%" PRIXPTR "\n---- ----\n",
            (unsigned int)blockHeader, (unsigned int)get_memory(blockHeader),
            blockHeader -> blockSize, blockHeader -> free? "TRUE" : "FALSE",
           (unsigned int)blockHeader->prev, (unsigned int)blockHeader->next);
}

int test_mm(FreeList freeList) {
    printf("Total Memory: %d \t Used Memory: %d\t Free Memory: %d \tBlocks Count: %d\n", freeList->total_memory,freeList->used_memory, freeList->total_memory - freeList->used_memory,freeList->blocks_count);
    printf("___________________________START TEST___________________________\n");
    BlockHeader block = (BlockHeader)(freeList + 1);
    while (block != NULL){
        printHeader(block);
        block = (BlockHeader)block -> next;
    }
    printf("___________________________END TEST___________________________\n");
}

int main() {
    int memory_size = 256;
    void * memory = malloc(memory_size);
    FreeList freeList = init_freelist(memory, memory_size);
    void * mem[4] = {0};
    for (int i = 0; i < 4; ++i) {
        mem[i] = alloc(freeList, 64);
        printf(mem[i] == NULL?"Failed to allocate memory\n" : "Succefully allocated memory: 0x%" PRIXPTR "\n", (unsigned int)mem[i]);
        test_mm(freeList);
    }

    ffree(freeList, mem[0]);
    test_mm(freeList);
    ffree(freeList, mem[2]);
    test_mm(freeList);
    ffree(freeList, mem[1]);
    test_mm(freeList);

    return 0;
}


