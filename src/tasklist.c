#include "tasklist.h"

// ajoute une tache en début de liste
void task_addToTaskList(task *t, tasklist *tl) {
	t->next = tl->first;
	tl->first = t;
}

// applique la fonction (task*) -> (void) à toutes les taches
// de la tasklist
void iter_tasklist(void (*operation)(task *), tasklist *tl) {
	task *t = tl->first;
	while (t != NULL) {
		operation(t);
		t = t->next;
	}
}

// alloue la mémoire necessaire à la création de la tache
// et renvoie un pointeur vers cette tache
task *task_create(uint16_t id, commandline *cmdl, timing timing) {
	task *task_p = malloc(sizeof(task));
	task_p->id = id;
	task_p->cmdl = cmdl;
	task_p->timing = timing;
	task_p->next = NULL;
	return task_p;
}

// free une tache seule, ne free pas t->next
void task_free(task *t) {
	commandline_free(t->cmdl);
	free(t);
}

// free une tache et toutes celles qui la suit
void task_freeAll(task *t) {
	if (t == NULL)
		return;
	task *t2 = t->next;
	task_free(t);
	task_freeAll(t2);
}
