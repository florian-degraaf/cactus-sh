#include <stdio.h>
#include <stdlib.h>
#include <string.h> // needed for strtok to work properly
#include <unistd.h>
#include <sys/wait.h>

/* function returns a string, so basically a pointer to a char
define allows you to create a constant/macro, which will be replaced by its value by the compiler at every point where its used
note how it does not have a type, and also no memory! It also cannot be modified, as it's not stored anywhere, but rather replaced during compilation
I was struggling to understand why and when you should use RL_BUFFSIZE and when you should use BUFFSIZE:
- Basically RL_BUFFSIZE is the growth step/chunk size
- but buffsize is the actual size of the buffer

Also apparently all of this could be done a lot easier with getline(), but i wanted to learn how this works so no shortcuts here!
*/
#define RL_BUFFSIZE 1024
char *read_line()
{
    int buffsize = RL_BUFFSIZE;
    char *buffer = malloc(sizeof(char) * buffsize);
    int pos = 0;
    int c; // c is the character we read, stored as an int as the EOF is an int, thus in order to check it you must use an int.
    while (1)
    {
        c = getchar();

        if (!buffer)
        {
            fprintf(stderr, "shell: buffer allocation error\n");
            exit(EXIT_FAILURE);
        }

        if (c == EOF || c == '\n')
        {
            buffer[pos] = '\0';
            return buffer;
        }
        else
        {
            buffer[pos] = c;
        }
        pos++;

        if (pos >= buffsize)
        {
            buffsize += RL_BUFFSIZE;
            buffer = realloc(buffer, buffsize);
            if (!buffer)
            {
                fprintf(stderr, "shell: buffer allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/* the tokens are basically just pointers to each of the arguments, with a \0 at the end. Starting buffer size of 32 seems reasonable.
The delimeters are the following:
" " Whitespace
\t tab
\r carriage return (left edge of current line)
\n newline
\a legacy thing which makes the terminal beep sound
*/
#define TOK_BUFFSIZE 32
#define TOK_DELIM " \t\r\n\a"
char **parse_line(char *line, char *delim)
{
    int buffsize = TOK_BUFFSIZE;
    char **tokens = malloc(sizeof(char *) * buffsize);
    char *token;
    int pos = 0;

    if (!tokens)
    {
        fprintf(stderr, "shell: buffer allocation error\n");
        exit(EXIT_FAILURE);
    }
    token = strtok(line, delim);
    while (token != NULL)
    {
        /* for tokenization, we'll be using strtok. strtok returns a pointer to the beginning of the token, and puts a \0 at the end, so it modifies the original string.
        It only gives the first token, after which you have to call it again with NULL, to tell it to not use a new string but to give you the next token of the initial string.
        Bit of a weird function, but that's how it works i guess.*/

        tokens[pos] = token;
        pos++;

        if (pos >= buffsize)
        {
            buffsize += TOK_BUFFSIZE;
            tokens = realloc(tokens, sizeof(char *) * buffsize);
            if (!tokens)
            {
                fprintf(stderr, "shell: buffer allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, delim);
    }

    tokens[pos] = NULL;
    return tokens;
}

int launch(char **args)
{
    pid_t pid;  // child process ID
    pid_t wpid; // return value of waitpid, which when succesful returns the PID of the process that exited, or -1 if an error occurs
    int status; // represents the status of the child process, whether it exited normally, was killed, paused, etc.

    pid = fork();

    if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
        {
            perror("shell"); // using perror prints the "string":error message, along with actual error code information etc. fprintf just prints an error message without any os error message, just your custom message
        }
        exit(EXIT_FAILURE); // the reason this exit() comes after and outside the IF condition, is because the execvp only returns a value, and returns to this code if there's an error, otherwise, the child process never comes back to this code and thus, the exit would never be executed.
    }
    else if (pid < 0)
    {
        perror("shell");
    }
    else
    {
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED); //&status tells it to write status information in the memory address of the status int we declared earlier
            // the WUNTRACED is a flag, which tells it to return even if the child is stopped, not just when it exits, apparently good practice -_o_-
        } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // two macros, which tell you if process was exited normally or killed, respectively, basically keep waiting until one of them is true
        
        }

    return 1;
}

int pipe_detection(char *args){
    int pipe_count = 0;
    for (int i = 0; args[i]; i++){
        if (args[i] == '|'){
            pipe_count++;
        }
    }
    return pipe_count;
}


//this is the function that handles the actual creating of pipes and launching the processes which uses them
// currently it is limited to only handle standard commands, not builtins, might have to add that later on.
int execute_pipe(char *line)
{
    char **commands;
    char **args;
    int command_count = 0;
    int fd[2];
    int prev_fd = -1;
    pid_t pid;

    commands = parse_line(line, "|");

    // coutning how many commands we need to execute
    while (commands[command_count] != NULL)
    {
        command_count++;
    }

    for (int i = 0; i < command_count; i++)
    {
        //parsing like we would any other command (except for builtins)
        args = parse_line(commands[i], TOK_DELIM);

        if (args[0] == NULL)
        {
            free(args);
            continue;
        }

        //we need to create a pipe for every command except the last one, as that one doesn't transmit its data to any other function,
        //but rather directly to stdout
        if (i < command_count - 1)
        {
            if (pipe(fd) == -1)
            {
                perror("shell: pipe error");
                return 1;
            }
        }

        pid = fork();

        if (pid == 0)
        {
            // child

            // if this is not the first command, we read from the previous pipe
            if (prev_fd != -1)
            {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            //if this is not the last command, we write it into the current pipe
            if (i < command_count - 1)
            {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }

            if (execvp(args[0], args) == -1)
            {
                perror("shell");
            }

            exit(EXIT_FAILURE);
        }
        else if (pid < 0)
        {
            perror("shell");
            return 1;
        }

        // parent

        if (prev_fd != -1)
        {
            close(prev_fd);
        }

        if (i < command_count - 1)
        {
            close(fd[1]);
            prev_fd = fd[0];
        }

        free(args);
    }

    if (prev_fd != -1)
    {
        close(prev_fd);
    }

    for (int i = 0; i < command_count; i++)
    {
        wait(NULL);
    }

    free(commands);

    return 1;
}

int cactus_help(char **args);
int cactus_cd(char **args);
int cactus_exit(char **args);

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

int num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

void welcome_banner(void)
{
    printf(
        "                   _                            _\n"
        "  ___  __ _   ___ | |_  _   _  ___         ___ | |__\n"
        " / __|/ _` | / __|| __|| | | |/ __| _____ / __|| '_ \\\n"
        "| (__| (_| || (__ | |_ | |_| |\\__ \\|_____|\\__ \\| | | |\n"
        " \\___|\\__,_| \\___| \\__| \\__,_||___/       |___/|_| |_|\n"
        "\n"
        "\n");

    printf("\033[32m"); //print the cactus in green
        
    printf(
        "                 .'.\n"
        "                lldl'\n"
        "         .c;.  .xddoc\n"
        "         ddoc  .xdxol    .\n"
        "         xdd;  .kxxdc   cdl.\n"
        "         kdd,  .kxxd:   kdx;\n"
        "        .kdx.  .kdxo,   xxxo\n"
        "         xod'  ,kdxd'   dkxd\n"
        "         ,ddxddxkdxd.   xkx:\n"
        "           ';::dkdxx,.,lOkd.\n"
        "               ;kdxxkkkkkl.\n"
        "               ;kxxx ...\n"
        "               ,kxxc\n"
        "               ;kdd:\n"
        "               ,kdd;\n"
        "               .Oxd:\n"
        "               .kxd:\n"
        "                .,'.\n"
        "\n"
    );
    printf("\033[0m"); //reset to default output color
}

int cactus_help(char **args)
{
    int i;
    printf("\033[H\033[J"); //clear the screen before printing anything, basically puts the cursor at the top left of the screen
    // and then clears anything below that
    welcome_banner();
    printf("Welcome to cactus-sh. This is a very basic shell implementation, written in C.\nThis was made to learn C and the way UNIX processes work, it's not suited for actual use.\n\n");
    printf("List of built-ins:\n");
    
    for (i = 0; i < num_builtins(); i++)
    {
        printf("%s\n", builtin_str[i]);
    }
    printf("\n");
    return 1;
}

int cactus_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "shell: no arguments provided for cd command");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("shell");
        }
    }
    int buffersize = 64;
    char buffer[buffersize];
    printf("Current working directory: %s\n", getcwd(buffer, buffersize));
    return 1;
}
int cactus_exit(char **args)
{
    return 0;
}

// returns int as that will be our status variable!
int execute(char **args)
{
    if (args[0] == NULL)
    {
        return 1;
    }

    for (int i = 0; i < num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }
    }

    return launch(args);
}

void loop(void)// using (void) to explicitly state that the function takes no arguments, () would work just as well but implicit.
{
    char *line; // string
    char **args; // array of strings, the same as saying char *args[], basically a pointer to a pointer to a string;
    int status = 1;
    int pcount;
    char *help_args[] = {"help", NULL};

    execute(help_args);
    /* we're using do while instead of while, as it executes the process once and then check the condition,
    a while loop would first check the condition, and the start the first loop, we need it to execute at least once.
    This allows us to initialize status in the first loop, then use it in the while loop*/
    do
    {
        printf("\033[32m$ \033[0m"); //print a green dollar sign, matches the theme of the cactus better
        line = read_line();

        pcount = pipe_detection(line);

        if (pcount > 0)
        {
            status = execute_pipe(line);
        }
        else
        {
            args = parse_line(line, TOK_DELIM);
            status = execute(args);
            free(args);
        }

        free(line);

    } while (status);
}

/* While so far i've always used int main() without arguments, in this case we need to use argc and argv.
argc is argument counter: the number of arguments the program was launched with. At least 1, as the program name is the first argument.
argv is argument vector: an array of strings, with each string being an argument
We need them as a shell can be launched with arguments, like pretty much any UNIX program
*/
int main(int argc, char *argv[])
{
    loop();

    return EXIT_SUCCESS;
}
