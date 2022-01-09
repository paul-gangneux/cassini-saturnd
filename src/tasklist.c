#include "tasklist.h"
#include "timing.h"
#include "timing-text-io.h"
#include "client-request.h"

// ajoute une tache en début de liste
void tasklist_addTask(tasklist *tl, task *t) {
	t->next = tl->first;
	tl->first = t;
}

// fonction auxiliaire
int recursive_remove(task *curr, uint64_t id) {
	if (curr->next == NULL) return 0;
	if (curr->next->id == id) {
		task* tmp = curr->next;
		curr->next = curr->next->next;
		task_free(tmp);
		return 1;
	}
	return recursive_remove(curr->next, id);
}

// renvoie 1 si la tache a été trouvée, 0 sinon
int takslist_remove(tasklist *tl, uint64_t id) {
	if (tl->first == NULL) return 0;
	if (tl->first->id == id) {
		task* tmp = tl->first;
		tl->first = tl->first->next;
		task_free(tmp);
		return 1;
	}
	return recursive_remove(tl->first, id);
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
	task_p->pid_of_exec = -1;
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


string_p task_toString(task *t) {

	// conversion en big-endian ici
	uint64_t taskid = htobe64(t->id);
	uint64_t minutes = htobe64(t->timing.minutes);
	uint32_t hours = htobe32(t->timing.hours);
	uint8_t daysofweek = t->timing.daysofweek;
	uint32_t cmd_argc = htobe32(t->cmdl->ARGC);

	string_p s = string_create("");

	string_p s1 = string_createln(&taskid, sizeof(taskid));
	string_p s2 = string_createln(&minutes, sizeof(minutes));
	string_p s3 = string_createln(&hours, sizeof(hours));
	string_p s4 = string_createln(&daysofweek, sizeof(daysofweek));
	string_p s5 = string_createln(&cmd_argc, sizeof(cmd_argc));
	string_p s6 = string_concatStringsKeepLength(t->cmdl->ARGVs, t->cmdl->ARGC);
	string_concat(s, s1);
	string_concat(s, s2);
	string_concat(s, s3);
	string_concat(s, s4);
	string_concat(s, s5);
	string_concat(s, s6);
	
	string_free(s1);
	string_free(s2);
	string_free(s3);
	string_free(s4);
	string_free(s5);
	string_free(s6);
	
	return s;
}

string_p tasklist_toString(tasklist *tl) {
	string_p s = string_create("");
	task* t = tl->first;
	while (t!=NULL) {
		string_p s2 = task_toString(t);
		string_concat(s, s2);
		string_free(s2);
		t = t->next;
	}
	return s;
}

void task_execute(task *t, char *tasks_dir) {
	if (t->pid_of_exec > 0)
		return;

	char specific_dir [strlen(tasks_dir) + 2 + 20];
	sprintf(specific_dir, "%s/%lu/", tasks_dir, t->id);

	char std_out_fp[strlen(specific_dir) + 7];
	sprintf(std_out_fp, "%s%s", specific_dir, "std_out");

	char std_err_fp[strlen(specific_dir) + 7];
	sprintf(std_err_fp, "%s%s", specific_dir, "std_err");

	pid_t p = fork();
	if (p == 0) {
		int std_out = open(std_out_fp, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
		dup2(std_out, 2);

		int std_err = open(std_err_fp, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
		dup2(std_err, 2);

		char* args[t->cmdl->ARGC];
		for (uint32_t i = 0; i < t->cmdl->ARGC; i++) {
			args[i] = t->cmdl->ARGVs[i]->chars;
		}

		execvp(args[0], args+1);
	} else {
		t->pid_of_exec = p;
	}
}

void execute_tasklist(tasklist *tl, char *tasks_dir) {
	task *t = tl->first;
	while (t!=NULL) {
		char specific_dir [strlen(tasks_dir) + 2 + 20];
		sprintf(specific_dir, "%s/%lu/", tasks_dir, t->id);

		char return_values_fp[strlen(specific_dir) + 13];
		sprintf(return_values_fp, "%s%s", specific_dir, "return_values");
		
		int *status = 0;
		if (t->pid_of_exec > 0) {
			waitpid(t->pid_of_exec, status, WNOHANG);

			int b = open(return_values_fp, O_APPEND | O_CREAT, S_IRWXU);
			write(b, status, sizeof(status));
			close(b);

			t->pid_of_exec = -1;
		} else /* if (is_it_my_time(t->timing) == 1) */ {
			task_execute(t, tasks_dir);
		}

		t=t->next;
	}
}
