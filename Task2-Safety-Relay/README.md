# Task 2 – Safety Relay and Fault Protection

## Objective

The objective of this task is to extend the Battery Management System with safety monitoring and relay protection. The system detects battery faults and sensor anomalies and controls the relay according to the battery safety condition.

## System Overview

The safety layer monitors the condition of each battery cell and checks for overvoltage, undervoltage, over-temperature and abnormal sensor behaviour. When a persistent fault is detected, the relay is tripped to disconnect the battery system. The relay also supports controlled recovery after the fault condition is cleared.

## Safety Parameters

* Maximum cell voltage: 4.20 V
* Minimum cell voltage: 2.50 V
* Maximum temperature: 45 °C
* Temperature recovery limit: 42 °C
* Relay debounce time: 2 seconds
* Relay recovery time: 3 seconds
* Number of monitored cells: 4

## Fault Detection

The BMS checks each cell for:

* Overvoltage
* Undervoltage
* Over-temperature
* Frozen sensor readings
* Unrealistic sensor jumps
* Sensor values outside the valid voltage range

The detected conditions are stored in the corresponding cell status and are also used by the relay protection logic.

## Relay State Machine

The relay operates using three states:

### RELAY_NORMAL

The relay remains active while the battery system is operating safely.

### RELAY_TRIP

When a fault persists beyond the debounce period, the relay is switched OFF to protect the battery.

### RELAY_RECOVERY

After the fault condition is cleared and the system is safe, the relay enters recovery mode. It returns to normal operation only after the recovery period is completed.

## Sensor Anomaly Detection

Sensor monitoring was added to distinguish between normal stationary readings and abnormal behaviour.

A stable potentiometer value with stable cell readings is treated as normal operation. A cell is considered frozen only when the battery input changes but the corresponding cell reading does not change.

The system also detects:

* Unrealistic voltage jumps
* Out-of-range voltage readings
* Genuine rapid changes caused by a changing input

## Testing

The safety and relay logic was tested in Wokwi using controlled input changes and simulated fault conditions.

The following conditions were verified:

1. Normal battery monitoring with no faults.
2. Persistent temperature fault causing relay trip.
3. Sensor anomaly detection.
4. Unrealistic voltage condition causing a fault.
5. Relay recovery after the fault condition is removed.
6. Final clean operation with all four cells reporting `Fault: 0`.

## Final Normal Operation

The final simulation was run without artificial fault injection. The system successfully monitored all four cells while keeping the relay in the normal state.

The final output showed:

* Relay State: NORMAL
* Four cells monitored
* Valid voltage and temperature readings
* SoC calculation for all cells
* Fault: 0 for all cells
* No false sensor anomaly detection

## Project Evidence

Screenshots from the Wokwi simulations are included in this folder to document the implemented safety and relay behaviour.

## Development Environment

* ESP32
* Arduino/C++
* Wokwi Simulator
* Serial Monitor
* GitHub

## Task Status

Task 2 – Safety Relay and Fault Protection completed and tested successfully.
