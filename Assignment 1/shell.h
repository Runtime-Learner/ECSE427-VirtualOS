#ifndef SHELL_HEADER
    #define SHELL_HEADER
    #include "terminal-io.h"

    #define NUM_TOKENS 100
    #define LINESIZE 300
    #define TOKENSIZE 200

    //call getString with '\n' as delimiter
    #define getLine(a, b, c) getString(a, b, '\n', c)

    /*
    * Function: parse
    * ----------------------------
    *   Parse string, using 'delimiter' as the token delimiter. 
    *   Double quotation marks are also handled.
    *
    *   string:    string to tokenize
    *   delimiter: the token delimiter
    *   tokens:    where to store the tokens
    *
    *   returns: Whether or not the operation was successful
    */
    extern int parse(char *string, char delimiter, char *tokens[]);

    /*
    * Function: getString
    * ----------------------------
    *   Read user input from input stream. Printable characters are saved as user input.
    *   Special characters trigger events.
    *   
    *   Special characters:
    *      tab ->                  set flag for autocomplete request
    *      L/R arrow keys ->       move left/right along user input
    *      U/D arrow keys ->       navigate through command history
    *      Ctrl-L/R arrow keys ->  move left/right by one word along user input
    * 
    *   terminalLine: stores the input stream to read from,
    *                 the character array to store user input into, 
    *                 the cursor position relative to the user input.
    * 
    *   bufferSize:   maximum number of characters allowed per command
    * 
    *   delimiter:    character that signals the end of a command
    *                 CANNOT BE TAB OR BACKSPACE
    * 
    *   echoON:       signal whether user input should be echoed onto STDOUT
    *
    *   returns: 0 if successful
    *            1 to request a tab autocomplete opeation
    *           -1 to flag that callee should stop asking for user input
    */
    extern int getString(TERMINAL_LINE* terminalLine, int bufferSize, char delimiter, int echoON);

    /*
    * Function: tokens_destroy
    * ----------------------------
    *   frees the memory allocated to all non-NULL char* elements in 'tokens' array
    *   set value of freed elements to NULL
    * 
    *   tokens: token array to clear
    */
    extern void tokens_destroy(char* tokens[]);
#endif