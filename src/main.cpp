#define DEBUG_NTPClient 1
#include "NTPClient.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <sun.h>
#include <wifi.h>
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, "pool.ntp.org", 0, 0);

#define LED 2  //On board LED
#define RELAY 0  //On board LED

String WIFI_SSID("Wawrzyniec");
String WIFI_PASS("1234qwer");
String HOSTNAME("xmasplug");

void scheduler();
void everySecond();
void everyMinute();
void everyHour();
void everyDay();

uint8_t initDone=0;
uint8_t ntpClientUpdate=1;

SunTime sunTime;

geoposition WARSZAWA = {.lat = 52.2298, .lng = 21.0118};

void setup()
{
	Serial.begin( 74880 );

	pinMode(LED, OUTPUT);
	pinMode(RELAY, OUTPUT);

	initializeWiFi(WIFI_SSID, WIFI_PASS, HOSTNAME,WIFI_STA);
	Serial.println("Starting http server");
	initializeHTTPServer();
	ntpClient.begin();
	timer1_isr_init();
  timer1_attachInterrupt(scheduler);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  timer1_write(5000000);
}

void scheduler(){
	uint32_t i=ntpClient.getEpochTime();
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

void recalculateOnOffTime(){
	//sunTime.sunrise = calculateSunrise(ts, WARSZAWA, SUN_SET_OR_RISE, SUNRISE);
		//sunTime.sunset = calculateSunrise(ts, WARSZAWA, SUN_SET_OR_RISE, SUNSET);
		//sunTime.sunrise = calculateSunrise(ts, WARSZAWA, CIVIL_DAWN, SUNRISE);
		/*if(ntpClient.getDayOfMonth()==24 && ntpClient.getMonth()==11){
			//Pierwszy dzień świąt
			sunTime.sunrise = 0;//światło gaśnie o 02:00 25 grudnia
		}else{
			sunTime.sunrise = 22*60;
		}*/
		//sunTime.sunset = calculateSunrise(ts, WARSZAWA, CIVIL_DAWN, SUNSET);
	struct tm ts = ntpClient.getTime();
	sunTime.sunrise = calculateSunrise(ts, WARSZAWA, CIVIL_DAWN, RISE);
	sunTime.sunset = calculateSunrise(ts, WARSZAWA, CIVIL_DAWN, SET);
}

void everySecond(){
	Serial.printf("%s %lu %d %d %d",ntpClient.getIsoDateTime().c_str(), millis(), ntpClient.getMinuteOfDay(), sunTime.sunrise, sunTime.sunset);
	sunTime.currentTime = ntpClient.getMinuteOfDay();

	
	if(sunTime.currentTime > sunTime.sunrise && sunTime.currentTime < sunTime.sunset){
		digitalWrite(RELAY,HIGH);
		Serial.println("OFF");
		sunTime.state=0;
	}else{
		digitalWrite(RELAY,LOW);
		Serial.println("ON");
		sunTime.state=1;
	}
	if(ntpClient.getSeconds()%2){
		digitalWrite(LED,HIGH);
	}else{
		digitalWrite(LED,LOW);

	}
}

void everyMinute(){
	recalculateOnOffTime();
	Serial.printf("Wschod slonca o: %02d:%02d\n", sunTime.sunrise/60, sunTime.sunrise%60);
	Serial.printf("Zachod slonca o: %02d:%02d\n", sunTime.sunset/60, sunTime.sunset%60);
}

void everyHour(){
	ntpClientUpdate=1;
}

void everyDay(){
	Serial.println("daily");
}

void loop()
{
	if(ntpClientUpdate==1){

		ntpClient.update();//update does not work when executed from timer
		ntpClientUpdate=0;
		recalculateOnOffTime();
	}
	handleClient();
}
