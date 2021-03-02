#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include "shellmemory.h"
#include "shell.h"
#include "interpreter.h"

static void tabAutocomplete_withCommand(char *string, char *tokens[]);

static int tokenlen(char *tokens[]);
static int validateNumberOfParameters(char *tokens[], int commandId, int expectedNumberOfParameters);
static int help_validateNumberOfParameters(char *tokens[], int maxNumberOfParameters);
static char *getLastOccurence(char *string, char *substring);
static char *getLargestCommonSubstring(char *strings[], char *initialSubstring);

//COMMANDSET must be the last element in the enum in order for it to equal the number of commands
static enum commandSet
{
    CLEAR,
    ECHO,
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

//command autocomplete functions
static int help_autocomplete(char *string, char *tokens[]);
static int run_autocomplete(char *string, char *tokens[]);

//array of command functions
static const int const (*command_functions[COMMANDSET])(char *tokens[]) = {[HELP] = help, [QUIT] = quit, [SET] = set, [PRINT] = print, [RUN] = run, [CLEAR] = clear, [ECHO] = echo};
//array of command-specific autocomplete functions
static const int const (*command_autocompleteFunctions[COMMANDSET])(char *string, char *tokens[]) = {[HELP] = help_autocomplete, [QUIT] = NULL, [SET] = NULL, [PRINT] = NULL, [RUN] = run_autocomplete, [CLEAR] = NULL, [ECHO] = NULL};
//array of command names
static const char const *commands[COMMANDSET] = {[HELP] = "help", [QUIT] = "quit", [SET] = "set", [PRINT] = "print", [RUN] = "run", [CLEAR] = "clear", [ECHO] = "echo"};


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
        COMMANDSET] = "Print STRING on a new line"};

/*
 * Function: tabAutocomplete_withCommand
 * ----------------------------
 *   check to see if first user token is a valid token.
 *   if it is, run the command specific autocomplete function.
 *      
 *   tokens: user command in tokenized form
 *   string: unparsed user command
 */
static void tabAutocomplete_withCommand(char *string, char *tokens[])
{
    //compare first token to all known commands. If a match is found, run that command
    for (int i = 0; i < COMMANDSET; i++)
    {
        if (!strcmp(commands[i], tokens[0]))
        {
            if (command_autocompleteFunctions[i] != NULL)
            {
                (*command_autocompleteFunctions[i])(string, tokens);
            }
            return;
        }
    }
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
 */
void tabAutocomplete(char *string, char *tokens[])
{
    //if user has input more than one token and
    if (tokens != NULL && (tokenlen(tokens) > 1 || (tokenlen(tokens) == 1 && isspace(string[strlen(string) - 1]))))
    {
        tabAutocomplete_withCommand(string, tokens);
        return;
    }
    
    if (tokens[0] == NULL) {
        tokens[0] = strdup("");
    }

    int autoCompleteOptions[COMMANDSET];
    size_t foundCommand = 0;

    

    for (size_t currentCommand = 0; currentCommand < COMMANDSET; currentCommand++)
    {
        if (strstr(commands[currentCommand], tokens[0]) == commands[currentCommand])
        {
            autoCompleteOptions[foundCommand++] = currentCommand;
        }
    }

    if (foundCommand > 0)
    {
        if (foundCommand == 1) //if only one possible command, just go ahead and autocomplete
        {
            char * autocompleteLocation = getLastOccurence(string, tokens[0]);
            strcpy(autocompleteLocation, commands[autoCompleteOptions[0]]);
        }
        else //if multiple possibilities, print them on screen
        {
            putchar('\n');
            size_t counter = 0;
            while (counter < foundCommand)
            {
                printf("%s\t", commands[autoCompleteOptions[counter++]]);
                if (counter % ITEMS_PER_LINE == 0)
                { 
                    putchar('\n');
                }
            }
            putchar('\n');
        }
    }
}

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
 * Function: help_autocomplete
 * ----------------------------
 *   attempt to autocomplete the second parameter of 'help'.
 *   autocomplete in exactly the same way as tabAutocomplete()
 *      
 *   tokens: user command in tokenized form
 *   string: unparsed user command
 * 
 *   returns: 0
 */
static int help_autocomplete(char *string, char *tokens[])
{
    int numTokens = tokenlen(tokens);
    int extraToken = isspace(string[strlen(string) - 1]);
    
    char* tempTokens[2] = {NULL}; 

    //have empty second token
    if (numTokens == 1 && extraToken)
    {
	tempTokens[0] = strdup("");
        tabAutocomplete("", tempTokens);
	free(tempTokens[0]);
    }

    //want to continue second token
    if (tokenlen(tokens) == 2 && !extraToken)
    {
        char *lastOccurence = getLastOccurence(string, tokens[1]);

        //run autocomplete on token and store it in the right location in string
	tempTokens[0] = strdup(tokens[1]);
        char tmpString[TOKENSIZE];
        strcpy(tmpString, tokens[1]);
        tabAutocomplete(tmpString, tempTokens);
        strcpy(lastOccurence, tmpString);
	free(tempTokens[0]);
    }
    return 0;
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
    if (!help_validateNumberOfParameters(tokens, 2))
    {
        return 1;
    }
    if (tokenlen(tokens) == 1)
    {
        for (int i = 0; i < COMMANDSET; i++)
        {
            printf("%16s    %s\n", commands_description[i], commands_description[i + COMMANDSET]);
        }
        return 0;
    }
    else
    {
        for (size_t currentCommand = 0; currentCommand < COMMANDSET; currentCommand++)
        {
            if (strstr(commands[currentCommand], tokens[1]) == commands[currentCommand] && strlen(tokens[1]) == strlen(commands[currentCommand]))
            {
                printf("%16s    %s\n", commands_description[currentCommand], commands_description[currentCommand + COMMANDSET]);
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
 *
 *   returns: 0 if exited with no error. 1 otherwise
 */
static int run_autocomplete(char *string, char *tokens[])
{
    // Get directory object
    struct dirent *de;
    DIR *dr;

    char *fileNameOffset = tokens[1];
    int extraToken = isspace(string[strlen(string) - 1]);

    if (tokenlen(tokens) == 1)
    {
        tokens[1] = strdup("");
        fileNameOffset = tokens[1];
        dr = opendir(".");
    }
    else if (tokenlen(tokens) == 2 && !extraToken)
    {
        if (strstr(tokens[1], "/") == NULL)
        {
            dr = opendir(".");
        }
        else
        {
            char directory[TOKENSIZE] = "./";
            strcat(directory, tokens[1]);
            *(getLastOccurence(directory, "/") + 1) = '\0';
            dr = opendir(directory);
            fileNameOffset = (tokens[1] + strlen(directory) - 2);
        }
    }
    else
    {
        return 0;
    }

    if (dr == NULL) // opendir returns NULL if couldn't open directory
    {
        printf("Could not open directory\n");
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

    //run autocomplete on token
    char *autoCompleteOptions[NUM_FILES] = {NULL};
    size_t foundFiles = 0;
    for (size_t currentFile = 0; currentFile < numberOfFiles; currentFile++)
    {
        if (strstr(files[currentFile], fileNameOffset) == files[currentFile])
        {
            autoCompleteOptions[foundFiles++] = files[currentFile];
        }
    }
    if (foundFiles > 0)
    {
        if (foundFiles == 1) //if only one possible command, just go ahead and autocomplete
        {
            //need to get a pointer to the second token in the string
            char *lastOccurence = getLastOccurence(string, fileNameOffset);
            strcpy(lastOccurence, autoCompleteOptions[0]);
        }
        else //if multiple possibilities, print them on screen
        {
            int alreadyLargestCommonString = 1;
            if (strlen(tokens[1]))
            {
                char *largestCommonString = getLargestCommonSubstring(autoCompleteOptions, fileNameOffset);
                alreadyLargestCommonString = (strlen(largestCommonString) == strlen(fileNameOffset));

                //need to get a pointer to the second token in the string
                char *lastOccurence = getLastOccurence(string, tokens[1]);
                strcat(lastOccurence, (largestCommonString + strlen(fileNameOffset)));
                free(largestCommonString);
            }
            if (alreadyLargestCommonString)
            {
                putchar('\n');
                size_t counter = 0;
                while (counter < foundFiles)
                {
                    printf("%s    ", autoCompleteOptions[counter++]);
                    if (counter % ITEMS_PER_LINE == 0)
                    {
                        putchar('\n');
                    }
                }
                putchar('\n');
            }
        }
    }
    return 0;
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

    FILE *script;
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
    terminalLine.string = (char *)malloc(sizeof(char) * TOKENSIZE);
    terminalLine.cursorPosition = 0;
    terminalLine.inputStream = script;

    int flag = 0;
    int error = 0;
    int lineNumber = 1;
    while (1)
    {
        //get user input
        error = getLine(&terminalLine, TOKENSIZE, 0);

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
 * Function: help_validateNumberOfParameters
 * ----------------------------
 *   verifies that the number of parameters passed to the help command is valid. Print an error otherwise.
 *   Print out valid format and a generic help message.
 *  
 *   help command expects AT MOST n parameters, while other commands expect a specific amount.
 *  
 *   tokens:    tokens representing the user command.
 *   commandId: index of command to reference it inside commands[] array.
 *   expectedNumberOfParameters: number of parameters the command expects to receive.
 *
 *   returns: 0 if number of parameters was valid. 1 otherwise.
 */
static int help_validateNumberOfParameters(char *tokens[], int maxNumberOfParameters)
{
    if (tokenlen(tokens) > maxNumberOfParameters)
    {
        setColor(RED);
        printf("Error: \'%s\' expected at most %d argument%s, but received %d\n", commands[HELP], maxNumberOfParameters - 1,
               maxNumberOfParameters == 2 ? "" : "s", tokenlen(tokens) - 1);
        printf("\'%s\' valid format:\n> %s\n", commands[HELP], commands_description[HELP]);
        setColor(DEFAULT);
        printf("Type \'help %s\' for more information\n", commands[HELP]);
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
static char *getLargestCommonSubstring(char *strings[], char *initialSubstring)
{
    char substring[TOKENSIZE];
    strcpy(substring, initialSubstring);
    int numberOfStrings = tokenlen(strings);
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
