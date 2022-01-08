#include "client-request.h"

void commandline_free(commandline *cmdl) {
  for (uint32_t i = 0; i < cmdl->ARGC; i++) {
    string_free(cmdl->ARGVs[i]);
  }
  free(cmdl->ARGVs);
  free(cmdl);
}


