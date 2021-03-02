#ifndef CPU_HEADER
    #define CPU_HEADER
    #include "pcb.h"

    #define QUANTA 20

    //cpuIsAvailable is false when the CPU is executing instructions (the run() function is running)
    extern volatile int cpuIsAvailable;

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
    extern int run(int quanta);

    /*
    * Function: moveToCPU
    * ----------------------------
    *   Move the PCB into the CPU to execute its process. 
    *   Update the CPU IP variable based on the PCB PC variable.
    * 
    *   pcb: the PCB to move to the CPU
    */
    extern void moveToCPU(struct PCB* pcb);

    /*
    * Function: removeFromCPU
    * ----------------------------
    *   Remove the pcb currently running on the CPU. 
    *   Update the PCB PC variable based on how many instructions the CPU has run.
    * 
    *   pcb: the PCB to remove from the CPU
    */
    extern void removeFromCPU(struct PCB* pcb);
#endif