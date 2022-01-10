#include "custom-string.h"
#include "timing.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

string_p string_create(const char *charArray) {
  int l = strlen(charArray);
  string_p str = (string_p)malloc(sizeof(string));
  str->length = l;
  str->chars = (char *)malloc(l);
  memcpy(str->chars, charArray, l);
  return str;
}

string_p string_createln(const void *buf, int length) {
  string_p str = (string_p)malloc(sizeof(string));
  str->length = length;
  str->chars = (char *)malloc(length);
  memcpy(str->chars, buf, length);
  return str;
}

// concatène plein de strings en une string
// la longueur des strings est écrit dans le
// string retourné
// s'assure que les tailles sont en big-endian
string_p string_concatStringsKeepLength(string_p *strArray, int nb) {

  string_p s = (string_p)malloc(sizeof(string));
  s->length = 0;
  int intsize = sizeof(uint32_t);

  for (int i = 0; i < nb; i++) {
    s->length += strArray[i]->length + intsize;
  }

  s->chars = (char *)malloc(s->length);

  int len = 0;
  for (int i = 0; i < nb; i++) {
    uint32_t be_size = htobe32(strArray[i]->length);
    memcpy(s->chars + len, &be_size, intsize);
    len += intsize;
    memcpy(s->chars + len, strArray[i]->chars, strArray[i]->length);
    len += strArray[i]->length;
  }

  return s;
}

void string_concat(string_p s1, string_p s2) {
  char *tmp = s1->chars;
  int oldlen = s1->length;

  s1->length = s1->length + s2->length;
  s1->chars = malloc(s1->length);

  memmove(s1->chars, tmp, oldlen);
  memmove(s1->chars + oldlen, s2->chars, s2->length);

  free(tmp);
}

void string_addChar(string_p s, char c) {
  char *tmp = s->chars;

  s->length = s->length + 1;
  s->chars = malloc(s->length);

  memmove(s->chars, tmp, s->length - 1);
  memmove(s->chars + s->length - 1, &c, 1);

  free(tmp);
}

void string_free(string_p s) {
  free(s->chars);
  free(s);
}

void string_print(string_p s) { write(STDOUT_FILENO, s->chars, s->length); }

void string_println(string_p s) {
  string_print(s);
  write(STDOUT_FILENO, "\n", 1);
}

// return NULL if file not found
string_p string_readFromFile(const char* path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    return NULL;
  }
  char buf[1024];
  int ssize;
  string_p str = string_create("");

  while((ssize = read(fd, buf, 1024))) {
    string_p str2 = string_createln(buf, ssize);
    string_concat(str, str2);
    string_free(str2);
  }

  close(fd);
  return str;
}

/*
void string_trimStart(string_p s, int n) {
  if (n <= 0) return;
  if (s->length - n < 1) {
    free(s->chars);
    s->chars = malloc(0);
    s->length = 0;
  } 
  else {
    char *tmp = s->chars;

    s->chars = malloc(s->length - n);
    s->length = s->length - n;
    memmove(s, tmp + n, s->length);
    free(tmp);
  }
}
*/
