#ifndef _TASK_NR_H_
#define _TASK_NR_H_

/** @brief Maximum size of the task number string **/
#define TASK_NR_STRING_SIZE 8

unsigned int load_task_nr(char *output_dir);
unsigned int save_task_nr(unsigned int task_nr, char *output_dir);

#endif
