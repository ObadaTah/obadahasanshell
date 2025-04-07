#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <utils.h>

#define MAX_INPUT_LINE 1024
#define MAX_ARGS 64
#define MAX_COMMANDS 10
#define PROMPT_SYMBOL " ObadaHasanShell >"
/**
 * @brief Prints the shell prompt, including the current working directory.
 */
void print_prompt()
{
    char cwd[PATH_MAX]; // Buffer to hold the current working directory path

    // Get the current working directory
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        // Print directory and prompt symbol
        printf("%s%s", cwd, PROMPT_SYMBOL);
    }
    else
    {
        // Error getting CWD, print a default prompt or error
        perror("myshell: getcwd error");
        printf("?%s", PROMPT_SYMBOL); // Simple fallback prompt
    }
}

/**
 * @brief Reads a line of input from stdin.
 * @return A dynamically allocated string containing the input line,
 * or NULL on EOF or allocation error. The caller must free the returned string.
 * Returns an empty string "" if the user just presses Enter.
 */
char *read_input_line()
{
    char *line = NULL;
    size_t bufsize = 0;

    if (getline(&line, &bufsize, stdin) == -1)
    {
        if (feof(stdin))
        {
            free(line);
            return NULL;
        }
        else
        {
            perror("myshell: getline error");
            free(line);
            return NULL;
        }
    }
    line[strcspn(line, "\n")] = 0; // Remove trailing newline
    return line;
}

/**
 * @brief Splits a line into commands based on the semicolon delimiter.
 * Modifies the input line by replacing ';' with null terminators.
 * @param line The input line string. This string will be modified.
 * @param commands An array of char pointers to store the start of each command.
 * @return The number of commands found, or -1 on error (e.g., too many commands).
 */
int split_commands(char *line, char **commands)
{
    int count = 0;
    char *token;
    char *rest = line;

    while ((token = strtok_r(rest, ";", &rest)) != NULL && count < MAX_COMMANDS)
    {
        while (*token == ' ' || *token == '\t')
            token++;
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t'))
            *end-- = '\0';
        if (strlen(token) > 0)
        {
            commands[count++] = token;
        }
    }

    if (count >= MAX_COMMANDS && strtok_r(rest, ";", &rest) != NULL)
    {
        fprintf(stderr, "myshell: Too many commands on one line (max %d).\n", MAX_COMMANDS);
        return -1;
    }
    commands[count] = NULL;
    return count;
}

/**
 * @brief Parses a single command string into an array of arguments.
 * Modifies the input command string by replacing spaces with null terminators.
 * @param command The command string to parse. This string will be modified.
 * @param args An array of char pointers to store the arguments.
 * @return The number of arguments (argc), or -1 on error (e.g. too many args).
 * Returns 0 if the command string is empty or contains only whitespace.
 */
int parse_command_args(char *command, char **args)
{
    int count = 0;
    char *token;
    char *rest = command;

    while ((token = strtok_r(rest, " \t\n\r\a", &rest)) != NULL && count < (MAX_ARGS - 1))
    {
        args[count++] = token;
    }

    if (count >= (MAX_ARGS - 1) && strtok_r(rest, " \t\n\r\a", &rest) != NULL)
    {
        fprintf(stderr, "myshell: Too many arguments for command (max %d).\n", MAX_ARGS - 1);
        args[count] = NULL;
        return -1;
    }
    args[count] = NULL;
    return count;
}

/**
 * @brief Executes an external command using execvp.
 * Should be called from the child process. Does not return if successful.
 * @param args Null-terminated array of strings (arguments).
 */
void execute_command(char **args)
{
    if (args[0] == NULL)
    {
        fprintf(stderr, "myshell: Attempted to execute an empty command.\n");
        exit(EXIT_FAILURE);
    }
    if (execvp(args[0], args) == -1)
    {
        perror("myshell"); // Prints execvp error
    }
    // If execvp fails, the child MUST exit. This is handled in the main loop.
}

/**
 * @brief Handles the built-in 'cd' command. Executes in the parent process.
 * @param args Null-terminated array of arguments (args[0] is "cd").
 * @param num_args The number of arguments, including "cd".
 * @return 0 on success, -1 on failure.
 */
int handle_cd_command(char **args, int num_args)
{
    char *target_dir = NULL;

    if (num_args > 2)
    {
        fprintf(stderr, "myshell: cd: too many arguments\n");
        return -1;
    }
    else if (num_args == 1)
    {
        target_dir = getenv("HOME");
        if (target_dir == NULL)
        {
            fprintf(stderr, "myshell: cd: HOME environment variable not set\n");
            return -1;
        }
    }
    else
    {
        target_dir = args[1];
    }

    if (chdir(target_dir) != 0)
    {
        perror("myshell: cd"); // Prints chdir error
        return -1;
    }
    return 0; // Success
}
