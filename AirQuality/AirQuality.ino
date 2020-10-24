/*#include <WiFi.h>
#include <HTTPClient.h>

// howsmyssl.com root certificate authority
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n" \
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
"DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n" \
"PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n" \
"Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
"AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n" \
"rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n" \
"OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n" \
"xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n" \
"7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n" \
"aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n" \
"HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n" \
"SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n" \
"ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n" \
"AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n" \
"R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n" \
"JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n" \
"Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n" \
"-----END CERTIFICATE-----\n";

void setup() {
	WiFi.disconnect();
	WiFi.mode(WIFI_STA);
	Serial.begin(9600);

	function();
}

void function() {
	Serial.println("Scanning for networks");
	int16_t networks = WiFi.scanNetworks(false, true);
	Serial.println("Scan complete\n\nNetworks within range:");

	int viableNetworks[networks] = {};
	int index = 0;
	
	for(int i = 0; i < networks; i++) {
		if(WiFi.RSSI(i) >= -70) {
			viableNetworks[index] = i;
			index++;
			Serial.println(String(i + 1) + ") " + WiFi.SSID(i) + ", " + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Secure"));
		}
	}

	label1:
	Serial.println("\nPlease type the corresponding number to choose the network:");
	while(Serial.available() <= 0) {}
	int choice = Serial.parseInt();

	// parseInt returns 0 when it times out so thats ultra handy!!! It will be caught by this statement id the user enters a non int
	if(choice < 1 or choice > index) {
		Serial.println("\nThat wasn't a valid choice!");
		goto label1;
	}

	// Get the number of the network from our choice.
	choice = viableNetworks[choice - 1];

	if(WiFi.encryptionType(choice) != WIFI_AUTH_OPEN) {
		label2:
		Serial.println("\nThis network requires a password. Please enter it now:");
		while(Serial.available() <= 0) {}
		String password = Serial.readString();

		if(WiFi.begin(WiFi.SSID(choice).c_str(), password.c_str()) != WL_CONNECTED) {
			Serial.print("\nSomething went wrong. You probably entered an incorrect password. Please try again.");
			goto label2;
		}
	}
	else {
		label3:
		if(WiFi.begin(WiFi.SSID(choice).c_str()) != WL_CONNECTED) {
			Serial.println("\nSomething went wrong. Attempting to reconnect...");
			goto label3;
		}
	}

	Serial.println("\nSuccessfully connected to the network!\nSending request to howsmyssl.com\n");

	HTTPClient http;
 
		http.begin("https://www.howsmyssl.com/a/check", root_ca); //Specify the URL and certificate
		int httpCode = http.GET();
 
		if (httpCode > 0) { //Check for the returning code
				String payload = http.getString();
				Serial.println(httpCode);
				Serial.println(payload);
			}
 
		else {
			Serial.println("Error on HTTP request");
		}
 
		http.end(); //Free the resources
}

void loop() {}*/


#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>

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

bool connectedToUserWifi = false;

void setup() {
	Serial.begin(9600);
	
	bool result = SPIFFS.begin();
	if(!result) {
		Serial.println("SPIFFS initializaion failed");
		while(true);
	}

	File file = SPIFFS.open("/networkDetails.txt");
	if(!file || file.size() < 1) {
		Serial.println("No previous data");
		// Setup an access point. This will be connected to by users, who will in turn give us their wifi password
		WiFi.softAP("FireImpact-WiFi", "xp59vg7p");
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
			// Clear the input buffer ("init") from the client so its not data wating to be read
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