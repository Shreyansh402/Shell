// Assignment 1: Buiding a basic shell
// Name: Shreyansh Jain
// Entry no.: 2021MT10230
// Professor: Ashutosh Rai
// Course: MTL458: Operating Systems

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_SIZE 100
#define MAX_ARGS 50
#define MAX_HISTORY 100
#define MAX_STORED 1000
#define MAX_RECENT 15

// Function to read input from the user
char *read_input()
{
    char *input = (char *)malloc(MAX_SIZE);
    fgets(input, MAX_SIZE, stdin);
    return input;
}

// Function to return array of last n commands from the shell_history file
void get_history(int n, char *temp[])
{
    FILE *file = fopen("shell_history", "r");
    char *line = NULL;
    size_t len = 0;
    int stored;
    getline(&line, &len, file);
    stored = atoi(line);
    if (n > stored)
    {
        n = stored;
    }
    for (int i = 0; i < stored - n; i++)
    {
        getline(&line, &len, file);
    }
    for (int i = 0; i < n; i++)
    {
        getline(&line, &len, file);
        temp[i] = strdup(line);
    }
    fclose(file);
}
// Function for history command
void history_command(int length, char *history[], char *args[])
{
    // if no argument is provided, print recent commands
    if (args[1] == NULL)
    {
        if (length - 1 < MAX_RECENT)
        {
            for (int i = 0; i < length - 1; i++)
            {
                if (history[i] != NULL)
                {
                    printf("%d. %s\n", i + 1, history[i]);
                }
            }
        }
        else
        {
            for (int i = length - 1 - MAX_RECENT; i < length - 1; i++)
            {
                if (history[i] != NULL)
                {
                    printf("%d. %s\n", i + MAX_RECENT - length, history[i]);
                }
            }
        }
    }
    // if argument is integer, print that many recent commands
    else if (atoi(args[1]) != 0)
    {
        int n = atoi(args[1]);
        // if n is greater than length of history in RAM, fetch from the shell_history file last n-length(history) commands
        if (n > length - 1)
        {
            char *temp[MAX_STORED];
            // if file does not exist, print recent commands
            FILE *file = fopen("shell_history", "r");
            if (file == NULL)
            {
                fclose(file);
                for (int i = 0; i < length - 1; i++)
                {
                    if (history[i] != NULL)
                    {
                        printf("%d. %s\n", i + 1, history[i]);
                    }
                }
            }
            else
            {
                fclose(file);
                get_history(n - length + 1, temp);
                for (int i = 0; i < n - length + 1; i++)
                {
                    printf("%d. %s", i + 1, temp[i]);
                }
                for (int i = 0; i < length - 1; i++)
                {
                    if (history[i] != NULL)
                    {
                        printf("%d. %s\n", i + n - length + 2, history[i]);
                    }
                }
            }
        }
        else
        {
            for (int i = length - 1 - n; i < length - 1; i++)
            {
                if (history[i] != NULL)
                {
                    printf("%d. %s\n", i - length + n + 2, history[i]);
                }
            }
        }
    }
    else
    {
        printf("Invalid argument\n");
    }
}

// Function to save the history of commands
void save_history(char *history[], int length)
{
    // check if shell_history file does not exist
    FILE *file = fopen("shell_history", "r");
    if (file == NULL)
    {
        fclose(file);
        file = fopen("shell_history", "w");
        fprintf(file, "%d\n", length);
        for (int i = 0; i < length; i++)
        {
            fprintf(file, "%s\n", history[i]);
            history[i] = NULL;
        }
        fclose(file);
        return;
    }

    // read first line of the shell_history file
    char *line = NULL;
    size_t len = MAX_SIZE;
    getline(&line, &len, file);

    if (atoi(line) == 0)
    { // File is corrupted, so delete it
        fclose(file);
        remove("shell_history");
        save_history(history, length);
        return;
    }

    int stored = atoi(line);
    if (stored + length > MAX_STORED)
    {
        fclose(file);
        // remove older half commands from the file
        char *temp[MAX_STORED];
        get_history(MAX_STORED / 2, temp);
        // write the recent commands in the file
        file = fopen("shell_history", "w");
        fprintf(file, "%d\n", length + MAX_STORED / 2);
        for (int i = 0; i < MAX_STORED / 2; i++)
        {
            fprintf(file, "%s\n", temp[i]);
        }
        for (int i = 0; i < length; i++)
        {
            fprintf(file, "%s\n", history[i]);
            history[i] = NULL;
        }
        fclose(file);
        return;
    }
    // write the recent commands in the file
    file = fopen("shell_history", "a");
    for (int i = 0; i < length; i++)
    {
        fprintf(file, "%s\n", history[i]);
        history[i] = NULL;
    }
    fclose(file);
    // update the first line of the file
    file = fopen("shell_history", "r+");
    fprintf(file, "%d\n", stored + length);
    fclose(file);
}

int main()
{
    char *input;
    char *arg;
    char *args[MAX_ARGS];
    char *history[MAX_HISTORY];
    int length = 0;
    int piping = 0;
    char *command2 = NULL;
    char *command = NULL;
    int original_stdout = dup(1);
    int original_stdin = dup(0);

    while (1)
    {
        char *copied = malloc(strlen(input) + 1);
        if (piping == 0)
        {
            // set the stdin to the terminal
            dup2(original_stdin, 0);

            printf("MTL458 >");
            input = read_input();
            input[strlen(input) - 1] = '\0'; // remove the newline character
            copied = strdup(input);
            free(command2);
        }
        else
        {
            input = strdup(command2);
            piping = 0;
            free(command2);
            copied = NULL;

            // fetch the output of the first command from the file as stdin
            FILE *file = fopen("temp.txt", "r");
            int fd = fileno(file);
            dup2(fd, 0);
            close(fd);

            // set the stdout to the terminal
            dup2(original_stdout, 1);
        }
        // create a copy of input to store the command

        command = (char *)malloc(strlen(input) + 1);
        command2 = (char *)malloc(strlen(input) + 1);
        // strcpy(command, input);

        // check if the input is empty
        if (strlen(input) == 0)
        {
            continue;
        }

        // split the input into arguments
        arg = strtok(input, " ");
        int i = 0;
        piping = 0;
        while (arg != NULL)
        {
            if (piping == 1)
            {
                // creating the second command
                command2 = strcat(command2, arg);
                command2 = strcat(command2, " ");
            }
            else
            {
                args[i] = strdup(arg);
                command = strcat(command, arg);
                command = strcat(command, " ");
                // check if the input command has piping and piping is supported
                if (strcmp(args[i], "|") == 0)
                {
                    piping = 1;
                    args[i] = NULL;
                    command[strlen(command) - 2] = '\0';

                    // store output of the first command in a file
                    FILE *file = fopen("temp.txt", "w");
                    int fd = fileno(file);

                    // set the stdout to the file
                    dup2(fd, 1);
                    close(fd);
                }
            }
            arg = strtok(NULL, " ");
            i++;
        }
        if (piping == 1)
        {
            command2[strlen(command2)] = '\0';
        }
        else
        {
            args[i] = NULL;
        }

        // store the command in history
        if (copied != NULL)
        {

            if (length < MAX_HISTORY)
            {
                history[length] = strdup(copied);
                length++;
            }
            else
            {
                // save all except some recent commands in the shell_history file
                save_history(history, MAX_HISTORY - MAX_RECENT);
                // store the recent commands in the history array
                for (int i = 0; i < MAX_RECENT; i++)
                {
                    history[i] = strdup(history[MAX_HISTORY - MAX_RECENT + i]);
                    free(history[MAX_HISTORY - MAX_RECENT + i]);
                    history[MAX_HISTORY - MAX_RECENT + i] = NULL;
                }
                history[MAX_RECENT] = strdup(copied);
                length = MAX_RECENT + 1;
            }
        }
        // exit command
        if (strcmp(args[0], "exit") == 0)
        {
            printf("Saving session...\n");
            // save all the commands in the shell_history file
            save_history(history, length);
            printf("...completed.\n");
            break;
        }

        // ls, cat, echo, sleep commands
        else if (strcmp(args[0], "ls") == 0 || strcmp(args[0], "cat") == 0 || strcmp(args[0], "echo") == 0 || strcmp(args[0], "sleep") == 0)
        {
            system(command);
        }
        // cd command
        else if (strcmp(args[0], "cd") == 0)
        {
            if (chdir(args[1]) != 0)
            {
                printf("The File Path Does not exist\n");
            }
        }
        // // history command
        else if (strcmp(args[0], "history") == 0)
        {
            history_command(length, history, args);
        }

        // exec system call
        else
        {
            // fork a child process
            pid_t pid = fork();

            if (pid == 0) // child process
            {
                execvp(args[0], args);
                printf("Error: Command not found\n");
                exit(0); // exit the child process
            }
            else // parent process
            {
                wait(NULL); // wait for the child process to complete
            }
        }
        free(input);
        free(copied);
        free(command);
    }
    return 0;
}