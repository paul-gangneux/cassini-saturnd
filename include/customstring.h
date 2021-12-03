#ifndef CUSTOMSTRING_H
#define CUSTOMSTRING_H

#include "timing.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint32_t length;
  char *chars;
} string;

string string_create(const char *str) {
  int l = strlen(str);
  string s;
  s.length = l;
  s.chars = (char *)calloc(l, 1);
  memcpy(s.chars, str, l);
  return s;
}

// concatène plein de strings en une string
// la longueur des strings est écrit dans le
// string retourné
// s'assure que les tailles sont en big-endian
string string_concatStringsKeepLength(string *strings, int nb) {
  string s;
  s.length = 0;
  int intsize = sizeof(uint32_t);
  for (int i = 0; i < nb; i++) {
    s.length += strings[i].length + intsize;
  }

  s.chars = (char *)calloc(s.length, 1);
  int len = 0;
  for (int i = 0; i < nb; i++) {
    uint32_t be_size = htobe32(strings[i].length);
    memcpy(&s.chars[len], &be_size, intsize);
    len += intsize;
    memcpy(&s.chars[len], strings[i].chars, strings[i].length);
    len += strings[i].length;
  }
  return s;
}

void string_free(string s) {
  free(s.chars);
  s.length = 0;
}

#endif