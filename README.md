# Introduction
I've been using shells and UNIX for years now, but I've never taken the time to actually learn how they work. How a shell reads user input, how processes are launched and terminated, how memory is handled, how pipes actually work. 

For the most part I've also been programming with high-level languages like Python and JavaScript, but I wanted to try out a lower-level language like C.

The goal of this project was to get deep understanding of how processes are handled on UNIX, how shells interpret user inputs and to learn how to program in C.
This is part of a bigger desire to understand how the tools we use on a day to day basis actually function. My goal is to work in the cyber-security industry, and thus I feel it's important to have a deep understanding of these tools for several reasons:
- Attackers and defenders both rely on low-level behavior (processes, memory, I/O)
- Many vulnerabilities and exploits depend on misunderstanding or abusing these mechanisms
- Knowing how things actually work allows you to spot anomalies, misconfigurations, or unexpected behavior more easily

Right now I'm starting at the very basics, with the goal of going into more and more advanced topics. 

<img width="933" height="710" alt="Screenshot From 2026-05-24 13-34-00" src="https://github.com/user-attachments/assets/fb2bd839-50d6-43b5-a694-ea9a316ed5a8" />

# Demo
<img width="1000" height="931" alt="Screencast From 2026-05-25 19-39-15" src="https://github.com/user-attachments/assets/dcfaf22c-c28f-4e41-aa3b-42455a00a520" />

# How it works

## The main loop
The shell operates with a main loop. The loop reads a command line, parses the commands and arguments from it, decides how they should be executed, executes them, frees the allocated memory and finally repeats the loop, unless the user calls an exit. 

It looks like this:
1. Print the help/welcome prompt
2. (start loop) Read line from stdin
3. Check whether command contains a pipe
4. Parse the command
5. Execute either: 
	- a builtin
	- a normal command
	- a pipeline
6. Repeat loop until exit is called

A status variable is used to determine whether the loop should keep running. When a 0 is detected, the value returned by the ```exit``` builtin, the loop stops. 

## Input reading and parsing
The shell reads user input from the `stdin` using the function `read_line()`. This could have been done a lot easier using `getline()`, but since this is a learning experience I didn't want to take any shortcuts. 
`read_line()` does the following:
- allocate a buffer with `malloc()`,
- read characters one by one using `getchar()`,
- grow the buffer with `realloc()` if needed,
- stop when it sees `\n` or `EOF`,
- return a null-terminated string.
We use dynamic buffer allocation as we don't know the size of the user input beforehand. 

Parsing is done using `parse_line()`, which splits an input using `strtok()` by whitespace as delimiter. This transforms a command like the following:
```bash
ls -la /Pictures
```
into 
```c
args[0] = "ls";
args[1] = "-la";
args[2] = "/Pictures";
args[3] = NULL;
```
with NULL at the end of the list to indicate the end of the command.

## Command execution
### Builtins
Most commands can be executed in a child process, with the exception of some, which need to be executed directly by the shell itself. These commands are called builtins. For example, to change the current working directory, you can use `cd`, but this must change the directory of the shell, not a child process. 
Builtins currently implemented:
- `help`
- `cd`
- `exit`
I'm using an array of function pointers to call the builtins, with `builtin_str` storing the names of the builtins, and `built_func` containing the memory addresses of those functions. The benefit of this approach is that it's much cleaner than a long `if/else` block, and is very easy to extend if needed.
```c
char *builtin_str[] = {
    "help",
    "cd",
    "exit",
};

int (*builtin_func[])(char **) = {
    &cactus_help,
    &cactus_cd,
    &cactus_exit,
};
```

### Command execution
`execute()` decides what type of command is being run:
1. If no command was entered, do nothing.
2. Check whether the command matches a builtin.
3. If yes, run the builtin.
4. If not, launch it as an external program.

`launch()` handles the `fork()` and `exec()` system calls. It uses `fork()` to create child program, then calls `execvp()` to replace that child with the requested program. Then the parent process uses `waitpid()` to wait until the child exits. This is used for all external commands.

### Pipeline execution
`pipe_detection()`, as the name implies, is used for detecting whether or not the input contains a pipe, and returns the number of pipes detected.
`execute_pipe()` handles the pipeline logic, and allows for chaining multiple pipes. 
The logic is as follows:
- Split the line into separate commands using `|`.
- For each command:
    - parse its arguments,
    - create a pipe if another command follows,
    - fork a child process,
    - connect stdin/stdout using `dup2()`,
    - run the command using `execvp()`.
- Parent process closes unused file descriptors.
- Parent waits for all child processes.

# What I learned
## The C language
This project allowed me to learn the basics of C, some notable features I used being:
- dynamic memory allocation
- pointers
- function pointers
- strings and tokenization
- arrays and buffers
- program structure
- core UNIX concepts (`fork()`, `execvp()`, `waitpid()`, etc.)
- system calls & error handling
- pipes
- basic OS interaction (`chdir()`, `getcwd()`)

## UNIX processes
At boot, the kernel is launched and initialized, and then the only process launched by the kernel is init, which also runs until the computer is shut down. Then, whenever you want to launch another process, it uses `fork()` to clone itself, then the clone uses `exec()` to replace itself with another process, the one you actually want to launch. Now this seems very weird, but it apparently is quite beneficial as it allows the child process to inherit a bunch of parameters/properties, and only change the ones it needs to change. It makes sense in the context of a shell, where the shell is the parent process, and the children are commands like `ls`, `pwd`, `wc`, etc.  
The parent process can continue operating, and using `wait()` can wait until the child process terminates, and returns its PID to the `wait()` system calls, which will inform the parent.

## stdin/stdout
I also learned that everything in UNIX is a file, including `stdin` and `stdout`. I saw these terms being used a lot, but didn't really know what they were. They're basically just files that programs read from and write to. `stdin` in this context for example is the user keyboard inputs, whereas the `stdout` is the output displayed to the user by the different programs.  

## File descriptors
Another important thing to understand are file descriptors. They are unique ID’s assigned to files, the following table shows some useful examples.

<img width="551" height="225" alt="Screenshot From 2026-05-25 18-47-29" src="https://github.com/user-attachments/assets/fadf733b-2ff4-471c-aac6-2ef9bcac70c3" />

*source: https://en.wikipedia.org/wiki/File_descriptor.*

## Pipelines
Pipes in UNIX are basically a way for the `stdout` of one program to become the `stdin` of another program. Only one can write it in while the other can read from it.
The pipes I’m interested in are unnamed pipes, basically the ones created using `pipe()`. These are pipes that can transfer data between a parent and a child process.

Pipes have two file descriptors when called using `pipe() `:
- `pipefd[0]` : read side
- `pipefd[1]` : write side

To avoid hanging or deadloops, each process must close their own unused fds. This is because when using `fork()` after creating your `pipe()`, it will create individual fds for each process, so basically copies. So each one must cleanup after themselves.

# Limitations
Since this program was developed solely for the purpose of learning, it is very limited in terms of functionality and is not suited for production use. I purposefully limited the scope of the project to the basics, as otherwise it's very easy to go down endless rabbit-holes. Here's a non-exhaustive list of limitations that I found, feel free to let me know if you find any others.

- Limited parser: the parser is very limited as it just parses based on whitespace, meaning certain commands like ```echo "hello world"``` would parse incorrectly. This is just one of the examples, but there are many more that would not work (operators, comments, backslashed, etc.)

- Pipelines do not support builtins: currently pipelines only support shell commands like ls, grep, wc, etc. It does not support builtins as those are launched differently from the other commands.

- No input/output redirection: commands like ```echo hello > file.txt``` do not work as I did not implement that functionality

- No background processes: the current code makes the parent shell wait for the child processes to exit, meaning background processes are not possible.

- No line editing and command history: you can't use the arrow keys to go back or forward a few characters, nor can you pull up previous commands using the arrow up key. Same goes for a lot of other keyboard shortcuts/tools.

# Challenges I encountered
The main challenge I encountered during this project was honestly just using C. The way memory is handled, arrays are declared, pointers are used, are all very new to me as I've mostly used high-level languages so far. 

Also implementing pipelines and making sure they work when chaining multiple ones in a row was not easy at first. However, once I really got a good understanding of how file descriptors are used and which data needs to go where, I was able to implement it. 

# Future developments

If I decide to ever expand this project, these are the main features I would focus on:
- Add input/output redirection
- improve the parser
- background process handling
- command history
- Make pipelines support builtins

I very much enjoyed working on this project and will definitely do more projects like this one, the main ones I have in mind are:
- TCP/IP network stack
- basic operating system
- basic password manager
- basic antivirus scanner
- SSH honeypot
- basic IDS

# Credits
I do have to give credit to the following tutorial from Stephen Brennan which greatly helped me build this shell https://brennan.io/2015/01/16/write-a-shell-in-c/.

Before starting this project, I learned the basics of C programming using https://www.learn-c.org/.

