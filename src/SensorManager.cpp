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
    motionSensor->enableAccelerator();  // Enable accelerometer for readings
}

// Detect water flow by analyzing accelerometer vibrations
void SensorManager::readFlowDetection(bool* isFlowing) {
    float sumAccel = 0.0f;
    int axes[3];
    float avgX = 0.0f, avgY = 0.0f, avgZ = 0.0f;
    
    // First pass: collect samples and calculate average (baseline gravity)
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        motionSensor->getXAxes(axes);  // Get accelerometer data (X, Y, Z in mg)
        
        // Convert from mg to g
        avgX += axes[0] / 1000.0f;
        avgY += axes[1] / 1000.0f;
        avgZ += axes[2] / 1000.0f;
        
        delayMicroseconds(500);
    }
    
    // Calculate average acceleration (baseline includes gravity)
    avgX /= SAMPLE_COUNT;
    avgY /= SAMPLE_COUNT;
    avgZ /= SAMPLE_COUNT;
    
    // Second pass: measure deviation from baseline (pure vibration)
    sumAccel = 0.0f;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        motionSensor->getXAxes(axes);  // Get accelerometer data (X, Y, Z in mg)
        
        // Convert from mg to g
        float accelX = axes[0] / 1000.0f;
        float accelY = axes[1] / 1000.0f;
        float accelZ = axes[2] / 1000.0f;
        
        // Calculate deviation from baseline (removes gravity component)
        float devX = accelX - avgX;
        float devY = accelY - avgY;
        float devZ = accelZ - avgZ;
        
        // Calculate magnitude of vibration vector
        float magnitude = sqrt(devX*devX + devY*devY + devZ*devZ);
        sumAccel += magnitude * magnitude;  // Sum of squares for RMS
        
        delayMicroseconds(500);
    }
    
    // Calculate RMS (root mean square) of vibration magnitude
    float rmsAccel = sqrt(sumAccel / SAMPLE_COUNT);
    
    Serial.printf("RMS Vibration: %f g (baseline avg: %.2f g)\n", rmsAccel, sqrt(avgX*avgX + avgY*avgY + avgZ*avgZ));
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