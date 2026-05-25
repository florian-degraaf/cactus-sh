# cactus-sh

`cactus-sh` is a small Unix-like shell written in C.

This project was created as a learning exercise to better understand how shells work internally, how Unix processes are created and managed, how user input is parsed, and how pipes/file descriptors are used to connect processes together.

> **Note:** This shell is not intended for real-world use. It is intentionally limited and exists mainly as an educational project.

---

## Features

- Basic interactive shell loop
- Reads user input from `stdin`
- Parses commands and arguments
- Executes external programs using `fork()` and `execvp()`
- Waits for child processes using `waitpid()`
- Supports a few built-in commands
- Supports basic chained pipelines using `pipe()` and `dup2()`
- Dynamically allocates memory for input and tokens
- Includes a simple welcome/help screen

---

## Built-in Commands

The shell currently implements the following builtins:

```text
help
cd
exit
```

### `help`

Displays a short help message and lists available built-in commands.

### `cd`

Changes the current working directory of the shell process.

### `exit`

Exits the shell loop and terminates the program.

---

## Example Usage

Compile the shell:

```bash
gcc -o cactus-sh main.c
```

Run it:

```bash
./cactus-sh
```

Run a normal command:

```bash
ls -la
```

Change directory:

```bash
cd /tmp
```

Run a pipeline:

```bash
ls | grep .c | wc -l
```

Exit the shell:

```bash
exit
```

---

## How It Works

At a high level, the shell follows this loop:

1. Display the prompt.
2. Read a line from `stdin`.
3. Check whether the input contains a pipe character `|`.
4. Parse the command into arguments.
5. Execute either:
   - a built-in command,
   - a normal external command,
   - or a pipeline.
6. Free allocated memory.
7. Repeat until `exit` is called.

---

## Main Components

### Input Reading

Input is read using a custom `read_line()` function.

Instead of using `getline()`, the function manually:

- allocates a buffer with `malloc()`;
- reads characters one by one using `getchar()`;
- grows the buffer using `realloc()` when needed;
- stops at newline or EOF;
- returns a null-terminated string.

This was done intentionally to better understand dynamic memory allocation and input handling in C.

---

### Parsing

The shell uses `parse_line()` to split input into tokens.

For example:

```bash
ls -la /tmp
```

becomes:

```c
args[0] = "ls";
args[1] = "-la";
args[2] = "/tmp";
args[3] = NULL;
```

The parser currently uses `strtok()`, which makes it simple but limited.

---

### Command Execution

External commands are executed using the classic Unix process model:

1. The shell calls `fork()` to create a child process.
2. The child process calls `execvp()` to replace itself with the requested program.
3. The parent process calls `waitpid()` to wait until the child process exits.

This allows the shell process itself to continue running after each command.

---

### Builtins

Some commands must be executed by the shell itself instead of in a child process.

For example, `cd` needs to modify the current working directory of the shell process. If it were executed in a child process, only the child would change directory, and the parent shell would remain in the same location.

Builtins are handled using an array of function pointers, which keeps the code cleaner and makes it easier to add new builtins later.

---

### Pipelines

The shell supports basic pipelines such as:

```bash
ls | grep .c | wc -l
```

Pipeline execution works by:

1. Splitting the input line by `|`.
2. Creating pipes between commands.
3. Forking one child process per command.
4. Using `dup2()` to connect the output of one command to the input of the next.
5. Closing unused file descriptors.
6. Waiting for all child processes to terminate.

---

## What I Learned

This project helped me learn and practice several important C and Unix concepts:

- Pointers and double pointers
- Dynamic memory allocation with `malloc()`, `realloc()`, and `free()`
- Strings and tokenization
- Function pointers
- Arrays and buffers
- Unix processes
- `fork()`, `execvp()`, and `waitpid()`
- File descriptors
- `stdin`, `stdout`, and `stderr`
- Pipes and inter-process communication
- `pipe()` and `dup2()`
- Basic error handling with `perror()`
- Basic OS interaction with `chdir()` and `getcwd()`

---

## Limitations

This shell is intentionally minimal. Some important missing features include:

- No advanced parser
- No support for quotes or escaping
- No environment variable expansion
- No input/output redirection such as `>` or `<`
- No background processes using `&`
- No job control
- No signal handling for shortcuts like `Ctrl+C`
- No command history
- No line editing or arrow-key navigation
- Pipelines do not currently support builtins
- Limited syntax error handling

---

## Possible Future Improvements

Some features that could be added in the future:

- Input/output redirection
- Better parser with support for quotes and escaping
- Environment variable support
- Command history
- Interactive line editing
- Background process handling
- Builtin support inside pipelines
- More robust error handling

---

## Credits

This project was heavily inspired by Stephen Brennan's tutorial:

- [Write a Shell in C](https://brennan.io/2015/01/16/write-a-shell-in-c/)

I also used the following resource to learn the basics of C:

- [learn-c.org](https://www.learn-c.org/)

---

## License

MIT license
