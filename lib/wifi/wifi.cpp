#include <wifi.h>

ESP8266WebServer server(80);
File fsUploadFile;
NTPClient *tc;

void setTimeClient(NTPClient *t){
	tc = t;
}

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

void initializeWiFi(String ssid, String password, String host, WiFiMode_t mode){
	WiFi.begin(ssid.c_str(), password.c_str());
	WiFi.mode(mode);
	MDNS.begin(host.c_str());
	while(WiFi.status() != WL_CONNECTED){
		Serial.print(".");
		delay(100);
	}
	IPAddress ip = WiFi.localIP();
	Serial.println();
	Serial.print("Wifi conencted, local IP: ");
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
		/*
		Serial.print("handleFileUpload Data: ");
		Serial.println(upload.currentSize);
		*/
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
	long currentmillis = millis();
	secs = currentmillis / 1000; 		//convect milliseconds to seconds
	mins = secs / 60; 					//convert seconds to minutes
	hours = mins / 60; 					//convert minutes to hours
	days = hours / 24; 					//convert hours to days
	secs = secs - (mins * 60); 			//subtract the coverted seconds to minutes in order to display 59 secs max
	mins = mins - (hours * 60); 		//subtract the coverted minutes to hours in order to display 59 minutes max
	hours = hours - (days * 24); 		//subtract the coverted hours to days in order to display 23 hours max
	return String("Uptime is ") + days + " days " + hours + ":" + mins + ":" + secs;
}

void initializeHTTPServer(){
	//SERVER INIT
	//list directory
	SPIFFS.begin();

	server.on("/list", HTTP_GET, handleFileList);
	//load editor
	server.on("/edit", HTTP_GET, []() {
		if (!handleFileRead("/edit.htm")){
			server.send(404, "text/plain", "FileNotFound");
		}
	});
	//create file
	server.on("/edit", HTTP_PUT, handleFileCreate);
	//delete file
	server.on("/edit", HTTP_DELETE, handleFileDelete);
	//first callback is called after the request has ended with all parsed arguments
	//second callback handles file uploads at that location
	server.on("/edit", HTTP_POST, []() {
		server.send(200, "text/plain", "");
	}, handleFileUpload);

	//called when the url is not defined here
	//use it to load content from SPIFFS
	server.onNotFound([]() {
		if (!handleFileRead(server.uri())){
			server.send(404, "text/plain", "FileNotFound");
		}
	});

	server.on("/gpio", []() {
		String state=server.arg("state");
		int pin = server.arg("pin").toInt();
		int curstate = (((((GPI) | (GPO)) & 0xFFFF) | ((GP16I & 0x01) << 16) & (1<<(pin))) >> pin) & 1;
		if (state == "on"){
			digitalWrite(pin, HIGH);
		}else if (state == "off"){
			digitalWrite(pin, LOW);
		}else if(state == "toggle"){

			if(curstate != 0){
				digitalWrite(pin, LOW);
			}else{
				digitalWrite(pin, HIGH);
			}
		}
		curstate = ((((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16) & (1<<(pin))) >> pin) & 1;
		server.send(200, "text/plain", String("{\"res\":\"OK\",\"pin\":")+pin+String(",\"state\":")+curstate+String("}"));
	});

	//get heap status, analog input value and all GPIO statuses in one json call
	server.on("/all", HTTP_GET, []() {
		String json = "{";
		json += "\"heap\":" + String(ESP.getFreeHeap());
		json += ", \"analog\":" + String(analogRead(A0));
		json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
		json += ", \"uptime\":\"" + uptime() + "\"";
		json += "}";
		server.send(200, "text/json", json);
		json = String();
	});

	server.on("/date",HTTP_GET, []() {
		String output = "{date:'"+tc->getIsoDateTime()+"'}";
		server.send(200,"text/json",output);
		output = String();
	});

	server.begin();
	Serial.println("HTTP server started");
}

void handleClient(){
	server.handleClient();
}
