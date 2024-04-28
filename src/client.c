// TODO remove ../include/
#include "../include/request.h"
#include "../include/orchestrator.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void execute_usage(char *name) {
    printf("%s execute <estimated_time> <-u|-p> \"prog-a [args]\"\n", name);
}

void status_usage(char *name) {
    printf("%s status\n", name);
}

int main(int argc, char **argv) {

    // parse arguments
    if (argc < 2) {
        printf("Usage:\n");
        execute_usage(argv[0]);
        status_usage(argv[0]);
        return 0;
    }

    // parse execute option
    if (strcmp(argv[1], "execute") == 0) {
        if (argc < 5) {
            printf("Usage: ");
            execute_usage(argv[0]);
            return 0;
        }

        // parse time
        int time = atoi(argv[2]);

        // parse command option (single or piped)
        bool is_pided = false;
        if (strcmp(argv[3], "-p") == 0) {
            is_pided = true;
        }
        else if (strcmp(argv[3], "-u") == 0) {
            is_pided = false;
        }
        else {
            printf("Usage: ");
            execute_usage(argv[0]);
            return 0;
        }

        // parse command
        char *command = argv[4];

        // create request
        Request *r = create_request(EXECUTE, time, command, is_pided);

        // send request via server FIFO
        int fd = open(SERVER_FIFO, O_WRONLY);
        if (fd == -1) {
            perror("Error: couldn't open server FIFO\n");
            return 1;
        }

        if (write(fd, r, sizeof_request()) == -1) {
            perror("Error: couldn't write to server FIFO\n");
            close(fd);
            return 1;
        }

        close(fd);
        free(r);
        return 0;
    }

    // parse status option
    if (strcmp(argv[1], "status") == 0) {
        if (argc != 2) {
            printf("Usage: ");
            status_usage(argv[0]);
            return 0;
        }

        // TODO create request
        // TODO send request via server FIFO
        // TODO create FIFO to receive status response
        // TODO read status response
        // TODO print status response
        return 0;
    }

    // invalid option
    printf("Usage:\n");
    execute_usage(argv[0]);
    status_usage(argv[0]);
    return 0;
}
