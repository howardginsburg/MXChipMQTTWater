#include "Arduino.h"
#include "SensorManager.h"

RGB_LED rgbLed;

// Constructor for SensorManager class
SensorManager::SensorManager() {
    // Define the I2C bus and the sensors
    i2c = new DevI2C(D14, D15);
    tempSensor = new HTS221Sensor(*i2c);
    pressureSensor = new LPS22HBSensor(*i2c);
    motionSensor = new LSM6DSLSensor(*i2c, D4, D5);

}

// Initialize the sensors
void SensorManager::initSensors() {
    tempSensor->init(NULL);
    pressureSensor->init(NULL);
    motionSensor->init(NULL);
    motionSensor->enableGyroscope();

    pinMode(USER_BUTTON_A, INPUT);
    pinMode(USER_BUTTON_B, INPUT);
}

// Read the temperature and humidity sensor data
void SensorManager::readTempSensorData(float* currentTemperature, float* currentHumidity) {
    tempSensor->enable();
    tempSensor->getHumidity(currentHumidity);
    
    tempSensor->getTemperature(currentTemperature);
    *currentTemperature = *currentTemperature * 1.8 + 32;
    
    tempSensor->disable();
    tempSensor->reset();
}

// Read the pressure sensor data
void SensorManager::readPressureSensorData(float* currentPressure) {
    pressureSensor->getPressure(currentPressure);
}

// Read the gyroscope sensor data
void SensorManager::readGyroSensorData(int* gyroX, int* gyroY, int* gyroZ) {
    //motionSensor->enableGyroscope();
    int axes[3];
    motionSensor->getGAxes(axes);
    *gyroX = axes[0];
    *gyroY = axes[1];
    *gyroZ = axes[2];
    //motionSensor->disableGyroscope();
}

void SensorManager::flashRGBLed(int red, int green, int blue) {
    rgbLed.setColor(red, green, blue);
    delay(1000);
    rgbLed.turnOff();
    delay(500);
    rgbLed.setColor(red, green, blue);
    delay(1000);
    rgbLed.turnOff();
}

void SensorManager::readButtonStates(int* buttonAState, int* buttonBState) {
    *buttonAState = !digitalRead(USER_BUTTON_A);
    *buttonBState = !digitalRead(USER_BUTTON_B);
}