#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include "interpreter.h"
#include "shellmemory.h"
#include "terminal-io.h"
#include "shell.h"

static int getNumberOfOccurences(char c, char *string); //user input validation
static int character_isPrintable(char c); //ensure only desired characters are printed on screen

static volatile int SIGINT_flag = 0;

/*
 * Function: sigintHandler
 * ----------------------------
 *   Triggered when user presses Ctrl-C during the getString method.
 *   getString() alters the Terminal settings, which need to be reset before the program stops its execution.
 *   This method sets a global flag to alert getString() that Ctrl-C has been pressed
 *
 *   sig_num: signal that triggered the interrupt (parameter handled by OS)
 */
static void sigintHandler(int sig_num)
{
    SIGINT_flag = 1;
    puts("\nctrl-c has been detected...press any key to exit program");
}

/*
 * Function: main
 * ----------------------------
 *   Infinite loop. 
 *      -> get user input
 *      -> parse user input
 *      -> attempt to run user command
 * 
 *   if user pressed tab, attempt to autocomplete their input
 *
 *   returns: 0 if shell exited without error.
 *            1 otherwise.
 */
int main(void)
{
    //intro message
    printf("\n%s%s", "Welcome to the shell!\n",
           "Version 1.0 Created January 2020\n");

    //declare variables
    char *tokens[NUM_TOKENS] = {NULL};
    TERMINAL_LINE terminalLine;
    terminalLine.string = (char *)malloc(sizeof(char) * LINESIZE);
    terminalLine.cursorPosition = 0;
    terminalLine.inputStream = stdin;

    int getString_flag = 0;
    char *prompt;
    int echoSTDIN;

    //check to see whether the program is getting redirected input. The prompt will only be printed if the program is not getting redirected input
    if (isatty(STDIN_FILENO)) {
        prompt = strdup("$ ");
        echoSTDIN = 1;
    } else {
        prompt = strdup("");
        echoSTDIN = 0;
    }

    //shell loop
    while (1)
    {
        //get user input
        printf("\r%s%s", prompt, getString_flag ? terminalLine.string : "");
        getString_flag = getLine(&terminalLine, LINESIZE, echoSTDIN);

        //if Ctrl-c pressed during getString()
        if (SIGINT_flag) {
            putchar('\n');
            break;
        }

        //if reached the end of the input stream (for when input is redirected)
        if (getString_flag == -1) {
            break;
        }

        //process user input
        if (getString_flag) //attempt to tab autocomplete
        {
            parse(terminalLine.string, ' ', tokens);
            tabAutocomplete(terminalLine.string, tokens);
            terminalLine.cursorPosition = strlen(terminalLine.string);
        }
        else    //attempt to parse and interpret user input
        {
            if (!parse(terminalLine.string, ' ', tokens))
            {
                if (interpreter(tokens) == -1)
                {
                    break;
                }
            }
            //reset user input to clear terminal line
            terminalLine.string[0] = '\0';
            terminalLine.cursorPosition = 0;
        }

        //free memory
        tokens_destroy(tokens);
    }

    //cleanup before closing program
    free(prompt);
    free(terminalLine.string);
    tokens_destroy(tokens);
    memory_clear();
    history_clear();
    return 0;
}

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
int parse(char *string, char delimiter, char *tokens[])
{
    if (getNumberOfOccurences('\"', string) % 2 == 1)
    {
        setColor(RED);
        printf("Parse error: Open quotation marks detected\n");
        setColor(DEFAULT);
        tokens[0] = NULL;
        return 1;
    }
    char currentDelimiter = delimiter;
    char token[TOKENSIZE];

    unsigned short stringPos = 0, curToken = 0, tokenPos;

    while (*(string + stringPos) != '\0')
    {
        for (; *(string + stringPos) == delimiter; stringPos++)
            ; //skip all instances of delimiter

        if (*(string + stringPos) == '\0')
            break; //break out of loop if there is no token to capture after delimiters

        //parse for "" delimiter
        if (*(string + stringPos) == '\"')
        {
            currentDelimiter = '\"';
            stringPos++;
        }

        //capture token
        for (tokenPos = 0; *(string + stringPos) != currentDelimiter && *(string + stringPos) != '\0'; stringPos++, tokenPos++)
        {
            if (tokenPos >= TOKENSIZE)
            {
                token[10] = '\0';
                strcat(token, "...");
                //user input is too large to be considered a valid token
                setColor(RED);
                printf("Parse error: Invalid token: Token was too large \'%s\'\n", token);
                setColor(DEFAULT);
                tokens[curToken] = NULL;
                return 1;
            }
            token[tokenPos] = *(string + stringPos);
        }

        //stop parsing for "" delimiter
        if (*(string + stringPos) == '\"')
        {
            currentDelimiter = delimiter;
            stringPos++;
        }

        //terminate *char with NUL, copy token into token list
        token[tokenPos] = '\0';
        if (curToken < NUM_TOKENS) {
            tokens[curToken++] = strdup(token);
        }
        else {
            setColor(RED);
            printf("Parse error: Number of tokens entered exceeded the maximum amount of %d\n", NUM_TOKENS);
            setColor(DEFAULT);
            return 1;
        }
    }
    return 0;
}

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
int getString(TERMINAL_LINE *terminalLine, int bufferSize, char delimiter, int echoON)
{
    //ctrl-c is handled by our own code to ensure terminal settings are reset before program ends.
    signal(SIGINT, sigintHandler);

    //change terminal settings so our program can receive user input without having to wait for them to press enter
        static struct termios oldt, newt;

        //get current settings, save them (will restore later)
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;

        //disable buffering till /n
        newt.c_lflag &= ~(ICANON | ECHO);

        //save new settings and apply them to terminal
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    /////////////////////////////////////////////////////////////////////////

    //restore command if TAB was pressed
    int exitFlag = 0;
    terminalLine->string[terminalLine->cursorPosition] = '\0';

    //initialize variables
    int loopFlag = 0, validCharFlag = 0;
    char c;

    //get user input until they trigger an exit condition
    while (!loopFlag && !SIGINT_flag)
    {
        c = getc(terminalLine->inputStream);

        while (detectEsc_and_ArrowKeys(terminalLine, &c) == 27)
            ;
        validCharFlag = character_isPrintable(c);

        switch (c)
        {
        //reached end of input stream (input was redirected or reading from file)
        case -1:
            //if reached eof but the current line had some command, we should run it before exiting
            if (strlen(terminalLine->string) == 0) {
                exitFlag = -1; //signal that we have reached the end of the input stream
            }
            loopFlag = 1;
            validCharFlag = 0;
            break;

        //backspace (mimi terminal)
        case 8:
            if (terminalLine->cursorPosition > 0)
            {
                printf("\b \b");
                if (terminalLine->cursorPosition != strlen(terminalLine->string))
                {
                    deleteChar(terminalLine);
                }
                else
                {
                    terminalLine->string[strlen(terminalLine->string) - 1] = '\0';
                    terminalLine->cursorPosition--;
                }
            }
            break;

        //backspace
        case '\x7f':
            if (terminalLine->cursorPosition > 0) {
                printf("\b \b");
                if (terminalLine->cursorPosition != strlen(terminalLine->string)) {
                    deleteChar(terminalLine);
                }
                else {
                    terminalLine->string[strlen(terminalLine->string) - 1] = '\0';
                    terminalLine->cursorPosition--;
                }
            }
            break;

        //tab
        case '\t': 
            loopFlag = 1;
            exitFlag = 1;
            break;

        //newline
        case '\n': 
            if (delimiter == '\n'){
                loopFlag = 1;
                validCharFlag = 0;
                if (echoON) {
                    putchar('\n');  //only print character if we want input to be shown
                }
            }
            break;
        //no special character
        default:
            //check if character is delimiter. If so, we break out of the loop
            if (c == delimiter) {
                loopFlag = 1;
                validCharFlag = 0;
                if (echoON) {
                    putchar('\n');  //only print character if we want input to be shown
                }
                break;
            }

            //if character is valid and user command is not too long, append it to user command. 
            if (strlen(terminalLine->string) < bufferSize - 1) {
                if (validCharFlag) {
                    if (terminalLine->cursorPosition == strlen(terminalLine->string)) {
                        if (echoON) {   
                            putchar(c); //only print character if we want input to be shown
                        }
                        terminalLine->string[strlen(terminalLine->string) + 1] = '\0';
                        terminalLine->string[strlen(terminalLine->string)] = c;
                        terminalLine->cursorPosition++;
                    }
                    else {
                        insertChar(terminalLine, c);
                    }
                }
            }
            //User command is too long. Abort
            else { 
                loopFlag = 1;
                terminalLine->string[0] = '\0';
                setColor(RED);
                printf("\nParse error: command was too long!\n");
                setColor(DEFAULT);
            }
            break;
        }
    }
    //change exitFlag to warn callee that Ctrl-C was pressed
    if (SIGINT_flag) {  
        exitFlag = -1;
    }

    //only save commands if user typed them from keyboard
    if (exitFlag == 0 && strlen(terminalLine->string) > 0 && echoON) {  
        history_saveString(terminalLine->string);
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); //restore terminal settings
    signal(SIGINT, SIG_DFL);                 //let ctrl-c be handled by OS again

    return exitFlag;
}

/*
 * Function: character_isPrintable
 * ----------------------------
 *   Returns whether or not a character should be printed out on screen
 *   Printable characters are any characters that are alphanumeric,
 *   punctuation, or spaces.
 * 
 *   c: character to check
 *
 *   returns: wether or not the character is printable.
 */
//detect unprintable characters
static inline int character_isPrintable(char c)
{
    return (isalnum(c) || isspace(c) || ispunct(c));
}

/*
 * Function: getNumberOfOccurences
 * ----------------------------
 *   Returns the number of times a character 'c' appears in string 'string'
 *
 *   c: character to detect
 *   string: string that we want to check
 *
 *   returns: the number of occurences of 'c' in 'string'
 */
static int getNumberOfOccurences(char c, char *string)
{
    int occurences = 0;
    for (int i = 0; string[i] != '\0'; i++)
    {
        if (string[i] == c)
        {
            occurences++;
        }
    }
    return occurences;
}

/*
 * Function: tokens_destroy
 * ----------------------------
 *   frees the memory allocated to all non-NULL char* elements in 'tokens' array
 *   set value of freed elements to NULL
 * 
 *   tokens: token array to clear
 */
void tokens_destroy(char *tokens[])
{
    int index = 0;
    while (index < NUM_TOKENS && tokens[index] != NULL)
    {
        free(tokens[index]);
        tokens[index] = NULL;
        index++;
    }
}
