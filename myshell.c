#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

#define MAX_INPUT_LINE 1024 // Maximum length of a single input line
#define MAX_ARGS 64         // Maximum number of arguments per command
#define MAX_COMMANDS 10     // Maximum number of commands separated by ';'
#define PROMPT_SYMBOL "> "  // The symbol part of the shell prompt

// --- Function Prototypes ---
char *read_input_line();
int split_commands(char *line, char **commands);
int parse_command_args(char *command, char **args);
void execute_command(char **args);
int handle_cd_command(char **args, int num_args);
void print_prompt(); // New function to print the prompt

// --- Main Function ---
int main()
{
    char *line;
    char *commands[MAX_COMMANDS];
    char *args[MAX_ARGS];
    int num_commands;
    pid_t pids[MAX_COMMANDS];
    int num_children = 0;

    // Main shell loop
    while (1)
    {
        print_prompt(); // Print the prompt with the current directory
        fflush(stdout);

        line = read_input_line();

        if (line == NULL)
        {
            printf("\n");
            break;
        }
        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        num_commands = split_commands(line, commands);
        if (num_commands < 0)
        {
            free(line);
            continue;
        }

        num_children = 0;

        for (int i = 0; i < num_commands; i++)
        {
            int num_args = parse_command_args(commands[i], args);

            if (num_args <= 0)
            {
                if (num_args < 0)
                {
                    fprintf(stderr, "myshell: Error parsing command: %s\n", commands[i]);
                }
                continue;
            }

            // --- Built-in Command Check ---
            if (strcmp(args[0], "quit") == 0)
            {
                free(line);
                for (int j = 0; j < num_children; j++)
                {
                    waitpid(pids[j], NULL, 0);
                }
                printf("Exiting myshell.\n");
                return EXIT_SUCCESS;
            }
            if (strcmp(args[0], "cd") == 0)
            {
                handle_cd_command(args, num_args);
                // No error check needed here as handle_cd prints errors
                continue; // Skip fork/exec for cd
            }

            // --- External Command Execution ---
            pid_t pid = fork();

            if (pid < 0)
            {
                perror("myshell: fork failed");
                continue;
            }
            else if (pid == 0)
            {
                // Child Process
                execute_command(args);
                exit(EXIT_FAILURE); // Exit child if execvp fails
            }
            else
            {
                // Parent Process
                if (num_children < MAX_COMMANDS)
                {
                    pids[num_children++] = pid;
                }
                else
                {
                    fprintf(stderr, "myshell: Too many concurrent commands, waiting...\n");
                    wait(NULL);
                }
            }
        } // End command loop

        // Wait for children
        for (int i = 0; i < num_children; i++)
        {
            int status;
            waitpid(pids[i], &status, 0);
        }

        free(line); // Free input line

    } // End main loop

    return EXIT_SUCCESS;
}
