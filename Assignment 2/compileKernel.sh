#!/bin/bash
echo "compiling mykernel..."
gcc -c kernel.c shell.c interpreter.c shellmemory.c terminal-io.c pcb.c ram.c cpu.c; gcc -o mykernel kernel.o shell.o interpreter.o shellmemory.o terminal-io.o pcb.o ram.o cpu.o
echo "done!"