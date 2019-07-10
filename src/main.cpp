#define DEBUG_NTPClient 1
#include "NTPClient.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <sun.h>
#include <wifi.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 0);

#define LED 2  //On board LED

String WIFI_SSID("kamm");
String WIFI_PASS("lama19ro");
String HOSTNAME("esp");

void scheduler();
void everySecond();
void everyMinute();
void everyHour();
void everyDay();

uint8_t initDone=0;
uint8_t timeClientUpdate=1;
uint16_t sunset;
uint16_t sunrise;

geoposition WARSZAWA = {.lat = 52.2298, .lng = 21.0118};

void setup()
{
	Serial.begin( 74880 );

	pinMode( LED,OUTPUT );

	initializeWiFi(WIFI_SSID, WIFI_PASS, HOSTNAME,WIFI_STA);
	initializeHTTPServer();
	setTimeClient(&timeClient);
	timeClient.begin();
	timer1_isr_init();
      	timer1_attachInterrupt(scheduler);
      	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
      	timer1_write(5000000);
}

void scheduler(){
	uint32_t i=timeClient.getEpochTime();
	if(initDone == 0){
		everyDay();
		everyMinute();
		initDone=1;
	}
	if(i%(60)==1){
		everyMinute();
	}
	if(i%(60*60)==10){
		everyHour();
	}
	if(i%(3600*24)==20){
		everyDay();
	}

	everySecond();
}

void everySecond(){
	uint16_t off = 18*24+50;
	uint16_t on = 18*24+55;
	Serial.printf("%s %lu %d %d %d",timeClient.getIsoDateTime().c_str(), millis(), timeClient.getMinuteOfDay(), off, on);
	if(timeClient.getMinuteOfDay() >= off && timeClient.getMinuteOfDay() <= on){
		digitalWrite(LED,HIGH);
		Serial.println("   ON");
	}else{
		digitalWrite(LED,LOW);
		Serial.println("   OFF");
	}
}

void everyMinute(){
	Serial.printf("Wschod slonca o: %02d:%02d\n", sunrise/60, sunrise%60);
	Serial.printf("Zachod slonca o: %02d:%02d\n", sunset/60, sunset%60);
}

void everyHour(){
	timeClientUpdate=1;
}

void everyDay(){
	Serial.println("daily");
	struct tm ts = timeClient.getTime();
	sunrise = calculateSunrise(ts, WARSZAWA, SUN_SET_OR_RISE, SUNRISE)+120;
	sunset = calculateSunrise(ts, WARSZAWA, SUN_SET_OR_RISE, SUNSET)+120;
}

void loop()
{
	if(timeClientUpdate==1){

		timeClient.update();//update does not work when executed from timer
		timeClientUpdate=0;
	}
	handleClient();
}

