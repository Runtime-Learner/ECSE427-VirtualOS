#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include "shellmemory.h"
#include "shell.h"
#include "terminal-io.h"
#include "interpreter.h"
#include "kernel.h"

static int tabAutocomplete_withCommand(char *string, char *tokens[], int cursorPosition, int tokenToAutocomplete);
static char * autocompleteToken(char* token, const char* listOfTerms[], int numberOfTerms);
int tabAutocomplete(char *string, char *tokens[], int cursorPosition);

static int tokenlen(char *tokens[]);
static int stokenlen(const char *tokens[]);
static int validateNumberOfParameters(char *tokens[], int commandId, int expectedNumberOfParameters);
static int validateNumberOfParameters_range(char *tokens[], int commandId, int minNumberOfParameters, int maxNumberOfParameters);
static char* getLastOccurence(char *string, char *substring);
static char* getLargestCommonSubstring(const char *strings[], const char *initialSubstring);
static int getTokenBasedOnCursor(char *string, char *tokens[], int cursorPosition);
static int getLocationOfToken(char *string, char *tokens[], int tokenNumber);

//COMMANDSET must be the last element in the enum in order for it to equal the number of commands
static enum commandSet
{
    CLEAR,
    ECHO,
    EXEC,
    HELP,
    PRINT,
    QUIT,
    RUN,
    SET,
    COMMANDSET
} commandId;

//command functions
static int echo(char *tokens[]);
static int help(char *tokens[]);
static int quit(char *tokens[]);
static int set(char *tokens[]);
static int print(char *tokens[]);
static int run(char *tokens[]);
static int clear(char *tokens[]);
static int exec(char* tokens[]);

//command autocomplete functions
static int help_autocomplete(char *string, char *tokens[], int cursorPosition, int tokenToAutocomplete);
static int run_autocomplete(char *string, char *tokens[], int cursorPosition, int tokenToAutocomplete);
static int exec_autocomplete(char *string, char*tokens[], int cursorPosition, int tokenToAutocomplete);

//array of command functions
static const int const (*command_functions[COMMANDSET])(char *tokens[]) = 
{[HELP] = help, [QUIT] = quit, [SET] = set, [PRINT] = print, [RUN] = run, [CLEAR] = clear, [ECHO] = echo, [EXEC] = exec};
//array of command-specific autocomplete functions
static const int const (*command_autocompleteFunctions[COMMANDSET])(char *string, char *tokens[], int cursorPosition, int tokenToAutocomplete) = 
{[HELP] = help_autocomplete, [QUIT] = NULL, [SET] = NULL, [PRINT] = NULL, [RUN] = run_autocomplete, [CLEAR] = NULL, [ECHO] = NULL, [EXEC] = exec_autocomplete};
//array of command names
static const char const *commands[COMMANDSET] = 
{[HELP] = "help", [QUIT] = "quit", [SET] = "set", [PRINT] = "print", [RUN] = "run", [CLEAR] = "clear", [ECHO] = "echo", [EXEC] = "exec"};


//commands_description is used by 'help' command to display information
static const char const *commands_description[COMMANDSET * 2] = {
    [HELP] = "help [COMMAND]",
    [HELP +
        COMMANDSET] = "Displays all the commands - prints out details about [COMMAND]",
    [QUIT] = "quit",
    [QUIT +
        COMMANDSET] = "Exits / terminates the shell",
    [SET] = "set VAR STRING",
    [SET +
        COMMANDSET] = "Assigns value STRING to shell variable VAR",
    [PRINT] = "print VAR",
    [PRINT +
        COMMANDSET] = "Displays the value assigned to VAR",
    [RUN] = "run SCRIPT.TXT",
    [RUN +
        COMMANDSET] = "Executes the file SCRIPT.TXT",
    [CLEAR] = "clear",
    [CLEAR +
        COMMANDSET] = "Clears the terminal screen",
    [ECHO] = "echo STRING",
    [ECHO +
        COMMANDSET] = "Print STRING on a new line",
    [EXEC]= "exec program1 [program2] [program3]",
    [EXEC +
        COMMANDSET] = "run programs concurrently"};

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
int interpreter(char *tokens[])
{
    if (tokenlen(tokens) == 0)
    {
        return 0;
    }

    //compare first token to all known commands. If a match is found, run that command
    for (int i = 0; i < COMMANDSET; i++)
    {
        if (!strcmp(commands[i], tokens[0]))
        {
            return (*command_functions[i])(tokens);
        }
    }

    //no match was found
    setColor(RED);
    printf("Unknown command \'%s\'\n", tokens[0]);
    setColor(DEFAULT);
    return 1;
}

/*
 * Function: autocompleteToken
 * ----------------------------
 *   Attempt to autocomplete a token with terms from a specified list.
 *   Returns the resulting string.
 *
 *   token:         token to autocomplete
 *   listOfTerms:   list of terms that we will compare to token
 *   numberOfTerms: number of terms in listOfTerms
 * 
 *   return: token if no autocomplete was possible. The new, autocompleted
 *   string is returned otherwise.
 */
static char * autocompleteToken(char* token, const char* listOfTerms[], int numberOfTerms) {
    //initialize array of autocomplete options
    const char **autoCompleteOptions = (const char**) malloc(sizeof(char*) * numberOfTerms);
    for (int i = 0; i < numberOfTerms; i++) {
        autoCompleteOptions[i] = NULL;
    }

    //get all possibilities for autocomplete
    size_t foundAutocompletePossibilities = 0;
    int sizeOfUserToken = strlen(token);
    for (size_t currentTerm = 0; currentTerm < numberOfTerms; currentTerm++)
    {
        if (strstr(listOfTerms[currentTerm], token) == listOfTerms[currentTerm])
        {
            autoCompleteOptions[foundAutocompletePossibilities++] = listOfTerms[currentTerm];
        }
    }

    if (foundAutocompletePossibilities > 0)
    {
        if (foundAutocompletePossibilities == 1) //if only one possible command, just go ahead and autocomplete
        {
            char* autocompletedToken = strdup(autoCompleteOptions[0]);
            free(autoCompleteOptions);
            return autocompletedToken;
        }
        else //if multiple possibilities, print them on screen
        {
            int alreadyLargestCommonString = 1;
            char *largestCommonString = NULL;
            if (strlen(token))
            {
                largestCommonString = getLargestCommonSubstring(autoCompleteOptions, token);    //autocomplete as much as possible (largest common substring)
                alreadyLargestCommonString = (strlen(largestCommonString) == strlen(token));
            } else {
                largestCommonString = strdup("");
            }
            if (alreadyLargestCommonString)
            {
                putchar('\n');
                size_t counter = 0;
                while (counter < foundAutocompletePossibilities)
                {
                    printf("%s    ", autoCompleteOptions[counter++]);
                    if (counter % ITEMS_PER_LINE == 0) {
                        putchar('\n');
                    }
                }
                if (counter % ITEMS_PER_LINE != 0) {
                    putchar('\n');
                }
                
            }
            free(autoCompleteOptions);
            return largestCommonString;
        }
    }

    //return the unaltered token if no autocomplete option found
    free(autoCompleteOptions);
    return strdup(token);
}


/*
 * Function: tabAutocomplete_withCommand
 * ----------------------------
 *   check to see if first user token is a valid command.
 *   if it is, run the command specific autocomplete function.
 *      
 *   tokens: user command in tokenized form
 *   string: unparsed user command
 *   cursorPosition: position of cursor relative to beginning of string
 *   tokenToAutocomplete: the index of the token the user is attempting to autocomplete
 * 
 *   return: how many characters we should move the cursor by to get to the end of the token that was autocompleted.
 */
static int tabAutocomplete_withCommand(char *string, char *tokens[], int cursorPosition, int tokenToAutocomplete)
{
    //compare first token to all known commands. If a match is found, run that command
    for (int i = 0; i < COMMANDSET; i++)
    {
        if (!strcmp(commands[i], tokens[0]))
        {
            if (command_autocompleteFunctions[i] != NULL)
            {
                return (*command_autocompleteFunctions[i])(string, tokens, cursorPosition, tokenToAutocomplete);
            }
            return 0;
        }
    }
    return 0;
}

/*
 * Function: tabAutocomplete
 * ----------------------------
 *   attempt to autocomplete the user command.
 *   Calls tabAutocomplete_withCommand if user 
 *   is attempting to autocomplete a command specific parameter
 * 
 *   tokens: user command in tokenized form
 *   string: unparsed user command
 *   cursorPosition: position of cursor relative to beginning of string
 * 
 *   return: how many characters we should move the cursor by to get to the end of the token that was autocompleted.
 */
int tabAutocomplete(char *string, char *tokens[], int cursorPosition)
{
    int tokenToAutocomplete = getTokenBasedOnCursor(string, tokens, cursorPosition);

    //if user has input more than one token, run a command specific autocomplete
    if (tokens != NULL && (tokenlen(tokens) > 1))
    {
        return tabAutocomplete_withCommand(string, tokens, cursorPosition, tokenToAutocomplete);
    }
    
    char* autocomplete = autocompleteToken(tokens[tokenToAutocomplete], commands, COMMANDSET);
    int tokenPosition = cursorPosition;
    if (strlen(tokens[tokenToAutocomplete]) != 0) {
        tokenPosition = getLocationOfToken(string, tokens, tokenToAutocomplete);
    }

    //alter user string to reflect autocomplete changes
    int moveCursorBy = 0;
    char test[500] = "";
    strncpy(test, string, tokenPosition);
    strcat(test, autocomplete);
    moveCursorBy = strlen(test) - cursorPosition;
    strcat(test, (string+tokenPosition+strlen(tokens[tokenToAutocomplete])));
    strcpy(string, test);
    free(autocomplete);
    return moveCursorBy;
}


/*
 * Function: help_autocomplete
 * ----------------------------
 *   attempt to autocomplete the second parameter of 'help'.
 *   autocomplete in exactly the same way as tabAutocomplete()
 *      
 *   tokens: user command in tokenized form
 *   string: unparsed user command
 *   cursorPosition: position of cursor relative to beginning of string
 *   tokenToAutocomplete: the index of the token the user is attempting to autocomplete
 * 
 *   return: how many characters we should move the cursor by to get to the end of the token that was autocompleted.
 */
static int help_autocomplete(char *string, char *tokens[], int cursorPosition, int tokenToAutocomplete)
{
    int numberOfTokens = tokenlen(tokens);
    if (numberOfTokens > 2) {
        return 0;
    }

    char* autocomplete = autocompleteToken(tokens[tokenToAutocomplete], commands, COMMANDSET);
    int tokenPosition = cursorPosition;
    if (strlen(tokens[tokenToAutocomplete]) != 0) {
        tokenPosition = getLocationOfToken(string, tokens, tokenToAutocomplete);
    }

    //alter user string to reflect autocomplete changes
    int moveCursorBy = 0;
    char test[500] = "";
    strncpy(test, string, tokenPosition);
    strcat(test, autocomplete);
    moveCursorBy = strlen(test) - cursorPosition;
    strcat(test, (string+tokenPosition+strlen(tokens[tokenToAutocomplete])));
    strcpy(string, test);
    free(autocomplete);
    return moveCursorBy;
}

/*
 * Function: help
 * ----------------------------
 *    no parameters:
 *      print out all valid commands and their format
 * 
 *   [command] parameter given:
 *      print out [command] and its valid format
 *
 *   tokens: user command in tokenized form
 *
 *   returns: 0 if exited with no error. 1 otherwise
 */

static int help(char *tokens[])
{
    if (!validateNumberOfParameters_range(tokens, HELP, 1, 2))
    {
        return 1;
    }
    if (tokenlen(tokens) == 1)
    {
        for (int i = 0; i < COMMANDSET; i++)
        {
            printf("%35s    %s\n", commands_description[i], commands_description[i + COMMANDSET]);
        }
        return 0;
    }
    else
    {
        for (size_t currentCommand = 0; currentCommand < COMMANDSET; currentCommand++)
        {
            if (strstr(commands[currentCommand], tokens[1]) == commands[currentCommand] && strlen(tokens[1]) == strlen(commands[currentCommand]))
            {
                printf("      %s    %s\n", commands_description[currentCommand], commands_description[currentCommand + COMMANDSET]);
                return 0;
            }
        }
        //no match was found
        setColor(RED);
        printf("Unknown command \'%s\'\n", tokens[1]);
        setColor(DEFAULT);
        return 1;
    }
}

/*
 * Function: quit
 * ----------------------------
 *   Print "bye!"
 *   Return -1 to signal to callee that it should stop asking
 *   for user input.
 *
 *   tokens: user command in tokenized form
 *
 *   returns: 1 if number of parameters is incorrect. 
 *           -1 otherwise.
 */
static int quit(char *tokens[])
{
    if (!validateNumberOfParameters(tokens, QUIT, 1))
    {
        return 1;
    }
    puts("bye!");
    return -1; //signal for main loop to quit
}

/*
 * Function: set
 * ----------------------------
 *   Save variable VAR with value STRING.
 *   Throw an error if VAR is empty
 *   Throw an error if variable could not be saved.
 *
 *   tokens: user command in tokenized form
 *
 *   returns: 0 if exited with no error. 1 otherwise
 */
static int set(char *tokens[])
{
    if (!validateNumberOfParameters(tokens, SET, 3))
    {
        return 1;
    }
    if (strlen(tokens[1]) == 0)
    { //do not save variable, it is illegal for VAR to be empty
        setColor(RED);
        puts("Variable name cannot be empty");
        setColor(DEFAULT);
        return 1;
    }

    int error = setVar(tokens[1], tokens[2]);
    if (error)
    {
        setColor(RED);
        puts("Error saving variable: memory is full");
        setColor(DEFAULT);
        return 1;
    }
    char *result = getVar(tokens[1]);
    if (!result)
    {
        setColor(RED);
        puts("Error retrieving variable. Please try saving again");
        setColor(DEFAULT);
        return 1;
    }
    printf("%s = %s\n", tokens[1], result);
    free(result);
    return 0;
}

/*
 * Function: print
 * ----------------------------
 *   Print out the value of VAR on a new line.
 *   if VAR does not exist, print out an error.
 *
 *   tokens: user command in tokenized form
 *
 *   returns: 0 if exited with no error. 1 otherwise
 */
static int print(char *tokens[])
{
    if (!validateNumberOfParameters(tokens, PRINT, 2))
    {
        return 1;
    }
    char *result = getVar(tokens[1]);
    if (!result)
    {
        setColor(RED);
        puts("Variable does not exist");
        setColor(DEFAULT);
        return 1;
    }
    printf("%s\n", result);
    free(result);
    return 0;
}

/*
 * Function: echo
 * ----------------------------
 *   Print out STRING on a new line.
 *   output is BLUE
 *
 *   tokens: user command in tokenized form
 *
 *   returns: 0 if exited with no error. 1 otherwise
 */
static int echo(char *tokens[])
{
    if (!validateNumberOfParameters(tokens, ECHO, 2))
    {
        return 1;
    }
    setColor(BLUE);
    printf("%s\n", tokens[1]);
    setColor(DEFAULT);
    return 0;
}

/*
 * Function: run_autocomplete
 * ----------------------------
 *   Attempt to autocomplete the filename user is planning
 *   to run.
 *
 *   tokens: user command in tokenized form
 *   string: unparsed user command
 *   cursorPosition: position of cursor relative to beginning of string
 *   tokenToAutocomplete: the index of the token the user is attempting to autocomplete
 * 
 *   return: how many characters we should move the cursor by to get to the end of the token that was autocompleted.
 */
static int run_autocomplete(char *string, char *tokens[], int cursorPosition, int tokenToAutocomplete)
{
    int numberOfTokens = tokenlen(tokens);
    if (numberOfTokens > 2 || tokenToAutocomplete != 1) {
        return 0;
    }

    // Get directory object
    struct dirent *de;
    DIR *dr;
    
    char *fileNameOffset = tokens[tokenToAutocomplete];
    char directory[TOKENSIZE] = "./";

    if (strlen(tokens[tokenToAutocomplete]) == 0)
    {
        dr = opendir(directory);
    }
    else
    {
        if (strstr(tokens[tokenToAutocomplete], "/") == NULL)
        {
            dr = opendir(directory);
        }
        else
        {
            strcat(directory, tokens[tokenToAutocomplete]);
            *(getLastOccurence(directory, "/") + 1) = '\0';
            dr = opendir(directory);
            fileNameOffset = (tokens[tokenToAutocomplete] + strlen(directory) - 2);
        }
    }

    if (dr == NULL) // opendir returns NULL if couldn't open directory
    {
        setColor(RED);
        printf("\nDirectory not found\n");
        setColor(DEFAULT);
        return 0;
    }

    //store names of all files + directories in array
    char *files[NUM_FILES] = {NULL};
    int numberOfFiles = 0;
    while ((de = readdir(dr)) != NULL)
    {
        files[numberOfFiles] = strdup(de->d_name);
        if (de->d_type == 4)
        {
            strcat(files[numberOfFiles], "/");  //add / to directory entries in order to make autocomplete more usable
        }
        numberOfFiles++;
    }
    closedir(dr);

    char* autocomplete = autocompleteToken(fileNameOffset, (const char**) files, numberOfFiles);
    int tokenPosition = cursorPosition;
    if (strlen(tokens[tokenToAutocomplete]) != 0) {
        tokenPosition = getLocationOfToken(string, tokens, tokenToAutocomplete);
    }

    //alter user string to reflect autocomplete changes
    int moveCursorBy = 0;
    char test[500] = "";
    strncpy(test, string, tokenPosition);
    if (tokenPosition != 0 && string[tokenPosition - 1] != '\"') {
        strcat(test, "\"");
    }
    if ((strlen(tokens[tokenToAutocomplete]) >= 2 && tokens[tokenToAutocomplete][0] == '.' && tokens[tokenToAutocomplete][1] == '/')) {
        strcat(test, (directory+2));
    } else {
        strcat(test, directory);
    }
    strcat(test, autocomplete);
    moveCursorBy = strlen(test) - cursorPosition;
    if (*(string+tokenPosition+strlen(tokens[tokenToAutocomplete])) != '\"') {
        strcat(test, "\"");
    }
    strcat(test, (string+tokenPosition+strlen(tokens[tokenToAutocomplete])));
    strcpy(string, test);
    free(autocomplete);
    return moveCursorBy;
}

/*
 * Function: run
 * ----------------------------
 *   Attempt to open a command script and run it.
 *   Autocomplete exists for this command.
 *
 *   tokens: user command in tokenized form
 *
 *   returns: 0 if exited with no error. 1 otherwise
 */
static int run(char *tokens[])
{
    if (!validateNumberOfParameters(tokens, RUN, 2))
    {
        return 1;
    }

    FILE *script = NULL;
    script = fopen(tokens[1], "r");
    if (!script)
    {
        setColor(RED);
        printf("run: Script \'%s\' not found\n", tokens[1]);
        setColor(DEFAULT);
        return 1;
    }

    char *scriptTokens[NUM_TOKENS] = {NULL};
    TERMINAL_LINE terminalLine;
    terminalLine.string = (char *)malloc(sizeof(char) * LINESIZE);
    strcpy(terminalLine.string, "");
    terminalLine.cursorPosition = 0;
    terminalLine.inputStream = script;

    int flag = 0;
    int error = 0;
    int lineNumber = 1;
    while (1)
    {
        //get user input
        error = getLine(&terminalLine, LINESIZE, 0);

        if (error == -1)
        {
            error = 0;
            break;
        }

        //process user input
        error = parse(terminalLine.string, ' ', scriptTokens);
        if (!error)
        {
            error = interpreter(scriptTokens);
        }

        if (error == -1)
        {
            error = 0;
            break;
        }

        //process errors
        if (error)
        {
            for (int i = 0; i < error; i++) {
                printf("  ");
            }
            setColor(YELLOW);
            printf("at \'%s\':line %d: %s\n", tokens[1], lineNumber, terminalLine.string);
            setColor(DEFAULT);
            error++; //increment error to let caller function know how deep the error stack is
            break;
        }

        terminalLine.string[0] = '\0';
        terminalLine.cursorPosition = 0;
        lineNumber++;
        tokens_destroy(scriptTokens);
    }
    fclose(script);
    free(terminalLine.string);
    tokens_destroy(scriptTokens);
    return error;
}

/*
 * Function: clear
 * ----------------------------
 *   Call system("clear")
 *   clear the terminal
 *
 *   tokens: user command in tokenized form
 *
 *   returns: 0 if number of parameters was valid. 1 otherwise.
 */
static int clear(char *tokens[])
{
    if (!validateNumberOfParameters(tokens, CLEAR, 1))
    {
        return 1;
    }
    system("clear"); // Clear screen
}

/*
 * Function: exec
 * ----------------------------
 *   Execute multiple scripts concurrently (3 at most).
 *   Scripts with the same name cannot be run at the same time.
 *
 *   tokens: user command in tokenized form
 *
 *   returns: 1 if an error is encountered. 0 otherwise.
 */
static int exec(char* tokens[]) {
    if (!validateNumberOfParameters_range(tokens, EXEC, 2, 4)) {
        return 1;
    }
    //check that none of the scripts we want to run have the same file name. Files with the
    //same name stored in different directories will be treated as if they were the same files.
    char* program1 = getLastOccurence(tokens[1], "/");
    if (program1 == NULL) {
        program1 = tokens[1];
    } else {
        program1++;
    }
    char* program2 = getLastOccurence(tokens[2], "/");
    if (program2 == NULL) {
        program2 = tokens[2];
    } else {
        program2++;
    }
    char* program3 = getLastOccurence(tokens[3], "/");
    if (program3 == NULL) {
        program3 = tokens[3];
    } else {
        program3++;
    }
    if (tokens[2]) {
        if (strcmp(program1, program2) == 0) {
            setColor(RED);
            printf("Error: Script \'%s\' already loaded\n", tokens[2]);
            setColor(DEFAULT);
            return 1;
        }
    }
    if (tokens[3]) {
        if (strcmp(program1, program3) == 0) {
            setColor(RED);
            printf("Error: Script \'%s\' already loaded\n", tokens[3]);
            setColor(DEFAULT);
            return 1;
        }
        if (strcmp(program2, program3) == 0) {
            setColor(RED);
            printf("Error: Script \'%s\' already loaded\n", tokens[2]);
            setColor(DEFAULT);
            return 1;
        }
    }

    //run myinit for all user parameters
    if (tokens[1][strlen(tokens[1]) - 1] == '/' || myinit(tokens[1])) {
        printf("could not load program1!\n");
        return 1;
    }
    if (tokens[2] && (tokens[2][strlen(tokens[2]) - 1] == '/' || myinit(tokens[2]))) {
        printf("could not load program2!\n");
        emptyReadyQueue();  //exec will not run. Should cleanup ready queue
        return 1;
    }
    if (tokens[3] && (tokens[3][strlen(tokens[3]) - 1] == '/' || myinit(tokens[3]))) {
        printf("could not load program3!\n");
        emptyReadyQueue();  //exec will not run. Should cleanup ready queue
        return 1;
    }


    return scheduler();
}

/*
 * Function: exec_autocomplete
 * ----------------------------
 *   Attempt to autocomplete the filenames user is planning
 *   to execute.
 *
 *   tokens: user command in tokenized form
 *   string: unparsed user command
 *   cursorPosition: position of cursor relative to beginning of string
 *   tokenToAutocomplete: the index of the token the user is attempting to autocomplete
 * 
 *   return: how many characters we should move the cursor by to get to the end of the token that was autocompleted.
 */
static int exec_autocomplete(char *string, char *tokens[], int cursorPosition, int tokenToAutocomplete) {
    int numberOfTokens = tokenlen(tokens);
    if (numberOfTokens > 4 || tokenToAutocomplete == 0) {
        return 0;
    }

    // Get directory object
    struct dirent *de;
    DIR *dr;
    
    char *fileNameOffset = tokens[tokenToAutocomplete];
    char directory[TOKENSIZE] = "./";

    if (strlen(tokens[tokenToAutocomplete]) == 0)
    {
        dr = opendir(directory);
    }
    else
    {
        if (strstr(tokens[tokenToAutocomplete], "/") == NULL)
        {
            dr = opendir(directory);
        }
        else
        {
            strcat(directory, tokens[tokenToAutocomplete]);
            *(getLastOccurence(directory, "/") + 1) = '\0';
            dr = opendir(directory);
            fileNameOffset = (tokens[tokenToAutocomplete] + strlen(directory) - 2);
        }
    }

    if (dr == NULL) // opendir returns NULL if couldn't open directory
    {
        setColor(RED);
        printf("\nDirectory not found\n");
        setColor(DEFAULT);
        return 0;
    }

    //store names of all files + directories in array
    char *files[NUM_FILES] = {NULL};
    int numberOfFiles = 0;
    while ((de = readdir(dr)) != NULL)
    {
        files[numberOfFiles] = strdup(de->d_name);
        if (de->d_type == 4)
        {
            strcat(files[numberOfFiles], "/");  //add / to directory entries in order to make autocomplete more usable
        }
        numberOfFiles++;
    }
    closedir(dr);

    char* autocomplete = autocompleteToken(fileNameOffset, (const char**) files, numberOfFiles);
    int tokenPosition = cursorPosition;
    if (strlen(tokens[tokenToAutocomplete]) != 0) {
        tokenPosition = getLocationOfToken(string, tokens, tokenToAutocomplete);
    }

    //alter user string to reflect autocomplete changes
    int moveCursorBy = 0;
    char test[500] = "";
    strncpy(test, string, tokenPosition);
    if (tokenPosition != 0 && string[tokenPosition - 1] != '\"') {
        strcat(test, "\"");
    }
    if ((strlen(tokens[tokenToAutocomplete]) >= 2 && tokens[tokenToAutocomplete][0] == '.' && tokens[tokenToAutocomplete][1] == '/')) {
        strcat(test, (directory+2));
    } else {
        strcat(test, directory);
    }
    strcat(test, autocomplete);
    moveCursorBy = strlen(test) - cursorPosition;
    if (*(string+tokenPosition+strlen(tokens[tokenToAutocomplete])) != '\"') {
        strcat(test, "\"");
    }
    strcat(test, (string+tokenPosition+strlen(tokens[tokenToAutocomplete])));
    strcpy(string, test);
    free(autocomplete);
    return moveCursorBy;
}

/*
 * Function: validateNumberOfParameters_range
 * ----------------------------
 *   verifies that the number of parameters passed to the command is valid. Print an error otherwise.
 *   Print out valid format and a generic help message.
 *  
 *   used command expects AT LEAST minNumberOfParameters parameters, and AT MOST maxNumberOfParameters.
 *  
 *   tokens:    tokens representing the user command.
 *   commandId: index of command to reference it inside commands[] array.
 *   minNumberOfParameters: minimum number of parameters the command expects to receive.
 *   maxNumberOfParameters: maximum number of parameters the command expects to receive.
 * 
 *   returns: 0 if number of parameters was valid. 1 otherwise.
 */
static int validateNumberOfParameters_range(char *tokens[], int commandId, int minNumberOfParameters, int maxNumberOfParameters)
{
    if (tokenlen(tokens) > maxNumberOfParameters)
    {
        setColor(RED);
        printf("Error: \'%s\' expected at most %d argument%s, but received %d\n", commands[commandId], maxNumberOfParameters - 1,
               maxNumberOfParameters == 2 ? "" : "s", tokenlen(tokens) - 1);
        printf("\'%s\' valid format:\n> %s\n", commands[commandId], commands_description[commandId]);
        setColor(DEFAULT);
        printf("Type \'help %s\' for more information\n", commands[commandId]);
        return 0;
    }
    if (tokenlen(tokens) < minNumberOfParameters)
    {
        setColor(RED);
        printf("Error: \'%s\' expected a minimum of %d argument%s, but received %d\n", commands[commandId], minNumberOfParameters - 1,
               minNumberOfParameters == 2 ? "" : "s", tokenlen(tokens) - 1);
        printf("\'%s\' valid format:\n> %s\n", commands[commandId], commands_description[commandId]);
        setColor(DEFAULT);
        printf("Type \'help %s\' for more information\n", commands[commandId]);
        return 0;
    }
    return 1;
}

/*
 * Function: validateNumberOfParameters
 * ----------------------------
 *   verifies that the number of parameters passed to a command is valid. Print an error otherwise.
 *   Print out valid format and a generic help message.
 * 
 *   tokens:    tokens representing the user command.
 *   commandId: index of command to reference it inside commands[] array.
 *   expectedNumberOfParameters: number of parameters the command expects to receive.
 *
 *   returns: 0 if number of parameters was valid. 1 otherwise.
 */
static int validateNumberOfParameters(char *tokens[], int commandId, int expectedNumberOfParameters)
{
    if (tokenlen(tokens) != expectedNumberOfParameters)
    {
        setColor(RED);
        printf("Error: \'%s\' expected %d argument%s, but received %d\n", commands[commandId], expectedNumberOfParameters - 1,
               expectedNumberOfParameters == 2 ? "" : "s", tokenlen(tokens) - 1);
        printf("\'%s\' valid format:\n> %s\n", commands[commandId], commands_description[commandId]);
        setColor(DEFAULT);
        printf("Type \'help %s\' for more information\n", commands[commandId]);
        return 0;
    }
    return 1;
}

/*
 * Function: tokenlen
 * ----------------------------
 *   Return the number of strings stored inside tokens[]
 *
 *   tokens: array of strings whose length we want
 *
 *   returns: the number of strings stored inside tokens[]
 */
static int tokenlen(char *tokens[])
{
    int length = 0;
    while (length < NUM_TOKENS && tokens[length++] != NULL)
        ;
    return length - 1;
}

/*
 * Function: ctokenlen
 * ----------------------------
 *   Return the number of strings stored inside tokens[]
 *
 *   tokens: const array of strings whose length we want
 *
 *   returns: the number of strings stored inside tokens[]
 */
static int ctokenlen(const char *tokens[])
{
    int length = 0;
    while (length < NUM_TOKENS && tokens[length++] != NULL)
        ;
    return length - 1;
}

/*
 * Function: getLastOccurence
 * ----------------------------
 *   Return the memory address of the last occurence of 'substring' in 'string'
 *
 *   string:    string to search
 *   substring: string to find
 *
 *   returns: the memory address of the last occurence of 'substring' in 'string'
 *            NULL if substring is not in string
 */
static char *getLastOccurence(char *string, char *substring)
{
    if (string == NULL || substring == NULL) {
        return NULL;
    }

    //need to get a pointer to the substring in the string
    char *ptrToTokenLocation = strstr(string, substring);
    if (!ptrToTokenLocation)
    {
        return NULL;
    }

    //get last occurence of of tokens[1]
    while (strstr(ptrToTokenLocation, substring) != NULL)
    {
        ptrToTokenLocation = strstr(ptrToTokenLocation, substring);
        ptrToTokenLocation++;
    }
    ptrToTokenLocation--;
    return ptrToTokenLocation;
}

/*
 * Function: getLargestCommonSubstring
 * ----------------------------
 *   Return the largest common substring of all the strings contained in the array strings[]
 *
 *   strings:           array of strings with a common substring
 *   initialSubstring:  our initial guess for the largest common substring
 *
 *   returns: The largest common substring of all the strings contained in the array strings[]
 */
static char* getLargestCommonSubstring(const char *strings[], const char *initialSubstring)
{
    char substring[TOKENSIZE];
    strcpy(substring, initialSubstring);
    int numberOfStrings = ctokenlen(strings);
    int matchingStrings = numberOfStrings;
    int strLen;
    int loopRan_flag = 0;
    while (matchingStrings == numberOfStrings && strlen(strings[0]) > strlen(substring))
    {
        loopRan_flag = 1;
        matchingStrings = 0;
        strLen = strlen(substring);
        substring[strLen + 1] = '\0';
        substring[strLen] = strings[0][strLen];
        for (size_t current = 0; current < numberOfStrings; current++)
        {
            if (strstr(strings[current], substring) == strings[current])
            {
                matchingStrings++;
            }
        }
        
    }
    if (loopRan_flag)
    {
        substring[strlen(substring) - 1] = '\0';
    }
    return strdup(substring);
}

/*
 * Function: getLocationOfToken
 * ----------------------------
 *   Return the token index that the command line cursor
 *   is located on.
 *
 *   string:            unparsed user command
 *   tokens:            array of tokens
 *   cursorPosition:    position of cursor in string
 *
 *   returns: The token the cursor is located on.
 */
static int getTokenBasedOnCursor(char *string, char *tokens[], int cursorPosition) {
    int numberOfTokens = tokenlen(tokens);
    char *ptrToTokenLocation = string;
    int tokenPosition = 0;

    for (int i = 0; i < numberOfTokens; i++) {
        ptrToTokenLocation = strstr(ptrToTokenLocation, tokens[i]);
        tokenPosition = (ptrToTokenLocation - string);
        if (tokenPosition > cursorPosition) {
            //shift every token to the right. Insert empty token at index i
            for (int j = numberOfTokens; j > i; j--) {
                tokens[j] = tokens[j-1];
            }
            tokens[i] = strdup("");
            return i;
        }

        if (tokenPosition <= cursorPosition && cursorPosition <= (tokenPosition + strlen(tokens[i]))) {
            return i;
        }
        ptrToTokenLocation += strlen(tokens[i]);
    }
    tokens[numberOfTokens] = strdup("");
    return numberOfTokens;
}

/*
 * Function: getLocationOfToken
 * ----------------------------
 *   Return the starting index of the token specified.
 *   The token must have a nonzero length.
 *
 *   string:       unparsed user command
 *   tokens:       array of tokens
 *   tokenNumber:  index of token to find in string
 *
 *   returns: The starting index of the token in string
 *            -1 if token is not in string
 */
static int getLocationOfToken(char *string, char *tokens[], int tokenNumber) {
    int numberOfTokens = tokenlen(tokens);
    char *ptrToTokenLocation = string;
    int tokenPosition = 0;

    if (tokenNumber >= numberOfTokens) {
        return -1;
    }

    for (int i = 0; i <= tokenNumber; i++) {
        ptrToTokenLocation = strstr(ptrToTokenLocation, tokens[i]);
        tokenPosition = (ptrToTokenLocation - string);
        ptrToTokenLocation += strlen(tokens[i]);
    }
    return tokenPosition;
}