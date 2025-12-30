#include "HTS221Sensor.h"
#include "LPS22HBSensor.h"
#include "LSM6DSLSensor.h"
#include <RGB_LED.h>

class SensorManager {
public:
    SensorManager();
    void initSensors();
    void readTempSensorData(float* currentTemperature, float* currentHumidity);
    void readPressureSensorData(float* currentPressure);
    void readGyroSensorData(int* gyroX, int* gyroY, int* gyroZ);
    void flashRGBLed(int red, int green, int blue);
    void readButtonStates(int* buttonAState, int* buttonBState);

private:
    DevI2C *i2c;
    HTS221Sensor* tempSensor;
    LPS22HBSensor* pressureSensor;
    LSM6DSLSensor* motionSensor;
    RGB_LED rgbLed;
};