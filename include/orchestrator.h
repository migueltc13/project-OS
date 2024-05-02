#ifndef _ORCHESTRATOR_H_
#define _ORCHESTRATOR_H_

/** @brief Name of the server fifo **/
#define SERVER_FIFO "server_fifo"

// Scheduling policies

/** @brief First Come First Serve **/
#define FCFS 0

/** @brief Shortest Job First **/
#define SJF  1

/** @brief Priority Escalonation Scheduling **/
#define PES  2

// Macros for buffer sizes

/** @brief Maximum size of the execution time string **/
#define EXEC_TIME_STRING_SIZE 16

/** @brief Maximum size of the execute message
 * transmitted between the orchestrator and the client **/
#define EXECUTE_MSG_SIZE (24 + TASK_NR_STRING_SIZE)

#endif
