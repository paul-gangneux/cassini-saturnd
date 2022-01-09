#include "tasklist.h"

// ajoute une tache en début de liste
void task_addToTasklist(task *t, tasklist *tl) {
	t->next = tl->first;
	tl->first = t;
}

// applique la fonction (task*) -> (void) à toutes les taches
// de la tasklist
void tasklist_iter(void (*operation)(task *), tasklist *tl) {
	task *t = tl->first;
	while (t != NULL) {
		operation(t);
		t = t->next;
	}
}

// alloue la mémoire necessaire à la création de la tache
// et renvoie un pointeur vers cette tache
// on suppose que la mémoire de cmdl a déjà été allouée (mauvaise idée ?)
task *task_create(uint16_t id, commandline *cmdl, timing timing) {
	task *task_p = malloc(sizeof(task));
	task_p->id = id;
	task_p->cmdl = cmdl;
	task_p->timing = timing;
	task_p->next = NULL;
	return task_p;
}

tasklist *tasklist_create() {
	tasklist *tasklist_p = malloc(sizeof(tasklist));
	return tasklist_p;
}

// fonction auxilière
int length_aux(int n, task* t) {
	if (t == NULL)
		return n;
	return length_aux(n + 1, t->next);
}

int tasklist_length(tasklist *tl) {
	return length_aux(0, tl->first);
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

void tasklist_free(tasklist *tl) {
	task_freeAll(tl->first);
	free(tl);
}
