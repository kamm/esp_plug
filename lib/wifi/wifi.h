#include <WString.h> 
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#ifndef __WIFI_H__
#define __WIFI_H_
using namespace std;
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
#endif
