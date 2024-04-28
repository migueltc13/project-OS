// TODO remove ../include/
#include "../include/orchestrator.h"
#include "../include/task_nr.h"
#include "../include/request.h"
#include "../include/command.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
// #include <stdlib.h>


int add_request(Request *requests[], int *N, Request *r);

int remove_request(Request *requests[], int *N, Request *r);

Request *select_request(Request *requests[], int *N, int policy);

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

    // Allocate memory for the requests arrays
    Request *executing[MAX_REQUESTS]; int N_executing = 0;
    Request *scheduled[MAX_REQUESTS]; int N_scheduled = 0;
    Request *completed[MAX_REQUESTS]; int N_completed = 0;

    // main loop
    while (1) {
        // check for client requests in the server FIFO TODO move this outside the loop
        int fd = open(SERVER_FIFO, O_RDONLY);
        if (fd == -1) {
            perror("Error: couldn't open server FIFO");
            return 1;
        }

        // read the request
        Request *r = malloc(sizeof_request());

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

                // increment the task number
                task_nr = increment_task_nr(output_dir);
                if (task_nr == -1) {
                    perror("Error: couldn't increment task number");
                    return 1;
                }

                break;

            case STATUS:
                // get the status of the task
                printf("Getting status...\n");
                break;

            case COMPLETED:
                // get the status of the task
                printf("Task %d completed\n", get_task_nr(r));

                // remove the request from the executing array
                if (remove_request(executing, &N_executing, r) == -1) {
                    perror("Error: couldn't remove request from executing array");
                    return 1;
                }

                // add the completed request to the completed array
                if (add_request(completed, &N_completed, r) == -1) {
                    perror("Error: couldn't add request to completed array");
                    return 1;
                }

                break;
                // check for scheduled tasks to run
                Request *next = select_request(scheduled, &N_scheduled, 0);
                if (next == NULL) {
                    printf("\nNo scheduled tasks\n");
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
