#ifndef _ORCHESTRATOR_H_
#define _ORCHESTRATOR_H_

#define SERVER_FIFO "tmp/server_fifo"

#define MAX_REQUESTS 1024

// Scheduling policies
#define FCFS 0  // First Come First Serve
#define SJF  1  // Shortest Job First
#define PES  2  // Priority Escalonation Scheduling

#define DEFAULT_POLICY FCFS

#endif
