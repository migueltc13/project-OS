// TODO remove ../include/
#include "../include/task_nr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @brief Get the task number from the TASK_NR_FILE file.
 * If the file does not exist, create it with the initial value 1.
 * @param output_dir The output directory to write the task number file
 * @return The task number, or -1 if an error occurs
 */
int get_task_nr(char *output_dir) {

    int fd = open(output_dir, O_DIRECTORY);
    if (fd < 0) {
        perror("Error: output directory does not exist\n");
        close(fd);
        return -1;
    }
    close(fd);

    // create the filename
    char *filename = malloc(strlen(output_dir) + strlen(TASK_NR_FILE) + 2);
    (void) sprintf(filename, "%s/%s", output_dir, TASK_NR_FILE); // TODO: check for buffer overflow

    // if TASK_NR_FILE doesn't exist, create it with value 1
    int task_nr = 1;
    if (access(filename, F_OK) == -1) {
        int fd = open(filename, O_CREAT | O_WRONLY, 0644);
        if (fd < 0) {
            perror("Error: could not create task number file\n");
            close(fd);
            return -1;
        }
        if (write(fd, &task_nr, sizeof(int)) == -1) {
            perror("Error: could not write to task number file\n");
            close(fd);
            return -1;
        }
        close(fd);
    }
    else {
        // read the task number from the file
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            perror("Error: could not open task number file\n");
            close(fd);
            return -1;
        }

        if (read(fd, &task_nr, sizeof(int)) == -1) {
            perror("Error: could not read from task number file\n");
            close(fd);
            return -1;
        }
        close(fd);
    }

    return task_nr;
}

/**
 * @brief Increment the task number in the TASK_NR_FILE file.
 * @param output_dir The output directory to write the task number file
 * @return The new incremented task number, or -1 if an error occurs
 */
int increment_task_nr(char *output_dir) {

    int fd = open(output_dir, O_DIRECTORY);
    if (fd < 0) {
        perror("Error: output directory doesn't exist\n");
        close(fd);
        return -1;
    }
    close(fd);

    // create the filename
    char *filename = malloc(strlen(output_dir) + strlen(TASK_NR_FILE) + 2);
    (void) sprintf(filename, "%s/%s", output_dir, TASK_NR_FILE); // TODO: check for buffer overflow

    // read the task number from the file
    int task_nr = 1;
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error: couldn't open task number file\n");
        close(fd);
        return -1;
    }

    if (read(fd, &task_nr, sizeof(int)) == -1) {
        perror("Error: couldn't read from task number file\n");
        close(fd);
        return -1;
    }
    close(fd);

    // increment the task number
    task_nr++;

    // write the new task number to the file
    fd = open(filename, O_WRONLY);
    if (fd < 0) {
        perror("Error: couldn't open task number file\n");
        close(fd);
        return -1;
    }

    if (write(fd, &task_nr, sizeof(int)) == -1) {
        perror("Error: couldn't write to task number file\n");
        close(fd);
        return -1;
    }
    close(fd);

    return task_nr;
}
