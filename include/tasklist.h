#ifndef TASKLIST_H
#define TASKLIST_H

#include "client-request.h"

struct task {
	// TODO precision d'une tache
	uint16_t* id;
	commandline* cmdl;
	timing timing;

	struct task* next;
};
typedef struct task task;

typedef struct {
	task* first;
} tasklist;



#endif
