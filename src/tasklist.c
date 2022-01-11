#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>
#include "tasklist.h"
#include "timing.h"
#include "timing-text-io.h"
#include "client-request.h"
#include "commandline.h"

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
// on suppose que la mémoire de cmdl et timing ont déjà été allouées
task *task_create(uint16_t id, commandline *cmdl, timing *timing) {
	task *task_p = malloc(sizeof(task));
	task_p->nb_of_runs = 0;
	task_p->id = id;
	task_p->cmdl = cmdl;
	task_p->timing = timing;
	task_p->next = NULL;
	task_p->exec_time = 0;
	task_p->pid_of_exec = -1;
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
	free(t->timing);
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
	uint64_t minutes = htobe64(t->timing->minutes);
	uint32_t hours = htobe32(t->timing->hours);
	uint8_t daysofweek = t->timing->daysofweek;
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
		int std_out = open(std_out_fp, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
		dup2(std_out, 1);

		int std_err = open(std_err_fp, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
		dup2(std_err, 2);

		char* args[t->cmdl->ARGC+1];
		for (uint32_t i = 0; i < t->cmdl->ARGC; i++) {
			string_addChar(t->cmdl->ARGVs[i], '\0');
			args[i] = t->cmdl->ARGVs[i]->chars;
		}

		args[t->cmdl->ARGC] = NULL;

		execvp(args[0], args);
		exit(1);

	} else {
		t->pid_of_exec = p;
		t->exec_time = time(0);
	}
}

void tasklist_execute(tasklist *tl, char *tasks_dir) {
	task *t = tl->first;
	while (t!=NULL) {
		char specific_dir [strlen(tasks_dir) + 2 + 20];
		sprintf(specific_dir, "%s/%lu/", tasks_dir, t->id);

		char return_values_fp[strlen(specific_dir) + 14];
		sprintf(return_values_fp, "%s%s", specific_dir, "return_values");
		
		int status = 0;
		if (t->pid_of_exec > 0) {
			int retval = waitpid(t->pid_of_exec, &status, WNOHANG);

			if (retval>0) {
				uint16_t st;
				if (WIFEXITED(status)) st = htobe16(WEXITSTATUS(status));
				else st = 0xFFFF;
				int b = open(return_values_fp, O_WRONLY | O_APPEND);
				
				uint64_t time = be64toh(t->exec_time);
				write(b, &time, sizeof(uint64_t));
				write(b, &st, sizeof(uint16_t));
				close(b);

				t->pid_of_exec = -1;
				t->nb_of_runs++;
			}
		} if (t->pid_of_exec ==-1 && is_it_my_time(t->timing) == 1) {
			task_execute(t, tasks_dir);
		}

		t=t->next;
	}
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

	write(tim_id, task->timing, sizeof(timing));

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

	sprintf(buf, "%s/%s", dir, "return_values");
	int ret_id = open(buf, O_RDONLY | O_TRUNC);

	char *buf_cmdl = calloc(2048, 1);
	timing *t = malloc(sizeof(timing));

	read(cmd_id, buf_cmdl, 2048);
	read(tim_id, t, sizeof(timing));

	commandline *cmld = commandline_charsToCommandline(buf_cmdl);

	task *task = task_create(id, cmld, t);

	free(buf_cmdl);
	free(buf);
	free(dir);

	close(cmd_id);
	close(tim_id);
	close(ret_id);

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

// fonction auxiliaire
uint32_t task_getNbExec(task *t, uint64_t id) {
	if (t==NULL) return 0;
	if (t->id==id) return t->nb_of_runs;
	return task_getNbExec(t->next, id);
}

uint32_t tasklist_getNbExec(tasklist *tl, uint64_t id) {
	return task_getNbExec(tl->first, id);
}
