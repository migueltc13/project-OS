// TODO remove ../include/
#include "../include/request.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct request {
    int type;
    int est_time;
    char command[MAX_CMD_SIZE];
    bool is_piped;
    int task_nr;
};

Request *create_request(int type, int est_time, char *command, bool is_piped) {

    Request *request = malloc(sizeof(Request));
    if (request == NULL) {
        perror("malloc");
        return NULL;
    }

    request->type = type;
    request->est_time = est_time;
    strcpy(request->command, command);
    request->is_piped = is_piped;

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

/* setters */

void set_type(Request *r, int type) {
    r->type = type;
}

void set_task_nr(Request *r, int task_nr) {
    r->task_nr = task_nr;
}

/* others */
char* type_to_string(int type) {
    switch (type) {
        case EXECUTE:
            return "EXECUTE";
        case STATUS:
            return "STATUS";
        case COMPLETED:
            return "COMPLETED";
        default:
            return "UNKNOWN";
    }
}

unsigned long sizeof_request() {
    return sizeof(Request);
}
