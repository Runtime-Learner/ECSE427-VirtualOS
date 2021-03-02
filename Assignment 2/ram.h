#ifndef RAM_HEADER
    #define RAM_HEADER

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
    extern int requestProcId(int size);

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
    extern int freeProcId(int ID);

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
    extern int requestProcRam_size(int procId);

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
    extern int requestProcRam_start(int procId);

    /*
    * Function: readRamLocation
    * ----------------------------
    *   Return the data stored at RAM location specified
    * 
    *   location: index in RAM
    * 
    *   return: the data stored at RAM location
    */
    extern char* readRamLocation(int location);
        
    /*
    * Function: addToRAM
    * ----------------------------
    *   Load file data into a RAM datablock associated with the specified process
    * 
    *   p: the file to read
    *   vRamId: the processId associated with the RAM datablock we are filling
    */
    extern void addToRAM(FILE* p, int vRamId);

    /*
    * Function: initializeVram
    * ----------------------------
    *   register the RAM cleanup function to be called at program termination.
    *   Initialize RAM by creating on RAM datablock of size RAM_SIZE.
    */
    extern void initializeVram();

    /*
    * Function: ram_clear
    * ----------------------------
    *   Destroy all RAM datablocks. Free all data stored in RAM.
    */
    extern void ram_clear();
#endif