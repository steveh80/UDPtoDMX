#if USE_ESPWIFI == 1

#define MQTT_KEEPALIVE 120

#include "log.h"
#include "network.h"
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#ifdef USE_WIFIMANAER
#include <DNSServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "192.168.1.8";
const char* mqtt_username = "H801";
const char* mqtt_password = "secret";

#endif  


WiFiUDP udp;


void configModeCallback (WiFiManager *myWiFiManager) {
  DEBUG_PRINT(LOG_INFO, F("Entered config mode"));

  DEBUG_BEGIN(LOG_INFO);
  DEBUG_PRINT(WiFi.softAPIP().toString().c_str());
  DEBUG_END();
  
  DEBUG_BEGIN(LOG_INFO);
  DEBUG_PRINT(myWiFiManager->getConfigPortalSSID().c_str());
  DEBUG_END();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // message received
}

void mqttReconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = "H801-testing";
    
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      // Once connected, publish an announcement...
      client.publish("H801/announce", "H801 connected");
      // ... and resubscribe
      client.subscribe("H801/commands");
    } else {
      // Serial.print("failed, rc=");
      // Serial.print(client.state());
      // Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void virt_networkClass::init() {
  
  #ifdef USE_WIFIMANAER
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    
    wifiManager.setAPCallback(configModeCallback);

    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    wifiManager.autoConnect();

    //in seconds
    wifiManager.setTimeout(180);
  
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    DEBUG_PRINT(LOG_INFO, F("try to connect using wifimanager"));
    if (!wifiManager.autoConnect()) {
      DEBUG_PRINT(LOG_INFO, F("failed to connect and hit timeout"));
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    } 
    //if you get here you have connected to the WiFi
    DEBUG_PRINT(LOG_INFO, F("connected by wifimanager"));
    //if you get here you have connected to the WiFi
    DEBUG_BEGIN(LOG_INFO);
    DEBUG_PRINT(WiFi.localIP().toString().c_str());
    DEBUG_END();

    // connect to mqtt server
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);

  #else
    const char* ssid = "dd-wrt_57b";  //  your network SSID (name)
    const char* pass = "password";       // your network password

   DEBUG_PRINT(LOG_INFO, F("Init Network"));
  
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, pass);
 
  // Wait for connect to AP
  DEBUG_PRINT(LOG_INFO, F("[Connecting]"));
  DEBUG_BEGIN(LOG_INFO);
  DEBUG_PRINT(ssid);
  DEBUG_END();
  int tries=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(LOG_INFO, F("."));
    tries++;
    if (tries > 30){
      break;
    }
  }
  #endif
  
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  udp.begin(udpPort);
  isUP = true;
  DEBUG_PRINT(LOG_INFO, F("UDP UP"));
  delay(100);  
  DEBUG_BEGIN(LOG_INFO);
  DEBUG_PRINT(F("my IP: "));
  DEBUG_PRINT(WiFi.localIP().toString().c_str());
  DEBUG_END();
  
  
}


void virt_networkClass::loop() {
int packetSize = udp.parsePacket(); 
  if(packetSize)
  {
    // read the packet into packetBufffer
    udp.read(packetBuffer, maxPacketSize);
    if (packetSize > maxPacketSize) {
      packetSize = maxPacketSize;
    }
    onNetworkData(packetBuffer, packetSize);
  }

  if (!client.connected()) {
    mqttReconnect();
  }
  client.loop();
}

void virt_networkClass::Register_OnNetworkData(event_networkData callback) {
  DEBUG_PRINT(LOG_INFO, F("Network Callback registred"));
  onNetworkData = callback;
}

void virt_networkClass::beginPacket() {
  if (isUP) {
    myip[3] = DEBUG_IP;
    udp.beginPacket(myip, DEBUG_PORT);
  }
}
void virt_networkClass::print(const __FlashStringHelper *ifsh) {
  if (isUP) {
    udp.print(ifsh);
  }
}

void virt_networkClass::print(const int i){
  if (isUP) {
    udp.print(i);
  }
}

void virt_networkClass::print(const char *c, int charlen){
  if (isUP) {
    if (charlen == 0) {
      udp.print(c);
    } else {
      udp.write(c, charlen);
    }
  }
}

void virt_networkClass::endPacket(){
  if (isUP) {
   /* udp.write(uint8_t(0));*/
    udp.endPacket();
    yield();
  }
}


virt_networkClass virt_network;

#endif