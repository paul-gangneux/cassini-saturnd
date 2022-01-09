#ifndef CUSTOM_STRING_H
#define CUSTOM_STRING_H

#include "timing.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint32_t length;
  char *chars;
} string;

typedef string *string_p;

string_p string_create(const char *charArray);

string_p string_createln(const void *charArray, int length);

// concatène plein de strings en une string
// la longueur des strings est écrit dans le
// string retourné
// s'assure que les tailles sont en big-endian
string_p string_concatStringsKeepLength(string_p *strArray, int nb);

void string_concat(string_p s1, string_p s2);
void string_trimStart(string_p s, int n);

void string_free(string_p s);

void string_print(string_p s);
void string_println(string_p s);

#endif
