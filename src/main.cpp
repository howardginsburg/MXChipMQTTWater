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

//Sleep Interval for sending telemetry. If not defined, use the default value of 10 seconds.
#ifndef SLEEP_INTERVAL
  #define SLEEP_INTERVAL 10000
#endif

//SensorManager instance.
SensorManager* sensorManager;

void configureWifi();
int sendMQTTMessage(float* currentTemperature, float* currentHumidity, float* currentPressure, int* gyroX, int* gyroY, int* gyroZ, int* buttonAState, int* buttonBState);

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

  // Placeholders for current sensor data.
  float currentTemperature = 0;
  float currentHumidity = 0;
  float currentPressure = 0;
  int gyroX = 0;
  int gyroY = 0;
  int gyroZ = 0;
  int buttonAState = 0;
  int buttonBState = 0;

  // Read sensor data
  sensorManager->readTempSensorData(&currentTemperature, &currentHumidity);
  Serial.printf("Temperature: %.2f \n", currentTemperature);
  Serial.printf("Humidity: %.2f \n", currentHumidity);
  sensorManager->readPressureSensorData(&currentPressure);
  Serial.printf("Pressure: %.2f \n", currentPressure);
  sensorManager->readGyroSensorData(&gyroX, &gyroY, &gyroZ);
  Serial.printf("Gyro: X=%.2f, Y=%.2f, Z=%.2f\n", gyroX, gyroY, gyroZ);
  sensorManager->readButtonStates(&buttonAState, &buttonBState);
  Serial.printf("Button A State: %d \n", buttonAState);
  Serial.printf("Button B State: %d \n", buttonBState);

  // Print to OLED with scrolling info
  static int scrollIndex = 0;
  String lines[] = {
    "Temp: " + String(currentTemperature),
    "Humidity: " + String(currentHumidity),
    "Pressure: " + String(currentPressure),
    "Gyro X:" + String(gyroX),
    "Gyro Y:" + String(gyroY),
    "Gyro Z:" + String(gyroZ),
    "BtnA=" + String(buttonAState) + " BtnB=" + String(buttonBState)
  };
  const int totalLines = sizeof(lines) / sizeof(lines[0]);
  static bool showIP = true;
  if (showIP) {
    Screen.print(0, (String(WiFi.localIP().get_address())).c_str());
  } else {
    Screen.print(0, (String(WiFiInterface()->get_mac_address())).c_str());
  }
  for (int i = 0; i < 3; ++i) {
    int idx = (scrollIndex + i) % totalLines;
    Screen.print(i + 1, lines[idx].c_str());
  }
  scrollIndex = (scrollIndex + 1) % totalLines;
  showIP = !showIP;

  // Send the message to the MQTT server
  int rc = sendMQTTMessage(&currentTemperature, &currentHumidity, &currentPressure, &gyroX, &gyroY, &gyroZ, &buttonAState, &buttonBState);

  //If the message send failed, flash the RGB LED red.  Otherwise, flash it green.
  if (rc != 0) {
    Serial.printf("Message send failed %d \n", rc);
    sensorManager->flashRGBLed(255,0,0);
    return;
  }
  Serial.println("Message sent to MQTT server successfully");
  sensorManager->flashRGBLed(0,255,0);

  // Sleep for the configured interval.  
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

int sendMQTTMessage(float* currentTemperature, float* currentHumidity, float* currentPressure, int* gyroX, int* gyroY, int* gyroZ, int* buttonAState, int* buttonBState)
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
  sprintf(buf, "{\"device\":\"%s\",\"mac\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f,\"gyroX\":%d,\"gyroY\":%d,\"gyroZ\":%d,\"buttonA\":%d,\"buttonB\":%d,\"deviceDateTime\":\"%s\"}", deviceId, WiFiInterface()->get_mac_address(), *currentTemperature, *currentHumidity, *currentPressure, *gyroX, *gyroY, *gyroZ, *buttonAState, *buttonBState, dateTimeStr);

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