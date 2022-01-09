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

}

void saturnd_loop(char* request_pipe_path, char* answer_pipe_path, char* tasks_dir) {

	// ouverture des tubes
	// request_pipe est ouvert en lecture-écriture pour éviter que 
	// poll() retourne immédiatement. il ne faut pas écrire dedans
	int request_pipe = open(request_pipe_path, O_RDWR);
	if (request_pipe < 0) perror("open request pipe");

	// définit l'id des nouvelles taches. n'est jamais décrémenté, même quand une tache est supprimée
	uint32_t taskNb = 0;

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
					uint32_t nbTasks = tasklist_length(tasklist);
					uint16_t rep = htobe16(SERVER_REPLY_OK);
					nbTasks = htobe32(nbTasks);
					write(answer_pipe, &rep, sizeof(uint16_t));
					write(answer_pipe, &nbTasks, sizeof(uint32_t));

					/* code */
					break;
				}
					
				case CLIENT_REQUEST_CREATE_TASK: {
					/* code */
					break;
				}
				case CLIENT_REQUEST_REMOVE_TASK: {
					/* code */
					break;
				}
				case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES: {
					/* code */
					break;
				}
				case CLIENT_REQUEST_TERMINATE: {
					close(request_pipe);
					uint16_t rep = htobe16(SERVER_REPLY_OK);
					write(answer_pipe, &rep, sizeof(uint16_t));
					close(answer_pipe);

					tasklist_free(tasklist);

					free(request_pipe_path);
					free(answer_pipe_path);
					free(tasks_dir);
					// TODO rm every task folder (pas forcément ?)
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
