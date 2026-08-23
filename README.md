# Smart Battery Management and Monitoring System

This project is an ESP32-based Battery Management and Safety Monitoring System developed as part of my embedded systems internship.
The system focuses on monitoring battery cell conditions, detecting faults, handling safety conditions, and sending important battery information to a Blynk dashboard. The project was developed step by step with each task adding new functionality to the same BMS system.

## Project Overview
The system is built around an ESP32 and is intended to monitor the condition of a multi-cell battery while also handling protection and safety decisions.
The BMS continuously processes cell voltage data to calculate the average State of Charge (SoC) identify the weakest and strongest cells and determine the voltage imbalance between cells. It also monitors how the imbalance changes over time and adjusts the imbalance threshold according to the battery SoC instead of relying on one fixed limit, along with battery monitoring, the system includes a non-blocking protection relay, sensor anomaly detection, a four-state fault state machine and a 16x2 I2C LCD for local status information. Battery and system events are also sent to Blynk for remote monitoring.The Blynk dashboard extends the monitoring system with live telemetry, communication health, offline telemetry queuing, battery health indicators, fault statistics, risk analysis, historical data and maintenance recommendations.

## System Features

* **Battery monitoring:** Cell voltage, temperature, SoC, SoH, weakest and strongest cell identification, and voltage imbalance calculation.
* **Adaptive battery analysis:** Imbalance thresholds are adjusted according to battery SoC, while the change in imbalance is monitored over time.
* **Protection and fault handling:** Non-blocking relay control with debounce and recovery timing, along with detection of abnormal sensor readings and battery faults.
* **Fault state management:** `NORMAL`, `DEGRADED`, `FAILSAFE` and `SHUTDOWN` states with structured fault identification and transition logging.
* **Local LCD monitoring:** A 16x2 I2C display with rotating status, system and telemetry pages, along with a dedicated critical-fault display.
* **Event-driven telemetry:** Battery information is transmitted when meaningful changes occur instead of continuously sending unchanged data.
* **Offline telemetry queue:** Events are stored in a fixed-size queue during connectivity loss and replayed after the connection is restored.
* **Wi-Fi monitoring:** Connection state and RSSI are monitored to indicate communication health.
* **Blynk monitoring and analytics:** Live battery data, fault statistics, risk score, battery health, uptime, state transitions, historical trends and maintenance recommendations are displayed on the dashboard.

## Development Platform

### Controller

* ESP32

### Programming

* C++
* Arduino framework

### Simulation and Testing

* Wokwi
* Serial Monitor

### Display and Communication

* 16x2 I2C LCD
* Wi-Fi
* Blynk

### Main Interfaces Used

* ADC for battery voltage simulation
* GPIO for relay control and feedback
* I2C for LCD communication
* UART through the Serial Monitor for debugging and fault testing

The complete implementation was developed and tested in Wokwi. The potentiometer in the simulation is used to represent changing battery voltage conditions, allowing different operating and fault conditions to be tested without physical battery hardware.

                    Battery / Sensor Inputs
                            │
                            ▼
                  ┌─────────────────────┐
                  │   BMS Data Engine   │
                  │                     │
                  │ Voltage / SoC / SoH │
                  │ Cell Analysis       │
                  │ Imbalance Analysis  │
                  └──────────┬──────────┘
                             │
                ┌────────────┴────────────┐
                ▼                         ▼
       ┌─────────────────┐       ┌──────────────────┐
       │ Fault Detection │       │ System State     │
       │ & Relay Safety  │──────►│ State Machine    │
       └─────────────────┘       │ NORMAL           │
                                 │ DEGRADED         │
                                 │ FAILSAFE         │
                                 │ SHUTDOWN         │
                                 └────────┬─────────┘
                                          │
              ┌───────────────────────────┼──────────────────────────┐
              ▼                           ▼                          ▼
     ┌────────────────┐        ┌─────────────────┐        ┌──────────────────┐
     │   LCD Engine   │        │ Telemetry Engine│        │ Analytics Engine │
     │ 16x2 I2C LCD   │        │ Event Detection │        │ Risk / Health    │
     │ Status Pages   │        │ Offline Queue   │        │ Fault Statistics │
     │ Fault Display  │        │ Wi-Fi / RSSI    │        │ Recommendations  │
     └────────────────┘        └────────┬────────┘        └────────┬─────────┘
                                        │                          │
                                        └────────────┬─────────────┘
                                                     ▼
                                            ┌──────────────┐
                                            │ Blynk Cloud  │
                                            │ Live Data    │
                                            │ Analytics    │
                                            │ Trends       │
                                            │ Dashboard    │
                                            └──────────────┘
## Testing and Validation

The complete system was tested in Wokwi under normal operating conditions and during fault and communication scenarios.
The final demonstration includes:
``
NORMAL → DEGRADED → NORMAL

Connected → Network Outage → Offline Queue → Reconnection → Queue Replay
``
The tests were monitored using both the Serial Monitor and the Blynk dashboard.

## Limitations and Future Improvements

The current implementation was developed and validated in simulation. Future improvements could include testing with physical battery cells and dedicated battery-monitoring hardware, improving SoC estimation methods, and extending the system to larger battery packs.

