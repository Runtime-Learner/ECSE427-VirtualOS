#include <stdio.h>
#include <stdlib.h>
#include "pcb.h"

/*
 * Function: makePCB
 * ----------------------------
 *   Create a PCB and return a pointer to it.
 *
 *   Id: The id to assign to the new PCB
 * 
 *   return: A pointer to the newly created PCB
 */
struct PCB* makePCB(int Id) {
    struct PCB* pcb = (struct PCB*) malloc(sizeof(struct PCB));
    pcb->ID = Id;
    pcb->PC = 0;
    return pcb;
}

/*
 * Function: destroyPCB
 * ----------------------------
 *   Frees the memory allocated to pcb
 *
 *   pcb: the pcb we want to destroy
 */
void destroyPCB(struct PCB* pcb) {
    free(pcb);
}