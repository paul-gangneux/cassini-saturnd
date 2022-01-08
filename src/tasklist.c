#include "tasklist.h"

void iter_tasklist(void (*operation)(task*, void*), tasklist* tl) {
	task* t = tl->first;
	do {
		operation(t);
		t = t->next;
	} while(t->next != NULL);
}

void execute_task_if_needed(task* t, void* return_value) {
	// Check time, if good execute and log.
	time_t timestamp = time( NULL );
	struct tm * now = localtime( & timestamp );

	// Compare with timing => encode tm, mask, diff 0.0

}
