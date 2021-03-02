#ifndef INTERPRETER_HEADER
    #define INTERPRETER_HEADER

    #define ITEMS_PER_LINE 8
    #define NUM_FILES 200
    
    /*
    * Function: tabAutocomplete
    * ----------------------------
    *   attempt to autocomplete the user command.
    * 
    *   tokens: user command in tokenized form
    *   string: unparsed user command
    */
    extern void tabAutocomplete(char* string, char *tokens[]);


    /*
    * Function: interpreter
    * ----------------------------
    *   attempt to run the user command by comparing it to all known commands
    *      
    *   tokens: user command in tokenized form
    * 
    *   returns: 0 if command exists and ran successfully.
    *            1 otherwise.
    */
    extern int interpreter(char* tokens[]);
#endif
