#ifndef TASKLIST_H
#define TASKLIST_H

#include "client-request.h"

typedef struct task task;
struct task {
	// TODO precision d'une tache
	uint16_t id;
	commandline *cmdl;
	timing timing;

	task *next;
};

typedef struct {
	task *first;
} tasklist;

void task_addToTasklist(task *t, tasklist *tl);
void iter_tasklist(void (*operation)(task *), tasklist *tl);
task *task_create(uint16_t id, commandline *cmdl, timing timing);
void task_free(task *t);
void task_freeAll(task *t);
int tasklist_length(tasklist *tl);
tasklist *tasklist_create();
void tasklist_free(tasklist *tl);

#endif
