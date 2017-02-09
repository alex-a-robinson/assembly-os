# cc-cw2
## Section 1
Implement 1 sec timer based sceduling which uses a list of processes and removes items from the list when they are complete i.e. they return a value

## Section 2
### Part 1
#### Kill
If killing current process, reset ctx and run scheduler in order to imediatly switch to the next process. If not current proccess then reset the processes ctx but continue execution of current process (rather then running the scheduler again).
Also set all orphaned processes ppid to 0

#### Fork, Exec
Copys current process into new process, updates ppid, returns 0 to child and the childs pid to the parent. When the child is being executed run exec which imediatly switches the childs program to the new program.

Problems:
- No errors if cannot create a new process

### Part 2

