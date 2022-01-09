#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <poll.h>

#include "saturnd.h"

int main() {
	// TODO parsing arguments

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
	if (fork() == 0) {
		if(fork() == 0) {
			saturnd_loop(req_pipe_path, ans_pipe_path, tasks_directory);
		}
	}

	free(req_pipe_path);
	free(ans_pipe_path);
	free(tasks_directory);
}

void saturnd_loop(char* request_pipe_path, char* answer_pipe_path, char* tasks_dir) {

	// ouverture des tubes
	// request_pipe est ouvert en lecture-écriture pour éviter que 
	// poll() retourne immédiatement. il ne faut pas écrire dedans
	int request_pipe = open(request_pipe_path, O_RDWR);
	if (request_pipe < 0) perror("open request pipe");

	// définit l'id des nouvelles taches. n'est jamais décrémenté, même quand une tache est supprimée
	uint64_t taskNb = 0;

	tasklist* tasklist = tasklist_create(); 
	// TODO lire les taches existantes (et adapter taskNb en conséquence)

	// Contenu possible des requetes
	uint16_t opcode = 0;

	// pour poll request_pipe
	struct pollfd pfd;
	pfd.fd = request_pipe;
	pfd.events = POLLIN;

	while(1) {
		// TODO execution des taches

		// on attend une requete pendant 500ms
		poll(&pfd, 1, 500);
		
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
					
					timing time;
					uint32_t argc;
					commandline* cmdl = malloc(sizeof(commandline));

					read(request_pipe, &time.minutes, sizeof(uint64_t));
					read(request_pipe, &time.hours, sizeof(uint32_t));
					read(request_pipe, &time.daysofweek, sizeof(uint8_t));
					read(request_pipe, &argc, sizeof(uint32_t));

					time.minutes = be64toh(time.minutes);
					time.hours = be32toh(time.hours);
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
					//TODO : créer repertoire et fichiers

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
					int found = takslist_remove(tasklist, id);

					if (found) {
						//TODO : supprimer fichiers et repertoires
						uint16_t rep = htobe16(SERVER_REPLY_OK);
						write(answer_pipe, &rep, sizeof(uint16_t));
					} else {
						uint16_t rep0 = htobe16(SERVER_REPLY_ERROR);
						uint16_t rep1 = htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
						uint16_t rep[2];
						rep[0] = rep0; rep[1] = rep1;
						write(answer_pipe, rep, sizeof(uint32_t));
					}
					break;
				}
				case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES: {
					/* code */
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
					// pas besoin de supprimer le répertoire avec les taches :
					// il est demandé que relancer saturnd permette de reprendre
					// les taches existantes.
					printf("Exiting\n");
					exit(0);
					break;
				}
				case CLIENT_REQUEST_GET_STDOUT: {
					/* code */
					break;
				}
				case CLIENT_REQUEST_GET_STDERR: {
					/* code */
					break;
				}
				default:
					break;
			}

			close(answer_pipe);
			pfd.revents = 0;
		}
	}
}
