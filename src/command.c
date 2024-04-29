// TODO remove ../include/
#include "../include/command.h"
#include "../include/request.h"
#include "../include/orchestrator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_CMDS 10

/**
 * Parse a piped command into an array of strings
 *
 * @param cmd The command to parse
 * @param N The number of commands in the array
 * @return An array of strings, or NULL if an error occurred
 */
char** parse_cmd(char *cmd, int *N) {
    // Count the number of pipes to determine the number of commands
    *N = 1; // At least one command
    for (char *c = cmd; *c != '\0'; c++) {
        if (*c == '|') {
            (*N)++;
        }
    }

    // Allocate memory for the array of strings
    char **commands = malloc(*N * sizeof(char*));
    if (commands == NULL) {
        perror("Error: couldn't allocate memory for piped commands");
        return NULL;
    }

    // Tokenize the input command based on pipe character
    char *token = strtok(cmd, "|");
    int i = 0;
    while (token != NULL) {
        // Remove leading and trailing whitespaces from each command TODO add new line
        while (*token == ' ' || *token == '\t') {
            token++;
        }
        size_t len = strlen(token);
        while (len > 0 && (token[len - 1] == ' ' || token[len - 1] == '\t')) {
            token[len - 1] = '\0';
            len--;
        }

        // Allocate memory for each command string
        commands[i] = malloc((len + 1) * sizeof(char));
        if (commands[i] == NULL) {
            perror("Error: couldn't allocate memory for piped command");
            return NULL;
        }

        // Copy the token into the array
        strcpy(commands[i], token);

        // Get the next token
        token = strtok(NULL, "|");
        i++;
    }

    return commands;
}

int exec_command(char* arg){

	char *exec_args[MAX_CMDS];

	char *string;
	int exec_ret = 0;

	char* command = strdup(arg);
    if (command == NULL) {
        perror("Error: couldn't allocate memory for command in exec_command");
        return -1;
    }

	string = strtok(command, " ");

	int i = 0;
	while (string != NULL) {
		exec_args[i] = string;
		string = strtok(NULL, " ");
		i++;
	}

	exec_args[i] = NULL;

	exec_ret = execvp(exec_args[0], exec_args);

	return exec_ret;
}

/**
 * Execute a command
 * @details TODO
 *
 * @param r The request containing the command to execute, if the command is piped and the task number
 * @param output_dir The output directory to write the task results
 * @param start_time The time when the execute request was received
 * @return 0 if successful
 */
int exec(Request *r, char *output_dir, struct timeval start_time) {

    char *cmd = strdup(get_command(r));
    if (cmd == NULL) {
        perror("Error: couldn't allocate memory for command in exec");
        return -1;
    }
    bool is_piped = get_is_piped(r);
    int task_nr = get_task_nr(r);

    int fd = open(output_dir, O_DIRECTORY);
    if (fd < 0) {
        perror("Error: output directory doesn't exist");
        close(fd);
        return -1;
    }
    close(fd);

    // get completed history file path
    char *history_file = malloc(strlen(output_dir) + strlen(HISTORY) + 2);
    if (history_file == NULL) {
        perror("Error: couldn't allocate memory for history file path");
        return -1;
    }

    if (sprintf(history_file, "%s/%s", output_dir, HISTORY) < 0) {
        perror("Error: couldn't create history file path with sprintf");
        return -1;
    }

    // create output directory for task results
    char *task_dir = malloc(strlen(output_dir) + strlen("task") + 2 + 8); // TODO max size of task number
    if (task_dir == NULL) {
        perror("Error: couldn't allocate memory for task directory");
        return -1;
    }

    if (sprintf(task_dir, "%s/task%d", output_dir, task_nr) < 0) {
        perror("Error: couldn't create task directory with sprintf");
        return -1;
    }

    if (mkdir(task_dir, 0755) != 0) {
        perror("Error: couldn't create task directory");
        return -1;
    }

    // open output file
    char *output_file = malloc(strlen(task_dir) + 3 + 2);
    if (output_file == NULL) {
        perror("Error: couldn't allocate memory for output file");
        return -1;
    }

    if (sprintf(output_file, "%s/out", task_dir) < 0) {
        perror("Error: couldn't create output file path with sprintf");
        return -1;
    }

    int fd_out = open(output_file, O_WRONLY | O_CREAT, 0644);
    if (fd_out < 0) {
        perror("Error: couldn't open output file");
        close(fd_out);
        return -1;
    }

    // open error file
    char *error_file = malloc(strlen(task_dir) + 3 + 2);
    if (error_file == NULL) {
        perror("Error: couldn't allocate memory for error file path");
        return -1;
    }

    if (sprintf(error_file, "%s/err", task_dir) < 0) {
        perror("Error: couldn't create error file path with sprintf");
        return -1;
    }

    int fd_err = open(error_file, O_WRONLY | O_CREAT, 0644);
    if (fd_err < 0) {
        perror("Error: couldn't open error file");
        close(fd_err);
        return -1;
    }

    // open execution time file
    char *time_file = malloc(strlen(task_dir) + 4 + 2);
    if (time_file == NULL) {
        perror("Error: couldn't allocate memory for time file path");
        return -1;
    }

    if (sprintf(time_file, "%s/time", task_dir) < 0) {
        perror("Error: couldn't create time file path with sprintf");
        return -1;
    }

    int fd_time = open(time_file, O_WRONLY | O_CREAT, 0644);
    if (fd_time < 0) {
        perror("Error: couldn't open time file");
        close(fd_time);
        return -1;
    }

    if (is_piped) {
        pid_t exec_pid = fork();
        if (exec_pid == -1) {
            perror("Error: couldn't fork");
            return 1;
        }

        if (exec_pid == 0) {
            int N = 0;

            char **commands = parse_cmd(cmd, &N);

            if (commands == NULL) {
                perror("Error: couldn't parse piped commands");
                return -1;
            }

            int pipes[N - 1][2];

            for (int i = 0; i < N; i++) {

                // create pipe for all commands except the last one
                if (i != N - 1) {
                    if (pipe(pipes[i]) == -1) {
                        perror("Error: couldn't create pipe");
                        return 1;
                    }
                }

                pid_t pid = fork();
                if (pid == -1) {
                    perror("Error: couldn't fork");
                    return 1;
                }

                if (pid == 0) {

                    // first command
                    if (i == 0) {
                        close(pipes[i][STDIN_FILENO]);
                        dup2(pipes[i][STDOUT_FILENO], STDOUT_FILENO);
                        close(pipes[i][STDOUT_FILENO]);

                        exec_command(commands[i]);
                        perror("Error: couldn't execute command");
                        _exit(1);
                    }
                    // last command
                    else if (i == N - 1) {
                        // redirect stdout to output file
                        if (dup2(fd_out, STDOUT_FILENO) == -1) {
                            perror("Error: couldn't redirect stdout to output file");
                            _exit(1);
                        }
                        close(fd_out);

                        // redirect stderr to error file
                        if (dup2(fd_err, STDERR_FILENO) == -1) {
                            perror("Error: couldn't redirect stderr to error file");
                            _exit(1);
                        }
                        close(fd_err);

                        close(pipes[i - 1][STDOUT_FILENO]);
                        dup2(pipes[i - 1][STDIN_FILENO], STDIN_FILENO);
                        close(pipes[i - 1][STDIN_FILENO]);

                        exec_command(commands[i]);
                        perror("Error: couldn't execute command");
                        _exit(1);
                    }
                    // intermediate commands
                    else {
                        close(pipes[i][STDIN_FILENO]);
                        dup2(pipes[i][STDOUT_FILENO], STDOUT_FILENO);
                        close(pipes[i][STDOUT_FILENO]);

                        close(pipes[i - 1][STDOUT_FILENO]);
                        dup2(pipes[i - 1][STDIN_FILENO], STDIN_FILENO);
                        close(pipes[i - 1][STDIN_FILENO]);

                        exec_command(commands[i]);
                        perror("Error: couldn't execute command");
                        _exit(1);
                    }
                }

                // parent process
                if (i == 0) {
                    // first command
                    close(pipes[i][STDOUT_FILENO]);
                }
                else if (i == N - 1) {
                    // last command
                    close(pipes[i - 1][STDIN_FILENO]);
                }
                else {
                    // intermediate commands
                    close(pipes[i - 1][STDIN_FILENO]);
                    close(pipes[i][STDOUT_FILENO]);
                }
            }
            // wait for all children
            for (int i = 0; i < N; i++) {

                int status = 0;

                if (wait(&status) == -1) {
                    perror("Error: couldn't wait for child process");
                    return 1;
                }

                if (WIFEXITED(status)) {
                    if (WEXITSTATUS(status) != 0) {
                        fprintf(stderr, "Command %d failed with status %d\n", i + 1, WEXITSTATUS(status));
                        return 1;
                    }
                }
            }

            // write execution time (ms) to file
            struct timeval end_time;
            gettimeofday(&end_time, NULL);

            long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000L +
                                (end_time.tv_usec - start_time.tv_usec) / 1000L;

            char time_str[20];
            if (sprintf(time_str, "%ld ms\n", elapsed_time) < 0) {
                perror("Error: couldn't create time string with sprintf");
                return 1;
            }

            if (write(fd_time, time_str, strlen(time_str)) == -1) {
                perror("Error: couldn't write to time file");
                return 1;
            }

            close(fd_time);

            // send this request to the orchestrator as completed
            set_type(r, COMPLETED);

            int fd_server = open(SERVER_FIFO, O_WRONLY);
            if (fd_server == -1) {
                perror("Error: couldn't open server FIFO");
                return 1;
            }

            if (write(fd_server, r, sizeof_request()) == -1) {
                perror("Error: couldn't write to server FIFO");
                return 1;
            }

            close(fd_server);

            char *history_str = malloc(8 + strlen(cmd) + strlen(time_str) + 5); // TODO max size of task number
            if (history_str == NULL) {
                perror("Error: couldn't allocate memory for history string");
                return -1;
            }

            if (sprintf(history_str, "%d %s %ld ms\n", task_nr, cmd, elapsed_time) < 0) {
                perror("Error: couldn't create history string with sprintf");
                return -1;
            }

            int fd_history = open(history_file, O_WRONLY | O_APPEND | O_CREAT, 0644);
            if (fd_history < 0) {
                perror("Error: couldn't open history file");
                close(fd_history);
                return -1;
            }

            if (write(fd_history, history_str, strlen(history_str)) == -1) {
                perror("Error: couldn't write to history file");
                return -1;
            }

            close(fd_history);

            free(history_str);
            free(cmd);
            free(history_file);
            free(task_dir);
            free(output_file);
            free(error_file);
            free(time_file);

            // free commands
            for (int i = 0; i < N; i++) {
                free(commands[i]);
            }
            free(commands);
            _exit(0);
        }
    }
    else {
        pid_t exec_pid = fork();
        if (exec_pid == -1) {
            perror("Error: couldn't fork");
            return 1;
        }

        if (exec_pid == 0) {

            pid_t pid = fork();
            if (pid == -1) {
                perror("Error: couldn't fork");
                return 1;
            }

            if (pid == 0) {
                // redirect stdout to output file
                if (dup2(fd_out, STDOUT_FILENO) == -1) {
                    perror("Error: couldn't redirect stdout to output file");
                    _exit(1);
                }
                close(fd_out);

                // redirect stderr to error file
                if (dup2(fd_err, STDERR_FILENO) == -1) {
                    perror("Error: couldn't redirect stderr to error file");
                    _exit(1);
                }
                close(fd_err);

                int exec_ret = exec_command(cmd);
                if (exec_ret == -1) {
                    perror("Error: couldn't execute command");
                    _exit(1);
                }

                close(fd_out);
                close(fd_err);
                _exit(0);
            }
            else {
                // TODO check for error
                (void) waitpid(pid, NULL, 0);

                // write execution time (ms) to file
                struct timeval end_time;
                gettimeofday(&end_time, NULL);

                long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000L +
                                    (end_time.tv_usec - start_time.tv_usec) / 1000L;

                char time_str[20];
                if (sprintf(time_str, "%ld ms\n", elapsed_time) < 0) {
                    perror("Error: couldn't create time string with sprintf");
                    _exit(1);
                }

                if (write(fd_time, time_str, strlen(time_str)) == -1) {
                    perror("Error: couldn't write to time file");
                    _exit(1);
                }

                close(fd_time);

                // send this request to the orchestrator as completed
                set_type(r, COMPLETED);

                int fd_server = open(SERVER_FIFO, O_WRONLY);
                if (fd_server == -1) {
                    perror("Error: couldn't open server FIFO");
                    return 1;
                }

                if (write(fd_server, r, sizeof_request()) == -1) {
                    perror("Error: couldn't write to server FIFO");
                    return 1;
                }

                close(fd_server);

                char *history_str = malloc(8 + strlen(cmd) + strlen(time_str) + 5); // TODO max size of task number
                if (history_str == NULL) {
                    perror("Error: couldn't allocate memory for history string");
                    return -1;
                }

                if (sprintf(history_str, "%d %s %ld ms\n", task_nr, cmd, elapsed_time) < 0) {
                    perror("Error: couldn't create history string with sprintf");
                    return -1;
                }

                int fd_history = open(history_file, O_WRONLY | O_APPEND | O_CREAT, 0644);
                if (fd_history < 0) {
                    perror("Error: couldn't open history file");
                    close(fd_history);
                    return -1;
                }

                if (write(fd_history, history_str, strlen(history_str)) == -1) {
                    perror("Error: couldn't write to history file");
                    return -1;
                }

                close(fd_history);

                free(history_str);
                free(cmd);
                free(history_file);
                free(task_dir);
                free(output_file);
                free(error_file);
                free(time_file);
            }
            _exit(0);
        }
    }

    close(fd_out);
    close(fd_err);
    close(fd_time);

    free(cmd);
    free(history_file);
    free(task_dir);
    free(output_file);
    free(error_file);
    free(time_file);
    return 0;
}
