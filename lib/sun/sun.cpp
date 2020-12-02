#include "sun.h"

uint16_t calculateSunrise(struct tm ts,geoposition geo, float position, int direction) {
    //2. convert the longitude to hour value and calculate an approximate time
    float lngHour = geo.lng / 15.0;
    float t = ts.tm_yday + 1 + ((6 - lngHour) / 24);

    //3. calculate the Sun's mean anomaly
    float M = (0.9856 * t) - 3.289;

    //4. calculate the Sun's true longitude
    float L = fmod(M + (1.916 * sin((PI/180)*M)) + (0.020 * sin(2 *(PI/180) * M)) + 282.634,360.0);

    //5a. calculate the Sun's right ascension
    float RA = fmod(180/PI*atan(0.91764 * tan((PI/180)*L)),360.0);

    //5b. right ascension value needs to be in the same quadrant as L
    float Lquadrant  = floor( L/90) * 90;
    float RAquadrant = floor(RA/90) * 90;
    RA = RA + (Lquadrant - RAquadrant);

    //5c. right ascension value needs to be converted into hours
    RA = RA / 15;

    //6. calculate the Sun's declination
    float sinDec = 0.39782 * sin((PI/180)*L);
    float cosDec = cos(asin(sinDec));

    //7a. calculate the Sun's local hour angle
    float cosH = (sin((PI/180)*position) - (sinDec * sin((PI/180)*geo.lat))) / (cosDec * cos((PI/180)*geo.lat));
    /*
    if (cosH >  1)
    the sun never rises on this location (on the specified date)
    if (cosH < -1)
    the sun never sets on this location (on the specified date)
    */

    //7b. finish calculating H and convert into hours
    float H=0;
    if(direction == RISE)
    {
    	H = 360 - (180/PI)*acos(cosH);   //   if if rising time is desired:
    }
    else if(direction == SET)
    {
	H = 360 + (180/PI)*acos(cosH); //   if setting time is desired:
    }
    H = H / 15;

    //8. calculate local mean time of rising/setting
    float T = H + RA - (0.06571 * t) - 6.622;

    //9. adjust back to UTC
    float UT = fmod(T - lngHour,24.0);

    //10. convert UT value to local time zone of latitude/longitude
    return (uint16_t)lroundf(UT*60);
}
