/*
  This project was completed by:
  Prasun Dhungana - @02969212
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_COMMAND_LINE_LEN 1024
#define MAX_COMMAND_LINE_ARGS 128

char prompt[] = "> ";
char delimiters[] = " \t\r\n";
extern char **environ;

// Signal handler for SIGINT (Ctrl+C)
void sigint_handler(int signum) {
    printf("\n%s%s", getcwd(NULL, 0), prompt);
    fflush(stdout);
}

pid_t fg_pid = -1;  // Stores the PID of the foreground process
int timeout = 10;  // Time limit for foreground processes (10 seconds)

// Signal handler for SIGALRM (Timeout after 10 seconds)
void sigalrm_handler(int signum) {
    if (fg_pid != -1) {
        // Kill the foreground process after 10 seconds
        kill(fg_pid, SIGKILL);
        printf("\nProcess exceeded time limit and was terminated.\n");
    }
}

void handle_redirection_and_pipes(char **arguments) {
    int is_background = 0; // Flag to check if the process is in the background
    int last_arg_index = 0;

    // Check for '&' indicating a background process
    while (arguments[last_arg_index] != NULL) {
        last_arg_index++;
    }

    if (last_arg_index > 0 && strcmp(arguments[last_arg_index - 1], "&") == 0) {
        is_background = 1;
        arguments[last_arg_index - 1] = NULL; // Remove '&' from arguments
    }

    int fd[2];
    pid_t pid;
    int pipe_index = -1;
    int input_redirect = -1, output_redirect = -1;

    // Check for piping '|' or redirection symbols '<' or '>'
    int i = 0;
    while (arguments[i] != NULL) {
        if (strcmp(arguments[i], "|") == 0) {
            pipe_index = i;
            break;
        }
        if (strcmp(arguments[i], "<") == 0) {
            input_redirect = i;
        }
        if (strcmp(arguments[i], ">") == 0) {
            output_redirect = i;
        }
        i++;
    }

    if (pipe_index != -1) {  // Handle piping
        arguments[pipe_index] = NULL;  // Null-terminate first command
        char *cmd1[MAX_COMMAND_LINE_ARGS];
        char *cmd2[MAX_COMMAND_LINE_ARGS];

        // Split the arguments into two parts (before and after '|')
        int i = 0;
        while (i < pipe_index) {
            cmd1[i] = arguments[i];
            i++;
        }
        cmd1[i] = NULL;

        i = pipe_index + 1;
        int j = 0;
        while (arguments[i] != NULL) {
            cmd2[j++] = arguments[i++];
        }
        cmd2[j] = NULL;

        if (pipe(fd) == -1) {
            perror("pipe failed");
            return;
        }

        pid_t pid1 = fork();
        if (pid1 == 0) {
            // First child: execute the first command and write to pipe
            close(fd[0]);  // Close the read end of the pipe
            dup2(fd[1], STDOUT_FILENO);  // Redirect stdout to the pipe
            close(fd[1]);  // Close the write end after duplication

            execvp(cmd1[0], cmd1);
            perror("execvp() failed");
            printf("An error occurred.\n");
            exit(1);
        } else if (pid1 < 0) {
            perror("fork failed for first command");
            return;
        }

        pid_t pid2 = fork();
        if (pid2 == 0) {
            // Second child: execute the second command and read from pipe
            close(fd[1]);  // Close the write end of the pipe
            dup2(fd[0], STDIN_FILENO);  // Redirect stdin to the pipe
            close(fd[0]);  // Close the read end after duplication

            execvp(cmd2[0], cmd2);
            perror("execvp() failed");
            printf("An error occurred.\n");
            exit(1);
        } else if (pid2 < 0) {
            perror("fork failed for second command");
            return;
        }

        // Parent: Close both ends of the pipe and wait for children
        close(fd[0]);
        close(fd[1]);

        waitpid(pid1, NULL, 0);  // Wait for the first child to finish
        waitpid(pid2, NULL, 0);  // Wait for the second child to finish
    }
    else if (input_redirect != -1) {  // Handle input redirection '<'
        char *input_file = arguments[input_redirect + 1];
        arguments[input_redirect] = NULL;  // Null-terminate the command

        fg_pid = fork();
        if (fg_pid == 0) {
            // Open the file for reading
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in == -1) {
                perror("Input file open failed");
                exit(1);
            }

            dup2(fd_in, STDIN_FILENO);  // Redirect stdin to file
            close(fd_in);

            execvp(arguments[0], arguments);
            perror("execvp() failed");
            printf("An error occurred.\n");
            exit(1);
        } else if (fg_pid > 0) {
            if (!is_background) {
                alarm(timeout);  // Set timeout for foreground process
                waitpid(fg_pid, NULL, 0);
                alarm(0);  // Cancel alarm after process finishes
                fg_pid = -1;  // Reset foreground PID
            } else {
                fg_pid = -1;  // Do not track background processes
            }
        } else {
            perror("fork failed");
        }
    } else if (output_redirect != -1) {  // Handle output redirection '>'
        char *output_file = arguments[output_redirect + 1];
        arguments[output_redirect] = NULL;  // Null-terminate the command

        fg_pid = fork();
        if (fg_pid == 0) {
            // Open the file for writing (create/truncate)
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out == -1) {
                perror("Output file open failed");
                exit(1);
            }

            dup2(fd_out, STDOUT_FILENO);  // Redirect stdout to file
            close(fd_out);

            execvp(arguments[0], arguments);
            perror("execvp() failed");
            printf("An error occurred.\n");
            exit(1);
        } else if (fg_pid > 0) {
            if (!is_background) {
                alarm(timeout);  // Set timeout for foreground process
                waitpid(fg_pid, NULL, 0);
                alarm(0);  // Cancel alarm after process finishes
                fg_pid = -1;  // Reset foreground PID
            } else {
                fg_pid = -1;  // Do not track background processes
            }
        } else {
            perror("fork failed");
        }
    } else {
        // No redirection or pipe, just execute the command normally
        fg_pid = fork();
        if (fg_pid == 0) {
            if (is_background) {
                setsid();  // Detach from terminal for background process
            }
            execvp(arguments[0], arguments);
            perror("execvp() failed");
            printf("An error occurred.\n");
            exit(1);
        } else if (fg_pid > 0) {
            if (!is_background) {
                alarm(timeout);  // Set timeout for foreground process
                waitpid(fg_pid, NULL, 0);
                alarm(0);  // Cancel alarm after process finishes
                fg_pid = -1;  // Reset foreground PID
            } else {
                fg_pid = -1;  // Do not track background processes
            }
        } else {
            perror("fork failed");
        }
    }
}

int main() {
    // Register the SIGINT and SIGALRM signal handlers
    signal(SIGINT, sigint_handler);
    signal(SIGALRM, sigalrm_handler);

    // Stores the string typed into the command line.
    char command_line[MAX_COMMAND_LINE_LEN];
    char cmd_bak[MAX_COMMAND_LINE_LEN];
  
    // Stores the tokenized command line input.
    char *arguments[MAX_COMMAND_LINE_ARGS];
    	
    while (true) {

        // Stores the current directory's path.
        char cwd[MAX_COMMAND_LINE_LEN];

        // Fetch the current working directory.
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd error");
            exit(1);
        }
      
        do { 
            // Print the shell prompt.
            printf("%s%s", cwd, prompt);
            fflush(stdout);

            // Read input from stdin and store it in command_line. If there's an
            // error, exit immediately. (If you want to learn more about this line,
            // you can Google "man fgets")
        
            if ((fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) && ferror(stdin)) {
                fprintf(stderr, "fgets error");
                exit(0);
            }
 
        } while(command_line[0] == 0x0A);  // while just ENTER pressed
        command_line[strlen(command_line) - 1] = '\0';
      
        // If the user input was EOF (ctrl+d), exit the shell.
        if (feof(stdin)) {
            printf("\n");
            fflush(stdout);
            fflush(stderr);
            return 0;
        }

        // 1. Tokenize the command line input (split it on whitespace)
        char *token = strtok(command_line, delimiters);
        int i = 0;

        while (token != NULL) {
            arguments[i] = token;
            i++;
            token = strtok(NULL, delimiters);
        }

        // Ensuring that the arguments end with a NULL
        arguments[i] = NULL;
      
        // 2. Implement Built-In Commands

        // Implementing cd:
        if (strcmp(arguments[0], "cd") == 0) {
            // if 'cd' is input, go to HOME
            if (arguments[1] == NULL) {
                chdir(getenv("HOME"));
            } else {
                // Change to the directory specified
                if (chdir(arguments[1]) != 0) {
                    perror("cd failed");
                }
            }
        }

        // Implementing pwd
        else if (strcmp(arguments[0], "pwd") == 0) {
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd failed");
            }
        }

        // Implementing echo
        else if (strcmp(arguments[0], "echo") == 0) {
            int i = 1;
            while (arguments[i] != NULL) {
                if (arguments[i][0] == '$') {
                    // If argument starts with $, it's an env variable
                    char *env_val = getenv(arguments[i] + 1); // Skip the $
                    if (env_val != NULL) {
                        printf("%s ", env_val);
                    } else {
                        printf("%s ", arguments[i]);
                    }
                } else {
                    printf("%s ", arguments[i]);
                }
                i++;
            }
            printf("\n");
        }

        // Implementing setenv
        else if (strcmp(arguments[0], "setenv") == 0) {
            if (arguments[1] != NULL) {
                // Split at '=' to separate the variable name and value
                char *equal_sign = strchr(arguments[1], '=');
                if (equal_sign != NULL) {
                    // Null-terminate the name part
                    *equal_sign = '\0';

                    // Trim spaces around the variable name
                    char *var_name = arguments[1];
                    while (isspace((unsigned char)*var_name)) var_name++;

                    // Get the value part
                    char *value = equal_sign + 1;
                    while (isspace((unsigned char)*value)) value++;

                    // Set the environment variable
                    if (setenv(var_name, value, 1) != 0) {
                        perror("setenv failed");
                    }
                } else {
                    fprintf(stderr, "Usage: setenv VAR=VALUE\n");
                }
            } else {
                fprintf(stderr, "Usage: setenv VAR=VALUE\n");
            }
        }

        // Implementing exit
        else if (strcmp(arguments[0], "exit") == 0) {
            if (arguments[1] != NULL) {
                fprintf(stderr, "Usage: exit\n");
            } else {
                exit(0);
            }
        }

        // Implementing env
        else if (strcmp(arguments[0], "env") == 0) {
            int i = 0;
            if (arguments[1] != NULL) {
                while (environ[i] != NULL) {
                    if (strncmp(environ[i], arguments[1], strlen(arguments[1])) == 0 && environ[i][strlen(arguments[1])] == '=') {
                        printf("%s\n", &environ[i][strlen(arguments[1]) + 1]);
                        break;
                    }
                    i++;
                }
            } else {
                while (environ[i] != NULL) {
                    printf("%s\n", environ[i]);
                    i++;
                }
            }
        }

        // 3. Handle non-built-in commands, including redirection and pipes
        else {
            handle_redirection_and_pipes(arguments);
        }
      
        // Hints (put these into Google):
        // man fork
        // man execvp
        // man wait
        // man strtok
        // man environ
        // man signals
        
        // Extra Credit
        // man dup2
        // man open
        // man pipes
    }

    // This should never be reached.
    return -1;
}
