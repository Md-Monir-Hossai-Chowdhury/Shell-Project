#define _POSIX_C_SOURCE 200809L  // For strtok_r
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "history.h"
#include "parser.h"

// Forward declaration for the internal execution function
void execute_command(char* cmd);

// Trims leading/trailing spaces
char* trim_whitespace(char* str) {
    while (*str == ' ') str++;
    if (*str == '\0') return str;
    
    char* end = str + strlen(str) - 1;
    while (end > str && *end == ' ') end--;
    *(end + 1) = '\0';
    
    return str;
}

// Main parser + executor
void parse_and_execute(char* input) {
    char* command_list = strdup(input); // work on a copy
    char* saveptr1;  // For outer strtok_r
    char* saveptr2;  // For inner strtok_r
    
    // First split by semicolons
    char* token = strtok_r(command_list, ";", &saveptr1);
    
    while (token != NULL) {
        token = trim_whitespace(token);
        if (*token == '\0') {  // Skip empty commands
            token = strtok_r(NULL, ";", &saveptr1);
            continue;
        }
        
        // Now split each semicolon-separated part by &&
        char* subtoken = strtok_r(token, "&&", &saveptr2);
        int success = 1;
        int run_next = 1;
        
        while (subtoken != NULL) {
            subtoken = trim_whitespace(subtoken);
            if (*subtoken == '\0') {  // Skip empty commands
                subtoken = strtok_r(NULL, "&&", &saveptr2);
                continue;
            }
            
            if (run_next && success) {
                int status;
                pid_t pid = fork();
                
                if (pid == 0) {
                    execute_command(subtoken);  // child executes command
                    exit(0);
                } else if (pid > 0) {
                    waitpid(pid, &status, 0);
                    success = (WIFEXITED(status) && WEXITSTATUS(status) == 0);
                } else {
                    perror("fork");
                }
            }
            
            subtoken = strtok_r(NULL, "&&", &saveptr2);
        }
        
        token = strtok_r(NULL, ";", &saveptr1);
    }
    
    free(command_list);
}

// Executes  single command (pipes and redirection handled here)
void execute_command(char* cmd_input) {
    // Make copy, need to tokenize multiple times
    char* cmd = strdup(cmd_input);
    
    // Check for pipes
    char* pipe_cmds[10];
    int pipe_count = 0;
    char* saveptr;

    pipe_cmds[pipe_count++] = strtok_r(cmd, "|", &saveptr);
    while ((pipe_cmds[pipe_count++] = strtok_r(NULL, "|", &saveptr)) != NULL && pipe_count < 10);
    pipe_count--;

    if (pipe_count > 1) {
        int prev_pipe = -1;
        int pipes[2];

        for (int i = 0; i < pipe_count; i++) {
            if (i < pipe_count - 1) {
                if (pipe(pipes) == -1) {
                    perror("pipe");
                    free(cmd);
                    return;
                }
            }

            pid_t pid = fork();

            if (pid == 0) {
                // Child process
                if (i > 0) {
                    dup2(prev_pipe, STDIN_FILENO);
                    close(prev_pipe);
                }
                if (i < pipe_count - 1) {
                    close(pipes[0]);
                    dup2(pipes[1], STDOUT_FILENO);
                    close(pipes[1]);
                }

                // Parse redirection for this piped command
                char* args[64];
                int arg_index = 0;
                char* input_file = NULL;
                char* output_file = NULL;
                int append = 0;

                char* cmd_copy = strdup(pipe_cmds[i]);
                char* tok = strtok_r(cmd_copy, " ", &saveptr);
                while (tok != NULL && arg_index < 63) {
                    if (strcmp(tok, "<") == 0) {
                        tok = strtok_r(NULL, " ", &saveptr);
                        input_file = tok;
                    } else if (strcmp(tok, ">") == 0) {
                        tok = strtok_r(NULL, " ", &saveptr);
                        output_file = tok;
                        append = 0;
                    } else if (strcmp(tok, ">>") == 0) {
                        tok = strtok_r(NULL, " ", &saveptr);
                        output_file = tok;
                        append = 1;
                    } else {
                        args[arg_index++] = tok;
                    }
                    tok = strtok_r(NULL, " ", &saveptr);
                }
                args[arg_index] = NULL;

                // Input redirection
                if (input_file) {
                    int fd = open(input_file, O_RDONLY);
                    if (fd == -1) {
                        perror("open input");
                        exit(1);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }

                // Output redirection
                if (output_file) {
                    int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
                    int fd = open(output_file, flags, 0644);
                    if (fd == -1) {
                        perror("open output");
                        exit(1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                if (args[0] != NULL) {
                    execvp(args[0], args);
                    perror("execvp");
                }
                exit(1);
            } else if (pid == -1) {
                perror("fork");
                free(cmd);
                return;
            }

            if (i > 0) close(prev_pipe);
            if (i < pipe_count - 1) {
                close(pipes[1]);
                prev_pipe = pipes[0];
            }
        }

        for (int i = 0; i < pipe_count; i++) {
            wait(NULL);
        }
    } else {
        // Handle single command (no pipes)
        char* args[64];
        int arg_index = 0;
        char* input_file = NULL;
        char* output_file = NULL;
        int append = 0;

        char* tok = strtok_r(cmd, " ", &saveptr);
        while (tok != NULL && arg_index < 63) {
            if (strcmp(tok, "<") == 0) {
                tok = strtok_r(NULL, " ", &saveptr);
                input_file = tok;
            } else if (strcmp(tok, ">") == 0) {
                tok = strtok_r(NULL, " ", &saveptr);
                output_file = tok;
                append = 0;
            } else if (strcmp(tok, ">>") == 0) {
                tok = strtok_r(NULL, " ", &saveptr);
                output_file = tok;
                append = 1;
            } else {
                args[arg_index++] = tok;
            }
            tok = strtok_r(NULL, " ", &saveptr);
        }
        args[arg_index] = NULL;

        // Handle built in commands
        if (args[0] && strcmp(args[0], "cd") == 0) {
            if (args[1] ? chdir(args[1]) != 0 : 1) {
                perror("cd");
            }
            free(cmd);
            return;
        }

        if (args[0] && strcmp(args[0], "history") == 0) {
            show_history();
            free(cmd);
            return;
        }

        // Fork to run external command
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (input_file) {
                int fd = open(input_file, O_RDONLY);
                if (fd == -1) {
                    perror("open input");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (output_file) {
                int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
                int fd = open(output_file, flags, 0644);
                if (fd == -1) {
                    perror("open output");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            if (args[0] != NULL) {
                execvp(args[0], args);
                perror("execvp");
            }
            exit(1);
        } else if (pid > 0) {
            wait(NULL);
        } else {
            perror("fork");
        }
    }
    
    free(cmd);
}
