#include <Arduino.h>
#include <AZ3166WiFi.h>
#include <MQTTNetwork.h>
#include <MQTTClient.h>
#include <OledDisplay.h>
#include "EEPROMInterface.h"
#include "SensorManager.h"
#include <time.h>

// Maximum packet size for MQTT messages.  We need to override the default of 100 in MQTTClient.h.
const int MQTT_MAX_PACKET_SIZE = 400;

// MQTT Broker
uint8_t mqttBroker[MQTT_MAX_LEN + 1] = {'\0'};

// MQTT Port - may be defined in the build environment.  If not, use the default.
#ifndef MQTT_PORT
  #define MQTT_PORT 1883
#endif

//Device ID
uint8_t deviceId[DEVICE_ID_MAX_LEN + 1] = {'\0'};

//Device Password
uint8_t devicePassword[DEVICE_PASSWORD_MAX_LEN + 1] = {'\0'};

//Topic - will be built from the device ID if not defined in the build environment.
char topic[100];

//Check interval for flow state changes. If not defined, use the default value of 2 seconds.
#ifndef SLEEP_INTERVAL
  #define SLEEP_INTERVAL 2000
#endif

//SensorManager instance.
SensorManager* sensorManager;

void configureWifi();
int sendMQTTMessage(bool* waterFlowing);

void setup() {
  //Set the Serial output speed so we can see the output in the Serial Monitor
  Serial.begin(115200);
  
  //Initialize the LCD screen.
  Screen.init();

  //Read the Device ID
  EEPROMInterface eeprom;
  int result = eeprom.readDeviceID((char*)deviceId, DEVICE_ID_MAX_LEN + 1);

  if (result != 0) {
    Serial.printf("Error reading device ID from EEPROM: %d \n", result);
  }

  // Print the device ID to the Serial Monitor
  Serial.printf("Device ID: '%s' \n", deviceId);  

  // Read the device password from EEPROM
  result = eeprom.readDevicePassword((char*)devicePassword, DEVICE_PASSWORD_MAX_LEN + 1);
  if (result != 0) {
    Serial.printf("Error reading device password from EEPROM: %d \n", result);
  }

  // Print the device password to the Serial Monitor
  Serial.printf("Device Password: '%s' \n", devicePassword);

  // Read the MQTT broker address from EEPROM
  result = eeprom.readMQTTAddress((char*)mqttBroker, MQTT_MAX_LEN + 1);
  if (result != 0) {
    Serial.printf("Error reading MQTT broker address from EEPROM: %d \n", result);
  }

  // Print the MQTT broker address to the Serial Monitor
  Serial.printf("MQTT Broker: '%s' \n", mqttBroker);


    //if MQTT_TOPIC is defined, use it, otherwise, build the topic from the device ID.
#ifndef MQTT_TOPIC
  sprintf(topic, "devices/%s/messages/events", deviceId);
#else
  sprintf(topic, MQTT_TOPIC);
#endif

  // Print the topic to the Serial Monitor
  Serial.printf("MQTT Topic: '%s' \n", topic);

  //Initialize the sensors
  sensorManager = new SensorManager();
  sensorManager->initSensors();
}

void loop() {
  
  // Initialize or reconnect the Wifi
  configureWifi();

  // Track previous state to detect changes
  static bool previousWaterFlowing = false;
  static bool firstRun = true;
  
  // Placeholder for water flow detection
  bool waterFlowing = false;

  // Read water flow sensor data
  sensorManager->readFlowDetection(&waterFlowing);
  Serial.printf("Water Flowing: %s \n", waterFlowing ? "true" : "false");

  // Print to OLED
  Screen.print(0, (String(WiFi.localIP().get_address())).c_str());
  Screen.print(1, "Water Flow");
  Screen.print(2, waterFlowing ? "FLOWING" : "IDLE");
  Screen.print(3, "");

  // Only send MQTT message if state changed or first run
  if (firstRun || waterFlowing != previousWaterFlowing) {
    Serial.println("Flow state changed, sending MQTT message...");
    
    // Send the message to the MQTT server
    int rc = sendMQTTMessage(&waterFlowing);

    //If the message send failed, flash the RGB LED red.  Otherwise, flash it green.
    if (rc != 0) {
      Serial.printf("Message send failed %d \n", rc);
      sensorManager->flashRGBLed(255,0,0);
      return;
    }
    Serial.println("Message sent to MQTT server successfully");
    sensorManager->flashRGBLed(0,255,0);
    
    // Update previous state
    previousWaterFlowing = waterFlowing;
    firstRun = false;
  }

  // Check frequently to detect state changes quickly
  delay(SLEEP_INTERVAL);
}

//Connects to the wifi network.
void configureWifi()
{
  // If the Wifi Status is not connected or the ip address is 0.0.0.0, then connect to the Wifi.
  while ((WiFi.status() != WL_CONNECTED) || (String(WiFi.localIP().get_address()) == "0.0.0.0"))
  {
    WiFi.begin();
  }
}

int sendMQTTMessage(bool* waterFlowing)
{
  

  MQTTNetwork mqttNetwork;
  MQTT::Client<MQTTNetwork, Countdown, MQTT_MAX_PACKET_SIZE> client = MQTT::Client<MQTTNetwork, Countdown, MQTT_MAX_PACKET_SIZE>(mqttNetwork);

  Serial.printf("Connecting to MQTT server %s:%d \n", mqttBroker, MQTT_PORT);

  //Connect to the MQTT broker.  You must convert the port to an integer.
  int rc = mqttNetwork.connect((const char*)mqttBroker, MQTT_PORT);
  if (rc != 0) {
    Serial.printf("Connected to MQTT server failed %d \n", rc);
    return rc;
  }

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.MQTTVersion = 4;
  data.clientID.cstring = (char*)deviceId;
  data.username.cstring = (char*)deviceId;
  data.password.cstring = (char*)devicePassword;

  if ((rc = client.connect(data)) != 0) {
    Serial.println("MQTT client connect to server failed");
    return rc;
  }

  Serial.printf("Connecting to MQTT topic %s \n", topic);

  // Get current time and format as yyyy/mm/dd hh:mm:ss:mmm
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  char dateTimeStr[25]; // yyyy/mm/dd hh:mm:ss:mmm\0
  int ms = millis() % 1000;
  snprintf(dateTimeStr, sizeof(dateTimeStr), "%04d/%02d/%02d %02d:%02d:%02d:%03d",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, ms);

  char buf[MQTT_MAX_PACKET_SIZE];
  sprintf(buf, "{\"device\":\"%s\",\"mac\":\"%s\",\"waterFlowing\":%s,\"deviceDateTime\":\"%s\"}", deviceId, WiFiInterface()->get_mac_address(), *waterFlowing ? "true" : "false", dateTimeStr);

  Serial.println(buf);

  MQTT::Message message;
  message.qos = MQTT::QOS0;
  message.retained = false;
  message.dup = false;
  message.payload = (void*)buf;
  message.payloadlen = strlen(buf);
  if (client.publish(topic, message) != 0) {
    Serial.printf("Message send failed %d \n", rc);
    return rc;
  }
  
  if ((rc = client.disconnect()) != 0) {
    Serial.printf("MQTT Client Disconnect failed %d \n", rc);
    return rc;
  }
  
  if ((rc = mqttNetwork.disconnect()) != 0) {
    Serial.printf("MQTT Network Disconnect failed %d \n", rc);
    return rc;
  }
}