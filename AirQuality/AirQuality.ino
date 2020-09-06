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

// The amount of clients that have connected
byte clientCount = 0;
// This holds all of the clients that have connected to the tcp server. At the moment I'll keep it at a maximum of 5
WiFiClient clients[5];

//create udp instance
WiFiUDP udp;
// Create tcp server instance
WiFiServer TCPServer(1305);

// We use this to determine if enough time has passed to update the sensor data
unsigned long startMillis = 0;

void setup(){
    Serial.begin(9600);
    
    //Connect to the WiFi network
    WiFi.begin("Hosking Family Wifi 2.4GHz", "resapujafe");
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(250);
      Serial.print(".");
    }
    
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    //This initializes udp
    udp.begin(WiFi.localIP(), 1304);

    // Start the socket server for when we want to send data to clients
    TCPServer.begin();
}

void loop() {
  WiFiClient incomingClient = TCPServer.available();
  // If we have an incoming client and they're sending us data, its the app on the users phone. We only want a maximum of 5 clients
  if(incomingClient && clientCount < 5) {
    // Store the client
    clients[clientCount] = incomingClient;
    Serial.println("Client connected. Client number: " + String(clientCount));
    clientCount++;
  }

  // If a second has passed then we need to update the sensor data
  if(millis() - startMillis >= 1000) {
    for(byte i = 0; i < clientCount; i++) {
      // If they aren't connected anymore then we can remove them and move on to the next client
      if(clients[i].connected() == false) {
        Serial.println("Client " + String(i) + " disconnected");
        clients[i].stop();
        removeClient(i);
        continue;
      }

      // Read sensors
      int value1 = analogRead(36);
      int value2 = analogRead(39);
      // Send the data to the app
      clients[i].print(String(value1) + "," + String(value2));
    }

    startMillis = millis();
  }

  // We can still listen for broadcasts and respond to them
  broadcastListener();
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
    char packetBuffer[255];

    // read the packet into packetBufffer
    int len = udp.read(packetBuffer, 16);
    
    Serial.print("Received packet. Contents: ");
    Serial.println(packetBuffer);

    // We don't want to reply to a random packet
    if(strcmp(packetBuffer, "areYouTheArduino")) return;
    
    // We now need to send a reply to the IP address and port that sent us the packet. We can begin the packet with this call
    udp.beginPacket(udp.remoteIP(), 1304);
    // The reply that we are sending
    udp.print("iAmAnArduino");
    // endPacket sends the packet to the client
    udp.endPacket();
  }
}
