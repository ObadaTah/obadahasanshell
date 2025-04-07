
// --- Function Prototypes ---
char *read_input_line();
int split_commands(char *line, char **commands);
int parse_command_args(char *command, char **args);
void execute_command(char **args);
int handle_cd_command(char **args, int num_args);
void print_prompt(); // New function to print the prompt