## Objective
The objective of this task was to develop an LCD display engine that updates only the information that has changed, avoiding unnecessary screen clearing and visible flickering.The display also needs to rotate automatically through multiple information pages using non-blocking timing. During a critical fault, the normal display must be overridden by a dedicated fault screen until the fault is explicitly cleared.

## Display Hardware

The LCD used in the Wokwi simulation is:

* 16x2 I2C LCD
* I2C address: `0x27`
* SDA: GPIO 21
* SCL: GPIO 22

The I2C interface is initialized using:

```cpp
Wire.begin(21, 22);
```

The LCD is created as:

```cpp
LiquidCrystal_I2C lcd(0x27, 16, 2);
```

## Display Pages

The normal display rotates through three pages.

### 1. Battery Status Page

The battery page provides an overview of the battery condition.

It displays:

* Average SoC
* Average SoH

Example:

```text
BATTERY STATUS
SOC:78% SOH:100%
```

### 2. System Status Page

The system page shows the relay condition, number of cells, and whether a cell fault is present.

Example:

```text
SYSTEM: NORMAL
CELLS:4 FAULT:NO
```

The relay status can also show:

```text
SYSTEM: TRIPPED
```

or:

```text
SYSTEM: RECOVERY
```

### 3. Telemetry Page

The telemetry page displays the minimum and maximum cell voltage together with voltage imbalance and average SoC.

Example:

```text
MIN:3.80 MAX:3.88
IMB:80mV SOC:78
```

## Non-Blocking Page Rotation

The LCD pages are rotated without using `delay()`.

The page rotation interval is:

```cpp
const unsigned long pageInterval = 3000;
```

The code checks elapsed time using `millis()`:

```cpp
if(currentTime-lastPageChange>=pageInterval)
{
    lastPageChange=currentTime;
    currentPage++;

    if(currentPage>=3)
        currentPage=0;
}
```

This allows the ESP32 to continue running the BMS analysis, protection logic, telemetry, and communication handling while the display changes pages.

## Flicker-Free Rendering

The display engine does not repeatedly clear and redraw the complete LCD.

Instead, it stores the previously displayed contents of each row:

```cpp
String previousRow0 = "";
String previousRow1 = "";
```

The `updateLCDRow()` function compares the new text with the previous text.

```cpp
if(text!=previousRow0)
{
    lcd.setCursor(0,0);
    lcd.print(text);
    previousRow0=text;
}
```

The same approach is used for the second row.

A row is therefore written only when its content changes. This reduces unnecessary LCD writes and avoids the visible flickering caused by repeatedly clearing and rewriting the display.

## Display Refresh Interval

The display update check uses:

```cpp
const unsigned long lcdRefreshInterval = 250;
```

The display is therefore checked every 250 ms, while the actual page changes every 3 seconds.

The refresh interval provides enough responsiveness for battery and fault information while avoiding unnecessary continuous LCD writes.

## Critical Fault Override

Critical battery and sensor faults have priority over the normal page rotation.

The function:

```cpp
checkLCDCriticalFault();
```

checks for conditions including:

* Overheat
* Overvoltage
* Undervoltage
* Frozen sensor
* Unrealistic sensor value

When a critical fault is detected, the normal pages are replaced by a dedicated fault screen.

Example:

```text
!! CRITICAL !!
CELL 1 OVERHEAT
```

Other fault messages can include:

```text
CELL 1 OVERVOLT
```

```text
CELL 1 UNDERVOLT
```

```text
CELL 1 SENSOR
```

```text
CELL 1 INVALID
```

The fault page remains active while `lcdFaultActive` is set.

## Explicit Fault Clearing

The LCD fault display is not allowed to disappear simply because the normal page timer expires.

The fault must be cleared using the explicit clear command:

```text
C
```

The `clearLCDFault()` function first checks that critical and battery faults are no longer active.

If a fault is still present, the system refuses to clear the display:

```text
Fault still active - cannot clear
```

This prevents the LCD from returning to normal while an unsafe condition still exists.

## Display Update Flow

The display logic can be summarized as:

```text
                Start LCD Update
                       │
                       ▼
               Critical fault?
                 /          \
               Yes           No
                │             │
                ▼             ▼
          Fault Display   Check page timer
                              │
                              ▼
                     Select current page
                              │
                              ▼
                     Update changed rows
```

A critical fault therefore has priority over normal page rotation.

## Testing and Validation

The LCD engine was developed and tested in Wokwi together with the rest of the BMS.

Testing covered:

* LCD initialization
* I2C communication
* Battery status page
* System status page
* Telemetry page
* Automatic page rotation
* Non-blocking refresh
* Row-change detection
* Flicker-free updates
* Critical fault override
* Fault message display
* Explicit fault clearing
* Continued LCD operation while other BMS functions are running

## Observed Operation

During normal operation, the LCD rotates between the three information pages without blocking the main BMS loop.

During a fault condition, the normal page is immediately replaced by the critical-fault display.

After the fault is cleared and the explicit clear command is issued, the LCD fault state is removed and normal page rotation resumes.

## Development Environment

| Item                 | Details      |
| -------------------- | ------------ |
| Microcontroller      | ESP32        |
| Programming Language | C++          |
| Framework            | Arduino      |
| Display              | 16x2 I2C LCD |
| I2C Address          | `0x27`       |
| SDA                  | GPIO 21      |
| SCL                  | GPIO 22      |
| Simulation           | Wokwi        |

## Task Status

Task 3 is completed and integrated into the main BMS system. The LCD engine continues to operate alongside the protection, state-machine, telemetry, and analytics functions without using blocking delays.
