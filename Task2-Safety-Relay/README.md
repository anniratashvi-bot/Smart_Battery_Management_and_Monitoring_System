# Task 2 - Non-Blocking Protection Relay and Safety System

## Objective
The objective of this task was to develop a non-blocking battery protection system that can detect unsafe battery conditions, control a protection relay and recover safely after a fault is cleared.The implementation avoids blocking `delay()` calls and uses timer- and state-based logic so that battery monitoring, fault detection, LCD updates, telemetry and communication handling can continue while the protection system is active.The task also required anti-chatter protection, sensor anomaly detection, distinction between genuine rapid changes and abnormal readings and a timed relay recovery sequence.

## Relay State Design
The relay is controlled using three states:
```cpp
enum RELAYState
{
    RELAY_NORMAL,
    RELAY_TRIP,
    RELAY_RECOVERY
};
```
The relay therefore does not simply switch directly between ON and OFF. It follows a controlled sequence depending on the battery condition.
```text
NORMAL
   │
   │ Fault detected and remains active
   ▼
TRIP
   │
   │ Fault cleared and conditions are safe
   ▼
RECOVERY
   │
   │ Recovery timer completed
   ▼
NORMAL
```
If the fault returns while the relay is in `RELAY_RECOVERY`, the relay is returned to `RELAY_TRIP`.

## Non-Blocking Timing
The relay protection logic does not use `delay()` for debounce or recovery.
The important timing parameters are:
```cpp
const unsigned long relayDebounceTime = 2000;
const unsigned long relayRecoveryTime = 3000;
```
Elapsed time is checked using `millis()`:
```cpp
millis() - faultStartTime
```
This allows the rest of the BMS to keep running while the relay waits for its debounce or recovery period.

The same approach is used throughout the integrated system for other periodic operations.

## Anti-Chatter and Debounce

A fault is not used to immediately trip the relay on the first abnormal sample.

When a fault is first detected, a fault timer is started:

```text
Relay Fault Timer Started
```

The relay trips only when the fault remains present for the configured debounce period.

This prevents a brief transient condition from causing repeated relay switching.

The recovery path also uses a separate timer so that the relay does not immediately reconnect as soon as the fault disappears.

## Fault Detection

The protection system monitors several types of unsafe or abnormal conditions.

### Overvoltage

A cell is considered overvoltage when:

```cpp
cells[i].voltage > max_voltage
```

with:

```cpp
max_voltage = 4.20;
```

### Undervoltage

A cell is considered undervoltage when:

```cpp
cells[i].voltage < min_voltage
```

with:

```cpp
min_voltage = 2.5;
```

### Overtemperature

The system checks each cell temperature against:

```cpp
const float max_temperature = 45.0;
```

A cell exceeding this limit is marked with an overheat condition and fault.

## Sensor Anomaly Detection

The system does not treat every sudden change as a genuine battery fault. It also checks for sensor-related anomalies.

### Frozen Reading

A reading can be considered frozen when the simulated base voltage changes while the corresponding cell voltage changes very little for repeated cycles.

The system uses a counter:

```cpp
const int frozenLimit = 3;
```

A fault is raised after the required number of repeated detections instead of reacting to a single sample.

### Unrealistic Jump

A very large voltage change is treated as an unrealistic sensor jump.

```cpp
const float unrealisticJump = 0.80;
```

When such a change is detected, the corresponding cell is marked as having an unrealistic value and the fault information is updated.

### Sensor Noise and Genuine Rapid Change

The system also compares the change in the simulated base voltage against the change seen in each cell.

The configured noise threshold is:

```cpp
const float noiseLimit = 0.05;
```

A rapid cell change that follows the expected change in the battery input can be classified as a genuine rapid change rather than immediately being treated as a sensor failure.

This is important because a real battery load change can also produce a fast voltage transition.

### Out-of-Range Sensor Values

Cell values outside the allowed voltage range are also marked as abnormal.

```text
Voltage < 2.50 V  → Undervoltage / invalid condition
Voltage > 4.20 V  → Overvoltage / invalid condition
```

## Fault and Recovery Behavior

The relay checks both the fault condition and whether the battery is safe for recovery.

During recovery, conditions such as:

* Remaining battery faults
* Overheat
* Near-limit overvoltage
* Near-limit undervoltage
* Sensor anomalies

are checked before the relay is allowed to return to normal.

If a fault appears again during recovery, the relay immediately returns to the trip state.

This prevents an unsafe condition from causing premature relay reactivation.

## Safety Conditions Used for Recovery

The recovery logic uses additional margins around the main voltage and temperature thresholds.

For example, recovery is prevented when the cell remains too close to an unsafe limit:

```cpp
cells[i].temperature > clear_temperature
```

where:

```cpp
const float clear_temperature = 42.0;
```

Similarly, the recovery logic checks voltage margins before allowing the relay to return to normal.

This creates a separation between the fault threshold and recovery condition and reduces relay chatter around the boundary.

## Continuous Fault Injection

The safety logic was tested while the simulated battery condition was changed repeatedly.

The BMS continued running its monitoring and analysis functions while the relay protection logic responded to abnormal conditions.

The system continued to produce Serial Monitor output and did not rely on blocking delays to handle the fault sequence.

The state-based relay control therefore remains active while other BMS functions continue operating.

## Testing and Validation

Testing in Wokwi covered:

* Normal relay operation
* Battery overvoltage conditions
* Battery undervoltage conditions
* Overtemperature fault handling
* Relay debounce timing
* Relay trip behavior
* Fault removal
* Timed recovery
* Sensor frozen detection
* Unrealistic jump detection
* Out-of-range detection
* Genuine rapid-change classification
* Repeated fault conditions
* Continuous monitoring during protection events

## Development Environment

| Item                 | Details        |
| -------------------- | -------------- |
| Microcontroller      | ESP32          |
| Programming Language | C++            |
| Framework            | Arduino        |
| Simulation           | Wokwi          |
| Debugging            | Serial Monitor |
| Relay Control        | GPIO           |
| Relay Feedback       | GPIO           |

## Task Status

Task 2 is completed and integrated into the main BMS system. The protection relay, anomaly detection, debounce timing and recovery logic are reused by the subsequent state-machine and safety functions.
