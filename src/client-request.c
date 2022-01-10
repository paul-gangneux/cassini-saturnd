#include "client-request.h"

void commandline_free(commandline *cmdl) {
  for (uint32_t i = 0; i < cmdl->ARGC; i++) {
    string_free(cmdl->ARGVs[i]);
  }
  free(cmdl->ARGVs);
  free(cmdl);
}

string_p commandline_toString(commandline *cmdl) {
  int n = cmdl->ARGC;
  string_p s = string_create("");
  string_p space = string_create(" ");
  for (int i = 0; i < n-1; i++) {
    string_concat(s, cmdl->ARGVs[i]);
    string_concat(s, space);
  }
  if (n>0) string_concat(s, cmdl->ARGVs[n-1]);
  string_free(space);

  return s;
}

commandline *commandline_charsToCommandline(char *str) {
  commandline *cmld = malloc(sizeof(commandline));
  int argc = 1;
  // on ignore les espaces consécutifs
  int lastWasSpace=0;
  int inQuotes=0;
  int inDQuotes=0;

  for (uint32_t i = 0; i < strlen(str); i++) {
    if (str[i] == ' ' && !(lastWasSpace) && !(inQuotes) && !(inDQuotes)) {
      argc++;
      lastWasSpace = 1;
    } 
    else if (str[i] == '\'' && i>0 && str[i-1] != '\\') inQuotes = !inQuotes;
    else if (str[i] == '\"' && i>0 && str[i-1] != '\\') inDQuotes = !inDQuotes;
    else if (str[i] != ' ') lastWasSpace = 0;
  }

  cmld->ARGC = argc;
  cmld->ARGVs = malloc(argc * sizeof(string_p));

  string_p s = string_create("");

  int n=0;
  lastWasSpace=0;
  inQuotes=0;
  inDQuotes=0;
  for (uint32_t i = 0; i < strlen(str); i++) {
    if (str[i] == ' ' && !(lastWasSpace) && !(inQuotes) && !(inDQuotes)) {
      argc++;
      lastWasSpace = 1;
      cmld->ARGVs[n] = s;
      n++;
      s = string_create("");
    } 
    else {
      if (str[i] == '\'' && i>0 && str[i-1] != '\\') inQuotes = !inQuotes;
      else if (str[i] == '\"' && i>0 && str[i-1] != '\\') inDQuotes = !inDQuotes;
      if (str[i] != ' ') lastWasSpace = 0;
      if (!lastWasSpace) string_addChar(s, str[i]);
    }
  }
  cmld->ARGVs[n] = s;
  
  return cmld;
}


