#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
//#include <sys/types.h>
#include <fcntl.h>

#include "customstring.h"
#include "cassini.h"
#include "timing-text-io.h"

const char usage_info[] = "\
   usage: cassini [OPTIONS] -l -> list all tasks\n\
      or: cassini [OPTIONS]    -> same\n\
      or: cassini [OPTIONS] -q -> terminate the daemon\n\
      or: cassini [OPTIONS] -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]\n\
          -> add a new task and print its TASKID\n\
             format & semantics of the \"timing\" fields defined here:\n\
             https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html\n\
             default value for each field is \"*\"\n\
      or: cassini [OPTIONS] -r TASKID -> remove a task\n\
      or: cassini [OPTIONS] -x TASKID -> get info (time + exit code) on all the past runs of a task\n\
      or: cassini [OPTIONS] -o TASKID -> get the standard output of the last run of a task\n\
      or: cassini [OPTIONS] -e TASKID -> get the standard error\n\
      or: cassini -h -> display this message\n\
\n\
   options:\n\
     -p PIPES_DIR -> look for the pipes in PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes)\n\
";

commandline commandlineFromArgs(int argc, char * argv[]) {
  commandline cmdl;
  // on suppose que la commande à executer se trouve à la fin 
  int i = 1; // indice du début de la commande
  while (i < argc) {
    if (argv[i][0]=='-') {
      if (argv[i][1]=='c') i++;
      else if (strlen(argv[i])>2) i++; // c'est vite fait mal fait mais ça fonctionne
      else i+=2;
    }
    else break;
  }
  cmdl.ARGC = argc-i;
  cmdl.ARGVs = (string*) malloc(cmdl.ARGC*sizeof(string));
  for (unsigned int j=0; j<cmdl.ARGC; j++) {
    cmdl.ARGVs[j] = string_create(argv[i+j]);
  }
  return cmdl;
}

int main(int argc, char * argv[]) {
  errno = 0;
  
  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = NULL; // TODO : valeur par défaut : /tmp/<USERNAME>/saturnd/pipes
  
  uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
  uint64_t taskid;
  
  int opt;
  char * strtoull_endp;
  while ((opt = getopt(argc, argv, "hlcqm:H:d:p:r:x:o:e:")) != -1) {
    switch (opt) {
    case 'm':
      minutes_str = optarg;
      break;
    case 'H':
      hours_str = optarg;
      break;
    case 'd':
      daysofweek_str = optarg;
      break;
    case 'p':
      pipes_directory = strdup(optarg);
      if (pipes_directory == NULL) goto error;
      break;
    case 'l':
      operation = CLIENT_REQUEST_LIST_TASKS;
      break;
    case 'c':
      operation = CLIENT_REQUEST_CREATE_TASK;
      break;
    case 'q':
      operation = CLIENT_REQUEST_TERMINATE;
      break;
    case 'r':
      operation = CLIENT_REQUEST_REMOVE_TASK;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'x':
      operation = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'o':
      operation = CLIENT_REQUEST_GET_STDOUT;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'e':
      operation = CLIENT_REQUEST_GET_STDERR;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'h':
      printf("%s", usage_info);
      return 0;
    case '?':
      fprintf(stderr, "%s", usage_info);
      goto error;
    }
  }

  int size;
  char* buf;

  // décide quoi envoyer au serveur en fonction de opcode
  // cas création d'une tache 
  if (operation == CLIENT_REQUEST_CREATE_TASK) {
    //printf("aaa\n");
    cli_request_create req;
    req.OPCODE = htobe16(operation);
    timing_from_strings(&req.TIMING, minutes_str, hours_str, daysofweek_str);
    req.COMMANDLINE = commandlineFromArgs(argc, argv);

    //string test[4] = {string_create("this"),string_create("is"),string_create("a"),string_create("test")};
    string cmdl = string_concatStringsKeepLength(req.COMMANDLINE.ARGVs, req.COMMANDLINE.ARGC);

    size = sizeof(req.OPCODE)+sizeof(req.TIMING)+sizeof(req.COMMANDLINE.ARGC)+cmdl.length;

    buf = (char*) calloc(size, 1);

    //  conversion en big-endian ici
    //req.TIMING.daysofweek=htobe16(req.TIMING.daysofweek);
    req.TIMING.hours=htobe32(req.TIMING.hours);
    req.TIMING.minutes=htobe64(req.TIMING.minutes);

    int len = 0;

    int localsize = sizeof(req.OPCODE);
    memcpy(&buf, &(req.OPCODE), localsize);
    len+=localsize;

    localsize = sizeof(req.TIMING);
    memcpy(&buf+len, &(req.TIMING), localsize); 
    len+=localsize;

    localsize = cmdl.length;
    memcpy(&buf[len], &cmdl.chars, localsize);
    
    commandline_free(req.COMMANDLINE);
    string_free(cmdl);

    write(STDOUT_FILENO, &buf, size);
    write(STDOUT_FILENO, "\n", 1);

  }
  // cas où la requête est juste l'opcode
  else if (operation == CLIENT_REQUEST_LIST_TASKS || operation == CLIENT_REQUEST_TERMINATE) {

    cli_request_simple req;
    req.OPCODE = htobe16(operation);
    
    size = sizeof(req);
    buf = (char*) malloc(size);
    //buf[size] = '\0';
    memcpy(&buf, &req, sizeof(req));

  } 
  // cas où la requete est opcode + taskid
  else {

    cli_request_task req;
    req.OPCODE = htobe16(operation);
    req.TASKID = htobe64(taskid);

    size = sizeof(req);
    buf = malloc(size);
    memcpy(&buf, &req, sizeof(req)); 
  }
  ////printf("buf: %s\n", buf);
  // definition du chemin vers le tube
  char* pipe_basename;
  if (pipes_directory[strlen(pipes_directory) - 1]=='/') pipe_basename = "saturnd-request-pipe";
  else pipe_basename = "/saturnd-request-pipe";

  char* pipe_path = (char*)calloc(strlen(pipe_basename) + strlen(pipes_directory) + 1, sizeof(char));
  memcpy(pipe_path, pipes_directory, strlen(pipes_directory));
  strcat(pipe_path, pipe_basename);

  ////printf("buf: %s\n", buf);

  // ouverture du tube
  int pipe_fd = open(pipe_path, O_WRONLY);
  if (pipe_fd < 0) {
    perror("open");
    goto error;
  }

  write(pipe_fd, &buf, size); // écriture dans le tube

  // TODO : attendre la réponse du serveur
  return EXIT_SUCCESS;

 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  free(pipe_path);
  free(buf);
  close(pipe_fd);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}

