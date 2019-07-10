#pragma once
#include <WString.h> 
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>


typedef struct SunTime{
        uint16_t sunset;
        uint16_t sunrise;
        uint16_t currentTime;
        uint8_t state;
} SunTime;

	String getContentType(String filename);
	bool handleFileRead(String path);
	void initializeWiFi(String, String, String, WiFiMode_t);
	void handleFileUpload();
	void handleFileDelete();
	void handleFileCreate();
	void handleFileList();
	String uptime();
	void initializeHTTPServer();
	void handleClient();
	void setTimeClient(NTPClient *);
	void setSunTime(SunTime *);
