# MXChip MQTT Water Flow Monitor

A smart water flow detection system using the Microsoft Azure IoT Developer Kit (MXChip AZ3166) that monitors water flow through pipes using vibration analysis and reports state changes via MQTT.

## Overview

This project transforms the MXChip into a non-invasive water flow sensor by mounting it against a water pipe. The built-in LSM6DSL accelerometer detects vibrations caused by flowing water, providing binary flow state detection (FLOWING/IDLE) without requiring any modifications to the plumbing.

### Key Features

- **Non-invasive detection** - No need to cut or modify pipes
- **Real-time monitoring** - Checks flow state every 2 seconds
- **Efficient communication** - Only sends MQTT messages when flow state changes
- **Visual feedback** - OLED display shows current state and IP address
- **Status indicators** - RGB LED flashes green on successful transmission, red on failure
- **WiFi connectivity** - Connects to your network and MQTT broker automatically
- **Configurable threshold** - Adjust sensitivity for different pipe sizes and flow rates

## Hardware Requirements

- **MXChip AZ3166** (Microsoft Azure IoT Developer Kit)
- **Mounting mechanism** - Velcro strap, clamp, or bracket to secure device against pipe
- **WiFi network** - 2.4GHz WiFi for connectivity
- **MQTT broker** - Local or cloud-based MQTT server

## How It Works

1. **Vibration Detection**: The LSM6DSL accelerometer samples vibrations 50 times per measurement
2. **RMS Calculation**: Calculates root mean square (RMS) of acceleration magnitude across X, Y, Z axes
3. **Threshold Comparison**: Compares RMS value against configurable threshold (default: 0.15g)
4. **State Detection**: Determines FLOWING if RMS > threshold, IDLE otherwise
5. **State Change Publishing**: Sends MQTT message only when state transitions occur
6. **Visual Feedback**: Updates OLED display and flashes LED to indicate status

## MQTT Message Format

```json
{
  "device": "mxchip-water-01",
  "mac": "AA:BB:CC:DD:EE:FF",
  "waterFlowing": true,
  "deviceDateTime": "2025-12-31T14:23:45.123Z"
}
```

## Getting Started

### Prerequisites

1. **PlatformIO** - Install [PlatformIO IDE](https://platformio.org/install) or PlatformIO Core
2. **VS Code** (recommended) - With PlatformIO extension
3. **MXChip USB Drivers** - Install STM32 drivers for your OS

### Building and Uploading

1. **Clone the repository**
   ```bash
   git clone <your-repo-url>
   cd MXChipMQTTWater
   ```

2. **Open in VS Code**
   ```bash
   code .
   ```

3. **Build the project**
   - Press `Ctrl+Alt+B` or use PlatformIO: Build task
   - Or run: `platformio run --environment mxchip_az3166`

4. **Upload to device**
   - Connect MXChip via USB
   - Press `Ctrl+Alt+U` or use PlatformIO: Upload task
   - Or run: `platformio run --target upload --environment mxchip_az3166`

5. **Monitor serial output**
   - Press `Ctrl+Alt+S` or use PlatformIO: Serial Monitor
   - Baud rate: 115200

6. **Configure WiFi and MQTT**

    Press Reset + A to enter configuration mode. Before first use, configure the device with your WiFi and MQTT credentials using the MXChip configuration tool:

    - **Device ID** - Unique identifier for your device
    - **Device Password** - MQTT authentication password
    - **MQTT Broker Address** - Hostname or IP of your MQTT server
    - **WiFi Credentials** - Configured through MXChip WiFi setup

### Physical Installation

1. **Mount the MXChip** against the water pipe using a secure mounting method:
   - Velcro straps (easiest)
   - Pipe clamps with bracket
   - 3D-printed mounting bracket
   
2. **Optimal placement**:
   - Mount on a straight section of pipe (not near bends or valves)
   - Ensure firm contact between device and pipe
   - Position with screen visible for monitoring
   
3. **Power**: Connect via USB to power adapter or power bank

### Calibration

The default vibration threshold is **0.01g**. Adjust if needed:

1. Open `src/SensorManager.h`
2. Modify the `flowThreshold` value:
   ```cpp
   float flowThreshold = 0.01f;  // Adjust this value
   ```
3. **Lower values** (0.01g) = More sensitive (may trigger on small vibrations)
4. **Higher values** (0.03g) = Less sensitive (requires stronger flow)
5. Rebuild and upload after changes

Test calibration by:
- Observing serial output while water flows
- Checking OLED display state matches actual flow
- Verifying MQTT messages align with flow events

## Configuration Options

### Build-Time Configuration

Configure in `platformio.ini` using build flags:

```ini
[env:mxchip_az3166]
build_flags = 
    -DSLEEP_INTERVAL=2000     ; Check interval in milliseconds
    -DMQTT_PORT=1883          ; MQTT broker port
    -DMQTT_TOPIC="custom/topic"  ; Custom MQTT topic
```

### Runtime Configuration

- **SLEEP_INTERVAL**: Flow state check interval (default: 2000ms)
- **flowThreshold**: Vibration sensitivity (default: 0.15g in SensorManager.h)
- **SAMPLE_COUNT**: Number of accelerometer samples per check (default: 50)

## Troubleshooting

### No Flow Detection

- **Symptom**: Display shows IDLE even when water is flowing
- **Solutions**:
  - Lower the threshold value in `SensorManager.h`
  - Ensure device is firmly mounted against pipe
  - Try mounting at a different location
  - Check serial monitor for RMS values during flow

### False Positives

- **Symptom**: Display shows FLOWING when no water is running
- **Solutions**:
  - Raise the threshold value
  - Isolate from environmental vibrations (footsteps, traffic)
  - Use firmer mounting to reduce resonance


