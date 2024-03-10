#include <unistd.h>
#include <fcntl.h>
#include <string.h>
/* #include <stdlib.h> */

/**
 * @brief Main function for the orchestrator program
 * The orchestrator program is responsible for managing the execution of the parallel tasks.
 * It receives the output directory, the number of parallel tasks, and the scheduling policy as arguments.
 * This function is responsible for validating the arguments, parsing them and
 * calling the appropriate functions to manage the parallel tasks.
 * @param argc The number of arguments
 * @param argv The arguments
 * @return 0 if the program runs successfully, 1 otherwise
 */
int main(int argc, char *argv[]) {

    // Validate the number of arguments
    if (argc < 3) {
        char *msg = "Usage: orchestrator <output_dir> <parallel-tasks> [sched-policy]\n";
        write(STDERR_FILENO, msg, strlen(msg));
        return 1;
    }

    // Get the output directory
    char *output_dir = argv[1];

    // open the output directory
    int fd = open(output_dir, O_DIRECTORY);
    if (fd < 0) {
        char *msg = "Error: output directory does not exist\n";
        write(STDERR_FILENO, msg, strlen(msg));
        return 1;
    }
    // close the output directory
    close(fd);

    // Get the number of parallel tasks
    /* int parallel_tasks = atoi(argv[2]); */

    // Get the scheduling policy if it exists
    /* char *sched_policy = NULL;
    if (argc > 3) {
        sched_policy = argv[3];
    } */

    char *msg = "Orchestrator server is running\n";
    write(STDOUT_FILENO, msg, strlen(msg));

    return 0;
}
