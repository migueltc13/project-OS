#ifndef _ORCHESTRATOR_H_
#define _ORCHESTRATOR_H_

#define SERVER_FIFO "tmp/server_fifo"

// Scheduling policies
#define FCFS 0  // First Come First Serve
#define SJF  1  // Shortest Job First
#define PES  2  // Priority Escalonation Scheduling

// Macros for buffer sizes
#define EXEC_TIME_STRING_SIZE 16
#define EXECUTE_MSG_SIZE (24 + TASK_NR_STRING_SIZE)

#endif
