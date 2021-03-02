#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "pcb.h"
#include "ram.h"
#include "shell.h"
#include "interpreter.h"

struct CPU {
    int IP; 
    char IR[1000]; 
    int quanta;
};

struct CPU cpu = {-1, "", QUANTA};
volatile int cpuIsAvailable = 1;


/*
 * Function: run
 * ----------------------------
 *   execute quanta number of instructions of the process
 *   currently running on the CPU.
 * 
 *   quanta: the number of instructions to execute.
 * 
 *   return: Whether the CPU encountered an error when running commands.
 */
int run(int quanta) {
    cpuIsAvailable = 0;
    char *tokens[NUM_TOKENS] = {NULL};
    int error = 0;
    while (quanta > 0) {
        strcpy(cpu.IR, readRamLocation(cpu.IP));
        if (!parse(cpu.IR, ' ', tokens))
        {
            if (interpreter(tokens) != 0)
            {
                tokens_destroy(tokens);
                error = 1;
                break;
            }
        }
        cpu.IP++;
        tokens_destroy(tokens);
        quanta--;
    }
    cpuIsAvailable = 1;
    return error;
}

/*
 * Function: moveToCPU
 * ----------------------------
 *   Move the PCB into the CPU to execute its process. 
 *   Update the CPU IP variable based on the PCB PC variable.
 * 
 *   pcb: the PCB to move to the CPU
 */
void moveToCPU(struct PCB* pcb) {
    cpu.IP = requestProcRam_start(pcb->ID) + pcb->PC;
}

/*
 * Function: removeFromCPU
 * ----------------------------
 *   Remove the pcb currently running on the CPU. 
 *   Update the PCB PC variable based on how many instructions the CPU has run.
 * 
 *   pcb: the PCB to remove from the CPU
 */
void removeFromCPU(struct PCB* pcb) {
    pcb->PC = cpu.IP - requestProcRam_start(pcb->ID);
}
