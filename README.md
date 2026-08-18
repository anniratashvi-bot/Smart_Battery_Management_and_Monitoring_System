# Smart Battery Management and Monitoring System

An ESP32-based battery management and safety monitoring system developed during my embedded systems internship.

The project is being developed and tested in Wokwi. The system is designed as a modular platform so that battery monitoring, protection, display, state control and cloud monitoring can be developed as separate modules and integrated into one system.

## Project Overview

The system focuses on monitoring battery cell conditions and detecting conditions that may affect battery safety or performance.

The main functions include:

- Cell voltage monitoring
- Temperature monitoring
- State of Charge (SoC) estimation
- State of Health (SoH) tracking
- Strongest and weakest cell detection
- Cell voltage imbalance calculation
- Imbalance trend monitoring
- Adaptive imbalance threshold based on SoC
- Cell fault detection
- Safety control
- LCD-based information display
- Blynk-based remote monitoring

## Development Platform

**Controller**
- ESP32

**Development and Simulation**
- Wokwi
- Arduino framework
- C/C++

**Monitoring**
- Serial Monitor
- Blynk

## Project Tasks

The internship project is being developed through the following modules:

1. Modular Battery Management Engine
2. Safety Relay
3. LCD Engine
4. Battery State Machine
5. Blynk Dashboard
6. Blynk Dashboard Integration

## Current Progress

### Task 1 - Modular Battery Management Engine

The first module implements a scalable battery analysis engine.

The number of cells is controlled using the compile-time constant:

```cpp
const int MAX_CELLS =4;
### Commit message

Use:

```text
Update project documentation and overview
