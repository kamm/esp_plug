#include "wifi.h"

ESP8266WebServer server(80);
File fsUploadFile;
extern NTPClient ntpClient;
extern SunTime sunTime;
extern int mode;
extern int state;
String getContentType(String filename) {
	if (server.hasArg("download")){
		return "application/octet-stream";
	}
	else if (filename.endsWith(".htm")){
		return "text/html";
	}
	else if (filename.endsWith(".html")){
		return "text/html";
	}
	else if (filename.endsWith(".css")){
		return "text/css";
	}
	else if (filename.endsWith(".js")){
		return "application/javascript";
	}
	else if (filename.endsWith(".png")){
		return "image/png";
	}
	else if (filename.endsWith(".gif")){
		return "image/gif";
	}
	else if (filename.endsWith(".jpg")){
		return "image/jpeg";
	}
	else if (filename.endsWith(".ico")){
		return "image/x-icon";
	}
	else if (filename.endsWith(".xml")){
		return "text/xml";
	}
	else if (filename.endsWith(".pdf")){
		return "application/x-pdf";
	}
	else if (filename.endsWith(".zip")){
		return "application/x-zip";
	}
	else if (filename.endsWith(".gz")){
		return "application/x-gzip";
	}else{
		return "text/plain";
	}
}

bool handleFileRead(String path) {
	Serial.println("handleFileRead: " + path);
	if (path.endsWith("/")){
		path += "index.htm";
	}
	String contentType = getContentType(path);
	String pathWithGz = path + ".gz";
	if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
		if (SPIFFS.exists(pathWithGz)){
			path += ".gz";
		}
		File file = SPIFFS.open(path, "r");
		server.streamFile(file, contentType);
		file.close();
		return true;
	}
	return false;
}

void initializeWiFi(String ssid, String password, String hostname, WiFiMode_t mode){
	WiFi.begin(ssid.c_str(), password.c_str());
	WiFi.mode(mode);
	WiFi.hostname(hostname);
	MDNS.begin(hostname.c_str());
	while(WiFi.status() != WL_CONNECTED){
		Serial.print(".");
		delay(100);
	}
	IPAddress ip = WiFi.localIP();
	Serial.println();
	Serial.print("Wifi connected\nlocal IP: ");
	Serial.println(ip);
	WiFi.printDiag(Serial);

}


void handleFileUpload() {
	if (server.uri() != "/edit"){
		return;
	}
	HTTPUpload& upload = server.upload();
	if (upload.status == UPLOAD_FILE_START){
		String filename = upload.filename;
		if (!filename.startsWith("/")){
			filename = "/" + filename;
		}
		Serial.print("handleFileUpload Name: ");
		Serial.println(filename);
		fsUploadFile = SPIFFS.open(filename, "w");
		filename = String();
	} else if (upload.status == UPLOAD_FILE_WRITE) {
		if (fsUploadFile){
			fsUploadFile.write(upload.buf, upload.currentSize);
		}
	} else if (upload.status == UPLOAD_FILE_END) {
		if (fsUploadFile){
			fsUploadFile.close();
		}
		Serial.print("handleFileUpload Size: ");
		Serial.println(upload.totalSize);
	}
}

void handleFileDelete() {
	if (server.args() == 0){
		return server.send(500, "text/plain", "BAD ARGS");
	}
	String path = server.arg(0);
	Serial.println("handleFileDelete: " + path);
	if (path == "/"){
		return server.send(500, "text/plain", "BAD PATH");
	}
	if (!SPIFFS.exists(path)){
		return server.send(404, "text/plain", "FileNotFound");
	}
	SPIFFS.remove(path);
	server.send(200, "text/plain", "");
	path = String();
}

void handleFileCreate() {
	if (server.args() == 0){
		return server.send(500, "text/plain", "BAD ARGS");
	}
	String path = server.arg(0);
	Serial.println("handleFileCreate: " + path);
	if (path == "/"){
		return server.send(500, "text/plain", "BAD PATH");
	}
	if (SPIFFS.exists(path)){
		return server.send(500, "text/plain", "FILE EXISTS");
	}
	File file = SPIFFS.open(path, "w");
	if (file){
		file.close();
	}
	else{
		return server.send(500, "text/plain", "CREATE FAILED");
	}
	server.send(200, "text/plain", "");
	path = String();
}

void handleFileList() {
	if (!server.hasArg("dir")) {
		server.send(500, "text/plain", "BAD ARGS");
		return;
	}

	String path = server.arg("dir");
	Serial.println("handleFileList: " + path);
	Dir dir = SPIFFS.openDir(path);
	path = String();

	String output = "[";
	while (dir.next()) {
		File entry = dir.openFile("r");
		if (output != "["){
			output += ',';
		}
		bool isDir = false;
		output += "{\"type\":\"";
		output += (isDir) ? "dir" : "file";
		output += "\",\"name\":\"";
		output += String(entry.name()).substring(1);
		output += "\"}";
		entry.close();
	}
	output += "]";
	server.send(200, "text/json", output);
}

String uptime(){
	long days = 0;
	long hours = 0;
	long mins = 0;
	long secs = 0;
	secs = calculateUptime(); 		//get uptime in seconds
	mins = secs / 60; 					//convert seconds to minutes
	hours = mins / 60; 					//convert minutes to hours
	days = hours / 24; 					//convert hours to days
	secs = secs - (mins * 60); 			//subtract the coverted seconds to minutes in order to display 59 secs max
	mins = mins - (hours * 60); 		//subtract the coverted minutes to hours in order to display 59 minutes max
	hours = hours - (days * 24); 		//subtract the coverted hours to days in order to display 23 hours max
	
	return String("Uptime is ") + days + " days " + hours + ":" + (mins<10?"0":"")+mins + ":" + (secs<10?"0":"")+secs;
}

void initializeHTTPServer(){
	SPIFFS.begin();

	server.on("/list", HTTP_GET, handleFileList);
	server.on("/edit", HTTP_GET, []() {
		if (!handleFileRead("/edit.htm")){
			server.send(404, "text/plain", "FileNotFound");
		}
	});
	server.on("/edit", HTTP_PUT, handleFileCreate);
	server.on("/edit", HTTP_DELETE, handleFileDelete);
	server.on("/edit", HTTP_POST, []() {
		server.send(200, "text/plain", "");
	}, handleFileUpload);

	server.onNotFound([]() {
		if (!handleFileRead(server.uri())){
			server.send(404, "text/plain", "FileNotFound");
		}
	});

	server.on("/all", HTTP_GET, []() {
		String json = "{";
		json += "\"t\":\"t\"";
		json += ", \"uptime\":\"" + uptime() + "\"";
		json += ", \"date\":\"" + ntpClient.getIsoDateTime() + "\"";
		json += ", \"epoch\":\"" + String(ntpClient.getEpochTime()) + "\"";
		json += ", \"sunrise\":\"" + String(sunTime.sunrise) + "\"";
		json += ", \"sunset\":\"" + String(sunTime.sunset) + "\"";
		json += ", \"currentTime\":\"" + String(sunTime.currentTime) + "\"";
		json += ", \"mode\":"+String(mode);
		json += ", \"state\":"+String(state);

		json += "}";
		server.send(200, "text/json", json);
		json = String();
	});

	server.on("/mode",HTTP_GET,[](){
		if(server.args()==1 && server.argName(0)=="mode" && (server.arg(0)=="0" || server.arg(0)=="1")){
			mode = server.arg(0).toInt();
			String json = "{";
			json += "\"mode\":"+String(mode);
			json +="}";
			server.send(200,"text/json",json);
		}else{
			server.send(200,"text/json","{\"mode\":"+String(mode)+"}");
		}
	});

	server.on("/state",HTTP_GET,[](){
		if(server.args()==1 && server.argName(0)=="state" && (server.arg(0)=="0" || server.arg(0)=="1" || server.arg(0)=="2")){
			state = server.arg(0).toInt();
			String json = "{";
			json += "\"state\":"+String(state);
			json +="}";
			server.send(200,"text/json",json);
		}else{
			server.send(200,"text/json","{\"state\":"+String(state)+"}");
		}
	});

	server.on("/status",HTTP_GET,[](){
		String json="{\"mode\":"+String(mode)+",\"state\":"+String(state)+"}";
		server.send(200,"text/json",json);
	});



	server.on("/date",HTTP_GET, []() {
		String output = "{date:''}";
		server.send(200,"text/json",output);
		output = String();
	});

	server.begin();
	Serial.println("HTTP server started");
}

void handleClient(){
	server.handleClient();
}

unsigned long calculateUptime(void){
    static unsigned int _rolloverCount = 0;
    static unsigned long _lastMillis = 0;

    unsigned long currentMillis = millis();
    if(currentMillis < _lastMillis){
	_rolloverCount+=1;
    }

    _lastMillis = currentMillis;

    return (0xFFFFFFFF/1000)*_rolloverCount + (_lastMillis / 1000);
}
