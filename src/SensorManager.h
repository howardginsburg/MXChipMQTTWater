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
    float flowThreshold = 0.01f;  // Vibration threshold in g's (start low, adjust up if false positives)
    static const int SAMPLE_COUNT = 50;  // Number of accelerometer samples to average
};