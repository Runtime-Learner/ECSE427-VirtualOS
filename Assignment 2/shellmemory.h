#ifndef SHELLMEMORY_HEADER
    #define SHELLMEMORY_HEADER
// SHELL MEMORY ///////////////////////////////////////////////
    /*
    * Function: setVar
    * ----------------------------
    *   Update variable 'var' with the value 'value'.
    *   Create variable if it does not exist.
    *
    *   var: name of variable
    *   value: value to assign to variable
    *
    *   returns: whether or not the operation was successful
    */
    extern int setVar(char *var, char *value);

    /*
    * Function: getVar
    * ----------------------------
    *   Returns the value of variable 'var'
    *
    *   var: name of variable
    *
    *   returns: the value of variable 'var'. 
    *            NULL if var does not exist.
    */
    extern char *getVar(char *var);

    /*
    * Function: memory_clear
    * ----------------------------
    *   Frees all memory allocated to console memory
    *
    *   returns: whether the operation was successful or not
    */
    extern int memory_clear();

// SHELL COMMAND HISTORY //////////////////////////////////////
    /*
    * Function: history_saveString
    * ----------------------------
    *   Save the string to shell command history
    *
    *   string: the string to save to memory
    *
    *   returns: whether the operation was successful or not 
    */
    extern int history_saveString(char *string);

    /*
    * Function: history_getPrevious
    * ----------------------------
    *   Returns the previous command relative to current position in history.
    *   Save 'string' to history if currently at history HEAD in order
    *   to be able to restore user input if they choose to return to it.
    *
    *   string: current string displayed on terminal
    *
    *   returns: the previous command stored in shell command history relative
    *            to the user's position in the history.
    */
    extern char *history_getPrevious(char *string);

    /*
    * Function: history_getNext
    * ----------------------------
    *   Returns the next command relative to current position in history.
    *   Return 'string' if currently at history HEAD.
    * 
    *   string: current string displayed on terminal
    *
    *   returns: the next command stored in shell command history relative
    *            to the user's position in the history.
    */
    extern char *history_getNext();

    /*
    * Function: history_clear
    * ----------------------------
    *   Frees all memory allocated to command history
    *
    *   returns: whether the operation was successful or not
    */
    extern int history_clear();
#endif