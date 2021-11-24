#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

struct timing {
  uint64_t minutes;
  uint32_t hours;
  uint8_t daysofweek;
};

typedef struct timing timing;

#endif // TIMING_H
