#include "shell.h"
#ifndef TERMINALIO_HEADER
    #define TERMINALIO_HEADER

    /* supported shell text colors */
    enum COLORS {
        RED, BLUE, YELLOW, DEFAULT, NUM_COLORS 
    }COLOURS;

    /*
     * This stuct encapsulates the shell terminal line where users send input.
     * It includes the input stream to read commands from, a character pointer
     * to store user input into, and the current cursor position relative to user
     * input.
    */
    typedef struct TERMINAL_LINE{
        FILE* inputStream;
        char* string;
        int cursorPosition;
    }TERMINAL_LINE;

    /*
    * Function: moveCursorToCursorPosition
    * ----------------------------
    *   Some terminal operations alter the terminal line.
    *   This method refreshes the entire terminal line and moves the
    *   cursor to its expected position.
    *
    *   terminalLine: stores the input stream to read from,
    *                 the character array to store user input into, 
    *                 the cursor position relative to the user input.
    */
    extern void moveCursorToCursorPosition(TERMINAL_LINE* terminalLine);

    /*
    * Function: insertChar
    * ----------------------------
    *   Insert the character 'c' at cursor position.
    *   Update the user input and terminal to reflect the change.
    *
    *   c: character to insert
    *   terminalLine: stores the input stream to read from,
    *                 the character array to store user input into, 
    *                 the cursor position relative to the user input.
    */
    extern void insertChar(TERMINAL_LINE* terminalLine, char c);

    /*
    * Function: deleteChar
    * ----------------------------
    *   Delete the character located to the left of the cursor.
    *   Update the user input and terminal to reflect the change.
    *
    *   terminalLine: stores the input stream to read from,
    *                 the character array to store user input into, 
    *                 the cursor position relative to the user input.
    */
    extern void deleteChar(TERMINAL_LINE* terminalLine);

    /*
    * Function: setColor
    * ----------------------------
    *   Change the color of text written to STDOUT
    */
    extern void setColor(int color);

    /*
    * Function: detectEsc_and_ArrowKeys
    * ----------------------------
    *   ESC and arrow keys share a common initial character. 
    *   This method loops through all instances of ESC on the
    *   input stream and then checks to see if a user pressed
    *   an arrow key.
    *  
    *   Calls the appropriate event_arrowKey() method depending
    *   on whether Ctrl and/or Shift was pressed when arrow keys
    *   were pressed.
    *
    *   terminalLine: stores the input stream to read from,
    *                 the character array to store user input into, 
    *                 the cursor position relative to the user input.
    *   c: character to store input stream data into
    */
    extern char detectEsc_and_ArrowKeys(TERMINAL_LINE* terminalLine, char* c);
#endif