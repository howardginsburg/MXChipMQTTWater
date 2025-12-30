# Water Flow Detection Plan - MXChip MQTT Water

## Overview
Implement binary water flow detection (flowing/idle) using the built-in LSM6DSL accelerometer on the MXChip mounted against the main water pipe. The accelerometer detects vibrations from flowing water and compares the vibration magnitude against a threshold.

## Architecture
- **Sensor**: LSM6DSL 6-axis accelerometer (already on board)
- **Detection Method**: RMS (root mean square) of acceleration magnitude
- **Output**: Boolean `waterFlowing` (true = flowing, false = idle)
- **MQTT Payload**: Simple JSON with device ID, MAC address, water flow status, and timestamp

## Implementation Steps

### 1. Modify `SensorManager.h`
**Changes**:
- Remove includes for `HTS221Sensor.h` and `LPS22HBSensor.h` (temperature, humidity, pressure)
- Keep only `LSM6DSLSensor.h` for accelerometer
- Remove method declarations: `readTempSensorData()`, `readPressureSensorData()`, `readGyroSensorData()`, `readButtonStates()`
- Add new method: `readFlowDetection(bool* isFlowing)`
- Add private members:
  - `float flowThreshold = 0.15f` (vibration threshold in g's - adjustable for calibration)
  - `static const int SAMPLE_COUNT = 50` (number of accelerometer samples per detection)

### 2. Modify `SensorManager.cpp`
**Changes**:
- Remove initialization of `tempSensor`, `pressureSensor` in constructor
- Remove all sensor init calls except `motionSensor->init(NULL)` in `initSensors()`
- Remove `enableGyroscope()` call
- Remove button GPIO setup
- Remove implementations of `readTempSensorData()`, `readPressureSensorData()`, `readGyroSensorData()`, `readButtonStates()`
- Add new `readFlowDetection()` method:
  - Collect 50 accelerometer samples
  - Get X, Y, Z axes in mg, convert to g (divide by 1000)
  - Calculate magnitude: `sqrt(x² + y² + z²)`
  - Calculate RMS: `sqrt(sum(magnitude²) / SAMPLE_COUNT)`
  - Return `isFlowing = (rmsAccel > flowThreshold)`
  - Use `getAAxes()` for accelerometer data (not gyro)

### 3. Modify `main.cpp`
**Changes**:
- Update `sendMQTTMessage()` signature: `int sendMQTTMessage(bool* waterFlowing)` (remove all other sensor parameters)
- Remove all sensor data placeholders and read calls (temperature, humidity, pressure, gyro, buttons)
- Add single flow detection call: `sensorManager->readFlowDetection(&waterFlowing)`
- Simplify OLED display to show: IP address, "Water Flow", and "FLOWING" or "IDLE"
- Update MQTT JSON payload to only include:
  ```json
  {
    "device": "deviceId",
    "mac": "MAC address",
    "waterFlowing": true/false,
    "deviceDateTime": "yyyy/mm/dd hh:mm:ss:mmm"
  }
  ```
- Simplify the `sendMQTTMessage()` function to only handle water flow data

## Calibration
After deployment:
1. Mount MXChip against the water pipe
2. Test with water flowing at normal flow rate
3. If detection is unreliable, adjust `flowThreshold` in `SensorManager.h`:
   - **Lower threshold** if water flows but is not detected (too sensitive)
   - **Raise threshold** if idle pipe shows as flowing (not sensitive enough)
4. Typical range: 0.1 - 0.3g depending on pipe diameter, material, and water pressure

## Notes
- Detection happens every 10 seconds (configurable via `SLEEP_INTERVAL`)
- Flow detection takes ~50ms (50 samples × 1ms delay)
- Provides simple yes/no detection, not volumetric flow rate
- No external hardware required - uses only built-in accelerometer
