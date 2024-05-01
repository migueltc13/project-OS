#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <stdbool.h>

#define EXECUTE 0
#define STATUS 1
#define COMPLETED 2
#define KILL 3

#define MAX_CMD_SIZE 300

typedef struct request Request;

Request *create_request(int type, int est_time, char *command, bool is_pipe, char *client_fifo);

int get_type(Request *r);

int get_est_time(Request *r);

char *get_command(Request *r);

bool get_is_piped(Request *r);

int get_task_nr(Request *r);

char *get_client_fifo(Request *r);

long get_time(Request *r);

void set_type(Request *r, int type);

void set_task_nr(Request *r, int task_nr);

void set_client_fifo(Request *r, char *client_fifo);

void print_request(Request *r, int policy);

char* type_to_string(int type);

unsigned long sizeof_request();

Request *clone_request(Request *r);

#endif
