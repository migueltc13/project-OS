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
        perror("malloc");
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
            perror("malloc");
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
        perror("strdup");
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
 * @details
 * TODO count execution time
 * TODO write to file
 * 
 * @param cmd The command to execute
 * @param is_piped Whether the command is piped
 * @param task_nr The task number
 * @return 0 if successful
 */
int exec(char *cmd, bool is_piped, char* output_dir, int task_nr, struct timeval start_time) {

    int fd = open(output_dir, O_DIRECTORY);
    if (fd < 0) {
        perror("Error: output directory doesn't exist\n");
        close(fd);
        return -1;
    }
    close(fd);

    // create output directory for task results
    char *task_dir = malloc(strlen(output_dir) + strlen("task") + 2 + 8); // TODO max size of task number
    if (task_dir == NULL) {
        perror("malloc");
        return -1;
    }

    if (sprintf(task_dir, "%s/task%d", output_dir, task_nr) < 0) {
        perror("sprintf");
        return -1;
    }

    if (mkdir(task_dir, 0755) != 0) {
        perror("Error: could not create task directory");
        return -1;
    }

    // open output file
    char *output_file = malloc(strlen(task_dir) + 3 + 2);
    if (output_file == NULL) {
        perror("malloc");
        return -1;
    }

    if (sprintf(output_file, "%s/out", task_dir) < 0) {
        perror("sprintf");
        return -1;
    }

    int fd_out = open(output_file, O_WRONLY | O_CREAT, 0644);
    if (fd_out < 0) {
        perror("Error: could not open output file\n");
        close(fd_out);
        return -1;
    }

    // open error file
    char *error_file = malloc(strlen(task_dir) + 3 + 2);
    if (error_file == NULL) {
        perror("malloc");
        return -1;
    }

    if (sprintf(error_file, "%s/err", task_dir) < 0) {
        perror("sprintf");
        return -1;
    }

    int fd_err = open(error_file, O_WRONLY | O_CREAT, 0644);
    if (fd_err < 0) {
        perror("Error: could not open error file\n");
        close(fd_err);
        return -1;
    }

    // open execution time file
    char *time_file = malloc(strlen(task_dir) + 4 + 2);
    if (time_file == NULL) {
        perror("malloc");
        return -1;
    }

    if (sprintf(time_file, "%s/time", task_dir) < 0) {
        perror("sprintf");
        return -1;
    }

    int fd_time = open(time_file, O_WRONLY | O_CREAT, 0644);
    if (fd_time < 0) {
        perror("Error: could not open time file\n");
        close(fd_time);
        return -1;
    }

    if (is_piped) {
        int N = 0;

        char **commands = parse_cmd(cmd, &N);

        if (commands == NULL) {
            perror("Error parsing command");
            return -1;
        }

        int pipes[N - 1][2];

        for (int i = 0; i < N; i++) {

            // create pipe for all commands except the last one
            if (i != N - 1) {
                if (pipe(pipes[i]) == -1) {
                    perror("pipe");
                    return 1;
                }
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                return 1;
            }

            if (pid == 0) {

                // first command
                if (i == 0) {
                    close(pipes[i][STDIN_FILENO]);
                    dup2(pipes[i][STDOUT_FILENO], STDOUT_FILENO);
                    close(pipes[i][STDOUT_FILENO]);

                    exec_command(commands[i]);
                    perror("exec_command");
                    _exit(1);
                }
                // last command TODO write to file tmp/task(task_nr).out
                else if (i == N - 1) {
                    close(pipes[i - 1][STDOUT_FILENO]);
                    dup2(pipes[i - 1][STDIN_FILENO], STDIN_FILENO);
                    close(pipes[i - 1][STDIN_FILENO]);

                    exec_command(commands[i]);
                    perror("exec_command");
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
                    perror("exec_command");
                    _exit(1);
                }
            }

            // TODO check if needs to be else statement here
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
                perror("wait");
                return 1;
            }

            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) != 0) {
                    fprintf(stderr, "Command %d failed with status %d\n", i + 1, WEXITSTATUS(status));
                    return 1;
                }
            }
        }

        // TODO free commands
    }
    else {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            // redirect stdout to output file
            if (dup2(fd_out, STDOUT_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }
            close(fd_out);

            // redirect stderr to error file
            if (dup2(fd_err, STDERR_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }
            close(fd_err);

            int exec_ret = exec_command(cmd);
            if (exec_ret == -1) {
                perror("exec_command");
                _exit(1);
            }

            close(fd_out);
            close(fd_err);
            _exit(0);
        }
        else {
            // wait to chil process to finish in other process
            pid_t wait_pid = fork();
            if (wait_pid == -1) {
                perror("fork");
                return 1;
            }

            if (wait_pid != 0) {

                (void) waitpid(pid, NULL, 0);

                // write execution time (ms) to file
                struct timeval end_time;
                gettimeofday(&end_time, NULL);

                long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000L +
                                    (end_time.tv_usec - start_time.tv_usec) / 1000L;

                char time_str[20];
                if (sprintf(time_str, "%ld ms\n", elapsed_time) < 0) {
                    perror("sprintf");
                    _exit(1);
                }

                if (write(fd_time, time_str, strlen(time_str)) == -1) {
                    perror("write");
                    _exit(1);
                }

                close(fd_time);
            }
        }
    }

    free(output_file);
    free(error_file);
    return 0;
}
