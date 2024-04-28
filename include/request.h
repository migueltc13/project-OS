#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <stdbool.h>

#define EXECUTE 0
#define STATUS 1
#define COMPLETED 2

#define MAX_CMD_SIZE 300

typedef struct request Request;

Request *create_request(int type, int est_time, char *command, bool is_pipe);

int get_type(Request *r);

int get_est_time(Request *r);

char *get_command(Request *r);

bool get_is_piped(Request *r);

int get_task_nr(Request *r);

void set_type(Request *r, int type);

void set_task_nr(Request *r, int task_nr);

char* type_to_string(int type);

unsigned long sizeof_request();

#endif
