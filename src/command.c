// TODO remove ../include/
#include "../include/command.h"
#include "../include/request.h"
#include "../include/orchestrator.h"
#include "../include/task_nr.h"  // task number size macro

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_ARGS MAX_CMD_SIZE

#define TASK_PREFIX_NAME "task"
#define OUTPUT_NAME      "out"
#define ERROR_NAME       "err"
#define TIME_NAME        "time"

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
        // Remove leading and trailing whitespaces from each command
        while (*token == ' ' || *token == '\t') {
            token++;
        }
        size_t len = strlen(token);
        while (len > 0 && (token[len - 1] == ' ' || token[len - 1] == '\t')) {
            token[len - 1] = '\0';
            len--;
        }

        // Allocate memory for each command string
        commands[i] = malloc(len + 1);
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

	char *exec_args[MAX_ARGS];

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
    if (fd == -1) {
        perror("Error: output directory doesn't exist");
        return -1;
    }
    (void) close(fd);

    int result;

    // get completed history file path
    int history_file_size = strlen(output_dir) + strlen(HISTORY_NAME) + 2; // +2 for '/' and '\0'
    char *history_file = malloc(history_file_size);
    if (history_file == NULL) {
        perror("Error: couldn't allocate memory for history file path");
        return -1;
    }

    result = snprintf(history_file, history_file_size, "%s/%s", output_dir, HISTORY_NAME);
    if (result < 0 || result >= history_file_size) {
        perror("Error: couldn't create history file path with snprintf");
        return -1;
    }

    // create output directory for task results
    int task_dir_size = strlen(output_dir) + strlen(TASK_PREFIX_NAME) + TASK_NR_STRING_SIZE + 2; // +2 for '/' and '\0'
    char *task_dir = malloc(task_dir_size);
    if (task_dir == NULL) {
        perror("Error: couldn't allocate memory for task directory");
        return -1;
    }

    result = snprintf(task_dir, task_dir_size, "%s/%s%d", output_dir, TASK_PREFIX_NAME, task_nr);
    if (result < 0 || result >= task_dir_size) {
        perror("Error: couldn't create task directory with snprintf");
        return -1;
    }

    if (mkdir(task_dir, 0755) != 0) {
        if (errno == EEXIST) {
            printf("Warning: overwriting task directory\n");
            printf("This is due to inproper termination of the server\n\n");
        }
        else {
            perror("Error: couldn't create task directory");
            return -1;
        }
    }

    // open output file
    int output_file_size = strlen(task_dir) + strlen(OUTPUT_NAME) + 2; // +2 for '/' and '\0'
    char *output_file = malloc(output_file_size);
    if (output_file == NULL) {
        perror("Error: couldn't allocate memory for output file");
        return -1;
    }

    result = snprintf(output_file, output_file_size, "%s/%s", task_dir, OUTPUT_NAME);
    if (result < 0 || result >= output_file_size) {
        perror("Error: couldn't create output file path with snprintf");
        return -1;
    }

    int fd_out = open(output_file, O_WRONLY | O_CREAT, 0644);
    if (fd_out == -1) {
        perror("Error: couldn't open output file");
        return -1;
    }

    // open error file
    int error_file_size = strlen(task_dir) + strlen(ERROR_NAME) + 2; // +2 for '/' and '\0'
    char *error_file = malloc(error_file_size);
    if (error_file == NULL) {
        perror("Error: couldn't allocate memory for error file path");
        return -1;
    }

    result = snprintf(error_file, error_file_size, "%s/%s", task_dir, ERROR_NAME);
    if (result < 0 || result >= error_file_size) {
        perror("Error: couldn't create error file path with snprintf");
        return -1;
    }

    int fd_err = open(error_file, O_WRONLY | O_CREAT, 0644);
    if (fd_err == -1) {
        perror("Error: couldn't open error file");
        return -1;
    }

    // open execution time file
    int time_file_size = strlen(task_dir) + strlen(TIME_NAME) + 2; // +2 for '/' and '\0'
    char *time_file = malloc(time_file_size);
    if (time_file == NULL) {
        perror("Error: couldn't allocate memory for time file path");
        return -1;
    }

    result = snprintf(time_file, time_file_size, "%s/%s", task_dir, TIME_NAME);
    if (result < 0 || result >= time_file_size) {
        perror("Error: couldn't create time file path with snprintf");
        return -1;
    }

    int fd_time = open(time_file, O_WRONLY | O_CREAT, 0644);
    if (fd_time == -1) {
        perror("Error: couldn't open time file");
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
                        (void) close(pipes[i][STDIN_FILENO]);
                        if (dup2(pipes[i][STDOUT_FILENO], STDOUT_FILENO) == -1) {
                            perror("Error: couldn't redirect stdout to pipe");
                            _exit(1);
                        }
                        (void) close(pipes[i][STDOUT_FILENO]);

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
                        (void) close(fd_out);

                        // redirect stderr to error file
                        if (dup2(fd_err, STDERR_FILENO) == -1) {
                            perror("Error: couldn't redirect stderr to error file");
                            _exit(1);
                        }
                        (void) close(fd_err);

                        (void) close(pipes[i - 1][STDOUT_FILENO]);
                        if (dup2(pipes[i - 1][STDIN_FILENO], STDIN_FILENO) == -1) {
                            perror("Error: couldn't redirect stdin to pipe");
                            _exit(1);
                        }
                        (void) close(pipes[i - 1][STDIN_FILENO]);

                        exec_command(commands[i]);
                        perror("Error: couldn't execute command");
                        _exit(1);
                    }
                    // intermediate commands
                    else {
                        (void) close(pipes[i][STDIN_FILENO]);
                        if (dup2(pipes[i][STDOUT_FILENO], STDOUT_FILENO) == -1) {
                            perror("Error: couldn't redirect stdout to pipe");
                            _exit(1);
                        }
                        (void) close(pipes[i][STDOUT_FILENO]);

                        (void) close(pipes[i - 1][STDOUT_FILENO]);
                        if (dup2(pipes[i - 1][STDIN_FILENO], STDIN_FILENO) == -1) {
                            perror("Error: couldn't redirect stdin to pipe");
                            _exit(1);
                        }
                        (void) close(pipes[i - 1][STDIN_FILENO]);

                        exec_command(commands[i]);
                        perror("Error: couldn't execute command");
                        _exit(1);
                    }
                }

                // parent process
                if (i == 0) {
                    // first command
                    (void) close(pipes[i][STDOUT_FILENO]);
                }
                else if (i == N - 1) {
                    // last command
                    (void) close(pipes[i - 1][STDIN_FILENO]);
                }
                else {
                    // intermediate commands
                    (void) close(pipes[i - 1][STDIN_FILENO]);
                    (void) close(pipes[i][STDOUT_FILENO]);
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

            char time_str[EXEC_TIME_STRING_SIZE];
            result = snprintf(time_str, EXEC_TIME_STRING_SIZE, "%ld ms\n", elapsed_time);
            if (result < 0 || result >= EXEC_TIME_STRING_SIZE) {
                perror("Error: couldn't create time string with snprintf");
                return 1;
            }

            if (write(fd_time, time_str, strlen(time_str)) == -1) {
                perror("Error: couldn't write to time file");
                return 1;
            }

            (void) close(fd_time);

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

            (void) close(fd_server);

            char *full_cmd = strdup(get_command(r));

            // 7 = 3 spaces + 2 chars + 1 new line + 1 null
            int history_str_size = TASK_NR_STRING_SIZE + strlen(full_cmd) + strlen(time_str) + 7;
            char *history_str = malloc(history_str_size);
            if (history_str == NULL) {
                perror("Error: couldn't allocate memory for history string");
                return -1;
            }

            result = snprintf(history_str, history_str_size, "%d %s %ld ms\n", task_nr, full_cmd, elapsed_time);
            if (result < 0 || result >= history_str_size) {
                perror("Error: couldn't create history string with snprintf");
                return -1;
            }

            int fd_history = open(history_file, O_WRONLY | O_APPEND | O_CREAT, 0644);
            if (fd_history == -1) {
                perror("Error: couldn't open history file");
                return -1;
            }

            if (write(fd_history, history_str, strlen(history_str)) == -1) {
                perror("Error: couldn't write to history file");
                return -1;
            }

            (void) close(fd_history);

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
                (void) close(fd_out);

                // redirect stderr to error file
                if (dup2(fd_err, STDERR_FILENO) == -1) {
                    perror("Error: couldn't redirect stderr to error file");
                    _exit(1);
                }
                (void) close(fd_err);

                int exec_ret = exec_command(cmd);
                if (exec_ret == -1) {
                    perror("Error: couldn't execute command");
                    _exit(1);
                }

                (void) close(fd_out);
                (void) close(fd_err);
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


                char time_str[EXEC_TIME_STRING_SIZE];
                result = snprintf(time_str, EXEC_TIME_STRING_SIZE, "%ld ms\n", elapsed_time);
                if (result < 0 || result >= EXEC_TIME_STRING_SIZE) {
                    perror("Error: couldn't create time string with snprintf");
                    return 1;
                }

                if (write(fd_time, time_str, strlen(time_str)) == -1) {
                    perror("Error: couldn't write to time file");
                    _exit(1);
                }

                (void) close(fd_time);

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

                (void) close(fd_server);

                // 7 = 3 spaces + 2 chars + 1 new line + 1 null
                int history_str_size = TASK_NR_STRING_SIZE + strlen(cmd) + strlen(time_str) + 7;
                char *history_str = malloc(history_str_size);
                if (history_str == NULL) {
                    perror("Error: couldn't allocate memory for history string");
                    return -1;
                }

                result = snprintf(history_str, history_str_size, "%d %s %ld ms\n", task_nr, cmd, elapsed_time);
                if (result < 0 || result >= history_str_size) {
                    perror("Error: couldn't create history string with snprintf");
                    return -1;
                }

                int fd_history = open(history_file, O_WRONLY | O_APPEND | O_CREAT, 0644);
                if (fd_history == -1) {
                    perror("Error: couldn't open history file");
                    return -1;
                }

                if (write(fd_history, history_str, strlen(history_str)) == -1) {
                    perror("Error: couldn't write to history file");
                    return -1;
                }

                (void) close(fd_history);

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

    (void) close(fd_out);
    (void) close(fd_err);
    (void) close(fd_time);

    free(cmd);
    free(history_file);
    free(task_dir);
    free(output_file);
    free(error_file);
    free(time_file);
    return 0;
}
