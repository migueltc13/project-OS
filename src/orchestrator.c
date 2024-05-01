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

// Default scheduling policy
#define DEFAULT_POLICY FCFS

// Max number of scheduled requests
#define MAX_SCHEDULED_REQUESTS 1024

void orchestrator_usage(char *name);

int add_request(Request *requests[], int *N, int max, Request *r);

int remove_request(Request *requests[], int *N, Request *r);

Request *select_request(Request *requests[], int N, int policy);

int handle_execute(Request *r,
                   Request *executing[], int *N_executing,
                   Request *scheduled[], int *N_scheduled,
                   char *output_dir, int tasks,
                   int *task_nr, struct timeval start_time);

int handle_status(Request *r,
                  Request *executing[], int N_executing,
                  Request *scheduled[], int N_scheduled,
                  char *output_dir);

int handle_completed(Request *r,
                     Request *executing[], int *N_executing,
                     Request *scheduled[], int *N_scheduled,
                     char *output_dir, int policy,
                     struct timeval start_time);

int send_status(char *client_fifo,
                Request *executing[], int N_executing,
                Request *scheduled[], int N_scheduled,
                char *output_dir);

void clean_up(Request *executing[], int N_executing,
              Request *scheduled[], int N_scheduled);

void clean_up_all(int fd, Request *r,
                  Request *executing[], int N_executing,
                  Request *scheduled[], int N_scheduled);

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

    // Check if the output directory exists
    // if it doesn't, create it
    if (access(output_dir, F_OK) == -1) {
        printf("Couldn't find output directory, creating it...\n");
        if (mkdir(output_dir, 0755) == -1) {
            perror("Error: couldn't create output directory");
            return 1;
        }
        printf("Output directory %s created\n", output_dir);
    }

    // Get the number of parallel tasks
    const int tasks = atoi(argv[2]);
    if (tasks <= 0) {
        printf("Error: invalid number of parallel tasks\n");
        return 1;
    }

    // Set the default scheduling policy
    int policy = DEFAULT_POLICY;

    // Get the scheduling policy if any was provided
    if (argc > 3) {
        char *policy_buffer = argv[3];
        // parse the scheduling policy to an int
        if (strcmp(policy_buffer, "FCFS") == 0) {
            policy = FCFS;
        }
        else if (strcmp(policy_buffer, "SJF") == 0) {
            policy = SJF;
        }
        else if (strcmp(policy_buffer, "PES") == 0) {
            policy = PES;
        }
        else {
            printf("Error: invalid scheduling policy\n");
            return 1;
        }
        printf("Scheduling policy: %s\n", policy_buffer);
    }
    else {
        printf("Using default scheduling policy: FCFS\n");
    }

    // create the server FIFO
    (void) unlink(SERVER_FIFO);

    if (mkfifo(SERVER_FIFO, 0644) == -1) {
        perror("Error: couldn't create server FIFO");
        return 1;
    }

    // Get the task number
    int task_nr = load_task_nr(output_dir);
    if (task_nr == -1) {
        perror("Error: couldn't get task number");
        return 1;
    }

    // Arrays to store the executing and scheduled requests
    Request *executing[tasks]; int N_executing = 0;
    Request *scheduled[MAX_SCHEDULED_REQUESTS]; int N_scheduled = 0;

    printf("Orchestrator server is running...\n");

    bool running = true;

    // main loop
    while (running) {

        // check for client requests in the server FIFO
        int fd = open(SERVER_FIFO, O_RDONLY);
        if (fd == -1) {
            perror("Error: couldn't open server FIFO");
            return 1;
        }

        // Allocate memory for the request
        Request *r = malloc(sizeof_request());
        if (r == NULL) {
            perror("Error: couldn't allocate memory for request");
            (void) close(fd);
            return 1;
        }

        // read the request
        ssize_t bytes_read = 0;
        while (running && (bytes_read = read(fd, r, sizeof_request())) > 0) {

            if (bytes_read == -1) {
                perror("Error: couldn't read from server FIFO");
                (void) close(fd);
                return 1;
            }

            // get the start time of the request to
            // calculate the waiting plus execution time
            struct timeval start_time;
            gettimeofday(&start_time, NULL);

            // request type
            int type = get_type(r);

            // print the request information
            if (type == EXECUTE)
                print_request(r, policy);

            int status;
            switch (type) {
                case EXECUTE:
                    status = handle_execute(r,
                                            executing, &N_executing,
                                            scheduled, &N_scheduled,
                                            output_dir, tasks,
                                            &task_nr, start_time);
                    if (status == -1) {
                        save_task_nr(task_nr, output_dir);
                        clean_up_all(fd, r,
                                     executing, N_executing,
                                     scheduled, N_scheduled);
                        return 1;
                    }
                    break;

                case STATUS:
                    status = handle_status(r,
                                           executing, N_executing,
                                           scheduled, N_scheduled,
                                           output_dir);
                    if (status == -1) {
                        save_task_nr(task_nr, output_dir);
                        clean_up_all(fd, r,
                                     executing, N_executing,
                                     scheduled, N_scheduled);
                        return 1;
                    }
                    break;

                case COMPLETED:
                    status = handle_completed(r,
                                              executing, &N_executing,
                                              scheduled, &N_scheduled,
                                              output_dir, policy,
                                              start_time);
                    if (status == -1) {
                        save_task_nr(task_nr, output_dir);
                        clean_up_all(fd, r,
                                     executing, N_executing,
                                     scheduled, N_scheduled);
                        return 1;
                    }
                    break;
                case KILL:
                    // kill the server
                    running = false;
                    break;
                default:
                    printf("Unknown request type\n");
                    break;
            }
        }
        // close the server FIFO
        (void) close(fd);

        // free the request
        free(r);
    }

    printf("Orchestrator server is shutting down...\n");

    // save the task number
    save_task_nr(task_nr, output_dir);

    // cleanup
    clean_up(executing, N_executing,
             scheduled, N_scheduled);

    return 0;
}

/**
 * @brief Print the usage of the orchestrator program
 * @param name The name of the program
 */
void orchestrator_usage(char *name) {
    printf("Usage: %s <output_dir> <parallel_tasks> [sched_policy]\n", name);
}

int add_request(Request *requests[], int *N, int max, Request *r) {
    if (*N >= max) {
        // requests array is full
        return -1;
    }

    requests[(*N)++] = clone_request(r);
    return 0;
}

int remove_request(Request *requests[], int *N, Request *r) {
    for (int i = 0; i < *N; i++) {
        if (get_task_nr(requests[i]) == get_task_nr(r)) {

            free(requests[i]);
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

Request *select_request(Request *scheduled[], int N, int policy) {

    Request *r = NULL;
    if (N == 0) {
        // no scheduled requests
        return r;
    }

    switch (policy) {
        case FCFS:
            // returns the first scheduled request
            r = scheduled[0];
            break;
        case SJF:
            // returns the request with the smallest estimated time
            r = scheduled[0];
            int min_est_time = get_est_time(r);

            for (int i = 0; i < N; i++) {
                if (get_est_time(scheduled[i]) < min_est_time) {
                    r = scheduled[i];
                    min_est_time = get_est_time(r);
                }
            }
            break;
        case PES:
            // returns the request with the highest priority
            r = scheduled[0];
            int max_priority = get_est_time(r);

            for (int i = 0; i < N; i++) {
                if (get_est_time(scheduled[i]) > max_priority) {
                    r = scheduled[i];
                    max_priority = get_est_time(r);
                }
            }
            break;
        default:
            printf("Unknown scheduling policy.\n");
            break;
    }

    return clone_request(r);
}

int handle_execute(Request *r,
                   Request *executing[], int *N_executing,
                   Request *scheduled[], int *N_scheduled,
                   char *output_dir, int tasks,
                   int *task_nr, struct timeval start_time) {
    // add the task_nr to the request
    set_task_nr(r, *task_nr);

    // increment the task number
    (*task_nr)++;

    // get the client FIFO name
    char *client_fifo = get_client_fifo(r);

    bool toSched = (*N_executing >= tasks);
    if (toSched) {
        // schedule the request
        printf("Task %d scheduled\n\n", get_task_nr(r));

        if (add_request(scheduled, N_scheduled, MAX_SCHEDULED_REQUESTS, r) == -1) {
            char *msg = "Critical: max number of scheduled tasks reached\n";
            printf("%s", msg);
            int fd_client = open(client_fifo, O_WRONLY);
            if (fd_client == -1) {
                perror("Error: couldn't open client FIFO");
                return -1;
            }

            if (write(fd_client, msg, strlen(msg)) == -1) {
                perror("Error: couldn't write to client FIFO");
                return -1;
            }

            (void) close(fd_client);
            return 0;
        }
    }
    else {
        // execute the request
        printf("Task %d executing\n\n", get_task_nr(r));

        // add the request to the executing array
        if (add_request(executing, N_executing, tasks, r) == -1) {
            perror("Error: couldn't add request to executing array");
            return -1;
        }

        // execute the command
        int exec_ret = exec(r, output_dir, start_time);
        if (exec_ret == -1) {
            perror("Error: couldn't execute command");
            return -1;
        }
    }

    // create message to send to client
    char msg[EXECUTE_MSG_SIZE];

    int bytes_writed = snprintf(msg, EXECUTE_MSG_SIZE,
                                "%s task %d\n",
                                toSched ? "Scheduled" : "Executing",
                                get_task_nr(r));
    if (bytes_writed < 0 || bytes_writed >= EXECUTE_MSG_SIZE) {
        perror("Error: Couldn't create message to client");
        return -1;
    }

    // send message with task number via client FIFO
    int fd_client = open(client_fifo, O_WRONLY);
    if (fd_client == -1) {
        perror("Error: couldn't open client FIFO");
        return -1;
    }

    if (write(fd_client, msg, bytes_writed) == -1) {
        perror("Error: couldn't write to client FIFO");
        return -1;
    }

    (void) close(fd_client);

    // increment the task number
    return 0;
}

int handle_status(Request *r,
                  Request *executing[], int N_executing,
                  Request *scheduled[], int N_scheduled,
                  char *output_dir) {
    // create process to execute the status command
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error: couldn't create process");
        return -1;
    }

    if (pid == 0) {
        // Get client FIFO name from the request
        char *client_fifo = get_client_fifo(r);

        // send the status to the client FIFO
        int status = send_status(client_fifo,
                                 executing, N_executing,
                                 scheduled, N_scheduled,
                                 output_dir);

        if (status == -1) {
            perror("Error: couldn't send status to client");
            return -1;
        }

        _exit(0);
    }
    return 0;
}

int handle_completed(Request *r,
                     Request *executing[], int *N_executing,
                     Request *scheduled[], int *N_scheduled,
                     char *output_dir, int policy,
                     struct timeval start_time) {
    // get the status of the task
    printf("Task %d completed\n\n", get_task_nr(r));

    // remove the request from the executing array
    if (remove_request(executing, N_executing, r) == -1) {
        printf("Warning: couldn't remove request from scheduled array");
        printf("This is due to interruption of the server mid execution\n");
    }

    // check for scheduled tasks to run
    Request *next = select_request(scheduled, *N_scheduled, policy);
    if (next == NULL) {
        // printf("No scheduled tasks\n\n");
        return 0;
    }

    // remove the request from the scheduled array
    if (remove_request(scheduled, N_scheduled, next) == -1) {
        printf("Warning: couldn't remove request from scheduled array");
        printf("This is due to interruption of the server mid execution\n");
    }

    // add the request to the executing array
    if (add_request(executing, N_executing, MAX_SCHEDULED_REQUESTS, next) == -1) {
        perror("Error: couldn't add request to executing array");
        return -1;
    }

    // execute the command
    int exec_ret = exec(next, output_dir, start_time);
    if (exec_ret == -1) {
        perror("Error: couldn't execute command");
        return -1;
    }

    free(next);

    return 0;
}

int send_status(char *client_fifo,
                Request *executing[], int N_executing,
                Request *scheduled[], int N_scheduled,
                char *output_dir) {
    int fd_client = open(client_fifo, O_WRONLY);
    if (fd_client == -1) {
        perror("Error: couldn't open client FIFO");
        return -1;
    }

    int bytes_writed = 0;
    const int buf_size = MAX_CMD_SIZE + TASK_NR_STRING_SIZE + EXEC_TIME_STRING_SIZE;

    // write executing requests
    if (N_executing == 0) {
        char *msg = "No tasks executing\n\n";
        bytes_writed = write(fd_client, msg, strlen(msg));
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return -1;
        }
    }
    else {
        char *msg = "Executing:\n";
        bytes_writed = write(fd_client, msg, strlen(msg));
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return -1;
        }

        char buffer[buf_size];
        for (int i = 0; i < N_executing; i++) {
            bytes_writed = snprintf(buffer, buf_size, "%d %s\n",
                                    get_task_nr(executing[i]),
                                    get_command(executing[i]));
            if (bytes_writed < 0 || bytes_writed >= buf_size) {
                perror("Error: couldn't create message to client");
                return -1;
            }
            bytes_writed = write(fd_client, buffer, strlen(buffer));
            if (bytes_writed == -1) {
                perror("Error: couldn't write to client FIFO");
                return -1;
            }
        }

        bytes_writed = write(fd_client, "\n", 1);
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return -1;
        }
    }

    // write scheduled requests
    if (N_scheduled == 0) {
        char *msg = "No tasks scheduled\n\n";
        bytes_writed = write(fd_client, msg, strlen(msg));
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return -1;
        }
    }
    else {
        char *msg = "Scheduled:\n";
        if (write(fd_client, msg, strlen(msg)) == -1) {
            perror("Error: couldn't write to client FIFO");
            return -1;
        }

        char buffer[buf_size];
        for (int i = 0; i < N_scheduled; i++) {
            int bytes_writed = snprintf(buffer, buf_size, "%d %s\n",
                                        get_task_nr(scheduled[i]),
                                        get_command(scheduled[i]));
            if (bytes_writed < 0 || bytes_writed >= buf_size) {
                perror("Error: couldn't create message to client");
                return -1;
            }
            bytes_writed = write(fd_client, buffer, strlen(buffer));
            if (bytes_writed == -1) {
                perror("Error: couldn't write to client FIFO");
                return -1;
            }
        }

        bytes_writed = write(fd_client, "\n", 1);
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return -1;
        }
    }

    // write completed requests by reading it from the history file
    int history_size = strlen(output_dir) + strlen(HISTORY_NAME) + 2; // +2 for '/' and '\0'
    char history_path[history_size];
    bytes_writed = snprintf(history_path, history_size,
                            "%s/%s",
                            output_dir,
                            HISTORY_NAME);
    if (bytes_writed < 0 || bytes_writed >= history_size) {
        perror("Error: couldn't create history file path");
        return -1;
    }

    int fd_history = open(history_path, O_RDONLY | O_CREAT, 0644);
    if (fd_history == -1) {
        perror("Error: couldn't open history file");
        return -1;
    }

    char buffer[buf_size];
    int bytes_read = read(fd_history, buffer, buf_size);

    if (bytes_read == -1) {
        perror("Error: couldn't read from history file");
        return -1;
    }

    if (bytes_read == 0) {
        char *msg = "No tasks completed\n";
        bytes_writed = write(fd_client, msg, strlen(msg));
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return -1;
        }
    }
    else {
        char *msg = "Completed:\n";
        bytes_writed = write(fd_client, msg, strlen(msg));
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return -1;
        }

        bytes_writed = write(fd_client, buffer, bytes_read);
        if (bytes_writed == -1) {
            perror("Error: couldn't write to client FIFO");
            return -1;
        }

        while ((bytes_read = read(fd_history, buffer, buf_size)) > 0) {
            bytes_writed = write(fd_client, buffer, bytes_read);
            if (bytes_writed == -1) {
                perror("Error: couldn't write to client FIFO");
                return -1;
            }
        }
    }

    // Assure that the status option doesn't
    // interrupt other requests with:
    // sleep(10);

    (void) close(fd_history);
    (void) close(fd_client);
    return 0;
}

void clean_up(Request *executing[], int N_executing,
              Request *scheduled[], int N_scheduled) {
    for (int i = 0; i < N_executing; i++) {
        free(executing[i]);
    }
    for (int i = 0; i < N_scheduled; i++) {
        free(scheduled[i]);
    }
}

void clean_up_all(int fd, Request *r,
                  Request *executing[], int N_executing,
                  Request *scheduled[], int N_scheduled) {
    (void) close(fd);
    free(r);
    clean_up(executing, N_executing,
             scheduled, N_scheduled);
}
