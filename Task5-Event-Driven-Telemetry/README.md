## Objective
The objective of this task was to develop an event-driven telemetry system that sends battery information when a meaningful change occurs instead of continuously transmitting unchanged data.The system also needs to continue operating during Wi-Fi or Blynk connectivity loss. Telemetry events must be stored in a fixed-size offline queue and transmitted in the correct order once communication is restored.
The dashboard should provide live battery information together with relay status, fault state, communication health and offline queue information.

## Event-Driven Telemetry Design

The telemetry engine uses a `TelemetryEvent` structure to collect the information required for remote monitoring.

```cpp
struct TelemetryEvent
{
    uint32_t eventId;
    unsigned long timestamp;
    float cellVoltage[MAX_CELLS];
    uint8_t weakestCell;
    uint8_t strongestCell;
    bool relayNormal;
    bool faultActive;
    SystemState systemState;
    FaultSource faultSource;
    FaultID faultID;
    int rssi;
};
```

Every generated event receives a unique event ID and a timestamp.

## Meaningful Event Detection

The system compares the current telemetry event with the previous meaningful event before transmitting it.

A new event can be generated when there is a significant change in:

* Cell voltage
* Weakest cell
* Strongest cell
* Relay status
* Fault state
* System state
* Fault source
* Fault ID

The cell-voltage event threshold is:

```cpp
const float CELL_VOLTAGE_EVENT_THRESHOLD = 0.05;
```

This corresponds to a 50 mV change.

If none of the monitored parameters changes meaningfully, no new telemetry event is transmitted.

This reduces unnecessary repeated data transmission while still allowing important battery and system changes to reach the dashboard.

## Offline Telemetry Queue

When the telemetry link is unavailable, meaningful events are stored locally.

The queue is fixed in size:

```cpp
const int TELEMETRY_QUEUE_SIZE = 10;
```

The implementation uses:

```cpp
int queueHead = 0;
int queueTail = 0;
int queueCount = 0;
```

to maintain the queue.

Events are inserted at the tail and removed from the head, providing FIFO ordering.

The queue therefore preserves the order in which offline events occurred.

## Queue Full Handling

The queue has a fixed memory limit.

When the queue reaches its maximum capacity, a new event is not allowed to overwrite an older stored event.

The system reports:

```text
Telemetry queue full - new event discarded
```

This keeps the memory requirement bounded and avoids silently replacing earlier telemetry events.

## Wi-Fi Connection State Machine

Wi-Fi connectivity is handled using three states:

```cpp
enum WiFiState
{
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED
};
```

The state is updated continuously using `millis()`.

The retry interval is:

```cpp
const unsigned long wifiRetryInterval = 5000;
```

The system therefore does not block the BMS while waiting for a Wi-Fi connection.

Blynk reconnection is also handled separately through periodic connection attempts.

## Communication Health and RSSI

When Wi-Fi is connected, the ESP32 reads the signal strength using `WiFi.RSSI()`.

The system classifies the communication condition as:

|                 RSSI | Health  |
| -------------------: | ------- |
| Greater than -60 dBm | GOOD    |
| Greater than -75 dBm | FAIR    |
|     -75 dBm or lower | WEAK    |
|                 -127 | OFFLINE |

The RSSI value and health classification are available to the telemetry/dashboard layer.

## Blynk Live Dashboard

The telemetry engine publishes live information to Blynk.

The main telemetry datastreams include:

| Datastream | Information           |
| ---------- | --------------------- |
| V0–V3      | Cell 1–4 voltage      |
| V4         | Weakest cell          |
| V5         | Strongest cell        |
| V6         | Relay status          |
| V7         | Fault state           |
| V8         | System state          |
| V9         | Wi-Fi RSSI            |
| V10        | Offline queue depth   |
| V11        | Data Source           |
| V12        | Telemetry event ID    |

The dashboard therefore provides both battery information and communication status.

## Live vs Queued Data

The telemetry system distinguishes between data generated while the BMS is connected and events that were stored during an outage.

During normal communication:

```text
Meaningful event detected
Live event sent
```

During a network outage:

```text
Meaningful event detected
Telemetry queued
Offline Queue Depth: 1
```

This allows the operator to see whether the dashboard is receiving current live telemetry or whether data is waiting to be transmitted.

## Network Outage Demonstration

The project includes a serial command for testing communication loss:

```text
N
```

When the command is enabled:

```text
Network outage test: ENABLED
```

The system disconnects Wi-Fi and Blynk for the test.

A meaningful battery change is then introduced using the potentiometer in Wokwi.

For example:

```text
Event reason: CELL 1
Meaningful event detected | Event:19
Telemetry queued | Event:19 | Depth:1
```

Additional meaningful changes can produce:

```text
Telemetry queued | Event:20 | Depth:2
Telemetry queued | Event:21 | Depth:3
```

The corresponding dashboard queue depth increases accordingly.

## Queue Replay After Reconnection

The network test is disabled using the same `N` command.

The system then starts the Wi-Fi reconnection process:

```text
Network outage test: DISABLED
WiFi state: CONNECTING
```

After communication is restored:

```text
Blynk: CONNECTED
```

Queued events are replayed in FIFO order.

Example:

```text
Queued event sent | Event:19 | RSSI:-93 | Queue:3
```

The queue is reduced as events are transmitted.

When all stored events have been replayed:

```text
Offline queue cleared
```

and the queue depth returns to zero.

## Telemetry Flow

The complete telemetry flow is:

```text
Battery / System Change
          │
          ▼
Meaningful Event Check
          │
     ┌────┴────┐
     │         │
   Online    Offline
     │         │
     ▼         ▼
 Live Send   Queue Event
     │         │
     │         ▼
     │      FIFO Queue
     │         │
     └────┬────┘
          ▼
      Reconnection
          │
          ▼
      Queue Replay
          │
          ▼
     Blynk Dashboard
```

## Testing and Validation

The telemetry system was tested in Wokwi under both normal and network-loss conditions.

Testing covered:

* Normal live telemetry
* Meaningful cell-voltage changes
* Event generation
* Prevention of unnecessary repeated transmission
* Wi-Fi disconnection
* Forced network outage
* Offline telemetry storage
* Multiple queued events
* Queue depth monitoring
* FIFO replay
* Wi-Fi reconnection
* Blynk reconnection
* Queue clearing
* RSSI monitoring
* Live and queued data indication

## Observed Network-Outage Test

During the successful outage test, the system produced:

```text
Network outage test: ENABLED

Event reason: CELL 1
Meaningful event detected | Event:19
Telemetry queued | Event:19 | Depth:1
```

Further battery changes generated additional queued events:

```text
Telemetry queued | Event:20 | Depth:2
Telemetry queued | Event:21 | Depth:3
```

The system remained in normal program execution while the queue accumulated events.

After communication was restored:

```text
Blynk: CONNECTED
Queued event sent | Event:19 | RSSI:-93 | Queue:3
```

The queue was then cleared after replay.

This demonstrates that meaningful telemetry is retained during a network outage and transmitted after reconnection rather than being lost immediately.

## Development Environment

| Item                 | Details        |
| -------------------- | -------------- |
| Microcontroller      | ESP32          |
| Programming Language | C++            |
| Framework            | Arduino        |
| Simulation           | Wokwi          |
| Cloud Platform       | Blynk          |
| Connectivity         | Wi-Fi          |
| Debugging            | Serial Monitor |

## Task Status

Task 5 is completed and integrated into the main BMS system. The telemetry engine provides event-driven data transmission, offline buffering, non-blocking reconnection, RSSI monitoring, and live Blynk monitoring.
