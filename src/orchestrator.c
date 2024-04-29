// TODO remove ../include/
#include "../include/orchestrator.h"
#include "../include/task_nr.h"
#include "../include/request.h"
#include "../include/command.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
// #include <stdlib.h>


int add_request(Request *requests[], int *N, Request *r);

int remove_request(Request *requests[], int *N, Request *r);

Request *select_request(Request *requests[], int *N, int policy);

int send_status(
    char *client_fifo,
    Request *executing[], int N_executing,
    Request *scheduled[], int N_scheduled,
    char *output_dir
);

/**
 * @brief Print the usage of the orchestrator program
 * @param name The name of the program
 */
void orchestrator_usage(char *name) {
    printf("Usage: %s <output_dir> <parallel-tasks> [sched-policy]\n", name);
}

/**
 * @brief Main function for the orchestrator program
 * The orchestrator program is responsible for managing the execution of the parallel tasks.
 * It receives the output directory, the number of parallel tasks, and the scheduling policy as arguments.
 * This function is responsible for validating the arguments, parsing them and
 * calling the appropriate functions to manage the parallel tasks.
 * @param argc The number of arguments
 * @param argv The arguments
 * @return 0 if the program runs successfully
 */
int main(int argc, char **argv) {

    // Validate the number of arguments
    if (argc < 3) {
        orchestrator_usage(argv[0]);
        return 0;
    }

    // Get the output directory
    char *output_dir = argv[1];

    // open the output directory TODO use access() instead
    int fd_out = open(output_dir, O_DIRECTORY);
    if (fd_out < 0) {
        perror("Error: output directory doesn't exist");
        return 1;
    }
    // close the output directory
    close(fd_out);

    // Get the number of parallel tasks
    int tasks = atoi(argv[2]);

    // Get the scheduling policy if it exists
    /* char *sched_policy = NULL;
    if (argc > 3) {
        sched_policy = argv[3];
    } */

    printf("Orchestrator server is running...\n");

    // create the server FIFO
    (void) unlink(SERVER_FIFO);

    if (mkfifo(SERVER_FIFO, 0644) == -1) {
        perror("Error: couldn't create server FIFO");
        return 1;
    }

    // Get the task number
    int task_nr = init_task_nr(output_dir);
    if (task_nr == -1) {
        perror("Error: couldn't get task number");
        return 1;
    }

    // Arrays to store the executing and scheduled requests
    Request *executing[MAX_REQUESTS]; int N_executing = 0;
    Request *scheduled[MAX_REQUESTS]; int N_scheduled = 0;

    Request *r;

    // main loop
    while (1) {
        // check for client requests in the server FIFO
        int fd = open(SERVER_FIFO, O_RDONLY);
        if (fd == -1) {
            perror("Error: couldn't open server FIFO");
            return 1;
        }

        // Allocate memory for the request
        r = malloc(sizeof_request());

        // read the request
        ssize_t bytes_read = read(fd, r, sizeof_request());
        if (bytes_read == -1) {
            perror("Error: couldn't read from server FIFO");
            close(fd);
            return 1;
        }

        // close the server FIFO
        close(fd);

        // get the start time of the request to calculate the response time
        struct timeval start_time;
        gettimeofday(&start_time, NULL);

        // request type
        int type = get_type(r);

        // print the request information
        printf("Request received\n");
        printf("Type: %s\n", type_to_string(type));
        printf("Command: %s\n", get_command(r));
        printf("Estimated time: %d\n", get_est_time(r));
        printf("%s command\n", get_is_piped(r) ? "Piped" : "Single");
        printf("\n");

        int exec_ret;
        pid_t pid;
        switch (type) {
            case EXECUTE:
                // add the task_nr to the request
                set_task_nr(r, task_nr);

                if (N_executing > tasks) {
                    // schedule the request
                    if (add_request(scheduled, &N_scheduled, r) == -1) {
                        perror("Error: couldn't add request to scheduled array");
                        return 1;
                    }
                    break;
                }

                // execute the request

                // add the request to the executing array
                if (add_request(executing, &N_executing, r) == -1) {
                    perror("Error: couldn't add request to executing array");
                    return 1;
                }

                // execute the command
                exec_ret = exec(r, output_dir, start_time);
                if (exec_ret == -1) {
                    perror("Error: couldn't execute command");
                    return 1;
                }

                // send the task number via client FIFO
                char *client_fifo = get_client_fifo(r);
                int fd_client = open(client_fifo, O_WRONLY);
                if (fd_client == -1) {
                    perror("Error: couldn't open client FIFO");
                    return 1;
                }

                char buffer[10];
                int bytes_writed = snprintf(buffer, 10, "%d", task_nr);
                if (write(fd_client, buffer, bytes_writed) == -1) {
                    perror("Error: couldn't write to client FIFO");
                    return 1;
                }

                close(fd_client);

                // increment the task number
                task_nr = increment_task_nr(output_dir);
                if (task_nr == -1) {
                    perror("Error: couldn't increment task number");
                    return 1;
                }

                break;

            case STATUS:
                // create process to execute the status command
                pid = fork();
                if (pid == -1) {
                    perror("Error: couldn't create process");
                    return 1;
                }

                if (pid == 0) {
                    // Get client FIFO name from the request
                    char *client_fifo = get_client_fifo(r);

                    // send the status to the client FIFO
                    int status = send_status(
                        client_fifo,
                        executing, N_executing,
                        scheduled, N_scheduled,
                        output_dir
                    );
                    if (status == -1) {
                        perror("Error: couldn't send status to client");
                        return 1;
                    }

                    printf("Sent status to client\n");

                    _exit(0);
                }

                break;

            case COMPLETED:
                // get the status of the task
                printf("Task %d completed\n\n", get_task_nr(r));

                // remove the request from the executing array
                if (remove_request(executing, &N_executing, r) == -1) {
                    perror("Error: couldn't remove request from executing array");
                    return 1;
                }

                // check for scheduled tasks to run
                Request *next = select_request(scheduled, &N_scheduled, 0);
                if (next == NULL) {
                    printf("No scheduled tasks\n\n");
                    break;
                }

                // remove the request from the scheduled array
                if (remove_request(scheduled, &N_scheduled, next) == -1) {
                    perror("Error: couldn't remove request from scheduled array");
                    return 1;
                }

                // add the request to the executing array
                if (add_request(executing, &N_executing, next) == -1) {
                    perror("Error: couldn't add request to executing array");
                    return 1;
                }

                // execute the command
                exec_ret = exec(next, output_dir, start_time);
                if (exec_ret == -1) {
                    perror("Error: couldn't execute command");
                    return 1;
                }

                // increment the task number
                task_nr = increment_task_nr(output_dir);
                if (task_nr == -1) {
                    perror("Error: couldn't increment task number");
                    return 1;
                }
                break;

            default:
                printf("Invalid request type\n");
                break;
        }
    }

    free(r);
    return 0;
}

int add_request(Request *requests[], int *N, Request *r) {
    if (*N == MAX_REQUESTS) {
        // requests array is full
        return -1;
    }

    requests[(*N)++] = r;
    return 0;
}

int remove_request(Request *requests[], int *N, Request *r) {
    for (int i = 0; i < *N; i++) {
        if (get_task_nr(requests[i]) == get_task_nr(r)) {
            for (int j = i; j < *N - 1; j++) {
                requests[j] = requests[j + 1];
            }
            (*N)--;
            return 0;
        }
    }

    // no request found
    return -1;
}

/* policies:
 * 0: FIFO - First In First Out
 * 1: SJF  - Shortest Job First
 * 2: RR   - Round Robin
 */
Request *select_request(Request *requests[], int *N, int policy) {
    // TODO implement scheduling policy
    switch (policy) {
        case 0:
            // FIFO
            break;
        case 1:
            // SJF
            break;
        case 2:
            // RR
            break;
        default:
            (*N) = 0;
            return requests[0];
    }

    return NULL;
}

int send_status(
    char *client_fifo,
    Request *executing[], int N_executing,
    Request *scheduled[], int N_scheduled,
    char *output_dir
) {
    int fd_client = open(client_fifo, O_WRONLY);
    if (fd_client == -1) {
        perror("Error: couldn't open client FIFO");
        return 1;
    }

    int bytes_writed = 0;
    int buf_size = MAX_CMD_SIZE + 50; // add 50 bytes for the task nr and exec time

    // write executing commands
    if (N_executing == 0) {
        char *msg = "No tasks executing\n\n";
        bytes_writed = write(fd_client, msg, strlen(msg));
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return 1;
        }
    }
    else {
        char *msg = "Executing:\n";
        bytes_writed = write(fd_client, msg, strlen(msg));
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return 1;
        }

        char buffer[buf_size];
        for (int i = 0; i < N_executing; i++) {
            (void) snprintf(
                buffer, buf_size, "%d %s\n",
                get_task_nr(executing[i]),
                get_command(executing[i])
            );
            bytes_writed = write(fd_client, buffer, strlen(buffer));
            if (bytes_writed == -1) {
                perror("Error: couldn't write to client FIFO");
                return 1;
            }
        }

        bytes_writed = write(fd_client, "\n", 1);
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return 1;
        }
    }

    // build scheduled string
    if (N_scheduled == 0) {
        char *msg = "No tasks scheduled\n\n";
        bytes_writed = write(fd_client, msg, strlen(msg));
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return 1;
        }
    }
    else {
        char *msg = "Scheduled:\n";
        write(fd_client, msg, strlen(msg));

        char buffer[buf_size];
        for (int i = 0; i < N_scheduled; i++) {
            (void) snprintf(
                buffer, buf_size, "%d %s\n",
                get_task_nr(scheduled[i]),
                get_command(scheduled[i])
            );
            bytes_writed = write(fd_client, buffer, strlen(buffer));
            if (bytes_writed == -1) {
                perror("Error: couldn't write to client FIFO");
                return 1;
            }
        }

        bytes_writed = write(fd_client, "\n", 1);
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return 1;
        }
    }

    // build completed string
    // read completed tasks from history file
    int history_size = strlen(output_dir) + strlen(HISTORY) + 2;
    char history_path[history_size];
    (void) snprintf(history_path, history_size, "%s/%s", output_dir, HISTORY);

    int fd_history = open(history_path, O_RDONLY | O_CREAT, 0644);
    if (fd_history == -1) {
        perror("Error: couldn't open history file");
        return -1;
    }

    char buffer[buf_size];
    int bytes_read = read(fd_history, buffer, buf_size);

    if (bytes_read == -1) {
        perror("Error: couldn't read from history file");
        return 1;
    }

    if (bytes_read == 0) {
        char *msg = "No tasks completed\n";
        bytes_writed = write(fd_client, msg, strlen(msg));
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return 1;
        }
    }
    else {
        char *msg = "Completed:\n";
        bytes_writed = write(fd_client, msg, strlen(msg));
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return 1;
        }

        bytes_writed = write(fd_client, buffer, bytes_read);
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return 1;
        }

        while ((bytes_read = read(fd_history, buffer, buf_size)) > 0) {
            bytes_writed = write(fd_client, buffer, bytes_read);
            if (bytes_writed == -1) {
                perror("Error: couldn't write to client FIFO");
                return 1;
            }
        }
    }

    // assure that the status option doesn't interrupt other requests with:
    // sleep(10);

    close(fd_history);
    close(fd_client);
    return 0;
}
