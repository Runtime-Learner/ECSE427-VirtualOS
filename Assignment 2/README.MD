# Assignment 2:
The goal of Assignment 2 was to implement the exec command. This command emulates loading programs to RAM, adding their PCBs to a queue of running processes, and task switching between them. This in turn leads to concurrent execution of the scripts passed to the exec command.

# New Features (added on top of features from assignment 1):
## Tab autocomplete has been modularized. Can now autocomplete tokens in the middle of a command
![GIF of tab autocomplete on exec command](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/exec_autocomplete.gif)

## Exec command can run scripts concurrently
| run script1 & run script2 | exec script1 script2 |
:-------------------------:|:-------------------------:
| ![GIF of running two scripts with run command](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/run_script1_script%202.gif) | ![GIF of running two scripts concurrently with exec](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/exec_script1_script2.gif) |
