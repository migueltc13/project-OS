// TODO remove ../include/
#include "../include/client.h" // CLIENT_FIFO_SIZE macro
#include "../include/orchestrator.h"  // SERVER_FIFO macro
#include "../include/request.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/** @brief The size of the buffer to read from the FIFO. */
#define BUF_SIZE 4096

void execute_usage(char *name);

void status_usage(char *name);

void kill_usage(char *name);

/**
 * @brief The main function of the client program.
 * @details The client program is responsible for sending requests
 * to the server.
 *
 * The client program can execute a command, check the status of
 * the executing, scheduled and completed requests, or kill the server.
 *
 * It's responsible for parsing the arguments and sending the
 * requests to the server program via the server FIFO.
 *
 * The status and execute options require the creation of a client
 * FIFO to receive the server response.
 * @param argc the number of arguments
 * @param argv the arguments
 * @return 0 if the program executed successfully, 1 otherwise
 */
int main(int argc, char **argv) {

    // parse arguments
    if (argc < 2) {
        printf("Usage:\n");
        execute_usage(argv[0]);
        status_usage(argv[0]);
        kill_usage(argv[0]);
        return 0;
    }

    // create client FIFO
    char *client_fifo = malloc(CLIENT_FIFO_SIZE);
    if (client_fifo == NULL) {
        perror("Error: couldn't allocate memory for client FIFO name");
        return 1;
    }

    int result = snprintf(client_fifo, CLIENT_FIFO_SIZE, "client-%d", getpid());
    if (result < 0 || result >= CLIENT_FIFO_SIZE) {
        perror("Error: couldn't create client FIFO name with snprintf");
        free(client_fifo);
        return 1;
    }

    (void) unlink(client_fifo);
    if (mkfifo(client_fifo, 0644) == -1) {
        perror("Error: couldn't create client FIFO");
        free(client_fifo);
        return 1;
    }

    // parse execute option
    if (strcmp(argv[1], "execute") == 0) {
        if (argc < 5) {
            printf("Usage: ");
            execute_usage(argv[0]);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 0;
        }

        // parse time
        int time = atoi(argv[2]);
        if (time < 0) {
            printf("Usage: ");
            execute_usage(argv[0]);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 0;
        }

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
            free(client_fifo);
            (void) unlink(client_fifo);
            return 0;
        }

        // parse command
        char *command = argv[4];

        // create request
        Request *r = create_request(EXECUTE, time, command, is_pided, client_fifo);
        if (r == NULL) {
            perror("Error: couldn't create status request");
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        // send request via server FIFO
        int fd = open(SERVER_FIFO, O_WRONLY);
        if (fd == -1) {
            perror("Error: couldn't open server FIFO");
            free(r);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        if (write(fd, r, sizeof_request()) == -1) {
            perror("Error: couldn't write to server FIFO");
            (void) close(fd);
            free(r);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        (void) close(fd);

        // receive task number via client FIFO
        int fd_client = open(client_fifo, O_RDONLY);
        if (fd_client == -1) {
            perror("Error: couldn't open client FIFO");
            free(r);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        // read and write the server message
        char buffer[BUF_SIZE];
        size_t n;
        while ((n = read(fd_client, buffer, BUF_SIZE)) > 0) {
            buffer[n] = '\0';
            if (write(STDOUT_FILENO, buffer, n) == -1) {
                perror("Error: couldn't write to stdout");
                (void) close(fd_client);
                free(r);
                free(client_fifo);
                (void) unlink(client_fifo);
                return 1;
            }
        }

        (void) close(fd_client);

        free(r);
    }

    // parse status option
    else if (strcmp(argv[1], "status") == 0) {
        if (argc != 2) {
            printf("Usage: ");
            status_usage(argv[0]);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 0;
        }

        // create status request
        Request *r = create_request(STATUS, 0, "", false, client_fifo);
        if (r == NULL) {
            perror("Error: couldn't create status request");
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        // send client FIFO in status request
        set_client_fifo(r, client_fifo);

        // send request via server FIFO
        int fd_server = open(SERVER_FIFO, O_WRONLY);
        if (fd_server == -1) {
            perror("Error: couldn't open server FIFO");
            free(r);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        if (write(fd_server, r, sizeof_request()) == -1) {
            perror("Error: couldn't write to server FIFO");
            (void) close(fd_server);
            free(r);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        (void) close(fd_server);

        // receive status response via client FIFO
        int fd_client = open(client_fifo, O_RDONLY);
        if (fd_client == -1) {
            perror("Error: couldn't open client FIFO");
            free(r);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        // read and print status response
        char buffer[BUF_SIZE];
        size_t n;
        while ((n = read(fd_client, buffer, BUF_SIZE)) > 0) {
            if (write(STDOUT_FILENO, buffer, n) == -1) {
                perror("Error: couldn't write to stdout");
                (void) close(fd_client);
                free(r);
                free(client_fifo);
                (void) unlink(client_fifo);
                return 1;
            }
        }

        (void) close(fd_client);
        free(r);
    }
    else if (strcmp(argv[1], "kill") == 0) {
        if (argc != 2) {
            printf("Usage: ");
            kill_usage(argv[0]);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 0;
        }

        // create kill request
        Request *r = create_request(KILL, 0, "", false, client_fifo);
        if (r == NULL) {
            perror("Error: couldn't create kill request");
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        // send request via server FIFO
        int fd = open(SERVER_FIFO, O_WRONLY);
        if (fd == -1) {
            perror("Error: couldn't open server FIFO");
            free(r);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        if (write(fd, r, sizeof_request()) == -1) {
            perror("Error: couldn't write to server FIFO");
            (void) close(fd);
            free(r);
            free(client_fifo);
            (void) unlink(client_fifo);
            return 1;
        }

        (void) close(fd);
        free(r);
    }
    else {
        // invalid option
        printf("Usage:\n");
        execute_usage(argv[0]);
        status_usage(argv[0]);
        kill_usage(argv[0]);
    }

    (void) unlink(client_fifo);
    free(client_fifo);
    return 0;
}

/**
 * @brief Print usage for the execute option.
 * @param name the name of the client executable
 */
void execute_usage(char *name) {
    printf("%s execute <estimated_time|priority> <-u|-p> \"prog-a [args]\"\n", name);
}

/**
 * @brief Print usage for the status option.
 * @param name the name of the client executable
 */
void status_usage(char *name) {
    printf("%s status\n", name);
}

/**
 * @brief Print usage for the kill option.
 * @param name the name of the client executable
 */
void kill_usage(char *name) {
    printf("%s kill\n", name);
}
