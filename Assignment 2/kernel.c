#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "shell.h"
#include "pcb.h"
#include "ram.h"
#include "cpu.h"
#include "kernel.h"

struct pcbQueueNode {
    struct PCB* pcb;
    struct pcbQueueNode* nextNode;
};

static struct pcbQueueNode* readyQueueTail = NULL;

static int getNumberOfLines(FILE *file);
static void addToReady(struct PCB* pcb);
static void removeNodeFromReady(struct pcbQueueNode* nodeToDelete);
static void destroyProcess(struct PCB* pcb);

/*
 * Function: sigintHandler
 * ----------------------------
 *   Triggered when user presses Ctrl-C during the getString method.
 *   Exits the program instead of aborting it. Ensures memory is deallocated and all that jazz.
 *
 *   sig_num: signal that triggered the interrupt (parameter handled by OS)
 */
static void sigintHandler(int sig_num)
{
    putchar('\n');
    exit(1);
}

/*
 * Function: main
 * ----------------------------
 *   Entry point of kernel.
 *   Initializes RAM and starts the shellUI.
 * 
 *   return: The shell exit code.
 */
int main(void){
    //register cleanup methods
    atexit(emptyReadyQueue);

    printf("%s\n", "Kernel 1.0 loaded!");

    //ctrl-c is handled by our own code to ensure terminal settings are reset before program ends.
    signal(SIGINT, sigintHandler);

    initializeVram();
    int error = shellUI();

    //let ctrl-c be handled by OS again
    signal(SIGINT, SIG_DFL);
    exit(error);
}

/*
 * Function: myinit
 * ----------------------------
 *   Open file filename.
 *   Get the number of lines in file.
 *   Request a process ID and a RAM memory block large enough to store the file.
 *   Load file data into RAM if memory block was successfully allocated.
 * 
 *   filename: name of file to load into RAM
 * 
 *   return: 0 if successful. 1 if error occurred.
 */
int myinit(char* filename) {
    FILE *script;
    script = fopen(filename, "r");
    if (!script)
    {
        setColor(RED);
        printf("exec: Script \'%s\' not found\n", filename);
        setColor(DEFAULT);
        return 1;
    }

    //get number of lines to allocate memory
    int numLines = getNumberOfLines(script);
    if (!numLines)
    {
        setColor(RED);
        printf("exec: Script \'%s\' is empty\n", filename);
        setColor(DEFAULT);
        return 1;
    }

    //get PCB
    int pcbId = requestProcId(numLines);
    if (pcbId == -1) {
        //could not get process ID
        fclose(script);
        return 1;
    }

    struct PCB* pcb = makePCB(pcbId);

    /**
     * load program in RAM. will NEVER fail.
     * requestProcId gets enough RAM allocated for script to be loaded into RAM */
    addToRAM(script, pcbId);

    addToReady(pcb);

    fclose(script);
    return 0;
}

/*
 * Function: scheduler
 * ----------------------------
 *   Handles the task switching of processes inside the ready queue. 
 *   Moves processes to/from the CPU.
 *   Calls run(quanta), which will execute quanta instructions.
 * 
 *   If a process ends, it is removed from the ready queue.
 *   The scheduler runs until the ready queue is empty.
 */
int scheduler() {
    struct PCB* currentProcess;
    int processSize;
    int error = 0;
    while (readyQueueTail != NULL) {
        currentProcess = readyQueueTail->nextNode->pcb;
        processSize = requestProcRam_size(currentProcess->ID);
        moveToCPU(currentProcess);
        if (currentProcess->PC + QUANTA < processSize) {
            error = run(QUANTA);
            removeFromCPU(currentProcess);
            if (error) {
                removeNodeFromReady(readyQueueTail->nextNode);
            } else {
                readyQueueTail = readyQueueTail->nextNode;
            }
        } else {
            error = run(processSize - currentProcess->PC);
            removeFromCPU(currentProcess);
            removeNodeFromReady(readyQueueTail->nextNode);
        }
    }
    return 0;
}

/*
 * Function: addToReady
 * ----------------------------
 *   Add a PCB to the tail of the ready queue
 * 
 *   pcb: process to add to the ready queue
 */
static void addToReady(struct PCB* pcb) {
    struct pcbQueueNode* newNode = (struct pcbQueueNode*) malloc(sizeof(struct pcbQueueNode));
    if (readyQueueTail == NULL) {
        readyQueueTail = newNode;
        newNode->pcb = pcb;
        newNode->nextNode = newNode;
    } else {
        newNode->pcb = pcb;
        newNode->nextNode = readyQueueTail->nextNode;
        readyQueueTail->nextNode = newNode;
        readyQueueTail = newNode;
    }
    return;
}

/*
 * Function: removeNodeFromReady
 * ----------------------------
 *   Removes the specified node from the ready queue.
 *   This leads to the PCB associated with the node being destroyed.
 * 
 *   nodeToDelete: the node we wish to remove from the queue.
 */
static void removeNodeFromReady(struct pcbQueueNode* nodeToDelete) {
    if (readyQueueTail == NULL || nodeToDelete == NULL ) {
        return;
    }
    if (nodeToDelete->nextNode == nodeToDelete) {   //only happens when queue has 1 element
        destroyProcess(nodeToDelete->pcb);
        free(nodeToDelete);
        readyQueueTail = NULL;
    } else {
        struct pcbQueueNode* tmpTail = readyQueueTail;
        while(tmpTail->nextNode != nodeToDelete) {
            tmpTail = tmpTail->nextNode;
        }
        tmpTail->nextNode = nodeToDelete->nextNode;
        destroyProcess(nodeToDelete->pcb);
        if (readyQueueTail == nodeToDelete) {
            readyQueueTail = tmpTail;
        }
        free(nodeToDelete);
    }
}

/*
 * Function: emptyReadyQueue
 * ----------------------------
 *   Remove all nodes from the ReadyQueue.
 *   Calls removeNodeFromReady()
 */
void emptyReadyQueue() {
    while (readyQueueTail != NULL) {
        removeNodeFromReady(readyQueueTail);
    }
}

/*
 * Function: destroyProcess
 * ----------------------------
 *   Free RAM and process ID associated with process.
 *   Destroy PCB 
 * 
 *   pcb: the PCB to destroy
 */
static void destroyProcess(struct PCB* pcb) {
    freeProcId(pcb->ID);
    destroyPCB(pcb);
}

/*
 * Function: getNumberOfLines
 * ----------------------------
 *   Return the number of lines inside the file
 * 
 *   file: the file whose size we are calculating
 * 
 *   return: the number of lines inside the file
 */
static int getNumberOfLines(FILE *file) {
    char c = getc(file);
    if (c == -1 && ftell(file) == 0) {
        return 0;
    }
    fseek(file, 0, SEEK_SET);
    int numberOfLines = 1;

    while ((c = getc(file)) != -1){
        if (c == '\n') {
            numberOfLines++;
        }
    }
    fseek(file, 0, SEEK_SET);
    return numberOfLines;
}