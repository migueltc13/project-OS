// TODO remove ../include/
#include "../include/request.h"
#include "../include/client.h" // MAX_FIFO_SIZE macro
#include "../include/orchestrator.h" // policy macros

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @struct request
 * @brief The request struct.
 * @details The request struct represents a request sent by the client,
 * or a request sent by the executor to the @ref orchestrator.c, marking commands
 * as completed.
 *
 * @var request::type
 * The type of the request.
 *
 * @var request::est_time
 * The estimated time or the priority of the request, based on the policy
 * being used in the @ref orchestrator.c server.
 *
 * @var request::command
 * The command to be executed or marked as completed.
 *
 * @var request::is_piped
 * A boolean indicating if the command is piped.
 *
 * @var request::task_nr
 * The task number of the request.
 *
 * @var request::client_fifo
 * The client FIFO name to send the response to the client.
 */
struct request {
    int type;
    int est_time; // estimated time or priority
    char command[MAX_CMD_SIZE];
    bool is_piped;
    unsigned int task_nr;
    char client_fifo[CLIENT_FIFO_SIZE];
};

/**
 * @brief Creates a new request.
 * @details Allocates memory for a new request and initializes it with the
 * given parameters.
 *
 * It initializes the task number with 0, which is not a valid task number.
 *
 * For the creation of a status request, the @param est_time should be `0`,
 * as it is not used, as well as the @param command should be `NULL`
 * and @param is_piped should be `false`.
 *
 * Example of the creation of an execute request:
 * @code
 * Request *execute_request = create_request(EXECUTE, 12, "ls -l | cat", true, "client-1234");
 * @endcode

 * Example of the creation of a status request:
 * @code
 * Request *status_request = create_request(STATUS, 0, NULL, false, "client-1234");
 * @endcode
 *
 * Example of the creation of a kill request:
 * @code
 * Request *kill_request = create_request(KILL, 0, NULL, false, "client-1234");
 * @endcode
 *
 * **The caller is responsible for freeing the returned request by this function.**
 * @param type the type of the request
 * @param est_time the estimated time or priority of the request
 * @param command the command to be executed
 * @param is_piped a boolean indicating if the command is piped
 * @param client_fifo the client FIFO name to send the response to the client
 */
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
    request->task_nr = 0; // 0 isn't a valid task number
    strcpy(request->client_fifo, client_fifo);

    return request;
}

/******** getters ********/

/**
 * @brief Get the type of the request.
 * @param r The request to get the type from
 * @return The type of the request
 */
int get_type(Request *r) {
    return r->type;
}

/**
 * @brief Get the estimated time or priority of the request.
 * @param r The request to get the estimated time or priority from
 * @return The estimated time or priority of the request
 */
int get_est_time(Request *r) {
    return r->est_time;
}

/**
 * @brief Get the command to be executed.
 * @param r The request to get the command from
 * @return The command to be executed
 */
char *get_command(Request *r) {
    return r->command;
}

/**
 * @brief Get the boolean indicating if the command is piped.
 * @param r The request to get the is_piped from
 * @return The boolean indicating if the command is piped
 */
bool get_is_piped(Request *r) {
    return r->is_piped;
}

/**
 * @brief Get the task number of the request.
 * @param r The request to get the task number from
 * @return The task number of the request
 */
unsigned int get_task_nr(Request *r) {
    return r->task_nr;
}

/**
 * @brief Get the client FIFO name to send the response to the client.
 * @param r The request to get the client FIFO name from
 * @return The client FIFO name to send the response to the client
 */
char *get_client_fifo(Request *r) {
    return r->client_fifo;
}

/******** setters ********/

/**
 * @brief Set the type of the request.
 * @param r The request to set the type
 * @param type The type to set
 */
void set_type(Request *r, int type) {
    r->type = type;
}

/**
 * @brief Set the task number of the request.
 * @param r The request to set the task number
 * @param task_nr The task number to set
 */
void set_task_nr(Request *r, unsigned int task_nr) {
    r->task_nr = task_nr;
}

/**
 * @brief Set the client FIFO name to send the response to the client.
 * @param r The request to set the client FIFO name
 * @param client_fifo The client FIFO name to set
 */
void set_client_fifo(Request *r, char *client_fifo) {
    strcpy(r->client_fifo, client_fifo);
}

/******** others ********/

/**
 * @brief Print the request.
 * @details Prints the request attributes to the standard output.
 *
 * Based on the policy being used, it prints the estimated time or the priority.
 *
 * If the policy is FCFS, it doesn't print either the estimated time or the priority.
 * @param r The request to print
 * @param policy The policy being used
 */
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
    printf("  Command: \"%s\"\n", r->command);
    printf("  Is piped: %s\n", r->is_piped ? "true" : "false");
    printf("  Task number: %d\n", r->task_nr);
    printf("  Client fifo: %s\n", r->client_fifo);
    printf("\n");
}

/**
 * @brief Convert the type of the request to a string.
 * @details Possible types are
 * @ref EXECUTE "EXECUTE",
 * @ref STATUS "STATUS",
 * @ref COMPLETED "COMPLETED" and
 * @ref KILL "KILL".
 * @param type The type of the request
 * @return The string representation of the type of the request
 */
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

/**
 * @brief Get the size of the request.
 * @details Used due to the fact that the @ref Request struct is not visible
 * to other modules, so @code sizeof(Request) @endcode wouldn't give the
 * correct size of the struct.
 * @return The size of the Request struct
 */
unsigned long sizeof_request() {
    return sizeof(Request);
}

/**
 * @brief Clone a request.
 * @details Allocates memory for a new request and initializes it with the
 * attributes of the given request.
 *
 * Used in the @ref orchestrator.c to clone a request before
 * adding it to the executing or scheduled array.
 *
 * **The caller is responsible for freeing the returned request by this function.**
 * @param r The request to clone
 * @return The cloned request
 */
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
