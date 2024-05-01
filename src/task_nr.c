// TODO remove ../include/
#include "../include/task_nr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define TASK_NR_FILENAME "task_number"

char* get_task_nr_filename(char *output_dir);

/**
 * @brief Load the task number from the file in the output directory.
 * If the file does not exist, create it with the initial value 1.
 * Used by the orchestrator to load the task number when starting.
 * @param output_dir The output directory to write the task number file
 * @return The task number, or -1 if an error occurs
 */
int load_task_nr(char *output_dir) {

    char *filename = get_task_nr_filename(output_dir);
    if (filename == NULL) {
        perror("Error: couldn't get task number filename");
        return -1;
    }

    // If the task number file doesn't exist,
    // create it with the initial value 1
    int task_nr = 1;
    if (access(filename, F_OK) == -1) {
        int fd = open(filename, O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
            perror("Error: couldn't create task number file");
            free(filename);
            return -1;
        }

        if (write(fd, &task_nr, sizeof(int)) == -1) {
            perror("Error: couldn't write to task number file");
            (void) close(fd);
            free(filename);
            return -1;
        }

        (void) close(fd);
    }
    else {
        // read the task number from the file
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            perror("Error: couldn't open task number file");
            free(filename);
            return -1;
        }

        if (read(fd, &task_nr, sizeof(int)) == -1) {
            perror("Error: couldn't read from task number file");
            (void) close(fd);
            free(filename);
            return -1;
        }

        (void) close(fd);
    }

    free(filename);
    return task_nr;
}

/**
 * @brief Save the task number to the file in the output directory.
 * Used by the orchestrator to save the task number before exiting.
 * @param task_nr The task number to save
 * @param output_dir The output directory to write the task number file
 * @return The task number, or -1 if an error occurs
 */
int save_task_nr(int task_nr, char *output_dir) {

    char *filename = get_task_nr_filename(output_dir);
    if (filename == NULL) {
        perror("Error: couldn't get task number filename");
        return -1;
    }

    // write the new task number
    int fd = open(filename, O_WRONLY);
    if (fd == -1) {
        perror("Error: couldn't open task number file");
        free(filename);
        return -1;
    }

    if (write(fd, &task_nr, sizeof(int)) == -1) {
        perror("Error: couldn't write to task number file");
        (void) close(fd);
        free(filename);
        return -1;
    }
    (void) close(fd);

    free(filename);
    return task_nr;
}

/**
 * @brief Get the filename of the task number file in the output directory.
 * The task number filename is defined in the TASK_NR_FILENAME macro.
 * Used both by load_task_nr and save_task_nr as an auxiliary function.
 * Note: the caller is responsible for freeing the returned string.
 * @param output_dir The output directory to write the task number file
 * @return The filename of the task number file, or NULL if an error occurs
 */
char* get_task_nr_filename(char *output_dir) {

    // check if output_dir exists
    if (access(output_dir, F_OK) == -1) {
        perror("Error: output directory does not exist");
        return NULL;
    }

    // create the filename
    int size = strlen(output_dir) + strlen(TASK_NR_FILENAME) + 2; // +2 for '/' and '\0'
    char *filename = malloc(size);
    if (filename == NULL) {
        perror("Error: couldn't allocate memory for task number filename");
        return NULL;
    }

    int result = snprintf(filename, size, "%s/%s", output_dir, TASK_NR_FILENAME);
    if (result < 0 || result >= size) {
        perror("Error: couldn't create task number filename with snprintf");
        free(filename);
        return NULL;
    }

    return filename;
}
