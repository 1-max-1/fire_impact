#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include <Esp.h>

// The amount of clients that have connected
byte clientCount = 0;
// This holds all of the clients that have connected to the tcp server. At the moment I'll keep it at a maximum of 5
WiFiClient clients[5];

// Create udp instance
WiFiUDP udp;
// Create tcp server instance. We only want a maximum of 5 clients
WiFiServer TCPServer(1305, 5);

// We use this to determine if enough time has passed to update the sensor data
unsigned long startMillis = 0;
// Tracks how long ago the reset button was pressed. Used for debouncing too
unsigned long buttonPressTime = 0;
int prevButtonState;

bool connectedToUserWifi = false;

struct pins {
	static const int SETUP_IN_PROGRESS = 25;
	static const int SETUP_SUCCESS = 26;
	static const int NEED_WIFI_DETAILS = 27;
	static const int CONNECTED = 14;
	static const int RESET_BUTTON = 13;
	static const int SENSOR_1 = 36;
	static const int SENSOR_2 = 39;

	static void clearLEDS() {
		digitalWrite(SETUP_SUCCESS, LOW);
		digitalWrite(SETUP_IN_PROGRESS, LOW);
		digitalWrite(CONNECTED, LOW);
		digitalWrite(NEED_WIFI_DETAILS, LOW);
	}
};

void setup() {
	Serial.begin(9600);

	// Button and sensors
	pinMode(pins::SENSOR_1, INPUT);
	pinMode(pins::SENSOR_2, INPUT);
	pinMode(pins::RESET_BUTTON, INPUT);
	prevButtonState = digitalRead(pins::RESET_BUTTON);

	// LED's
	pinMode(pins::SETUP_SUCCESS, OUTPUT);
	pinMode(pins::NEED_WIFI_DETAILS, OUTPUT);
	pinMode(pins::CONNECTED, OUTPUT);
	pinMode(pins::SETUP_IN_PROGRESS, OUTPUT);
	digitalWrite(pins::SETUP_IN_PROGRESS, HIGH);
	
	bool result = SPIFFS.begin();
	if(!result) {
		Serial.println("SPIFFS initializaion failed");
		while(true);
	}

	File file = SPIFFS.open("/networkDetails.txt");
	if(!file || file.size() < 3) {
		Serial.println("No previous data");
		// Setup an access point. This will be connected to by users, who will in turn give us their wifi password
		WiFi.softAP("FireImpact-WiFi", "xp59vg7p");
		digitalWrite(pins::NEED_WIFI_DETAILS, HIGH);
	}
	// If there is some stuff in the file, then we read the details out of the file
	else {
		String networkName = file.readStringUntil(',');
		String networkPassword = file.readString();
		file.close();
		Serial.println("Name: " + networkName + "\nPassword: " + networkPassword);
		WiFi.begin(networkName.c_str(), networkPassword.c_str());

		// Wait for connection
		while (WiFi.status() != WL_CONNECTED) {
			delay(250);
			yield();
		}

		digitalWrite(pins::CONNECTED, HIGH);
		connectedToUserWifi = true;
	}

	// Allows devices to find us
	result = udp.begin(WiFi.localIP(), 1304);
	if(!result) {
		Serial.println("UDP initializaion failed");
		while(true);
	}

	// Start the socket server for when we want to send data to clients
	TCPServer.begin();
	Serial.println("Started");

	digitalWrite(pins::SETUP_IN_PROGRESS, LOW);
	digitalWrite(pins::SETUP_SUCCESS, HIGH);
}

void loop() {
	WiFiClient incomingClient = TCPServer.available();
	// If we have an incoming client and they're sending us data, its the app on the users phone.
	if(incomingClient) {
		// Incoming client will be true if an old client is sending data, so technically they are not new and we don't want to store them
		bool clientAlreadyConnected = false;
		for(uint8_t i = 0; i < clientCount; i++) {
			if(clients[i] == incomingClient) {
				clientAlreadyConnected = true;
				break;
			} 
		}
		
		// Store the client if we dont have them already
		if(!clientAlreadyConnected) {
			// Clear the input buffer ("init") from the client so it's not data wating to be read
			uint8_t unusedBuffer[4];
			incomingClient.readBytes(unusedBuffer, 4);
			clients[clientCount] = incomingClient;

			Serial.println("Client connected. Client number: " + String(clientCount));
			clientCount++;
		}
	}

	// If a second has passed then we need to run updates for the tcp server
	if(millis() - startMillis >= 1000) {
		for(byte i = 0; i < clientCount; i++) {
			// If they aren't connected anymore then we can remove them and move on to the next client
			if(clients[i].connected() == false) {
				Serial.println("Client " + String(i) + " disconnected");
				clients[i].stop();
				removeClient(i);
				Serial.println("Client count: " + String(clientCount));
				continue;
			}

			int availableBytes = clients[i].available();

			// If the client is sendig us data then it will be the name ssid and password for their wifi network
			if(availableBytes > 0 && connectedToUserWifi == false) {
				Serial.print("Received WiFi details.\nAvailable bytes: ");
				Serial.println(availableBytes);

				// Read in password and name
				char* buffer = new char[availableBytes];
				clients[i].readBytes(buffer, availableBytes);

				// The first byte of the buffer will hold the length of the network name
				int nameBufferSize = (int)buffer[0];
				int passwordBufferSize = availableBytes - nameBufferSize - 1;
				char* nameBuffer = new char[nameBufferSize];
				char* passwordBuffer = new char[passwordBufferSize];

				// Split buffer into separate null terminated strings for wifi
				memcpy(nameBuffer, &buffer[1], nameBufferSize);
				memcpy(passwordBuffer, &buffer[1 + nameBufferSize], passwordBufferSize);

				Serial.print("Name buffer: ");
				Serial.println(nameBuffer);
				Serial.print("Password buffer: ");
				Serial.println(passwordBuffer);

				WiFi.begin(nameBuffer, passwordBuffer);

				bool incorrectPassword = false;
				byte connectionAttemptLoops = 0;

				// Wait for connection
				while (WiFi.status() != WL_CONNECTED) {
					Serial.println((int)WiFi.status());
					delay(250);
					connectionAttemptLoops++;
					yield();

					if(WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL || connectionAttemptLoops > 36) {
						Serial.println("Connect failed");

						// When the connection fails, we notify the app
						clients[i].print("i");
						incorrectPassword = true;
						break;
					}
				}

				// Dont want to do the below stuff if the password was actually wrong
				if(incorrectPassword) {
					// Memory leaks are bad...
					delete[] buffer;
					delete[] nameBuffer;
					delete[] passwordBuffer;
					continue;
				}

				// Store details for next time
				File file = SPIFFS.open("/networkDetails.txt", "w");
				file.write((unsigned char*)nameBuffer, nameBufferSize);
				file.write((unsigned char*)",", 1);
				file.write((unsigned char*)passwordBuffer, passwordBufferSize);
				file.close();

				// Memory leaks are bad...
				delete[] buffer;
				delete[] nameBuffer;
				delete[] passwordBuffer;

				digitalWrite(pins::NEED_WIFI_DETAILS, LOW);
				digitalWrite(pins::CONNECTED, HIGH);

				Serial.println("correcto password and SSID");
				clients[i].print("c");
				Serial.println("Printed c to the client");
				connectedToUserWifi = true;
				return;
			}

			// Read sensors. We only need to do this if we are actually connected to the user's wifi
			if(connectedToUserWifi) {
				int value1 = analogRead(36);
				int value2 = analogRead(39);
				// Send the data to the app
				clients[i].print("d," + String(value1) + "," + String(value2));
			}
		}

		startMillis = millis();
	}

	// We still want to listen for discovery broadcasts and respond to them
	broadcastListener();

	// The access point is there so we can get network details. When we get the correct details, the esp32 will connect to the regular network. However, the acces point will still be
	// kept open so that the client can be informed their details were correct. We cant close the AP straight away since that will stop the message. It doesn't do any harm to keep
	// it open, we can just let it sit there and close it at a later date, if there are no clients connected to it anymore. Note that doing this wont close the TCP server (handily)
	if(connectedToUserWifi && WiFi.softAPgetStationNum() < 1)
		WiFi.softAPdisconnect(true);

	int buttonState = digitalRead(pins::RESET_BUTTON);
	// Captures first time button is pressed
	if(buttonState == HIGH && prevButtonState == LOW && millis() - buttonPressTime > 100) {
		Serial.println("Button pressed first time");
		buttonPressTime = millis();
	}
	// Captures when button has been held down for more than 4 seconds
	else if(buttonState == HIGH && prevButtonState == HIGH && millis() - buttonPressTime > 4000) {
		Serial.println("resetting");

		// Clear file
		File file = SPIFFS.open("/networkDetails.txt", "w");
		file.write(44);
		file.close();

		// Reboot
		pins::clearLEDS();
		ESP.restart();
	}

	prevButtonState = buttonState;
}

// Small util function
void removeClient(byte index) {
	// Move every client down by one in the clients array
	for(byte i = index; i < clientCount - 1; i++) {
		clients[i] = clients[i + 1];
	}
	// We now have one less client
	clientCount--;
}

// Function to receive UDP broadcasts from clients who want to connect
void broadcastListener() {
	// if there's data available, read a packet
	int packetSize = udp.parsePacket();
	if (packetSize) {
		// Buffer to hold incoming packet
		char packetBuffer[17];

		// read the packet into packetBufffer
		int len = udp.read(packetBuffer, 17);
		
		Serial.print("Received packet. Contentsss: ");
		Serial.println(packetBuffer);
		Serial.println("BOI");

		// We don't want to reply to a random packet
		if(strcmp(packetBuffer, "areYouTheArduino")) return;
		
		// We now need to send a reply to the IP address and port that sent us the packet. We can begin the packet with this call
		udp.beginPacket(udp.remoteIP(), 1304);
		// The reply that we are sending
		String reply = connectedToUserWifi ? "iAmAnArduino" : "iAmAnArduinonp";
		Serial.println("Sending " + reply);
		udp.print(reply.c_str());
		// endPacket sends the packet to the client
		udp.endPacket();
	}
}