#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <stdint.h>
#include "custom-string.h"

typedef struct {
  uint32_t ARGC;
  string_p *ARGVs;
} commandline;

void commandline_free(commandline *cmdl);
string_p commandline_toString(commandline *cmdl);

// parse une chaine de caractère et renvoie la ligne de 
// commande correspondante
commandline *commandline_charsToCommandline(char *str);

// transforme la ligne de commande complète de cassini
// en la ligne de commande que doit exécuter saturnd
commandline *commandlineFromArgs(int argc, char *argv[]);

#endif