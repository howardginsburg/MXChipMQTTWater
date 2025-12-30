#include "Arduino.h"
#include "SensorManager.h"

RGB_LED rgbLed;

// Constructor for SensorManager class
SensorManager::SensorManager() {
    // Define the I2C bus and the accelerometer sensor
    i2c = new DevI2C(D14, D15);
    motionSensor = new LSM6DSLSensor(*i2c, D4, D5);
}

// Initialize the sensors
void SensorManager::initSensors() {
    motionSensor->init(NULL);
    // Accelerometer only (no gyroscope needed)
}

// Detect water flow by analyzing accelerometer vibrations
void SensorManager::readFlowDetection(bool* isFlowing) {
    float sumAccel = 0.0f;
    int axes[3];
    
    // Collect multiple samples and calculate RMS of acceleration magnitude
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        motionSensor->getXAxes(axes);  // Get accelerometer data (X, Y, Z in mg)
        
        // Convert from mg to g
        float accelX = axes[0] / 1000.0f;
        float accelY = axes[1] / 1000.0f;
        float accelZ = axes[2] / 1000.0f;
        
        // Calculate magnitude of acceleration vector
        float magnitude = sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
        sumAccel += magnitude * magnitude;  // Sum of squares for RMS
        
        delayMicroseconds(1000);  // Small delay between samples
    }
    
    // Calculate RMS (root mean square) of acceleration magnitude
    float rmsAccel = sqrt(sumAccel / SAMPLE_COUNT);
    
    // Determine if water is flowing based on vibration threshold
    *isFlowing = (rmsAccel > flowThreshold);
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