#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "shellmemory.h"
#include "terminal-io.h"


/* supported shell text colors */
char* color[NUM_COLORS] = {[RED] = "\033[0;31m", [BLUE] = "\033[0;34m", [YELLOW] = "\033[1;33m", [DEFAULT] = "\033[0m"};

//update user input/terminal
static void clearTerminalLine(int strLength);
static void refreshTerminal(TERMINAL_LINE* terminalLine);
static void printStringFrom(const char* const string, int start);

//move cursor methods
static void event_arrowKey(TERMINAL_LINE* terminalLine, char* c);
static void event_arrowKey_ctrl(TERMINAL_LINE* terminalLine, char* c);
static void event_arrowKey_shift(TERMINAL_LINE* terminalLine, char* c);
static void event_arrowKey_ctrl_shift(TERMINAL_LINE* terminalLine, char* c);
static void jumpToPreviousWord(const char* const string, int* currentPos);
static void jumpToNextWord(const char* const string, int* currentPos);
static void moveCursorLeft(int moveBy, const char* const string, int* currentPos);
static void moveCursorRight(int moveBy, const char* const string, int* currentPos);
static void moveCursor(int moveBy, char direction);

/*
 * Function: setColor
 * ----------------------------
 *   Change the color of text written to STDOUT
 */
void setColor(int colorIndex) {
    printf("%s", color[colorIndex]);
}

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
void insertChar(TERMINAL_LINE* terminalLine, char c){
    int previousLength = strlen(terminalLine->string);
    terminalLine->string[previousLength + 1] = '\0';
    for (int curChar = previousLength; curChar > terminalLine->cursorPosition; curChar--){
        terminalLine->string[curChar] = terminalLine->string[curChar-1];
    }
    terminalLine->string[terminalLine->cursorPosition] = c;
    putchar(c);
    terminalLine->cursorPosition = terminalLine->cursorPosition + 1;
    refreshTerminal(terminalLine);
}

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
void deleteChar(TERMINAL_LINE* terminalLine){
    if (terminalLine->cursorPosition <= 0){
        return;
    }
    for (int curChar = terminalLine->cursorPosition - 1; terminalLine->string[curChar] != '\0'; curChar++){
        terminalLine->string[curChar] = terminalLine->string[curChar+1];
    }
    terminalLine->cursorPosition = terminalLine->cursorPosition - 1;
    refreshTerminal(terminalLine);
}

//update user input in terminal ///////////////////////////////////////////////////////////////////

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
void moveCursorToCursorPosition(TERMINAL_LINE* terminalLine) {
    clearTerminalLine(strlen(terminalLine->string));
    if (terminalLine->inputStream == stdin && isatty(STDIN_FILENO)) {
        printf(SHELL_PROMPT);
    }
    printStringFrom(terminalLine->string, 0);
    moveCursor(strlen(terminalLine->string) - terminalLine->cursorPosition, 'D');
}

/*
 * Function: refreshTerminal
 * ----------------------------
 *   Some terminal operations alter the terminal line.
 *   This method overwrites the altered area and returns the
 *   cursor to its initial position.
 *
 *   terminalLine: stores the input stream to read from,
 *                 the character array to store user input into, 
 *                 the cursor position relative to the user input.
 */
static void refreshTerminal(TERMINAL_LINE* terminalLine) {
    printStringFrom(terminalLine->string, terminalLine->cursorPosition);
    moveCursor(strlen(terminalLine->string) - terminalLine->cursorPosition, 'D');
}

/*
 * Function: clearTerminalLine
 * ----------------------------
 *   Erase all user data on the terminal line
 *
 *   strLength: size of user input to erase
 */
static void clearTerminalLine(int strLength){
    putchar('\r'); //go to begining of line
    strLength += strlen(SHELL_PROMPT); //account for shell prompt
    while (strLength-- > 0){
        putchar(' ');
    }
    putchar('\r');
}

/*
 * Function: printStringFrom
 * ----------------------------
 *   Print out a substring of 'string', starting at index
 *   'start' and ending at strlen(string)
 *
 *   string: string to print
 *   start: index of string where we want to start printing
 */
static void printStringFrom(const char* const string, int start){
    for (; string[start]!= '\0'; start++){
        putchar(string[start]);
    }
    //clean up end of terminal line
    printf("    \b\b\b\b");
}

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
char detectEsc_and_ArrowKeys(TERMINAL_LINE* terminalLine, char* c){

    //loop until current character is not ESC
    while (*c == 27) {
        *c = getc(terminalLine->inputStream);
    }

    //if next character == 91, we assume an arrow key was pressed (could be a normal keypress)
    if (*c != 91)
    {
        return *c;
    } 

    *c = getc(terminalLine->inputStream);

    if (*c == 49)   //might be crtl||shift + arrow key
    {
        *c = getc(terminalLine->inputStream);
        if (*c == 59)
        {
            *c = getc(terminalLine->inputStream);
            switch (*c)
            {
                case 50:
                    event_arrowKey_shift(terminalLine, c);
                break;
                case 53:
                    event_arrowKey_ctrl(terminalLine, c);
                break;
                case 54:
                    event_arrowKey_ctrl_shift(terminalLine, c);
                break;
            }
            *c = getc(terminalLine->inputStream);
        }
        return *c;
    }

    //if next character == 65 | 66 | 67 | 68, we pressed an arrow key
    if (*c >= 65 && *c <= 68)
    {
        event_arrowKey(terminalLine, c);
        //get next character
        *c = getc(terminalLine->inputStream);
    }
    return *c;
}

/*
 * Function: event_arrowKey
 * ----------------------------
 *   If user presses arrow keys, this function is called
 * 
 *   If user presses left:  move cursor left by one character
 *   If user presses right: move cursor right by one character
 *   If user presses up:    get the previous command stored in history
 *   If user presses down:  get the next command stored in history
 * 
 *   terminalLine: stores the input stream to read from,
 *                 the character array to store user input into, 
 *                 the cursor position relative to the user input.
 *   c: character to store input stream data into
 */
static void event_arrowKey(TERMINAL_LINE* terminalLine, char* c){
    //run cursor operation
    switch (*c) 
    {
        case 65:    //up
            //clear terminal line and print out the retrieved string
            clearTerminalLine(strlen(terminalLine->string));
            strcpy(terminalLine->string, history_getPrevious(terminalLine->string));
            terminalLine->cursorPosition = strlen(terminalLine->string);
            printf("%s%s", SHELL_PROMPT, terminalLine->string);
        break;
        case 66:    //down
            //clear terminal line and print out the retrieved string
            clearTerminalLine(strlen(terminalLine->string));
            strcpy(terminalLine->string, history_getNext(terminalLine->string));
            terminalLine->cursorPosition = strlen(terminalLine->string);
            printf("%s%s", SHELL_PROMPT, terminalLine->string);
        break;
        case 67:    //right
            moveCursorRight(1, terminalLine->string, &terminalLine->cursorPosition);
        break;
        case 68:    //left
            moveCursorLeft(1, terminalLine->string, &terminalLine->cursorPosition);
        break;
    }
}

/*
 * Function: event_arrowKey_ctrl
 * ----------------------------
 *   If user presses Ctrl-arrow keys, this function is called
 * 
 *   If user presses Ctrl-left:  move cursor left by one word
 *   If user presses Ctrl-right: move cursor right by one word
 *
 *   terminalLine: stores the input stream to read from,
 *                 the character array to store user input into, 
 *                 the cursor position relative to the user input.
 *   c: character to store input stream data into
 */
static void event_arrowKey_ctrl(TERMINAL_LINE* terminalLine, char* c){
    *c = getc(terminalLine->inputStream);
    switch (*c)
    {
        case 65:    //up
        
        break;
        case 66:    //down
        
        break;
        case 67:    //right
            jumpToNextWord(terminalLine->string, &terminalLine->cursorPosition);
        break;
        case 68:    //left
            jumpToPreviousWord(terminalLine->string, &terminalLine->cursorPosition);
        break;
    }
}

/*
 * Function: event_arrowKey_shift
 * ----------------------------
 *   copy of event_arrowKey_ctrl
 *   If user presses Shift-arrow keys, this function is called
 * 
 *   If user presses Shift-left:     move cursor left by one word
 *   If user presses Shift-right:    move cursor right by one word
 *
 *   terminalLine: stores the input stream to read from,
 *                 the character array to store user input into, 
 *                 the cursor position relative to the user input.
 *   c: character to store input stream data into
 */
static void event_arrowKey_shift(TERMINAL_LINE* terminalLine, char* c){
    *c = getc(terminalLine->inputStream);
    switch (*c)
    {
        case 65:    //up
        
        break;
        case 66:    //down
        
        break;
        case 67:    //right
            jumpToNextWord(terminalLine->string, &terminalLine->cursorPosition);
        break;
        case 68:    //left
            jumpToPreviousWord(terminalLine->string, &terminalLine->cursorPosition);
        break;
    }
}

/*
 * Function: event_arrowKey_ctrl_shift
 * ----------------------------
 *   copy of event_arrowKey_ctrl
 *   If user presses Ctrl-Shift-arrow keys, this function is called
 * 
 *   If user presses Ctrl-Shift-left:    move cursor left by one word
 *   If user presses Ctrl-Shift-right:   move cursor right by one word
 *
 *   terminalLine: stores the input stream to read from,
 *                 the character array to store user input into, 
 *                 the cursor position relative to the user input.
 *   c: character to store input stream data into
 */
static void event_arrowKey_ctrl_shift(TERMINAL_LINE* terminalLine, char* c){
    *c = getc(terminalLine->inputStream);
    switch (*c)
    {
        case 65:    //up
 
        break;
        case 66:    //down

        break;
        case 67:    //right
            jumpToNextWord(terminalLine->string, &terminalLine->cursorPosition);
        break;
        case 68:    //left
            jumpToPreviousWord(terminalLine->string, &terminalLine->cursorPosition);
        break;
    }
}

/*
 * Function: jumpToPreviousWord
 * ----------------------------
 *   Move the cursor to the left by one word. A word is
 *   defined as text that does not include whitespace.
 *   If reached the end of the line, do not go further.
 * 
 *   string: terminal line current input
 *   currentPos: current cursor position relative to user input
 */
static void jumpToPreviousWord(const char* const string, int* currentPos) {
    clearTerminalLine(strlen(string));
    printf("%s%s", SHELL_PROMPT, string);
    moveCursor(strlen(string) - *currentPos, 'D');

    int position = *currentPos - 1;
    while (position > 0 && (isspace(string[position]) || isspace(string[position]) == isspace(string[position-1]))) {
        position--;
    }
    moveCursorLeft(*currentPos - position, string, currentPos);
}

/*
 * Function: jumpToNextWord
 * ----------------------------
 *   Move the cursor to the right by one word. A word is
 *   defined as text that does not include whitespace.
 *   If reached the end of the line, do not go further.
 * 
 *   string: terminal line current input
 *   currentPos: current cursor position relative to user input
 */
static void jumpToNextWord(const char* const string, int* currentPos) {
    clearTerminalLine(strlen(string));
    printf("%s%s", SHELL_PROMPT, string);
    moveCursor(strlen(string) - *currentPos, 'D');

    int position = *currentPos + 1;
    while (position < strlen(string) && (!isspace(string[position]) || isspace(string[position]) == isspace(string[position+1]))) {
        position++;
    }
    moveCursorRight(position - *currentPos, string, currentPos);
}

//terminal user navigation/////////////////////////////////////////////////////////////////////////
/*
 * Function: moveCursorLeft
 * ----------------------------
 *   Move the cursor to the left by 'moveBy' characters.
 *   If reached the end of the line, do not go further.
 *   
 *   moveBy: how many characters to move by
 *   string: terminal line current input
 *   currentPos: current cursor position relative to user input
 */
static void moveCursorLeft(int moveBy, const char* const string, int* currentPos){
    if (*currentPos - moveBy >= 0){
        *currentPos = *currentPos - moveBy;
        moveCursor(moveBy, 'D');
        return;
    } 
    if (*currentPos == 0){
        clearTerminalLine(strlen(string));
        printf("%s%s", SHELL_PROMPT, string);
        moveCursor(strlen(string), 'D');
        return;
    }
}

/*
 * Function: moveCursorRight
 * ----------------------------
 *   Move the cursor to the right by 'moveBy' characters.
 *   If reached the end of the line, do not go further.
 *   
 *   moveBy: how many characters to move by
 *   string: terminal line current input
 *   currentPos: current cursor position relative to user input
 */
static void moveCursorRight(int moveBy, const char* const string, int* currentPos){
    if (*currentPos + moveBy <= strlen(string)){
        *currentPos = *currentPos + moveBy;
         moveCursor(moveBy, 'C');
         return;
    }  
    if (*currentPos == 0){
        clearTerminalLine(strlen(string));
        printf("%s%s", SHELL_PROMPT, string);
        moveCursor(strlen(string), 'D');
        return;
    }
    if (*currentPos == strlen(string)) {
        clearTerminalLine(strlen(string));
        printf("%s%s", SHELL_PROMPT, string);
        return;
    }
}

/*
 * Function: moveCursor
 * ----------------------------
 *   move cursor by 'moveBy' characters in the direction
 *   described by 'direction'
 *   valid directions: A = up, B = down, C = right, D = left
 * 
 *   moveBy: number of characters to move by
 *   direction: direction we want to move in
 */
static void moveCursor(int moveBy, char direction){
    if (moveBy <= 0){
        return;
    }
    if (direction != 'A' && direction != 'B' && direction != 'C' && direction != 'D'){
        return;
    }

    printf("\033[%d%c", moveBy, direction);
}
