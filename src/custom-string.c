#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "timing.h"
#include "custom-string.h"

string_p string_create(const char* charArray) {
    int l = strlen(charArray);
    string_p str = (string_p) malloc(sizeof(string));
    str->length=l;
    str->chars = (char*)malloc(l);
    memcpy(str->chars, charArray, l);
    return str;
}

// concatène plein de strings en une string
// la longueur des strings est écrit dans le
// string retourné
// s'assure que les tailles sont en big-endian
string_p string_concatStringsKeepLength(string_p* strArray, int nb) {

  string_p s = (string_p) malloc(sizeof(string));
  s->length=0;
  int intsize = sizeof(uint32_t);

  for (int i=0; i<nb; i++) {
    s->length+=strArray[i]->length+intsize;
  }
  
  s->chars=(char*) malloc(s->length);

  // je crois qu'il y a un big ici mais je sais pas où exactement
  int len=0;
  for (int i=0; i<nb; i++) {
    uint32_t be_size = htobe32(strArray[i]->length);
    memcpy(&s->chars[len], (char*)&be_size, intsize);
    len += intsize;
    memcpy(&s->chars[len], strArray[i]->chars, strArray[i]->length);
    len += strArray[i]->length;
  }

  return s;
}

void string_free(string_p s) {
  free(s->chars);
  free(s);
}

void string_print(string_p s) {
  write(STDOUT_FILENO, s->chars, s->length);
}

void string_println(string_p s) {
  string_print(s);
  write(STDOUT_FILENO, "\n", 1);
}