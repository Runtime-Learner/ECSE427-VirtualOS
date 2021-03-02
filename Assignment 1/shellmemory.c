#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"

// SHELL MEMORY ///////////////////////////////////////////////
#define MEMORY_SIZE 1000

/*
 * This structure is used to store shell variables.
 * Variables are a key value pair.
 * Shell memory is implemented as a dictionary.  
 */
static struct MEM
{
    char *var;
    char *value;
} SHELLMEMORY;

static struct MEM consoleMemory[MEMORY_SIZE];
static size_t mem_HEAD = 0;

static int createVar(char *var, char *value);
static int findVar(char *var);

// SHELL COMMAND HISTORY //////////////////////////////////////
#define HISTORY_SIZE 100

static char *commandHistory[HISTORY_SIZE] = {NULL};
static int hist_HEAD = 0;
static int hist_TAIL = 0;
static int hist_POS = 0;

static int history_saveString_noIncrement(char *string);
static int history_incrementVariable(int *variable);
static int history_decrementVariable(int *variable);
static int history_incrementHEAD();


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
int setVar(char *var, char *value)
{
    int variableIndex = findVar(var);
    //variable does not exist. Need to create it
    if (variableIndex == -1)
    {
        variableIndex = createVar(var, value);
        //could not create variable b/c no more space
        if (variableIndex == -1)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        //delete previous value
        free(consoleMemory[variableIndex].value);
        //set variable value
        consoleMemory[variableIndex].value = strdup(value);
        return 0;
    }
}

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
char *getVar(char *var)
{
    int variableIndex = findVar(var);
    if (variableIndex == -1)
    {
        return NULL;
    }
    return strdup(consoleMemory[variableIndex].value);
}

/*
* Function: memory_clear
* ----------------------------
*   Frees all memory allocated to console memory
*
*   returns: whether the operation was successful or not
*/
int memory_clear() {
    for (int i = 0; i < mem_HEAD; i++) {
        free(consoleMemory[i].var);
        free(consoleMemory[i].value);
    }
    return 0;
}

/*
 * Function: findVar
 * ----------------------------
 *   Returns the index of variable var in consoleMemory[] array
 *
 *   var: name of variable
 *
 *   returns: index of variable var.
 *            -1 if var does not exist.
 */
static int findVar(char *var)
{
    for (int i = 0; i < mem_HEAD; i++)
    {
        if (!strcmp(consoleMemory[i].var, var))
        {
            return i;
        }
    }
    return -1;
}

/*
 * Function: createVar
 * ----------------------------
 *   Update the consoleMemory[] array by adding the new variable to
 *   the head of the array and updating the head.
 *
 *   var: name of variable
 *   value: value to assign to variable
 *
 *   returns: the index of the newly created variable
 *            -1 if consoleMemory[] is full and variable could not be created
 */
static int createVar(char *var, char *value)
{
    if (mem_HEAD == MEMORY_SIZE)
    {
        return -1;
    }
    consoleMemory[mem_HEAD].var = strdup(var);
    consoleMemory[mem_HEAD].value = strdup(value);
    mem_HEAD++;
    return mem_HEAD - 1;
}

/*
 * Function: history_saveString
 * ----------------------------
 *   Save the string to the head of commandHistory[] and update the head.
 *
 *   string: the string to save to memory
 *
 *   returns: Whether the operation was successful or not 
 */
int history_saveString(char *string)
{
    if (commandHistory[hist_HEAD] != NULL)
    {
        free(commandHistory[hist_HEAD]);
    }
    commandHistory[hist_HEAD] = strdup(string);
    history_incrementHEAD();
    hist_POS = hist_HEAD; //reset user position to HEAD of history
    return 0;
}

/*
 * Function: history_getPrevious
 * ----------------------------
 *   Returns the string located at hist_POS -1. update hist_POS.
 * 
 *   if hist_POS == hist_TAIL, we have reached the oldest command 
 *   stored in commandHistory[]. Do not update hist_POS and return
 *   commandHistory[hist_TAIL].
 * 
 *   If hist_POS == hist_HEAD, we save the parameter 'string'
 *   to allow the user to navigate back to the command they
 *   were typing before viewing commands saved in commandHistory[]
 *
 *   string: current string displayed on terminal
 *
 *   returns: the previous command stored in commandHistory[] relative
 *            to the user's position in the history hist_POS.
 */
char *history_getPrevious(char *string)
{
    if (hist_POS == hist_HEAD)
    {
        history_saveString_noIncrement(string);
    }
    if (hist_POS != hist_TAIL)
    {
        history_decrementVariable(&hist_POS);
    }
    return commandHistory[hist_POS];
}

/*
 * Function: history_getNext
 * ----------------------------
 *   Returns the string located at hist_POS + 1. update hist_POS.
 *   if hist_POS == hist_HEAD, return the parameter 'string' as
 *   no next history command exists.
 * 
 *   string: current string displayed on terminal
 *
 *   returns: the next command stored in commandHistory[] relative
 *            to the user's position in the history hist_POS.
 */
char *history_getNext(char *string)
{
    if (hist_POS != hist_HEAD)
    {
        history_incrementVariable(&hist_POS);
        return commandHistory[hist_POS];
    }
    else
    {
        return string;
    }
}

/*
 * Function: history_clear
 * ----------------------------
 *   Frees all memory allocated to command history
 *
 *   returns: whether the operation was successful or not
 */
int history_clear() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (commandHistory[i] != NULL){
            free(commandHistory[i]);
        }
    }
    return 0;
}

/*
 * Function: history_saveString_noIncrement
 * ----------------------------
 *   Used in history_getPrevious(). 
 *   Behaves exactly like history_saveString, except that
 *   hist_HEAD is not incremented.
 *
 *   string: string to save
 *
 *   returns: whether the operation was successful or not
 */
static int history_saveString_noIncrement(char *string)
{
    if (commandHistory[hist_HEAD] != NULL)
    {
        free(commandHistory[hist_HEAD]);
    }
    commandHistory[hist_HEAD] = strdup(string);
    return 0;
}

/*
 * Function: history_incrementHEAD
 * ----------------------------
 *   Increment the hist_HEAD variable using the following operation:
 *   hist_HEAD = (hist_HEAD + 1) % HISTORY_SIZE
 * 
 *   Increment history_TAIL if history_HEAD ==  history_TAIL.
 *   This is done in order for the commandHistory array to behave
 *   like a circular array
 *
 *   returns: the new value of history_HEAD
 */
static int history_incrementHEAD()
{
    if (history_incrementVariable(&hist_HEAD) == hist_TAIL)
    {
        history_incrementVariable(&hist_TAIL);
    }
    return hist_HEAD;
}

/*
 * Function: history_incrementVariable
 * ----------------------------
 *   Increment the 'variable' variable using the following operation:
 *   variable = (variable + 1) % variable
 *
 *   variable: variable to increment
 * 
 *   returns: the new value of variable
 */
static int history_incrementVariable(int *variable)
{
    *variable = (*variable + 1) % HISTORY_SIZE;
    return *variable;
}

/*
 * Function: history_decrementVariable
 * ----------------------------
 *   Decrement the 'variable' variable using the following operation:
 *   variable = (variable - 1) % variable
 *
 *   variable: variable to decrement
 * 
 *   returns: the new value of variable
 */
static int history_decrementVariable(int *variable)
{
    if (*variable == 0)
    {
        *variable = HISTORY_SIZE - 1;
    }
    else
    {
        *variable = *variable - 1;
    }
    return *variable;
}