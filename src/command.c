// TODO remove ../include/
#include "../include/command.h"
#include "../include/orchestrator.h"
#include "../include/task_nr.h"  // task number size macro

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

/** @brief The max number of arguments in a command. **/
#define MAX_ARGS MAX_CMD_SIZE

/** @brief The max size of the error message. **/
#define ERROR_MSG_SIZE MAX_CMD_SIZE + 100

/** @brief The prefix name for the task directories. **/
#define TASK_PREFIX_NAME "task"

/** @brief The name of the output file. **/
#define OUTPUT_NAME "out"

/** @brief The name of the error file. **/
#define ERROR_NAME "err"

/** @brief The name of the time file. **/
#define TIME_NAME "time"

char** parse_cmd_pipes(char *cmd, int *N);

char** parse_cmd(char* cmd);

void write_error(char *cmd_name);

/**
 * @brief Executes a command, single or piped, using process forking,
 * it also handles the redirection of stdout and stderr to respective
 * task files, as well as the writing of the execution time to a file,
 * and the writing of the task number, command and execution time to
 * the history file.
 *
 * @details This is the main function of the @ref command.c
 * module, used in @ref orchestrator.c server to execute a command.
 *
 * First, all the necessary files are created, such as the output
 * directory for the task files, such as the output file,
 * the error file and the time file. Also opens the history file
 * to write the task number, the command and the execution time.
 *
 * This file names are defined in the macros @ref OUTPUT_NAME,
 * @ref ERROR_NAME, @ref TIME_NAME and @ref HISTORY_NAME.
 *
 * It uses the fork system call to create a child process that will
 * execute the command. The parent process will wait for the child
 * process to finish. This fork call is inside another fork call
 * so the parent process can wait for the child process to finish
 * and send the request to the orchestrator as completed. This way
 * the server can continue to receive and handle new requests while
 * the child process is executing the command.
 *
 * The inner parent process, after waiting for the child process to
 * finish executing the command, writes the total time since the
 * request was received and the command finnished executing to the
 * time file and the history file.
 *
 * It uses the @ref parse_cmd function to parse the command into an
 * array of arguments. It also uses the @ref parse_cmd_pipes function
 * in case that the command is piped.
 *
 * After the command is parsed is executed using the execvp system call.
 * When execvp fails (e.g returns -1) it writes an error message to
 * stderr using the @ref write_error function.
 *
 * @param r The request containing the command to execute,
 * if the command is piped and the task number
 * @param output_dir The output directory to create the task directory
 * and respective task result files
 * @param start_time The time when the execute request was received
 * to calculate the total time since the request was received
 * and the command finnished executing
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

    int fd_err = open(error_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
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
            return -1;
        }

        if (exec_pid == 0) {
            int N = 0;

            char **commands = parse_cmd_pipes(cmd, &N);
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
                        return -1;
                    }
                }

                pid_t pid = fork();
                if (pid == -1) {
                    perror("Error: couldn't fork");
                    return -1;
                }

                if (pid == 0) {

                    // first command
                    if (i == 0) {
                        (void) close(pipes[i][STDIN_FILENO]);
                        if (dup2(pipes[i][STDOUT_FILENO], STDOUT_FILENO) == -1) {
                            perror("Error: couldn't redirect stdout to pipe");
                            _exit(i + 1);
                        }
                        (void) close(pipes[i][STDOUT_FILENO]);

                        // redirect stderr to error file
                        if (dup2(fd_err, STDERR_FILENO) == -1) {
                            perror("Error: couldn't redirect stderr to error file");
                            _exit(i + 1);
                        }
                        (void) close(fd_err);

                        char **exec_args = parse_cmd(commands[i]);
                        int exec_ret = execvp(exec_args[0], exec_args);
                        if (exec_ret == -1) {
                            write_error(exec_args[0]);
                            // perror("Error: couldn't execute command");
                            _exit(i + 1);
                        }
                        free(exec_args);
                    }
                    // last command
                    else if (i == N - 1) {
                        // redirect stdout to output file
                        if (dup2(fd_out, STDOUT_FILENO) == -1) {
                            perror("Error: couldn't redirect stdout to output file");
                            _exit(i + 1);
                        }
                        (void) close(fd_out);

                        (void) close(pipes[i - 1][STDOUT_FILENO]);
                        if (dup2(pipes[i - 1][STDIN_FILENO], STDIN_FILENO) == -1) {
                            perror("Error: couldn't redirect stdin to pipe");
                            _exit(i + 1);
                        }
                        (void) close(pipes[i - 1][STDIN_FILENO]);

                        // redirect stderr to error file
                        if (dup2(fd_err, STDERR_FILENO) == -1) {
                            perror("Error: couldn't redirect stderr to error file");
                            _exit(i + 1);
                        }
                        (void) close(fd_err);

                        char **exec_args = parse_cmd(commands[i]);
                        int exec_ret = execvp(exec_args[0], exec_args);
                        if (exec_ret == -1) {
                            write_error(exec_args[0]);
                            // perror("Error: couldn't execute command");
                            _exit(i + 1);
                        }
                        free(exec_args);
                    }
                    // intermediate commands
                    else {
                        (void) close(pipes[i][STDIN_FILENO]);
                        if (dup2(pipes[i][STDOUT_FILENO], STDOUT_FILENO) == -1) {
                            perror("Error: couldn't redirect stdout to pipe");
                            _exit(i + 1);
                        }
                        (void) close(pipes[i][STDOUT_FILENO]);

                        (void) close(pipes[i - 1][STDOUT_FILENO]);
                        if (dup2(pipes[i - 1][STDIN_FILENO], STDIN_FILENO) == -1) {
                            perror("Error: couldn't redirect stdin to pipe");
                            _exit(i + 1);
                        }
                        (void) close(pipes[i - 1][STDIN_FILENO]);

                        // redirect stderr to error file
                        if (dup2(fd_err, STDERR_FILENO) == -1) {
                            perror("Error: couldn't redirect stderr to error file");
                            _exit(i + 1);
                        }
                        (void) close(fd_err);

                        char **exec_args = parse_cmd(commands[i]);
                        int exec_ret = execvp(exec_args[0], exec_args);
                        if (exec_ret == -1) {
                            write_error(exec_args[0]);
                            // perror("Error: couldn't execute command");
                            _exit(i + 1);
                        }
                        free(exec_args);
                    }
                }

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
                (void) wait(&status);

                if (WIFEXITED(status)) {
                    if (WEXITSTATUS(status) != 0) {
                        char *msg = malloc(MAX_CMD_SIZE);
                        int result = snprintf(msg,
                                              MAX_CMD_SIZE,
                                              "Task %d piped command %d failed\n\n",
                                              task_nr,
                                              WEXITSTATUS(status));
                        if (result < 0 || result >= MAX_CMD_SIZE) {
                            perror("Error: couldn't create error message with snprintf");
                        }
                        if (write(STDERR_FILENO, msg, strlen(msg)) == -1) {
                            perror("Error: couldn't write to error file");
                        }
                        free(msg);
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
                return -1;
            }

            if (write(fd_time, time_str, strlen(time_str)) == -1) {
                perror("Error: couldn't write to time file");
                return -1;
            }

            (void) close(fd_time);

            // send this request to the orchestrator as completed
            set_type(r, COMPLETED);

            int fd_server = open(SERVER_FIFO, O_WRONLY);
            if (fd_server == -1) {
                perror("Error: couldn't open server FIFO");
                return -1;
            }

            if (write(fd_server, r, sizeof_request()) == -1) {
                perror("Error: couldn't write to server FIFO");
                return -1;
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
            return -1;
        }

        if (exec_pid == 0) {

            pid_t pid = fork();
            if (pid == -1) {
                perror("Error: couldn't fork");
                return -1;
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

                char **exec_args = parse_cmd(cmd);
                int exec_ret = execvp(exec_args[0], exec_args);
                if (exec_ret != 0) {
                    write_error(exec_args[0]);
                    // perror("Error: couldn't execute command");
                    _exit(1);
                }

                free(exec_args);
                _exit(0);
            }
            else {
                (void) wait(NULL);

                // write execution time (ms) to file
                struct timeval end_time;
                gettimeofday(&end_time, NULL);

                long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000L +
                                    (end_time.tv_usec - start_time.tv_usec) / 1000L;


                char time_str[EXEC_TIME_STRING_SIZE];
                result = snprintf(time_str, EXEC_TIME_STRING_SIZE, "%ld ms\n", elapsed_time);
                if (result < 0 || result >= EXEC_TIME_STRING_SIZE) {
                    perror("Error: couldn't create time string with snprintf");
                    return -1;
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
                    return -1;
                }

                if (write(fd_server, r, sizeof_request()) == -1) {
                    perror("Error: couldn't write to server FIFO");
                    return -1;
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

/**
 * @brief Parse a piped command into an array of single commands.
 *
 * @details Used in the function @ref exec to parse a piped command
 * into an array of single commands.
 *
 * It also strips leading and trailing whitespaces from each command,
 * see the sencond and third example below.
 *
 * It also can handle quoted strings, as demonstrated in the third example.
 *
 * It can handle new lines as it's possible to have a command with
 * multiple lines, see the fourth example.
 *
 * Examples:
 * @code
 * - "ls -l | cat" -> ["ls -l", "cat"]
 *
 * - "  ls -l |  cat  " -> ["ls -l", "cat"]
 *
 * - "echo \"Hello   World\" |cat    " -> ["echo \"Hello   World\"", "cat"]
 *
 * - "echo \"Hello\nWorld\" | cat\n\n" -> ["echo \"HelloWorld\"", "cat"]
 * @endcode
 *
 * Note: Uncomment the last for loop in this function to print the
 * resulting array of single commands.
 *
 * @param cmd The piped command to parse
 * @param N The number of commands in the array
 * @return An array of strings, or NULL if an error occurred
 */
char** parse_cmd_pipes(char *cmd, int *N) {
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
        while (*token == ' ' || *token == '\t' || *token == '\n') {
            token++;
        }
        size_t len = strlen(token);
        while (len > 0 && (token[len - 1] == ' ' || token[len - 1] == '\t' || token[len - 1] == '\n')) {
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

    // print the array of single commands
    /*
    for (int j = 0; j < *N; j++) {
        printf("Command %d: \"%s\"\n", j + 1, commands[j]);
    }
    */

    return commands;
}

/**
 * @brief Parse a command into an array of arguments.
 *
 * @details Used in the function @ref exec to parse a command
 * into an array of arguments.
 *
 * It also strips leading and trailing whitespaces from each argument,
 * see the sencond and third example below.
 *
 * It also can handle quoted strings, as demonstrated in the third example.
 *
 * Examples:
 * @code
 * - "ls -l -a" -> ["ls", "-l", "-a", NULL]
 *
 * - "  ls -l    -a" -> ["ls", "-l", "-a", NULL]
 *
 * - "echo \"Hello   World\"" -> ["echo", "Hello   World", NULL]
 * @endcode
 *
 * @param cmd The command to parse
 * @return An array of arguments, or NULL if an error occurred
 */
char** parse_cmd(char* cmd) {

	char **exec_args = malloc(MAX_ARGS * sizeof(char*));

	char *string;

	char* command = strdup(cmd);
    if (command == NULL) {
        perror("Error: couldn't allocate memory for command in parse_cmd");
        return NULL;
    }

	string = strtok(command, " ");

	int i = 0;
	while (string != NULL) {
		exec_args[i] = string;
		string = strtok(NULL, " ");
		i++;
	}

	exec_args[i] = NULL;
    return exec_args;
}

/**
 * @brief Write an error message to stderr.
 *
 * @details Used in the function @ref exec to write an error
 * message to stderr when a command fails.
 *
 * It's meant to be used after execvp fails, and the
 * redirection of stderr to an error log file, via the dup2
 * function.
 *
 * It checks the errno to determine the type of error, being:
 * - ENOENT: command not found
 *
 * - EACCES: permission denied
 *
 * - EINVAL: invalid argument(s)
 *
 * @param cmd_name The name of the command that failed
 */
void write_error(char *cmd_name) {
    char *msg = malloc(ERROR_MSG_SIZE);

    // check errors using errno
    if (errno == ENOENT) {
        int result = snprintf(msg, ERROR_MSG_SIZE, "%s: not found\n", cmd_name);
        if (result < 0 || result >= ERROR_MSG_SIZE) {
            perror("Error: couldn't create error message with snprintf");
        }
        if (write(STDERR_FILENO, msg, strlen(msg)) == -1) {
            perror("Error: couldn't write to stderr");
        }
    }
    else if (errno == EACCES) {
        int result = snprintf(msg, ERROR_MSG_SIZE, "%s: permission denied\n", cmd_name);
        if (result < 0 || result >= ERROR_MSG_SIZE) {
            perror("Error: couldn't create error message with snprintf");
        }
        if (write(STDERR_FILENO, msg, strlen(msg)) == -1) {
            perror("Error: couldn't write to stderr");
        }
    }
    else if (errno == EINVAL) {
        perror("Error: invalid argument");
        int result = snprintf(msg, ERROR_MSG_SIZE, "%s: invalid argument\n", cmd_name);
        if (result < 0 || result >= ERROR_MSG_SIZE) {
            perror("Error: couldn't create error message with snprintf");
        }
    }

    free(msg);
}
