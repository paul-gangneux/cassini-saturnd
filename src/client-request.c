#include "client-request.h"

void commandline_free(commandline *cmdl) {
  for (uint32_t i = 0; i < cmdl->ARGC; i++) {
    string_free(cmdl->ARGVs[i]);
  }
  free(cmdl->ARGVs);
  free(cmdl);
}

/*
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
*/


