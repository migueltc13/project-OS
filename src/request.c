// TODO remove ../include/
#include "../include/request.h"
#include "../include/orchestrator.h" // policy constants

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct request {
    int type;
    int est_time; // estimated time or priority
    char command[MAX_CMD_SIZE];
    bool is_piped;
    int task_nr;
    char client_fifo[MAX_FIFO_SIZE];
};

Request *create_request(int type, int est_time, char *command, bool is_piped, char* client_fifo) {

    Request *request = malloc(sizeof(Request));
    if (request == NULL) {
        perror("Error: couldn't allocate memory for request in create_request");
        return NULL;
    }

    request->type = type;
    request->est_time = est_time;
    strcpy(request->command, command);
    request->is_piped = is_piped;
    request->task_nr = -1;
    strcpy(request->client_fifo, client_fifo);

    return request;
}

/* getters */

int get_type(Request *r) {
    return r->type;
}

int get_est_time(Request *r) {
    return r->est_time;
}

char *get_command(Request *r) {
    return r->command;
}

bool get_is_piped(Request *r) {
    return r->is_piped;
}

int get_task_nr(Request *r) {
    return r->task_nr;
}

char *get_client_fifo(Request *r) {
    return r->client_fifo;
}

/* setters */

void set_type(Request *r, int type) {
    r->type = type;
}

void set_task_nr(Request *r, int task_nr) {
    r->task_nr = task_nr;
}

void set_client_fifo(Request *r, char *client_fifo) {
    strcpy(r->client_fifo, client_fifo);
}

/* others */
void print_request(Request *r, int policy) {
    printf("Request:\n");
    printf("  Type: %s\n", type_to_string(r->type));
    switch (policy) {
        case SJF:
            printf("  Estimated time: %d\n", r->est_time);
            break;
        case PES:
            printf("  Priority: %d\n", r->est_time);
            break;
        default: // FCFS
            break;
    }
    printf("  Command: %s\n", r->command);
    printf("  Is piped: %s\n", r->is_piped ? "true" : "false");
    printf("  Task number: %d\n", r->task_nr);
    printf("  Client fifo: %s\n", r->client_fifo);
    printf("\n");
}

char* type_to_string(int type) {
    switch (type) {
        case EXECUTE:
            return "EXECUTE";
        case STATUS:
            return "STATUS";
        case COMPLETED:
            return "COMPLETED";
        case KILL:
            return "KILL";
        default:
            return "UNKNOWN";
    }
}

unsigned long sizeof_request() {
    return sizeof(Request);
}

Request *clone_request(Request *r) {
    Request *new = malloc(sizeof(Request));
    if (new == NULL) {
        perror("Error: couldn't allocate memory for request in clone_request");
        return NULL;
    }

    new->type = r->type;
    new->est_time = r->est_time;
    strcpy(new->command, r->command);
    new->is_piped = r->is_piped;
    new->task_nr = r->task_nr;
    strcpy(new->client_fifo, r->client_fifo);

    return new;
}
