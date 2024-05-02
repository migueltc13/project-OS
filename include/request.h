#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <stdbool.h>

// Request types

/** @brief Execute request code **/
#define EXECUTE 0

/** @brief Status request code **/
#define STATUS 1

/** @brief Completed request code **/
#define COMPLETED 2

/** @brief Kill request code **/
#define KILL 3

/** @brief Maximum size of the command string **/
#define MAX_CMD_SIZE 300

typedef struct request Request;

Request *create_request(int type, int est_time, char *command, bool is_pipe, char *client_fifo);

int get_type(Request *r);

int get_est_time(Request *r);

char *get_command(Request *r);

bool get_is_piped(Request *r);

unsigned int get_task_nr(Request *r);

char *get_client_fifo(Request *r);

long get_time(Request *r);

void set_type(Request *r, int type);

void set_task_nr(Request *r, unsigned int task_nr);

void set_client_fifo(Request *r, char *client_fifo);

void print_request(Request *r, int policy);

char* type_to_string(int type);

unsigned long sizeof_request();

Request *clone_request(Request *r);

#endif
