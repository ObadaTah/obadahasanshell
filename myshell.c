#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

#define MAX_INPUT_LINE 1024
#define MAX_ARGS 64
#define MAX_COMMANDS 10
#define PROMPT_SYMBOL " ObadaHasanShell> "

char *read_input_line();
int split_commands(char *line, char **commands);
int parse_command_args(char *command, char **args);
void execute_command(char **args);
int handle_cd_command(char **args, int num_args);
void print_prompt();

int main()
{
    char *line;
    char *commands[MAX_COMMANDS];
    char *args[MAX_ARGS];
    int num_commands;
    pid_t pids[MAX_COMMANDS];
    int num_children = 0;

    while (1)
    {
        print_prompt();
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

                continue;
            }

            pid_t pid = fork();

            if (pid < 0)
            {
                perror("myshell: fork failed");
                continue;
            }
            else if (pid == 0)
            {
                execute_command(args);
                exit(EXIT_FAILURE);
            }
            else
            {
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
        }
        for (int i = 0; i < num_children; i++)
        {
            int status;
            waitpid(pids[i], &status, 0);
        }

        free(line);
    }

    return EXIT_SUCCESS;
}

void print_prompt()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s%s", cwd, PROMPT_SYMBOL);
    }
    else
    {
        perror("myshell: getcwd error");
        printf("?%s", PROMPT_SYMBOL);
    }
}

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
    line[strcspn(line, "\n")] = 0;
    return line;
}

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

void execute_command(char **args)
{
    if (args[0] == NULL)
    {
        fprintf(stderr, "myshell: Attempted to execute an empty command.\n");
        exit(EXIT_FAILURE);
    }
    if (execvp(args[0], args) == -1)
    {
        perror("myshell");
    }
}

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
        perror("myshell: cd");
        return -1;
    }
    return 0;
}
