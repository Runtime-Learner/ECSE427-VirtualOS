The compileKernel.sh script can be used to compile the code.
Make sure to run the command "chmod +x compileKernel.sh" if the bash script is not running.

The bash script can be run with the following command: "./compileKernel.sh"



I have chosen to abstract away the RAM in order to make my program more resilient to memory relocation/defragmentation.
The following diagram is a visual description of my RAM structure. Each PCB stores a Process ID, which is associated with 
a RAM datablock. RAM datablocks are used to hide the RAM from the processes. RAM datablocks can be set to an unallocated state
to signal that they can be used. We avoid having to clear RAM cells every time we are removing a process from RAM.


      |PCB1|          |PCB2|
        |               |
        V               V
  |Process ID|    |Process ID|
        |               |
        V               V

|RAM block   ||RAM block          ||RAM block|
|(allocated) || (allocated)       || (free)  |
_____________________________________________
                    RAM
_____________________________________________