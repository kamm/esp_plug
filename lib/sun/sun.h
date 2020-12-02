#ifndef SUN_H
#define SUN_H

#include <time.h>
#include <math.h>
#include <c_types.h>

#define SUNSET -.83
#define SUNRISE -.83
#define CIVIL_DAWN -6
#define CIVIL_DUSK -6
#define NAUTICAL_DAWN -12
#define NAUTICAL_DUSK -12
#define ASTRONOMICAL_DAWN -18
#define ASTRONOMICAL_DUSK -18

#define RISE 0
#define SET 1

#define PI 3.1415926535897932384626433832795

typedef struct geoposition{
	float lat;
	float lng;
} geoposition;

uint16_t calculateSunrise(struct tm,geoposition, float, int);
#endif
