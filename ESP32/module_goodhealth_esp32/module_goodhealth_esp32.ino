#include <WiFi.h> // #change WiFi connection library for ESP32 compatibility
#include <WiFiClient.h> // #change WiFi connection library for ESP32 compatibility

#include <PubSubClient.h>

#include <rom/rtc.h> // #change for reset reason ESP32

#define BUILTIN_LED 2 // #change define built in led for ESP32 board

// Update these with values suitable for your network.

const char* ssid = ""; // #change ssid
const char* password = ""; // #change password
const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

uint32_t minFreeHeap = 330000; // #change minfreeheap for comparison

//
#ifdef __cplusplus
extern "C" {
#endif

uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif

uint8_t temprature_sens_read();

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

uint32_t ESP_getChipId(){ // #change chipID for ESP32 (https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/ChipID/GetChipID/GetChipID.ino)
  uint32_t chipId = 0;
  for(int i=0; i<17; i=i+8) { chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i; }
  return chipId;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-"; // #change from ESP8266 to ESP32
    clientId += ESP_getChipId(); // #change from random to chipID
    Serial.println("ChipID: " + String(ESP_getChipId())); // #change print chipID
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      String outTopic = String(ESP_getChipId())+"/status"; // #change from general to chipID
      client.publish(outTopic.c_str(), "hello world"); // #change from general to chipID
      // ... and resubscribe
      String inTopic = String(ESP_getChipId())+"/inTopic"; // #change from general to chipID
      client.subscribe(inTopic.c_str()); // #change from general to chipID
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < minFreeHeap){
    minFreeHeap = freeHeap;
  }

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    
    String status_json = "{"; // #change json for module health status monitoring
    
    status_json += "\"chipid\": \""+String(ESP_getChipId())+"\",";

    status_json += "\"minfreeheap\": \""+String(minFreeHeap)+"\","; // bytes
    uint32_t minFreeHeap = 330000; // #change update minfreeheap for comparison
    status_json += "\"freeheap\": \""+String(ESP.getFreeHeap())+"\",";

    float internal_temperature = (temprature_sens_read() - 32) / 1.8; // Celsius
    status_json += "\"internal_temperature\": \""+String(internal_temperature)+"\",";

    int16_t wifi_level = WiFi.RSSI(); // dBm
    status_json += "\"wifi_level\": \""+String(wifi_level)+"\",";

    int16_t reset_reason1 = rtc_get_reset_reason(0); // reset reason core 1
    status_json += "\"reset_reason1\": \""+String(reset_reason1)+"\",";

    int16_t reset_reason2 = rtc_get_reset_reason(1); // reset reason core 2
    status_json += "\"reset_reason2\": \""+String(reset_reason2)+"\"";
    
    status_json += "}";
    
    String outTopic = String(ESP_getChipId())+"/status"; // #change from general to chipID
    Serial.print("Publish message in " + outTopic + ": ");
    Serial.println(status_json); // #change from msg to status_json
    client.publish(outTopic.c_str(), status_json.c_str()); // #change from general to chipID, from msg to status_json
  }
}
