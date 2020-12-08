#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

//Error Message
char error_message[30] = "An error has occurred\n";
int batch = 0;

//Path Definition
#define max_path 256
char paths[max_path][max_path] = { { "/bin" } };
int paths_size = 1;

//Function Declarations
void promptMessage();
void errorMessage();
void interactiveMode();
void batchMode(const char* filename);
int whichMode(int argc, char const* argv[]);
void parse_parallel_commands(char* string);
void parse_redirect_command(char* string);
void parse_single_command(char* command, char* redirect);
void Path(char* string);
int execute_command(char* command, char* command_name, char* redirect);

//Wish prompt
void promptMessage()
{
    write(STDOUT_FILENO, "wish> ", strlen("wish> "));
}

//Error message
void errorMessage()
{
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
}

//Function to determine shell mode
int whichMode(int argc, char const* argv[])
{
    // Interactive mode, no arguments passed by the user
    if (argc == 1) {
        interactiveMode();
    }

    //Batch mode, user passed in a file for commands to be executed
    else if (argc == 2) {
        batchMode(argv[1]);
    } else {
        errorMessage();
    }
}

//Main Program
int main(int argc, char const* argv[])
{
    whichMode(argc, argv);
}

//Interactive mode
void interactiveMode()
{
    printf("\nInteractive mode running\n");
    while (1) {
        promptMessage();
        char* user_input = NULL;
        ssize_t string_size = 0;
        size_t string_buffer_size = 0;
        string_size = getline(&user_input, &string_buffer_size, stdin);
        parse_parallel_commands(user_input);
    }
}

//Batch mode
void batchMode(const char* filename)
{
    printf("\nBatch mode running using: %s\n", filename);
    FILE* file = fopen(filename, "r");

    char* user_input = NULL;
    ssize_t string_size = 0;
    size_t string_buffer_size = 0;

    while ((string_size = getline(&user_input, &string_buffer_size, file)) >= 0) {
        parse_parallel_commands(user_input);
    };
}

// Parallel commands
void parse_parallel_commands(char* string)
{

    char* command_copy = strdup(string);

    command_copy = strtok_r(command_copy, "\r\n", &command_copy);
    command_copy = strtok_r(command_copy, "\n", &command_copy);

    char* command;

    while ((command = strtok_r(command_copy, "&", &command_copy)) != NULL) {
        parse_redirect_command(command);
    }
}

//Redirection commands
void parse_redirect_command(char* string)
{
    char* command_copy = strdup(string);
    char *command, *redirect;

    command = strtok_r(command_copy, ">", &command_copy);
    redirect = strtok_r(command_copy, " ", &command_copy);
    parse_single_command(command, redirect);
}

//Single commands
void parse_single_command(char* command, char* redirect)
{
    char* command_copy = strdup(command);
    char* command_name = strtok_r(command_copy, " ", &command_copy);

    if ((strcmp(command_name, "path")) == 0) {
        Path(command_copy);
        printf("Path(s) added: ");
        for (int i = 0; i < paths_size; i++) {
            printf("%s\t", paths[i]);
        }
        printf("\n");
    } else if ((strcmp(command_name, "exit")) == 0) {
        exit(0);
    } else if ((strcmp(command_name, "cd")) == 0) {
        if (chdir(command_copy) != 0) {
            errorMessage();
        } else {
            printf("Directory changed!\n");
        }
    } else {
        // handle non-built-in command
        printf("Command: %s (%s), redirecting output to %s\n", command_name, command, redirect);
        execute_command(command, command_name, redirect);
    }
}

//Function for non-built commands
int execute_command(char* command, char* command_name, char* redirect)
{

    //Check if the command can be accessed directly, or in CWD
    if ((access(command_name, X_OK)) == 0) {
    } else

        //If command not found in CWD, check other paths to access command and execute
        for (int i = 0; i < paths_size; i++) {
            char access_path[1024];
            strcat(access_path, paths[i]);
            strcat(access_path, "/");
            strcat(access_path, command_name);

            if ((access(access_path, X_OK)) == 0) {
                command_name = access_path;
            }
        }

    int pid = fork();
    if (pid == 0) {
        // Child process

        char *token, *tmp = strdup(command);
        int num = 0;
        while ((token = strtok_r(tmp, " ", &tmp)) != NULL)
            num++;

        char* commandArgs[num + 1];

        tmp = strdup(command);

        printf("\n");

        for (int i = 0; i <= num; i++) {
            token = strtok_r(tmp, " ", &tmp);
            commandArgs[i] = token;
        }

        printf("\n");

        if (execv(command_name, commandArgs) == -1) {
            errorMessage();
        }

    } else if (pid < 0) {
        // Error with forking
        errorMessage();
    } else {
        // Parent process
        wait(NULL);
    }

    return 1;
}

//Function for path
void Path(char* string)

{
    char* path;
    int index = 0;
    while ((path = strtok_r(string, " ", &string)) != NULL) {
        strcpy(paths[index], path);
        index++;
    }
    paths_size = index;
}
