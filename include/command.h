#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <sys/time.h>

int exec(char *cmd, bool is_piped, char* output_dir, int task_nr, struct timeval start_time);

#endif
