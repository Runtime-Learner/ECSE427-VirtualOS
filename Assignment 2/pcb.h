#ifndef PCB_HEADER
    #define PCB_HEADER
    
    struct PCB {
        int ID;
        int PC;
    };

    /*
    * Function: makePCB
    * ----------------------------
    *   Create a PCB and return a pointer to it.
    *
    *   Id: The id to assign to the new PCB
    * 
    *   return: A pointer to the newly created PCB
    */
    extern struct PCB* makePCB(int Id);

    /*
    * Function: destroyPCB
    * ----------------------------
    *   Frees the memory allocated to pcb
    *
    *   pcb: the pcb we want to destroy
    */
    extern void destroyPCB(struct PCB*);
#endif