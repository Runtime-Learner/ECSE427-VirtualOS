#ifndef KERNEL_HEADER
    #define KERNEL_HEADER

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
    extern int myinit(char* filename);

    /*
    * Function: scheduler
    * ----------------------------
    *   Handles the task switching of processes inside the ready queue. 
    *   Moves processes to/from the CPU.
    * 
    *   If a process ends, it is removed from the ready queue.
    *   The scheduler runs until the ready queue is empty.
    */
    extern int scheduler();

    /*
    * Function: emptyReadyQueue
    * ----------------------------
    *   Remove all nodes from the ReadyQueue.
    */
    extern void emptyReadyQueue();
#endif