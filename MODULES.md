# Firmware modules

The sketch is split into include modules to keep Arduino's single-translation-unit behavior while making the firmware easier to maintain.

- `esp32_vault_panel.ino` - configuration, BME280/I2C addresses, global state, `setup()`, and the FreeRTOS runtime tasks.
- `module_ui_helpers.h` - shared text labels, CRT backdrop helpers, and small drawing helpers.
- `module_relay_io.h` - relay GPIO output.
- `module_persistence.h` - Preferences storage for pressure history, MQ burn-in, sample log state, and the last failsafe fault code.
- `module_forecast_core.h` - pressure trend regression and forecast readiness.
- `module_time_sync.h` - Wi-Fi/NTP synchronization.
- `module_environment_logic.h` - light, MQ-135, absolute humidity, predictive outside-temperature cooling, thermal, and pressure state machines.
- `module_ventilation.h` - ventilation decision and relay cycle guard.
- `module_sensors.h` - mock sensors, real BME280 reads, analog EMA reads, validation, retry, and conversion helpers.
- `module_display.h` - boot animation, main screen, forecast screen, and PC-link animation.
- `module_controls.h` - physical button, power state, theme, and manual mode handling.
- `module_serial_bridge.h` - JSON-over-Serial PC bridge protocol, command acks, catch-up dump, and PC standby/manual controls.

Keep hardware-specific pin constants and feature flags in `esp32_vault_panel.ino`.

## Runtime split

The firmware uses two pinned FreeRTOS tasks:

1. `soch-control` on core 1, priority 4: sensor readout, environment state machines, failsafe, ventilation decision, and relay GPIO output.
2. `soch-bridge-ui` on core 0, priority 2: Serial protocol, history dump, offline log persistence, physical button handling, time sync polling, TFT refresh, and PC-link animations.

`loop()` only feeds the Arduino task watchdog and sleeps. The control task is also subscribed to the ESP task watchdog, so a blocked UI/Serial path should not silently stop the safety loop. LittleFS archive writes are intentionally kept out of the control task.

## Summer intake ventilation profile

The automatic relay logic in `module_ventilation.h` follows this priority order:

1. Manual mode, with emergency outside-temperature limits.
2. MQ-135 bad air override. `MQ_SUMMER_BAD_RAW_ON = 1000` is converted to the firmware AQI scale before comparison.
3. Humidity lock: intake is blocked only when outside absolute humidity is higher than room absolute humidity.
4. Morning overcool lock: room temperature at or below `ROOM_COOL_OFF` blocks intake.
5. Heat lock: outside air at or above room temperature blocks intake.
6. Predictive night hold: if outside temperature drops by at least `PREDICTIVE_TEMP_DROP_C_PER_HOUR` over roughly an hour and the room is already near `PREDICTIVE_ROOM_HOLD_C`, normal intake is held off to avoid overcooling.
7. Cooling/night intake: room above `ROOM_HOT_ON`, outside at least `COOLING_DELTA_C` colder than room, and outside air not wetter by absolute humidity.

Relay cycle protection still applies after the decision, so non-urgent transitions cannot chatter the relay.
