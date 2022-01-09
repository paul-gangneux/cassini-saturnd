#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "tasklist.h"
#include "timing.h"
#include "timing-text-io.h"
#include "client-request.h"

#define max(a,b) a>b?a:b;

// ajoute une tache en début de liste
void tasklist_addTask(tasklist *tl, task *t) {
	t->next = tl->first;
	tl->first = t;
}



// fonction auxiliaire
void delete_files(uint64_t id, const char *path) {
	char *dir = malloc(strlen(path) + 16);
	char *buf = malloc(strlen(path) + 64);

	sprintf(dir, "%s/%lu", path, id);
	sprintf(buf, "%s/%s", dir, "command_line");
	unlink(buf);
	sprintf(buf, "%s/%s", dir, "timing");
	unlink(buf);
	sprintf(buf, "%s/%s", dir, "return_values");
	unlink(buf);
	sprintf(buf, "%s/%s", dir, "std_out");
	unlink(buf);
	sprintf(buf, "%s/%s", dir, "std_err");
	unlink(buf);
	rmdir(dir);

	free(dir);
	free(buf);
}

// fonction auxiliaire
int recursive_remove(task *curr, uint64_t id, const char *path) {
	if (curr->next == NULL)
		return 0;
	if (curr->next->id == id)
	{
		task *tmp = curr->next;
		curr->next = curr->next->next;
		task_free(tmp);
		delete_files(id, path);
		return 1;
	}
	return recursive_remove(curr->next, id, path);
}

// renvoie 1 si la tache a été trouvée, 0 sinon
int takslist_remove(tasklist *tl, uint64_t id, const char *path) {
	if (tl->first == NULL)
		return 0;
	if (tl->first->id == id) {
		task *tmp = tl->first;
		tl->first = tl->first->next;
		delete_files(id, path);
		task_free(tmp);
		return 1;
	}
	return recursive_remove(tl->first, id, path);
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
	tasklist_p->first = NULL;
	return tasklist_p;
}

// fonction auxilière
int length_aux(int n, task *t) {
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
	task *t = tl->first;
	while (t != NULL) {
		string_p s2 = task_toString(t);
		string_concat(s, s2);
		string_free(s2);
		t = t->next;
	}
	return s;
}

void task_createFiles(task *task, const char *path) {
	char *dir = malloc(strlen(path) + 16);
	char *buf = malloc(strlen(path) + 64);

	sprintf(dir, "%s/%lu", path, task->id);
	mkdir(dir, 0777);

	sprintf(buf, "%s/%s", dir, "command_line");
	int cmd_id = open(buf, O_CREAT | O_WRONLY | O_TRUNC, 0666);

	sprintf(buf, "%s/%s", dir, "timing");
	int tim_id = open(buf, O_CREAT | O_WRONLY | O_TRUNC, 0666);

	sprintf(buf, "%s/%s", dir, "return_values");
	int ret_id = open(buf, O_CREAT | O_WRONLY | O_TRUNC, 0666);

	sprintf(buf, "%s/%s", dir, "std_out");
	int out_id = open(buf, O_CREAT | O_WRONLY | O_TRUNC, 0666);

	sprintf(buf, "%s/%s", dir, "std_err");
	int err_id = open(buf, O_CREAT | O_WRONLY | O_TRUNC, 0666);

	string_p cmld = commandline_toString(task->cmdl);
	write(cmd_id, cmld->chars, cmld->length);

	write(tim_id, &task->timing, sizeof(timing));

	string_free(cmld);
	close(cmd_id);
	close(tim_id);
	close(ret_id);
	close(out_id);
	close(err_id);
	free(dir);
	free(buf);
}

task* task_fromDirectory(const char* path, const char* dir_basename) {
	uint64_t id = strtoul(dir_basename, NULL, 10);
	char *dir = malloc(strlen(path) + 16);
	char *buf = malloc(strlen(path) + 64);

	sprintf(dir, "%s/%s", path, dir_basename);

	sprintf(buf, "%s/%s", dir, "command_line");
	int cmd_id = open(buf, O_RDONLY);

	sprintf(buf, "%s/%s", dir, "timing");
	int tim_id = open(buf, O_RDONLY);

	char *buf_cmdl = calloc(2048, 1);
	timing t;

	read(cmd_id, buf_cmdl, 2048);
	read(tim_id, &t, sizeof(timing));

	commandline *cmld = commandline_charsToCommandline(buf_cmdl);

	task *task = task_create(id, cmld, t);

	free(buf_cmdl);
	free(buf);
	free(dir);

	close(cmd_id);
	close(tim_id);

	return task;
}

uint64_t tasklist_readTasksInDir(tasklist *tl, const char *path) {
	struct dirent *ent;
	DIR *dir = opendir(path);
	uint64_t nb = 0;

	while ((ent = readdir(dir)) != NULL) {
		if (ent->d_name[0] != '.') {
			tasklist_addTask(tl, task_fromDirectory(path, ent->d_name));
			uint64_t x = strtoul(ent->d_name, NULL, 10) + 1;
			nb = max(nb, x);
		}
	}

	closedir(dir);
	return nb;
}
