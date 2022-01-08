#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>

#include "saturnd.h"

int main() {
  // TODO parsing arguments
  char* request_tube_path = "/tmp/m242/saturnd/pipes/saturnd-request-pipe";
  char* answer_tube_path = "/tmp/m242/saturnd/pipes/saturnd-reply-pipe";

  // La boucle principale est dans un double fork, et est ainsi adoptee
  // par init lors de la fin du processus principal
  if (fork() == 0) {
    if(fork() == 0) {
      saturnd_loop(request_tube_path, answer_tube_path);
    }
  }
}

void saturnd_loop(char* rtp, char* atp) {
	int request_pipe = open(rtp, O_RDONLY | O_CREAT, S_IRWXU);
	int answer_pipe = open(atp, O_WRONLY | O_CREAT, S_IRWXU);

	task* tasklist; // TODO lire les taches existantes
	// TODO liste chainee plus adaptee pour liste des taches ?

	// Contenu possible des requetes
	uint16_t opcode = 0;
	while(1) {
		// TODO execution des taches

		// TODO gestion de toutes les requetes
		read(request_pipe, &opcode, sizeof(uint16_t));

		if (opcode == CLIENT_REQUEST_LIST_TASKS) {}
		if (opcode == CLIENT_REQUEST_CREATE_TASK) {}
		if (opcode == CLIENT_REQUEST_REMOVE_TASK) {}
		if (opcode == CLIENT_REQUEST_GET_TIMES_AND_EXITCODES) {}

		if (opcode == CLIENT_REQUEST_TERMINATE) {
			close(request_pipe);
			// TODO answer
			close(answer_pipe);
			// TODO rm every task folder
			// TODO free tasklist
			printf("tada");
			exit(0);
		}

		if (opcode == CLIENT_REQUEST_GET_STDOUT) {}
		if (opcode == CLIENT_REQUEST_GET_STDERR) {}
	}
}
