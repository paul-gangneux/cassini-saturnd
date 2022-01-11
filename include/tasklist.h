#ifndef TASKLIST_H
#define TASKLIST_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

#include "client-request.h"
#include "custom-string.h"
#include "timing.h"

typedef struct task task;
struct task {
	uint32_t nb_of_runs;
	pid_t pid_of_exec;
	time_t exec_time;
	uint64_t id;
	commandline *cmdl;
	timing *timing;

	task *next;
};

typedef struct {
	task *first;
} tasklist;

void tasklist_addTask(tasklist *tl, task *t);
int takslist_remove(tasklist *tl, uint64_t id, const char *path);
void iter_tasklist(void (*operation)(task *), tasklist *tl);
task *task_create(uint16_t id, commandline *cmdl, timing *timing);
void task_free(task *t);
void task_freeAll(task *t);
int tasklist_length(tasklist *tl);
tasklist *tasklist_create();
void tasklist_free(tasklist *tl);
string_p tasklist_toString(tasklist *tl);
void task_createFiles(task *t, const char *path);
uint64_t tasklist_readTasksInDir(tasklist *tl, const char *path);

void task_execute(task* t, char* tasks_dir);
void tasklist_execute(tasklist *tl, char *tasks_dir);
uint32_t tasklist_getNbExec(tasklist *tl, uint64_t id);

uint64_t timing_field_from_int(uint64_t *dest, int i, unsigned int min, unsigned int max);
#endif
