Compile using the following commands:
gcc -c shell.c interpreter.c shellmemory.c terminal-io.c
gcc -o mysh shell.o interpreter.o shellmemory.o terminal-io.o

***extra features***
-> tab autocomplete 
-> can navigate left and right with arrow keys
-> can navigate word by word with Ctrl-left/right
-> command history can be accessed with up/down arrow keys 
