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

#endif
