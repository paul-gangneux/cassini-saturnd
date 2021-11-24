#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
//#include <fcntl.h>
#include <sys/stat.h>

/*
void forkAndExecute(int argc, char* argv[]) {

    pid_t pid = fork();

    if (pid == 0) {
        if (execvp(argv[0], argv)==-1) {
            perror("Error execv");
            // (?) envoyer message erreur dans fifo  
        }
        exit(EXIT_FAILURE);
    }
    
}
*/

int main() {
    // initialisation des répertoires
    // (?) faire chemins absolus dans un répertoire dédié ?
    mkdir("run", 0777);
    mkdir("run/pipes", 0777);
    mkdir("run/tasks", 0777);
    // initialise 2 fifos

    //while (1) {

        // reads from fifo

        

    //}
}