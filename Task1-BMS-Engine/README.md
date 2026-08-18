# Task 1 - Modular Battery Management Engine

## Objective

The objective of this task was to develop a modular Battery Management System (BMS) engine for monitoring multiple battery cells. The design was created with scalability in mind so that the number of cells can be changed without rewriting the complete monitoring logic.

## System Overview

The BMS engine maintains information for each battery cell and processes the cell data through a common data structure. The implementation was developed using C++ in the Arduino environment and tested using ESP32 simulation in Wokwi.

The current implementation uses a compile-time cell limit:

```cpp
const int MAX_CELLS = 4;
```

The cell information is maintained using a `Cell` structure and an array of cells:

```cpp
Cell cells[MAX_CELLS];
```

This approach keeps the battery data organized and allows the same monitoring logic to be used for different battery configurations.

## Cell Parameters

The `Cell` structure contains the parameters used by the BMS engine:

* Cell ID
* Cell voltage
* Cell temperature
* Fault status
* Fault type
* State of Health (SoH)
* State of Charge (SoC)

Each cell is initialized before the monitoring process begins.

## Functions Implemented

The Task 1 implementation focuses on the basic modular BMS engine and battery-cell data management.

The implemented functionality includes:

* Multiple-cell configuration
* Cell data storage using a structure
* Cell initialization
* Cell voltage monitoring
* Cell temperature monitoring
* Cell fault status tracking
* Fault type identification
* State of Charge (SoC) information
* State of Health (SoH) information
* Serial monitoring output

## Scalable Design

The number of battery cells is controlled using the `MAX_CELLS` constant.

For example:

```cpp
const int MAX_CELLS = 4;
```

The same architecture can be extended by changing the cell configuration and initializing the required number of cells.

This makes the design easier to expand for larger battery packs without creating separate variables for every individual cell.

## Testing

The BMS engine was tested in the Wokwi simulation environment.

The initial implementation was tested using a 4-cell configuration. The cell data was initialized and monitored through the serial output.

A larger cell configuration was also tested to verify that the data structure and monitoring logic could be extended beyond the initial configuration.

## Simulation Output

The serial monitor was used to verify that the BMS engine was initialized correctly and that battery monitoring was running.

Example startup output:

```text
BMS Started!
Monitoring Battery...
```

The simulation output was used as evidence during development and testing.

## Development Environment

* Microcontroller: ESP32
* Programming Language: C++
* Simulation Platform: Wokwi
* Development Environment: Arduino

## Project Evidence

Testing screenshots will be added to the `screenshots` directory after the corresponding Wokwi tests are completed.

## Wokwi Simulation

Wokwi simulation link:

**To be added after the final simulation is saved and shared.**

## Task Status

Task 1 focuses on establishing the modular BMS engine that will serve as a foundation for the remaining battery management and monitoring functions developed during the internship.
