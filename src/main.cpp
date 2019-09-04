#define DEBUG_NTPClient 1
#include "NTPClient.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <sun.h>
#include <wifi.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 0);

#define LED 2  //On board LED
#define RELAY 0  //On board LED

String WIFI_SSID("Wawrzyniec");
String WIFI_PASS("12345678");
String HOSTNAME("esp");

void scheduler();
void everySecond();
void everyMinute();
void everyHour();
void everyDay();

uint8_t initDone=0;
uint8_t timeClientUpdate=1;

SunTime sunTime_;

geoposition WARSZAWA = {.lat = 52.2298, .lng = 21.0118};

void setup()
{
	Serial.begin( 74880 );

	pinMode(LED, OUTPUT);
	pinMode(RELAY, OUTPUT);

	initializeWiFi(WIFI_SSID, WIFI_PASS, HOSTNAME,WIFI_STA);
	Serial.println("Starting http server");
	initializeHTTPServer();
	setTimeClient(&timeClient);
	setSunTime(&sunTime_);
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
	Serial.printf("%s %lu %d %d %d",timeClient.getIsoDateTime().c_str(), millis(), timeClient.getMinuteOfDay(), sunTime_.sunrise, sunTime_.sunset);
	sunTime_.currentTime = timeClient.getMinuteOfDay();
	if(sunTime_.currentTime > sunTime_.sunrise && sunTime_.currentTime < sunTime_.sunset){
		digitalWrite(LED,HIGH);
		digitalWrite(RELAY,HIGH);
		Serial.println("OFF");
		sunTime_.state=0;
	}else{
		digitalWrite(LED,LOW);
		digitalWrite(RELAY,LOW);
		Serial.println("ON");
		sunTime_.state=1;
	}
}

void everyMinute(){
	Serial.printf("Wschod slonca o: %02d:%02d\n", sunTime_.sunrise/60, sunTime_.sunrise%60);
	Serial.printf("Zachod slonca o: %02d:%02d\n", sunTime_.sunset/60, sunTime_.sunset%60);
}

void everyHour(){
	timeClientUpdate=1;
}

void everyDay(){
	Serial.println("daily");
	struct tm ts = timeClient.getTime();
	sunTime_.sunrise = calculateSunrise(ts, WARSZAWA, SUN_SET_OR_RISE, SUNRISE);
	sunTime_.sunset = calculateSunrise(ts, WARSZAWA, SUN_SET_OR_RISE, SUNSET);

}

void loop()
{
	if(timeClientUpdate==1){

		timeClient.update();//update does not work when executed from timer
		timeClientUpdate=0;
	}
	handleClient();
}

