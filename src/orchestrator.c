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

    // open the output directory
    int fd = open(output_dir, O_DIRECTORY);
    if (fd < 0) {
        perror("Error: output directory doesn't exist");
        return 1;
    }
    // close the output directory
    close(fd);

    // Get the number of parallel tasks
    // int parallel_tasks = atoi(argv[2]);

    // Get the scheduling policy if it exists
    /* char *sched_policy = NULL;
    if (argc > 3) {
        sched_policy = argv[3];
    } */

    printf("Orchestrator server is running...\n");

    // Get the task number
    int task_nr = get_task_nr(output_dir);
    if (task_nr == -1) {
        perror("Error: couldn't get task number\n");
        return 1;
    }

    // create the server FIFO
    (void) unlink(SERVER_FIFO);

    if (mkfifo(SERVER_FIFO, 0644) == -1) {
        perror("Error: couldn't create server FIFO\n");
        return 1;
    }

    // main loop
    while (1) {
        // check for client requests in the server FIFO
        int fd = open(SERVER_FIFO, O_RDONLY);
        if (fd == -1) {
            perror("Error: couldn't open server FIFO\n");
            return 1;
        }

        // read the request
        Request *r = malloc(sizeof_request());

        ssize_t bytes_read = read(fd, r, sizeof_request());
        if (bytes_read == -1) {
            perror("Error: couldn't read from server FIFO\n");
            close(fd);
            return 1;
        }

        // close the server FIFO
        close(fd);

        // get the start time of the request to calculate the response time
        struct timeval start_time;
        gettimeofday(&start_time, NULL);

        // print the request information
        printf("Request received\n");
        printf("Type: %s\n", get_type(r) == EXECUTE ? "EXECUTE" : "STATUS");
        printf("Command: %s\n", get_command(r));
        printf("Estimated time: %d\n", get_est_time(r));
        printf("%s command\n", get_is_piped(r) ? "Piped" : "Single");

        // execute the request
        switch (get_type(r)) {
            case EXECUTE:
                // execute the command
                printf("Executing command...\n");

                // execute the command
                if (exec(get_command(r), get_is_piped(r), output_dir, task_nr++, start_time) == -1) {
                    perror("Error: couldn't execute command\n");
                    return 1;
                }

                // increment the task number
                task_nr = increment_task_nr(output_dir);
                if (task_nr == -1) {
                    perror("Error: couldn't increment task number\n");
                    return 1;
                }

                break;

            case STATUS:
                // get the status of the task
                printf("Getting status...\n");
                break;

            default:
                break;
        }
    }

    return 0;
}
