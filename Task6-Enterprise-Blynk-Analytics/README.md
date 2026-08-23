## Objective
The objective of this task was to extend the Blynk dashboard beyond basic live telemetry and provide a higher-level view of battery condition.
The dashboard combines live battery information with calculated risk, battery health, fault statistics, historical trends, maintenance recommendations, uptime and system-state information so that the operator can understand the current condition of the battery without relying only on raw sensor values.

## Analytics Data

The analytics engine calculates and publishes the following information:

* Risk score
* Average SoC
* Voltage imbalance
* Fault frequency
* Uptime
* Maintenance recommendation
* Battery health
* Total fault count
* Current system state
* State transition count
* Maintenance recommendation text

Analytics are updated periodically using:

```cpp
const unsigned long analyticsInterval = 5000;
```

This separates the analytics update rate from the faster BMS monitoring loop.

## Composite Risk Score

The risk score combines three main factors:

* Voltage imbalance risk
* Fault frequency risk
* State of Charge risk

The weighted calculation used by the implementation is:

```text
Risk Score =(40% × Imbalance Risk) +(30% × Fault Risk) +(30% × SoC Risk)
```

### Imbalance Risk

Voltage imbalance is compared against the reference value used by the analytics engine.

An increasing imbalance also increases the calculated imbalance risk.

### Fault Risk

Fault frequency is converted into a normalized risk contribution.

The system uses the number of faults observed within the configured analysis window.

### SoC Risk

The SoC contribution is based on how far the average battery SoC is from 100%.

The final score is constrained to the range:

```text
0 to 100
```

Higher values indicate a greater calculated risk.

## Battery Health

Battery health is derived from the risk score:

```text
Battery Health = 100 - Risk Score
```

The value is limited to remain within the `0–100%` range.

For critical states, additional minimum or fixed risk values are applied so that the dashboard reflects the severity of FAILSAFE and SHUTDOWN conditions.

## Fault History and Frequency

The system maintains a fault history using a fixed-size time buffer.

```cpp
const unsigned long faultFrequencyWindow = 3600000UL;
const int MAX_FAULT_HISTORY = 20;
```

The one-hour window is used to determine the number of recent fault occurrences.

Each recorded fault stores its `millis()` timestamp.

The resulting value is published as fault frequency:

```text
Fault Frequency = number of recorded faults within the analysis window
```

The system also maintains a cumulative fault count:

```cpp
int totalFaultCount = 0;
```

A new fault occurrence increments this count and can also generate a Blynk event.

## Structured Fault Events

When a new fault is recorded, the system creates a descriptive event containing information such as:

* Fault source
* Fault ID
* Affected cell, when applicable

Example:

```text
Fault: BATTERY | ID: BATTERY_CELL | Cell: 1
```

This creates a structured fault history instead of displaying only a generic fault indication.

## Maintenance Recommendation Engine

The analytics engine converts the calculated battery condition into an operator-oriented recommendation.

The recommendation levels are:

| Code | Recommendation               |
| ---: | ---------------------------- |
|    0 | Battery operating normally   |
|    1 | Monitor battery condition    |
|    2 | Maintenance recommended      |
|    3 | Immediate attention required |

The decision uses factors such as:

* Current system state
* Risk score
* Fault frequency
* Voltage imbalance trend
* Average SoC

The numerical recommendation and the human-readable recommendation text are both published to Blynk.

## Historical Trends

The Blynk dashboard provides historical views for selected battery and system values.

The historical information can be used to observe changes in:

* Risk score
* Battery health
* SoC
* Voltage imbalance
* Fault-related activity
* Other selected dashboard parameters

This makes it possible to see how the battery condition changes over time rather than looking only at the latest reading.

## State Machine Integration

Task 6 uses the same state codes as the Task 4 backend state machine:

| Code | State    |
| ---: | -------- |
|    0 | NORMAL   |
|    1 | DEGRADED |
|    2 | FAILSAFE |
|    3 | SHUTDOWN |

The dashboard therefore reflects the state reported by the backend instead of using an independent dashboard-only state.

The state transition count is also published:

```cpp
int stateTransitionCount = 0;
```

This allows the dashboard to indicate that the system has changed operating states during the demonstration.

## Severity Visualization

The dashboard uses the state and risk information to distinguish different levels of operating severity.

The state machine provides the main operating condition:

```text
NORMAL
DEGRADED
FAILSAFE
SHUTDOWN
```

The analytics layer adds the calculated risk and recommendation so that the operator can see both:

* what state the BMS is currently in
* why the battery may require attention

The exact visual representation can use the configured Blynk widget colors, icons, indicators, or status widgets.

## Executive Summary

The dashboard provides a higher-level summary using:

* Battery Health
* Uptime
* Total Fault Count
* Current System State
* Risk Score
* Average SoC
* Maintenance Recommendation

This gives an operator a quick overview before looking at individual cell values or detailed telemetry.

## Backend State Reflection

The dashboard is connected to values generated by the same backend BMS implementation.

For example, when a battery fault was injected during testing, the backend reported:

```text
STATE_TRANSITION |
PREVIOUS:NORMAL |
NEW:DEGRADED |
SOURCE:BATTERY |
FAULT:BATTERY_CELL |
CELL:1
```

The analytics values changed at the same time, including:

```text
Risk Score: 50.3 %
Battery Health: 49.7 %
Fault Frequency: 1.0 faults/hour
Total Fault Count: 1
System State: DEGRADED
State Transition Count: 1
```

When the fault was cleared the backend recorded:

```text
PREVIOUS:DEGRADED
NEW:NORMAL
```

and the dashboard state returned to `NORMAL`.

This demonstrates that the dashboard is reflecting the backend state-machine output rather than displaying static values.

## Uptime

The dashboard also displays system uptime.

The uptime is calculated from `millis()`:

```text
Uptime (minutes) = millis() / 60000
```

The displayed value therefore represents the elapsed runtime of the ESP32 simulation.

During testing, the Serial Monitor and dashboard were checked to confirm that the uptime value increased with continued runtime.

## Testing and Validation

The analytics dashboard was tested using normal operation, changing battery conditions, fault injection, state transitions, and network-loss scenarios.

Testing covered:

* Risk score calculation
* Battery health calculation
* Average SoC display
* Voltage imbalance display
* Fault frequency
* Total fault count
* Maintenance recommendations
* Uptime
* State transition count
* State reflection in Blynk
* Historical dashboard behavior
* Network recovery interaction
* Executive summary information

## Development Environment

| Item                 | Details        |
| -------------------- | -------------- |
| Microcontroller      | ESP32          |
| Programming Language | C++            |
| Framework            | Arduino        |
| Simulation           | Wokwi          |
| Cloud Platform       | Blynk          |
| Dashboard            | Blynk          |
| Debugging            | Serial Monitor |

## Task Status

Task 6 is completed and integrated into the main BMS system. The analytics layer combines live backend data, calculated battery health indicators, fault information, recommendations and system-state information into the final Blynk decision dashboard.
