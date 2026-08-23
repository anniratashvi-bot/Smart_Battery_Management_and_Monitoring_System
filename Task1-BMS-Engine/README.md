# Task 1 - Modular Battery Management Engine

## Objective
The objective of this task was to develop a modular Battery Management System (BMS) engine for monitoring multiple battery cells using a scalable architecture. The number of cells should be controlled from a single compile-time constant without requiring the monitoring logic to be rewritten for each battery configuration.
The engine is responsible for identifying the weakest and strongest cells, calculating cell voltage imbalance, monitoring the imbalance trend over time and using an adaptive imbalance threshold based on battery State of Charge (SoC). The calculated battery information is also made available to other parts of the BMS so that the same analysis does not have to be duplicated.

## Design Overview
The implementation was developed in C++ using the Arduino framework and tested with an ESP32 in Wokwi.
The number of cells is controlled using a single compile-time constant:
```cpp
const int MAX_CELLS = 4;
```
The cell data is stored in an array:
```cpp
Cell cells[MAX_CELLS];
```
This allows the same monitoring and analysis loops to operate on the configured number of cells.

## Cell Data Structure
The BMS uses a `Cell` structure to keep the parameters associated with each battery cell together.
```cpp
struct Cell
{
    uint8_t cell_id;
    float voltage;
    float temperature;
    bool fault;
    bool Overvoltage;
    bool Undervoltage;
    bool Overheat;
    bool sensorFrozen;
    bool unrealisticValue;
    bool sensorNoise;
    bool genuineRapidChange;
    float soh;
    float soc;
};
```
Each cell is initialized before the monitoring process begins.

## Weakest and Strongest Cell Detection
The BMS compares the voltage of all configured cells during each monitoring cycle.The cell with the highest voltage is identified as the strongest cell while the cell with the lowest voltage is identified as the weakest cell.
The resulting values are stored in the common analysis structure:
```cpp
analysis.strongestCell
analysis.weakestCell
analysis.maximumVoltage
analysis.minimumVoltage
```
This allows other parts of the system to use the same calculated values.

## Voltage Imbalance Calculation
The voltage imbalance is calculated from the difference between the strongest and weakest cell voltages:
```text
Voltage Imbalance = Maximum Cell Voltage - Minimum Cell Voltage
```
The calculated imbalance is stored in:
```cpp
analysis.voltageImbalance
```

## Imbalance Trend Monitoring
The BMS stores the previous imbalance value and compares it with the current value.

```text
Imbalance Change =Current Imbalance - Previous Imbalance
```
The result is used to identify whether the imbalance is:
* Increasing
* Decreasing
* Stable

## Adaptive Imbalance Threshold
A fixed imbalance limit is not used for the complete operating range.
Instead, the threshold changes according to the average battery SoC:
| Average SoC      | Adaptive Threshold |
| ---------------- | -----------------: |
| Greater than 80% |              50 mV |
| 50% to 80%       |             100 mV |
| 50% or below     |             150 mV |
The adaptive threshold is stored in:
```cpp
analysis.adaptiveThreshold
```
and used to determine:

```cpp
analysis.imbalanceWarning
```
## Reusable Analysis Interface
The BMS exposes the common battery information through a small interface instead of recalculating the same values in every part of the system.

```cpp
const BMSAnalysis& getBMSAnalysis();
const Cell* getCellData();
int getCellCount();
```
The `BMSAnalysis` structure contains the calculated information:
```cpp
struct BMSAnalysis
{
    uint8_t weakestCell;
    uint8_t strongestCell;
    float minimumVoltage;
    float maximumVoltage;
    float voltageImbalance;
    float previousImbalance;
    float imbalanceChange;
    bool imbalanceIncreasing;
    float averageSOC;
    float adaptiveThreshold;
    bool imbalanceWarning;
};
```
This common interface is reused by the LCD, telemetry, state machine, and analytics sections of the complete BMS.

## Scalability from 4 Cells to 16 Cells
The current working configuration uses:
```cpp
const int MAX_CELLS = 4;
```
The same architecture can be configured for a 16-cell battery pack by changing the constant to:
```cpp
const int MAX_CELLS = 16;
```
The same `for` loops are used to initialize and process the configured cells. No separate variables such as `cell1`, `cell2`, `cell3`, and so on are required.

### Data Structure Scaling
The cell data is stored as an array of `Cell` structures.
Conceptually:
```text
4-cell configuration  → 4 Cell structures
16-cell configuration → 16 Cell structures
```
Therefore, memory used for per-cell information increases approximately linearly with the number of cells.
The main structure of the program does not need to change when the cell count is increased.

### Execution-Time Scaling
The main battery-processing operations iterate through all configured cells. Therefore, the processing cost scales approximately linearly with the number of cells.
In simplified form:
```text
Processing complexity ≈ O(N)
```
where `N` is the number of configured cells.Moving from 4 to 16 cells therefore increases the amount of per-cycle processing, but the monitoring logic remains the same.

### Design Benefit
The important benefit of this approach is that the battery size is represented as configuration rather than as separate application logic.
The same,can be reused for different battery pack sizes:
* Cell data structure
* Initialization logic
* Monitoring loops
* Cell comparison logic
* Imbalance calculation
* Analysis interface

## Testing and Validation
The testing covered:
* Initialization of the cell data structure
* Multi-cell configuration
* Cell voltage monitoring
* Strongest cell detection
* Weakest cell detection
* Voltage imbalance calculation
* Imbalance trend calculation
* Adaptive threshold selection
* Reuse of calculated analysis data by the integrated system
* Extension of the cell configuration beyond the initial 4-cell setup
The 4-cell configuration is the main working configuration used in the integrated BMS. The larger cell-count configuration was used to verify that the same array-based design can be extended without rewriting the core monitoring logic.

## Evidence

The task evidence includes:

* 4-cell BMS configuration
* Extended cell-count configuration
* Serial Monitor output
* Strongest and weakest cell results
* Voltage imbalance results
* Adaptive threshold output
* Wokwi simulation

Screenshots and supporting files can be found in the task evidence directory.

## Wokwi Simulation
Wokwi project:https://wokwi.com/projects/471623147263209473

## Task Status
Task 1 is completed and its BMS analysis functionality is reused by the subsequent protection, display, state-machine, telemetry and analytics features of the complete system.
