#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>

#include "saturnd.h"

int main() {
	// TODO parsing arguments

	char *pipes_directory;
	char *username = getenv("USER");
	char *buf = calloc(512, 1);

  if (username != NULL) {
    int u = strlen(username);
    pipes_directory = calloc(20 + u, 1);
    sprintf(pipes_directory, "/tmp/%s/saturnd/pipes", username);
		// TODO : garder /tmp/[USERNAME]/saturnd/ en mémoire dans une variable globale, pourra être utile
  } else {
		printf("Cannot get environment variable USER");
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

	mkfifo(req_pipe_path, 0666);
	mkfifo(ans_pipe_path, 0666);

	free(pipes_directory);
	free(buf);

	// La boucle principale est dans un double fork, et est ainsi adoptee
	// par init lors de la fin du processus principal
	if (fork() == 0) {
		if(fork() == 0) {
			saturnd_loop(req_pipe_path, ans_pipe_path);
		}
	}
}

void saturnd_loop(char* rtp, char* atp) {
	int request_pipe = open(rtp, O_RDONLY);
	int answer_pipe = open(atp, O_WRONLY);

	task* tasklist; // TODO lire les taches existantes
	// TODO liste chainee plus adaptee pour liste des taches ?

	// Contenu possible des requetes
	uint16_t opcode = 0;
	while(1) {
		// TODO execution des taches

		// TODO gestion de toutes les requetes
		read(request_pipe, &opcode, sizeof(uint16_t));
		opcode = be16toh(opcode);

		switch (opcode) {
			case CLIENT_REQUEST_LIST_TASKS:
				/* code */
				break;
			case CLIENT_REQUEST_CREATE_TASK:
				/* code */
				break;
			case CLIENT_REQUEST_REMOVE_TASK:
				/* code */
				break;
			case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
				/* code */
				break;
			case CLIENT_REQUEST_TERMINATE:
				close(request_pipe);
				uint16_t rep = htobe16(SERVER_REPLY_OK);
				write(answer_pipe, &rep, sizeof(uint16_t));
				close(answer_pipe);

				// TODO rm every task folder
				// TODO free tasklist
				printf("Exiting");
				exit(0);
				break;
			case CLIENT_REQUEST_GET_STDOUT:
				/* code */
				break;
			case CLIENT_REQUEST_GET_STDERR:
				/* code */
				break;
			default:
				break;
		}
	}
}
