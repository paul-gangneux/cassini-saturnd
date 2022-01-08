#include "tasklist.h"

void iter_tasklist(void (*operation)(task*), tasklist* tl) {
	task* t = tl->first;
	do {
		operation(t);
		t = t->next;
	} while(t->next != NULL);
}
