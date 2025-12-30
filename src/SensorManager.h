#include "LSM6DSLSensor.h"
#include <RGB_LED.h>

class SensorManager {
public:
    SensorManager();
    void initSensors();
    void readFlowDetection(bool* isFlowing);
    void flashRGBLed(int red, int green, int blue);

private:
    DevI2C *i2c;
    LSM6DSLSensor* motionSensor;
    RGB_LED rgbLed;
    float flowThreshold = 0.15f;  // Vibration threshold in g's (0.15g is default, adjust based on calibration)
    static const int SAMPLE_COUNT = 50;  // Number of accelerometer samples to average
};