#include "timing-text-io.h"
#include "timing.h"

uint64_t timing_field_from_int(uint64_t *dest, int i, unsigned int min, unsigned int max) {
  char str[3 * sizeof(char)];
  sprintf(str, "%d", i);
  return timing_field_from_string(dest, str, min, max);
}

// revoie 1 si la minute actuelle correspond au timming en argument, 0 sinon
int is_it_my_time (timing *exeTiming) {
	time_t timer = time(NULL);
	struct tm* realTime = localtime(&timer);
	uint64_t buff;

	// vérifie d'abord le jour de la semaine
	timing_field_from_int(&buff, realTime->tm_wday, 0, 6);
	if ((exeTiming-> daysofweek) & ((uint8_t)buff)) {  // S'il y a un 1 ici, c'est qu'on est le bon jour (normalement)
		//Verifions ensuite l'heure
		timing_field_from_int(&buff, realTime->tm_hour, 0, 23);
		if ((exeTiming-> hours) & ((uint32_t)buff)){  // S'il y a un 1 ici, c'est qu'on est la bonne heure (normalement)
			//Verifions ensuite la minute
			timing_field_from_int(&buff, realTime->tm_min, 0, 59);
			if ((exeTiming->minutes) & ((uint64_t)buff)){  //S'il y a un 1 ici, c'est qu'on est la bonne minute (normalement)
				return 1;
			}
		}
	}
	return 0;
}