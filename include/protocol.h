#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

typedef struct {
	uint64_t MINUTES;
	uint32_t HOURS;
	uint8_t  DAYSOFWEEK;
} timing;

typedef struct {
	uint32_t ARGC;
	char**   ARGVs;
} commandline;

#endif // PROTOCOL
