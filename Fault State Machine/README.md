## Objective
The objective of this task was to implement a deterministic fault state machine for the BMS with four operating states:

* `NORMAL`
* `DEGRADED`
* `FAILSAFE`
* `SHUTDOWN`

The system must identify the source and type of faults, maintain this information during operation, log every state transition with timing information and use a controlled verification process before recovering from `FAILSAFE`.

## State Machine Design

The BMS uses an enum to define the four operating states:

```cpp
enum SystemState
{
    STATE_NORMAL,
    STATE_DEGRADED,
    STATE_FAILSAFE,
    STATE_SHUTDOWN
};
```

The corresponding dashboard state codes are:

| Code | State    |
| ---: | -------- |
|    0 | NORMAL   |
|    1 | DEGRADED |
|    2 | FAILSAFE |
|    3 | SHUTDOWN |

Keeping the states in an enum makes the state machine deterministic and avoids using unrelated numeric values throughout the program.

## State Behavior

### NORMAL

`STATE_NORMAL` is the normal operating condition.

A battery-related fault can move the system to:

```text
NORMAL → DEGRADED
```

A critical fault can move the system directly to:

```text
NORMAL → FAILSAFE
```

### DEGRADED

`STATE_DEGRADED` is used when a battery fault is present but the system can continue operating under degraded conditions.

If the battery fault clears:

```text
DEGRADED → NORMAL
```

If a critical fault occurs:

```text
DEGRADED → FAILSAFE
```

### FAILSAFE

In `STATE_FAILSAFE`, the system keeps the protection relay in the safe/tripped condition.

The system does not immediately return to `NORMAL` when the fault disappears. Instead, it starts a recovery verification period.

### SHUTDOWN

`STATE_SHUTDOWN` is the final protective state used when a critical fault remains active for the configured shutdown period.

In this state, the relay remains tripped.

## Fault Source Classification

The system identifies where a fault originated using a separate enum:

```cpp
enum FaultSource
{
    FAULT_NONE,
    FAULT_BATTERY,
    FAULT_RELAY,
    FAULT_COMMUNICATION,
    FAULT_ADC
};
```

This separates the source of a problem from the system operating state.

## Fault Identification

The specific fault is stored using another enum:

```cpp
enum FaultID
{
    FAULT_ID_NONE,
    FAULT_ID_BATTERY_CELL,
    FAULT_ID_RELAY_MISMATCH,
    FAULT_ID_COMMUNICATION_LOSS,
    FAULT_ID_ADC_FROZEN
};
```

The active fault information is maintained using:

```cpp
FaultSource activeFaultSource;
FaultID activeFaultID;
uint8_t activeFaultCell;
```

This allows the system to record which type of fault occurred and, when relevant, which cell was affected.

## Fault Detection

The state machine receives fault information from several detection paths.

### Battery Fault

Battery faults are identified from cell conditions such as:

* Overvoltage
* Undervoltage
* Overtemperature
* Sensor-related abnormal values

A battery fault can cause:

```text
NORMAL → DEGRADED
```

### ADC Frozen Fault

The system includes a dedicated ADC freeze test.

The ADC monitoring logic detects repeated frozen readings and assigns:

```text
FAULT_ID_ADC_FROZEN
```

The test can be enabled and disabled through the Serial Monitor using:

```text
F
```

### Relay Mismatch Fault

The actual relay feedback is compared with the expected relay output state.

A mismatch counter is used so that a single incorrect feedback reading does not immediately create a system fault.

The resulting fault ID is:

```text
FAULT_ID_RELAY_MISMATCH
```

### Communication Fault

The system can monitor communication heartbeats.

When communication monitoring is active and a heartbeat is not received for the configured timeout:

```cpp
const unsigned long communicationTimeout = 10000;
```

the communication fault is identified as:

```text
FAULT_ID_COMMUNICATION_LOSS
```

## State Transition Logging

Every state transition is logged in a structured format.

The logged information includes:

* Timestamp
* Previous state
* New state
* Fault source
* Fault ID
* Affected cell when applicable

Example:

```text
STATE_TRANSITION |
TIME:52898 |
PREVIOUS:NORMAL |
NEW:DEGRADED |
SOURCE:BATTERY |
FAULT:BATTERY_CELL |
CELL:1
```

The transition count is also maintained:

```cpp
int stateTransitionCount = 0;
```

This count is displayed on the Blynk analytics dashboard.

The same state transition is also sent to Blynk as:

```text
state_transition
```

## Deterministic Transition Flow

The main transition behavior can be summarized as:

```text
                    ┌───────────────┐
                    │    NORMAL     │
                    └───────┬───────┘
                            │
               Battery fault│Critical fault
                            │
                 ┌──────────┴──────────┐
                 ▼                     ▼
        ┌────────────────┐     ┌────────────────┐
        │   DEGRADED     │     │    FAILSAFE    │
        └───────┬────────┘     └───────┬────────┘
                │                      │
        Fault cleared           Verification passed
                │                      │
                ▼                      ▼
        ┌────────────────┐     ┌────────────────┐
        │    NORMAL      │     │    NORMAL      │
        └────────────────┘     └────────────────┘

                    FAILSAFE
                       │
             Critical fault persists
                       │
                       ▼
                  SHUTDOWN
```

The actual transition is controlled by the current state and the currently detected fault conditions rather than by unrestricted state changes.

## FAILSAFE Recovery Verification

The FAILSAFE recovery process uses:

```cpp
const unsigned long failsafeVerificationTime = 5000;
```

When the critical fault clears, the system first starts recovery verification:

```text
FAILSAFE Recovery Verification Started
```

During this period the relay remains in the safe condition.

Only after the required verification time has elapsed and the required fault conditions remain clear does the system return to `NORMAL`.

After successful verification, the system logs:

```text
FAILSAFE Recovery Verification Passed
```

This prevents an immediate return to normal operation after a short-lived fault disappearance.

## Shutdown Protection

If a critical fault remains active in `FAILSAFE` beyond the shutdown timeout:

```cpp
const unsigned long shutdownTimeout = 15000;
```

the system transitions to:

```text
FAILSAFE → SHUTDOWN
```

The relay is kept tripped in the shutdown state.

## Recovery and Reset

A separate reset command is supported through:

```text
R
```

A reset from `SHUTDOWN` is accepted only when there are no remaining critical or battery faults.

This prevents the system from being manually returned to `NORMAL` while an unsafe condition is still active.

## Testing and Validation

The state machine was tested in the Wokwi simulation together with the battery, relay, LCD, telemetry, and analytics functions.

Testing covered:

* NORMAL operation
* Battery fault injection
* NORMAL → DEGRADED
* DEGRADED → NORMAL
* Critical fault handling
* FAILSAFE operation
* FAILSAFE recovery verification
* Persistent critical fault handling
* SHUTDOWN transition
* ADC frozen detection
* Relay mismatch monitoring
* Communication timeout monitoring
* Structured transition logging
* State transition count

## Development Environment

| Item                 | Details        |
| -------------------- | -------------- |
| Microcontroller      | ESP32          |
| Programming Language | C++            |
| Framework            | Arduino        |
| Simulation           | Wokwi          |
| Debugging            | Serial Monitor |
| State Representation | C++ `enum`     |
| Relay Control        | GPIO           |
| Relay Feedback       | GPIO           |

## Task Status

Task 4 is completed and integrated into the main BMS system. The state machine provides the operating-state control used by the protection, telemetry, LCD, and Blynk analytics functions.
