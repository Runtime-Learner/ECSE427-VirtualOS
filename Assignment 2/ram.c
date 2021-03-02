#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "terminal-io.h"
#include "ram.h"

#define RAM_SIZE 1000
#define MAX_CONCURRENT_PROCS 5 //number of processes allowed to run at the same time

struct ramDataBlock {
    unsigned int start;
    unsigned int size;
    short isAllocated;
    struct ramDataBlock* nextBlock;
};

struct vMemMngr {
    struct ramDataBlock* ramBlock;
};

static struct ramDataBlock* allocateRamDataBlock(int size);
static struct ramDataBlock* createRamBlock(unsigned int start, unsigned int size);

static void deallocateRamDataBlock(struct ramDataBlock* block);
static void defragmentRAM();
static void consolidateRAM();

static struct ramDataBlock* blockPtr;
static struct vMemMngr processList[MAX_CONCURRENT_PROCS]; 
static char* ram[RAM_SIZE] = {NULL};
static unsigned int unallocatedRAM = RAM_SIZE;


/*
 * Function: initializeVram
 * ----------------------------
 *   register the RAM cleanup function to be called at program termination.
 *   Initialize RAM by creating on RAM datablock of size RAM_SIZE.
 */
void initializeVram() {
    atexit(ram_clear);  //register RAM cleanup function to be called at program termination
    ram_clear();
    blockPtr = createRamBlock(0, RAM_SIZE);
    for (int i = 0; i < MAX_CONCURRENT_PROCS; i++){
        processList[i].ramBlock = NULL;
    }
    unallocatedRAM = RAM_SIZE;
    return;
}

/*
 * Function: ram_clear
 * ----------------------------
 *   Destroy all RAM datablocks. Free all data stored in RAM.
 */
void ram_clear() {
    if (blockPtr == NULL) {
        return;
    }

    struct ramDataBlock* currentBlock = blockPtr->nextBlock;
    struct ramDataBlock* previousBlock = blockPtr;
    for (; currentBlock != NULL; previousBlock = currentBlock, currentBlock = currentBlock->nextBlock) {
        free(previousBlock);
    }
    free(previousBlock);

    //go through all of RAM and free() all addresses that hold data
    for (int i = 0; i < RAM_SIZE; i++){
        if (ram[i] != NULL) {
            free(ram[i]);
            ram[i] = NULL;
        }
    }
}

/*
 * Function: requestProcRam_size
 * ----------------------------
 *   Return the location in RAM of the RAM datablock associated with 
 *   the specific process
 * 
 *   procId: the process ID associated with the RAM datablock
 * 
 *   return: the location in RAM of the RAM datablock associated with 
 *   the specific process
 */
int requestProcRam_start(int procId) {
    //TODO: validate input
    return processList[procId].ramBlock->start;
}

/*
 * Function: requestProcRam_size
 * ----------------------------
 *   Return the RAM datablock size associated with 
 *   the specific process
 * 
 *   procId: the process ID associated with the RAM datablock
 * 
 *   return: the RAM datablock size associated with 
 *   the specific process
 */
int requestProcRam_size(int procId) {
    //TODO: validate input
    return processList[procId].ramBlock->size;
}

/*
 * Function: readRamLocation
 * ----------------------------
 *   Return the data stored at RAM location specified
 * 
 *   location: index in RAM
 * 
 *   return: the data stored at RAM location
 */
char* readRamLocation(int location) {
    //TODO: validate input
    return ram[location];
}

/*
 * Function: requestProcId
 * ----------------------------
 *   Verifies that the process can be added to the list of currently running processes
 *   If so, attempt to allocate a RAM datablock large enough to load it into memory.
 *   If both steps succeed, return the process ID that is associated with the allocated
 *   RAM datablock.
 * 
 *   size: the size of the process we want to load into RAM
 * 
 *   return: process ID that is associated with the allocated RAM datablock
 */
int requestProcId(int size) {
    int id = 0;
    while (id < MAX_CONCURRENT_PROCS && processList[id].ramBlock != NULL) {
        id++;
    }
    if (id == MAX_CONCURRENT_PROCS) {
        setColor(RED);
        printf("Error: Cannot add program. Too many programs running concurrently!\n");
        setColor(DEFAULT);
        return -1;
    }

    //allocate a ram datablock for process to use
    struct ramDataBlock* memoryLocation = allocateRamDataBlock(size);

    if (memoryLocation == NULL) {
        if (unallocatedRAM >= size) {
            defragmentRAM(); //TODO: cannot run defragmentRAM: would stall thread -> need to have a way to wait for defrag to HAPPEN
            memoryLocation = allocateRamDataBlock(size);
        }

        //if we couldn't allocate ram datablock
        if (memoryLocation == NULL) {
            setColor(RED);
            printf("Error: Not enough RAM to add program!\n");
            setColor(DEFAULT);
            return -1;
        }
    }
    else {
        processList[id].ramBlock = memoryLocation;
    }

    return id;
}

/*
 * Function: createRamBlock
 * ----------------------------
 *   Create an unallocated RAM datablock with the specified parameters.
 * 
 *   start: the RAM index where the datablock begins
 *   size: the size of the RAM datablock
 * 
 *   return: a pointer to the newly created RAM datablock
 */
static struct ramDataBlock* createRamBlock(unsigned int start, unsigned int size) {
    struct ramDataBlock* blkPtr = (struct ramDataBlock*) malloc(sizeof(struct ramDataBlock));
    blkPtr->start = start;
    blkPtr->size = size;
    blkPtr->isAllocated = 0;
    blkPtr->nextBlock = NULL;
    return blkPtr;
}

/*
 * Function: deallocateRamDataBlock
 * ----------------------------
 *   Change the status of the specified RAM datablock to unallocated
 *   so it can be used by other processes.
 * 
 *   block: the datablock to deallocate
 */
static void deallocateRamDataBlock(struct ramDataBlock* block) {
    //TODO: validate parameters
    block->isAllocated = 0;
}

/*
 * Function: allocateRamDataBlock
 * ----------------------------
 *   attempt to create a RAM datablock of size size.
 *   Set the RAM datablock to allocated.
 * 
 *   size: the size we want to allocate in RAM
 * 
 *   return: a pointer to the newly created RAM datablock if successful. NULL otherwise.
 */
static struct ramDataBlock* allocateRamDataBlock(int size) {
    int foundLocation = 0;
    struct ramDataBlock* currentBlock = blockPtr;
    struct ramDataBlock* previousBlock = NULL;
    for (; currentBlock != NULL; previousBlock = currentBlock, currentBlock = currentBlock->nextBlock) {
        if (!currentBlock->isAllocated && size <= currentBlock->size) {
            unallocatedRAM -= size; //update how much RAM is left after allocation
            if (size == currentBlock->size) {
                currentBlock->isAllocated = 1;
                return currentBlock;
            }
            else {
                if (previousBlock == NULL) {
                    blockPtr = createRamBlock(0, size);
                    blockPtr->nextBlock = currentBlock;
                    blockPtr->isAllocated = 1;
                    currentBlock->start = currentBlock->start + size;
                    currentBlock->size = currentBlock->size - size;
                    return blockPtr;
                }
                else {
                    previousBlock->nextBlock = createRamBlock(currentBlock->start, size);
                    previousBlock->nextBlock->nextBlock = currentBlock;
                    previousBlock->nextBlock->isAllocated = 1;
                    currentBlock->start = currentBlock->start + size;
                    currentBlock->size = currentBlock->size - size;
                    return previousBlock->nextBlock;
                }
            }
        }
    }
    return NULL;
}

/*
 * Function: freeProcId
 * ----------------------------
 *   Deallocate the RAM datablock associated with the process ID,
 *   Consolidate RAM,
 *   free the process ID.
 *   
 * 
 *   ID: the process ID we want to free
 */
int freeProcId(int ID) {
    deallocateRamDataBlock(processList[ID].ramBlock);
    consolidateRAM();
    processList[ID].ramBlock = NULL;
}

/*
 * Function: addToRAM
 * ----------------------------
 *   Load file data into a RAM datablock associated with the specified process
 * 
 *   p: the file to read
 *   vRamId: the processId associated with the RAM datablock we are filling
 */
void addToRAM(FILE* p, int vRamId) {
    //vRamId points to beginning of allocated block.
    char temp[1000];
    for (int i = 0; i < processList[vRamId].ramBlock->size; i++){
        
        //if the RAM address already holds data, free it in order to write to that address
        if (ram[processList[vRamId].ramBlock->start + i] != NULL) {
            free(ram[processList[vRamId].ramBlock->start + i]);
        }

        //get the next line of data to store in RAM address
        void* error = fgets(temp, 1000, p);

        //if the line was empty, add an empty line into RAM. Otherwise, add line to RAM
        if (error == NULL) {
            ram[processList[vRamId].ramBlock->start + i] = strdup("");
        }
        else {
            //ensure the data we are loading to RAM does not include the newline character
            if (temp[strlen(temp) - 1] == '\n') {
                temp[strlen(temp) - 1] = '\0';
            }
            ram[processList[vRamId].ramBlock->start + i] = strdup(temp);
        }
    }
}

/*
 * Function: consolidateRAM
 * ----------------------------
 *   Merge adjacent unallocated RAM datablocks. Helps reduce RAM fragmentation
 */
static void consolidateRAM() {
    struct ramDataBlock* currentBlock = blockPtr;
    while (currentBlock->nextBlock != NULL) {
        if (currentBlock->isAllocated || currentBlock->nextBlock->isAllocated) {
            currentBlock = currentBlock->nextBlock;
        } else {
            struct ramDataBlock* blockToDelete = currentBlock->nextBlock;
            currentBlock->nextBlock = blockToDelete->nextBlock;
            currentBlock->size += blockToDelete->size;
            free(blockToDelete);
        }
    }
}

/*
 * Function: defragmentRAM
 * ----------------------------
 *   Move allocated RAM datablocks to the beginning of RAM space in order to consolidate unallocated RAM blocks
 *   HAS NOT BEEN IMPLEMENTED AT THIS TIME
 */
static void defragmentRAM(){
    //TODO: implement
    // find first program, move it to RAM[0]
    // find next program, move it to RAM[sizeof(previous_program)]

    // CPU does not care if we move code that has already been executed. ONLY THE CASE SINCE WE HAVE NO LOOPS/GOTO/FUNCTIONS
    // in list of legal operations


    // best solution is to have defragmentRAM be its own process. No need to worry about CPU accessing invalid location if 
    // we are in control of the CPU when we do it. ONLY VALID IN SINGLE CPU SYSTEM

    //maybe move one process per quanta? <- too often; defrag is costly. Should only run if we NEED to.
}