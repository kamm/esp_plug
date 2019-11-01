#pragma once

#include <time.h>
#include <math.h>
#include <c_types.h>

#define SUN_SET_OR_RISE -.83
#define CIVIL_DAWN -6
#define NAUTICAL_DAWN -12
#define ASTRONOMICAL_DAWN -18

#define SUNRISE 0
#define SUNSET 1

#define PI 3.1415926535897932384626433832795

typedef struct geoposition{
	float lat;
	float lng;
} geoposition;

uint16_t calculateSunrise(struct tm,geoposition, float, int);
