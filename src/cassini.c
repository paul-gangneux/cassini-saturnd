#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
//#include <sys/types.h>
#include <fcntl.h>


#include "../include/cassini.h"
#include "../include/timing-text-io.h"

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

  // definition du chemin vers le tube
  char* pipe_basename;
  if (pipes_directory[strlen(pipes_directory) - 1]=='/') pipe_basename = "saturnd-request-pipe";
  else pipe_basename = "/saturnd-request-pipe";

  char* pipe_path = (char*)calloc(strlen(pipe_basename) + strlen(pipes_directory) + 1, sizeof(char));
  memcpy(pipe_path, pipes_directory, strlen(pipes_directory));
  strcat(pipe_path, pipe_basename);

  // ouverture du tube
  int pipe_fd = open(pipe_path, O_WRONLY | O_NONBLOCK);
  if (pipe_fd < 0) {
    perror("open");
    goto error;
  }

  printf("aaa\n");

  // décide quoi envoyer au serveur en fonction de opcode
  if (operation == CLIENT_REQUEST_CREATE_TASK) {

    cli_request_create req;
    req.OPCODE = operation;
    timing_from_strings(&req.TIMING, minutes_str, hours_str, daysofweek_str);
    req.COMMANDLINE.ARGC=0; // TODO
    req.COMMANDLINE.ARGVs=NULL; //TODO

    write(pipe_fd, &req, sizeof(req)); // éctirure dans pipe

  } else if (operation == CLIENT_REQUEST_LIST_TASKS || operation == CLIENT_REQUEST_TERMINATE) {

    cli_request_simple req;
    req.OPCODE = operation;

    write(pipe_fd, &req, sizeof(req)); // éctirure dans pipe
    
  } else {

    cli_request_task req;
    req.OPCODE = operation;
    req.TASKID = taskid;

    write(pipe_fd, &req, sizeof(req)); // éctirure dans pipe
  }
  

  // TODO : attendre la réponse du serveur
  return EXIT_SUCCESS;

 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  free(pipe_path);
  close(pipe_fd);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}

