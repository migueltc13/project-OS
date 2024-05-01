#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "request.h"
#include <sys/time.h>

#define HISTORY_NAME "history"

int exec(Request *r, char *output_dir, struct timeval start_time);

#endif
