#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <poll.h>
#include <time.h>

#include "saturnd.h"
#include "timing-text-io.h"
#include "commandline.h"

int main() {

	char *pipes_directory;
	char *tasks_directory;
	char *username = getenv("USER");
	char *buf = calloc(512, 1);

  if (username != NULL) {
    int u = strlen(username);
    pipes_directory = calloc(20 + u, 1);
		tasks_directory = calloc(20 + u, 1);
    sprintf(pipes_directory, "/tmp/%s/saturnd/pipes", username);
		sprintf(tasks_directory, "/tmp/%s/saturnd/tasks", username);
  } else {
		printf("Cannot get environment variable USER\n");
		return 1;
	}

	char* req_pipe_basename = "saturnd-request-pipe";
	char* ans_pipe_basename = "saturnd-reply-pipe";

	char* req_pipe_path = calloc(strlen(pipes_directory) + strlen(req_pipe_basename) + 2, 1);
	char* ans_pipe_path = calloc(strlen(pipes_directory) + strlen(ans_pipe_basename) + 2, 1);

	sprintf(req_pipe_path, "%s/%s", pipes_directory, req_pipe_basename);
	sprintf(ans_pipe_path, "%s/%s", pipes_directory, ans_pipe_basename);

	// créé les repretoires dans tmp
	sprintf(buf, "/tmp/%s", username);
	mkdir(buf, 0777);
	sprintf(buf, "/tmp/%s/saturnd", username);
	mkdir(buf, 0777);
	sprintf(buf, "/tmp/%s/saturnd/pipes", username);
	mkdir(buf, 0777);
	sprintf(buf, "/tmp/%s/saturnd/tasks", username);
	mkdir(buf, 0777);

	// création des tubes
	mkfifo(req_pipe_path, 0666);
	mkfifo(ans_pipe_path, 0666);

	free(pipes_directory);
	free(buf);

	// La boucle principale est dans un double fork, et est ainsi adoptee
	// par init lors de la fin du processus principal
	// On ne fork que si DEBUG n'est pas defini, le fork rendant le
	// debug plus complique
#ifndef DEBUG
	if (fork() == 0) {
		if(fork() == 0) {
#endif
			saturnd_loop(req_pipe_path, ans_pipe_path, tasks_directory);
#ifndef DEBUG
		}
	}
#endif

	free(req_pipe_path);
	free(ans_pipe_path);
	free(tasks_directory);
}

void write_error_not_found(int fd) {
	uint16_t rep0 = htobe16(SERVER_REPLY_ERROR);
	uint16_t rep1 = htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
	uint16_t rep[2];
	rep[0] = rep0; rep[1] = rep1;
	write(fd, rep, sizeof(uint32_t));
}

void answer_with_file_content_at_id(uint64_t id, const char* file_dir, const char* file_name, int answer_fd) {
	char* path = calloc(strlen(file_dir) + strlen(file_name) + 32, 1);
	sprintf(path, "%s/%lu/%s", file_dir, id, file_name);

	string_p output = string_readFromFile(path);
	if (output == NULL) {
		write_error_not_found(answer_fd);
	} else {
		uint16_t rep = htobe16(SERVER_REPLY_OK);
		string_p ans = string_createln(&rep, sizeof(rep));
		uint32_t strl = ans->length; // si je met en big-endian ici ça marche pas jsp pourquoi

		string_p len = string_createln(&strl, sizeof(strl));
		string_concat(ans, len);
		string_concat(ans, output);

		write(answer_fd, ans->chars, ans->length);
		string_free(ans);
		string_free(len);
		string_free(output);
	}
	free(path);
}

// le fd passé en argument sera fermé lors d'un exec
int set_cloexec_flag(int fd) {
  int oldflags = fcntl(fd, F_GETFD, 0);
	// on retourne immédiatement en cas d'échec
  if (oldflags < 0)
    return oldflags;

	oldflags |= FD_CLOEXEC;
  return fcntl(fd, F_SETFD, oldflags);
}

void saturnd_loop(char* request_pipe_path, char* answer_pipe_path, char* tasks_dir) {

	// ouverture des tubes
	// request_pipe est ouvert en lecture-écriture pour éviter que 
	// poll() retourne immédiatement. il ne faut pas écrire dedans
	int request_pipe = open(request_pipe_path, O_RDWR);
	if (request_pipe < 0) {
		perror("open request pipe");
		exit(1);
	}
	// on s'assure que request_pipe soit fermé lors d'un exec
	if (set_cloexec_flag(request_pipe) < 0) {
		perror("fcntl");
		exit(1);
	}

	tasklist* tasklist = tasklist_create();
	time_t last_exec = 0;
	time_t now;

	// taskNb définit l'id des nouvelles taches. n'est jamais décrémenté, même quand une tache est supprimée
	// ici on lit les taches déjà présentes sur le disque
	uint64_t taskNb = tasklist_readTasksInDir(tasklist, tasks_dir);

	// Contenu possible des requetes
	uint16_t opcode = 0;

	// pour poll request_pipe
	struct pollfd pfd;
	pfd.fd = request_pipe;
	pfd.events = POLLIN;

	while(1) {
		now = time(0);
		if ( now - last_exec >= 60 ) {
			tasklist_execute(tasklist, tasks_dir);
			last_exec = now;
		}

		// on attend une requete pendant 1s
		poll(&pfd, 1, 1000);
		
		// on ne lis et répond à la requête que s'il y en a effectivement une
		if (pfd.revents & POLLIN) {
			read(request_pipe, &opcode, sizeof(uint16_t));
			opcode = be16toh(opcode);

			// ouverture du tube de réponse
			int answer_pipe = open(answer_pipe_path, O_WRONLY);
			if (answer_pipe < 0) perror("open answer pipe");

			switch (opcode) {
				case CLIENT_REQUEST_LIST_TASKS: {
					uint16_t rep = htobe16(SERVER_REPLY_OK);
					uint32_t nbTasks = htobe32(tasklist_length(tasklist));

					string_p ans = string_createln(&rep, sizeof(rep));
					string_p s1 = string_createln(&nbTasks, sizeof(nbTasks));
					string_p s2 = tasklist_toString(tasklist);

					string_concat(ans, s1);
					string_concat(ans, s2);

					write(answer_pipe, ans->chars, ans->length);

					string_free(ans);
					string_free(s1);
					string_free(s2);
					break;
				}
					
				case CLIENT_REQUEST_CREATE_TASK: {
					
					timing *time = calloc(1, sizeof(timing));
					uint32_t argc;
					commandline* cmdl = malloc(sizeof(commandline));

					read(request_pipe, &time->minutes, sizeof(uint64_t));
					read(request_pipe, &time->hours, sizeof(uint32_t));
					read(request_pipe, &time->daysofweek, sizeof(uint8_t));
					read(request_pipe, &argc, sizeof(uint32_t));

					time->minutes = be64toh(time->minutes);
					time->hours = be32toh(time->hours);
					argc = be32toh(argc);

					cmdl->ARGC = argc;
					cmdl->ARGVs = malloc(sizeof(string_p)*argc);

					for (uint32_t i = 0; i < argc; i++) {

						uint32_t len;
						read(request_pipe, &len, sizeof(uint32_t));
						len = be32toh(len);
						char *lilbuf = (char *)malloc(len);
						read(request_pipe, lilbuf, len);

						cmdl->ARGVs[i] = string_createln(lilbuf, len);

						free(lilbuf);
					}

					task *newTask = task_create(taskNb, cmdl, time);
					tasklist_addTask(tasklist, newTask);

					task_createFiles(newTask, tasks_dir);

					uint16_t rep = htobe16(SERVER_REPLY_OK);
					uint64_t taskId = htobe64(taskNb);

					string_p ans = string_createln(&rep, sizeof(rep));
					string_p s = string_createln(&taskId, sizeof(taskId));

					string_concat(ans, s);

					write(answer_pipe, ans->chars, ans->length);

					string_free(ans);
					string_free(s);

					taskNb++;

					break;
				}
				case CLIENT_REQUEST_REMOVE_TASK: {
					uint64_t id;
					read(request_pipe, &id, sizeof(uint64_t));
					id = be64toh(id);
					int found = takslist_remove(tasklist, id, tasks_dir);

					if (found) {
						uint16_t rep = htobe16(SERVER_REPLY_OK);
						write(answer_pipe, &rep, sizeof(uint16_t));
					} else {
						write_error_not_found(answer_pipe);
					}
					break;
				}
				case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES: {
					uint64_t id;
					read(request_pipe, &id, sizeof(uint64_t));
					id = be64toh(id);
					char* path = calloc(strlen(tasks_dir) + 64, 1);
					sprintf(path, "%s/%lu/return_values", tasks_dir, id);
					string_p output = string_readFromFile(path);
					if (output == NULL) {
						write_error_not_found(answer_pipe);
					} else {
						uint16_t rep = htobe16(SERVER_REPLY_OK);
						uint32_t nbexec = tasklist_getNbExec(tasklist, id);
						nbexec = htobe32(nbexec);
						string_p ans = string_createln(&rep, sizeof(uint16_t));
						string_p s = string_createln(&nbexec, sizeof(uint32_t));

						string_concat(ans, s);
						string_concat(ans, output);
						
						write(answer_pipe, ans->chars, ans->length);
						string_free(ans);
						string_free(output);
						string_free(s);
					}
					free(path);
					break;
				}
				case CLIENT_REQUEST_TERMINATE: {
					
					uint16_t rep = htobe16(SERVER_REPLY_OK);
					write(answer_pipe, &rep, sizeof(uint16_t));

					close(request_pipe);
					close(answer_pipe);

					tasklist_free(tasklist);

					free(request_pipe_path);
					free(answer_pipe_path);
					free(tasks_dir);
					// pas besoin de supprimer le répertoire avec les taches
					printf("Exiting\n");
					exit(0);
					break;
				}
				case CLIENT_REQUEST_GET_STDOUT: {
					uint64_t id;
					read(request_pipe, &id, sizeof(uint64_t));
					id = be64toh(id);
					answer_with_file_content_at_id(id, tasks_dir, "std_out", answer_pipe);
					break;
				}
				case CLIENT_REQUEST_GET_STDERR: {
					uint64_t id;
					read(request_pipe, &id, sizeof(uint64_t));
					id = be64toh(id);
					answer_with_file_content_at_id(id, tasks_dir, "std_err", answer_pipe);
					break;
				}
				default: {
					uint16_t rep = htobe16(SERVER_REPLY_ERROR);
					write(answer_pipe, &rep, sizeof(uint16_t));
					break;
				}
			}

			close(answer_pipe);
			pfd.revents = 0;
		}
	}
}
