#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
//#include <sys/types.h>
#include <fcntl.h>
#include <inttypes.h>

#include "custom-string.h"
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

commandline* commandlineFromArgs(int argc, char * argv[]) {
  commandline* cmdl = (commandline*) malloc(sizeof(commandline));
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
  cmdl->ARGC = argc-i;
  cmdl->ARGVs = (string_p*) malloc(cmdl->ARGC*sizeof(string_p));
  for (unsigned int j=0; j<cmdl->ARGC; j++) {
    cmdl->ARGVs[j] = string_create(argv[i+j]);
  }
  return cmdl;
}

//Ouvre le tube saturnd-request-pipe en ecriture si resquest==1 et le tube saturnd-reply-pipe sinon
//renvoi -1 si erreure
int openPipe (int request, char * pipes_directory){
  char* pipe_basename;
  
  if (strlen(pipes_directory)==0) return -1;
  if (request==1){
    if (pipes_directory[strlen(pipes_directory) - 1]=='/') pipe_basename = "saturnd-request-pipe";
    else pipe_basename = "/saturnd-request-pipe";
  }else{
    if (pipes_directory[strlen(pipes_directory) - 1]=='/') pipe_basename = "saturnd-reply-pipe";
    else pipe_basename = "/saturnd-reply-pipe";
  }
  
  char* pipe_path = (char*)calloc(strlen(pipe_basename) + strlen(pipes_directory) + 1, sizeof(char));
  memcpy(pipe_path, pipes_directory, strlen(pipes_directory));
  strcat(pipe_path, pipe_basename);
  int pipe_fd = -1;

  // ouverture du tube
  if (request==1){
    pipe_fd = open(pipe_path, O_WRONLY);
  }else{
    pipe_fd = open(pipe_path, O_RDONLY);
  }
  free(pipe_path);
  return pipe_fd;
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

  if (pipes_directory==NULL) goto error;

  int size;
  char* buf;

  // décide quoi envoyer au serveur en fonction de opcode
  // cas création d'une tache 
  if (operation == CLIENT_REQUEST_CREATE_TASK) {
    
    cli_request_create_chars req;
    uint16_t opcode = htobe16(operation);
    timing tim;
    timing_from_strings(&tim, minutes_str, hours_str, daysofweek_str);
    commandline* cmdl = commandlineFromArgs(argc, argv);
    string_p cmdl_string = string_concatStringsKeepLength(cmdl->ARGVs, cmdl->ARGC);

    // conversion en big-endian ici
    uint64_t minutes = htobe64(tim.minutes);
    uint32_t hours = htobe32(tim.hours);
    uint8_t daysofweek = tim.daysofweek;
    uint32_t cmd_argc = htobe32(cmdl->ARGC);

    size = sizeof(req)+cmdl_string->length;
    buf = (char*) malloc(size);    

    memcpy(req.opcode, &opcode, sizeof(uint16_t));
    memcpy(req.min, &minutes, sizeof(uint64_t));
    memcpy(req.hours, &hours, sizeof(uint32_t));
    memcpy(req.day, &daysofweek, sizeof(uint8_t));
    memcpy(req.argc, &cmd_argc, sizeof(uint32_t));

    char* buf2 = (char*) (buf+sizeof(req));
    memcpy(buf, &req, sizeof(req));
    memcpy(buf2, cmdl_string->chars, cmdl_string->length);

    commandline_free(cmdl);
    string_free(cmdl_string);

  }
  // cas où la requête est juste l'opcode
  else if (operation == CLIENT_REQUEST_LIST_TASKS || operation == CLIENT_REQUEST_TERMINATE) {

    uint16_t opcode_be = htobe16(operation);
    
    size = sizeof(uint16_t);
    buf = (char*) malloc(size);
    memcpy(buf, &opcode_be, sizeof(uint16_t));

  } 
  // cas où la requete est opcode + taskid
  else {

    cli_request_task_chars req;
    uint16_t opcode_be = htobe16(operation);
    uint64_t taskid_be = htobe64(taskid);
    memcpy(req.opcode, &opcode_be, sizeof(uint16_t));
    memcpy(req.taskid, &taskid_be, sizeof(uint64_t));

    size = sizeof(req);
    buf = malloc(size);
    memcpy(buf, &req, sizeof(req)); 
  }
  
  // definition du chemin vers le tube
  /*
  char* pipe_basename;
  
  if (strlen(pipes_directory)==0) goto error;
  if (pipes_directory[strlen(pipes_directory) - 1]=='/') pipe_basename = "saturnd-request-pipe";
  else pipe_basename = "/saturnd-request-pipe";
  
  char* pipe_path = (char*)calloc(strlen(pipe_basename) + strlen(pipes_directory) + 1, sizeof(char));
  memcpy(pipe_path, pipes_directory, strlen(pipes_directory));
  strcat(pipe_path, pipe_basename);

  // ouverture du tube
  int pipe_fd = open(pipe_path, O_WRONLY);
  if (pipe_fd < 0) {
    perror("open");
    goto error;
  }
*/
  int pipe_fd = openPipe(1, pipes_directory);
  if (pipe_fd < 0) {
    perror("open");
    goto error;
  }
  write(pipe_fd, buf, size); // écriture dans le tube
  close(pipe_fd);

  //Ouvertrure du tube de reponse pour la lire
  int reply_pipe = openPipe(1, pipes_directory);
  if (reply_pipe < 0) {
    perror("open");
    goto error;
  }

  uint16_t* reply_buffer = malloc (sizeof(uint16_t));
  read(reply_pipe, reply_buffer, sizeof(uint16_t)); // lecture dans le tube

  // RE because big endian, faster to compare this way
  // FIXME this might segfault
/*  if (strcmp(*reply_buffer, "RE") == 0) {
    puts("got ER reply");
    goto error;
  }
  //Pas sure que ce soit le meilleure solution, les erreures ne doivent pas toujours interrompre cassini
  */


//Si on lit autre chose que ce qui est autorisé: goto error
// Case LIST
  if (operation == CLIENT_REQUEST_LIST_TASKS ){
    if (*reply_buffer == SERVER_REPLY_OK) {
      printf ("0 TODO");
      //TODO : Lire la suite pour tout afficher
    }else{ //Le cas ER n'est pas a gerer car il n'est pas sensé exister
      perror("liste");
      goto error;
    }
  }
  //Case CREATE
  else if (operation == CLIENT_REQUEST_CREATE_TASK){
    if (*reply_buffer == SERVER_REPLY_OK)  {
      printf ("0");
      uint64_t* get_task_id = malloc (sizeof(uint64_t));
      read(reply_pipe, get_task_id, sizeof(uint64_t));
      printf("%"PRIu64"\n", *get_task_id);
    }else{ //Le cas ER n'est pas a gerer car il n'est pas sensé exister
      perror("create");
      goto error;
    }
  }
  //Case REMOVE
  else if (operation == CLIENT_REQUEST_REMOVE_TASK){
    if (*reply_buffer == SERVER_REPLY_OK)  {
      printf ("0"); //Affiche 0 si reussite
    }else if (*reply_buffer == SERVER_REPLY_ERROR) {
      printf ("1"); 
      
      //TODO: verifier que NF est le prochain truc a lire
    } else{
      perror("remove");
      goto error;
    }
  }
  //Case TIMES_EXITCODE
  else if (operation == CLIENT_REQUEST_GET_TIMES_AND_EXITCODES){
    if (*reply_buffer == SERVER_REPLY_OK)  {
      printf ("0 TODO"); 
      //TODO : Afficher info (time + exit code) on all the past runs of a task
    }else if (*reply_buffer == SERVER_REPLY_ERROR) {
      printf ("1"); 
      //TODO: verifier que NF est le prochain truc a lire. meme chose que remove
    } else{
      perror("times exitcode");
      goto error;
    }
  }
    //Case TERMINATE
  else if (operation == CLIENT_REQUEST_TERMINATE){
    if (*reply_buffer == SERVER_REPLY_OK)  {
      printf ("0");
    } else{
      perror("times exitcode");
      goto error;
    }
  }
  //Case STDOUT
  else if (operation == CLIENT_REQUEST_GET_STDOUT || operation == CLIENT_REQUEST_GET_STDERR){
    if (*reply_buffer == SERVER_REPLY_OK)  {
      printf ("0");
      //TODO print la reponse 
    }else if (*reply_buffer == SERVER_REPLY_ERROR) {
      printf ("1"); 
      //TODO: Deux erreures possibles, NF et NR
    } else{
      perror("times exitcode");
      goto error;
    }
  }

  free (reply_buffer);
  free(pipes_directory);
  free(buf);
  close(pipe_fd);
  close(reply_pipe);
  return EXIT_SUCCESS;

 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  free(buf);
  close(pipe_fd);
  close(reply_pipe);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}

