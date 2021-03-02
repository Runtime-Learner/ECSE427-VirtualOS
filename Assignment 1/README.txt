Compile using the following commands:
gcc -c shell.c interpreter.c shellmemory.c terminal-io.c
gcc -o mysh shell.o interpreter.o shellmemory.o terminal-io.o

Prof. Vybihal approved the addition of terminal-io.c in order to have proper modular programming.

Additional testfiles were included in order to better show the capabilities of my shell.
Executing TESTFILE.txt will execute all the others automatically.

The 'echo' and 'clear' commands were added in order to improve the
shell's usability. 'echo' is used extensively inside TESTFILE.txt in order to narrate what the script is doing.


Please contact me if you find any bugs, I would like to fix them before Assignment 2 is due.


***extra features***
-> tab autocomplete 
-> can navigate left and right with arrow keys
-> can navigate word by word with Ctrl-left/right
-> command history can be accessed with up/down arrow keys 