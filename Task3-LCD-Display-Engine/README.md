### Objective

To develop an optimized LCD display engine for the BMS that reduces unnecessary display updates and visible flickering. The LCD should automatically rotate between different information pages using a non-blocking timer and should immediately display a dedicated fault screen whenever a critical battery fault is detected.

### Implementation

A 16x2 I2C LCD was integrated with the existing ESP32-based BMS. The LCD was programmed to display three different information pages:

1. Battery Status – displays average State of Charge (SoC) and State of Health (SoH).
2. System State – displays the relay state, number of cells and fault status.
3. Telemetry – displays minimum and maximum cell voltage, voltage imbalance and average SoC.

The pages are automatically changed every 3 seconds using `millis()`. This allows the display to rotate between pages without using blocking delays.

To reduce unnecessary LCD operations, the previous contents of both LCD rows are stored. A row is rewritten only when its displayed value changes. This reduces unnecessary screen updates and helps prevent visible flickering.

A critical fault override was also implemented. When a serious fault such as overheat, overvoltage, undervoltage or a sensor anomaly is detected, the normal LCD pages are stopped and a dedicated critical fault screen is displayed.

### Critical Fault Testing

The critical fault functionality was tested in the Wokwi simulation by temporarily setting the cell temperature to `50.0°C`. The configured maximum temperature is `45.0°C`, so this caused an overheat condition.

The LCD immediately switched to the critical fault display and showed:

`!! CRITICAL !!`

`CELL 1 OVERHEAT`

After completing the test, the temperature was restored to `25.0°C` and normal LCD operation was verified.

### Refresh Interval Justification

A LCD refresh interval of `250 ms` was selected because the information displayed by the BMS does not need extremely fast visual updates. A 250 ms interval provides sufficiently responsive information while avoiding unnecessary repeated writes to the LCD. The page rotation interval of 3 seconds provides enough time for each page to be read before changing to the next page.

### Result

The LCD successfully displayed all three information pages and automatically rotated between them. The display was updated without continuously clearing the screen, reducing unnecessary updates and visible flickering. The critical fault override was also successfully tested using the simulated overheat condition.
