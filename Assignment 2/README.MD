# Assignment 2:
The goal of Assignment 2 was to implement the exec command. This command emulates loading programs to RAM, adding their PCBs to a queue of running processes, and task switching between them. This in turn leads to concurrent execution of the scripts passed to the exec command.

# Features:
## 1) Can navigate user command using arrow keys
| Character-by-character movement | Token-by-token movement |
:-------------------------:|:-------------------------:
| ![GIF of character navigation](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/Assignment1_navigateByChar.gif) | ![GIF of word navigation](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/Assignment1_navigateByToken.gif) |

## 2) Shell has command history that can be accessed using up/down arrow keys
![GIF of command history](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/Assignment1_history.gif)

## 3) Commands can be Tab autocompleted
| Autocomplete command | Autocomplete command parameters |
:-------------------------:|:-------------------------:
| ![GIF of tab autocomplete](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/Assignment1_autocomplete.gif) | ![GIF of tab autocomplete](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/Assignment1_commandSpecificAutocomplete.gif) |

## Tab autocomplete has been modularized. Can now autocomplete tokens in the middle of a command
![GIF of tab autocomplete on exec command](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/exec_autocomplete.gif)

## Exec command can run scripts concurrently
| run script1 & run script2 | exec script1 script2 |
:-------------------------:|:-------------------------:
| ![GIF of running two scripts with run command](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/run_script1_script%202.gif) | ![GIF of running two scripts concurrently with exec](https://github.com/Runtime-Learner/ECSE427-VirtualOS/blob/main/media/exec_script1_script2.gif) |
