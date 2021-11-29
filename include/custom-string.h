#ifndef CUSTOM_STRING_H
#define CUSTOM_STRING_H

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "timing.h"

typedef struct {
  uint32_t length;
  char* chars;
} string;

typedef string* string_p;

string_p string_create(const char* charArray);

// concatène plein de strings en une string
// la longueur des strings est écrit dans le
// string retourné
// s'assure que les tailles sont en big-endian
string_p string_concatStringsKeepLength(string_p* strArray, int nb);

void string_free(string_p s);

void string_print(string_p s);
void string_println(string_p s);

#endif