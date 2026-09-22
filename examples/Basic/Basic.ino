/*
 * MicroMDNS - Basic example
 *
 * Joins your WiFi network, starts a tiny web server, and makes it reachable as
 * http://mydevice-<chipid>.local/ from phones and PCs on the same network.
 * Browsers that look for web servers on the network (DNS-SD / Bonjour) will
 * also list it.
 *
 * Fill in your network name and password below, upload, and open the Serial
 * Monitor at 115200 baud to see the address to browse to.
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <MicroMDNS.h>

const char* WIFI_SSID     = "your-network";
const char* WIFI_PASSWORD = "your-password";

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // MicroMDNS does not check whether anyone else already uses the name, so it
  // must be unique on your network. Adding the chip ID is the easy way.
  String name = "mydevice-" + String(ESP.getChipId(), HEX);

  // Optional: print what the responder is doing.
  mdnsSetLogger([](const char* message) { Serial.println(message); });

  mdnsBegin(name.c_str());
  mdnsAddService("http", "tcp", 80);
  mdnsAddTxt("path=/");

  server.on("/", []() {
    server.send(200, "text/plain", "Hello from MicroMDNS");
  });
  server.begin();

  Serial.print("Browse to http://");
  Serial.print(name);
  Serial.println(".local/");
}

void loop() {
  server.handleClient();
  mdnsLoop();   // answers queries - call it every pass

  // Before restarting, call mdnsEnd() so phones and PCs forget the name at
  // once rather than up to two minutes later:
  //   mdnsEnd();
  //   ESP.restart();
}
