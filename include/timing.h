#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>
#include <time.h>

struct timing {
  uint64_t minutes;
  uint32_t hours;
  uint8_t daysofweek;
};

typedef struct timing timing;

int is_it_my_time(timing *t);
uint64_t timing_field_from_int(uint64_t *dest, int i, unsigned int min, unsigned int max);

#endif // TIMING_H
