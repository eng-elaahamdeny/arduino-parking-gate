# Arduino Parking Gate

Automatic car-park gate built around an Arduino UNO: two HC-SR04 ultrasonic sensors detect cars at the entry and the exit, an SG90 servo moves the barrier, and a 16x2 I2C LCD shows the number of free slots (4 by default).

## Credit and what is original here

The **idea, the component list and the pin layout** come from the tutorial by **Simple Circuits** (blog + YouTube channel):

- Video: *"How to make a car parking system using arduino and ultrasonic sensor | Arduino Project"*, published 18 April 2025: <https://www.youtube.com/watch?v=OF_rAykZSZI>
- Written post with materials and code: *"Car Parking System Using Arduino and Ultrasonic Sensor"* (17 April 2025): <https://simplecircuitslol.blogspot.com/2025/04/car-parking-system-using-arduino-and.html>

The tutorial does not state a license, so **I did not copy its code or text**. The sketch in this repo is my own rewrite, built as a small state machine, and the simulation test in `test/` is mine too. If you want the original build instructions and wiring diagram, please go to the video and post above.

### What I changed, and why
| Tutorial sketch | This repo |
|---|---|
| `pulseIn()` with no timeout: when no echo returns it gives 0, which is read as "0 cm = car present" (readers of the post report the gate opening with no car) | Timeout on `pulseIn()`; "no echo" is handled as *no car* |
| Single reading triggers the gate | Needs 2 consecutive readings below the threshold |
| Gate closes after a fixed 3 s delay and the slot count changes as soon as the gate opens | The 2nd sensor confirms the car actually passed; the count changes only then |
| Nothing if the car never drives through | 10 s timeout: gate closes, count unchanged |
| `delay(1000)` in the main loop, LCD rewritten every pass | Short loop, status text built from the current state |

## Hardware

| Part | Qty |
|---|---|
| Arduino UNO | 1 |
| HC-SR04 ultrasonic sensor | 2 |
| SG90 servo (gate) | 1 |
| 16x2 LCD with I2C backpack (address `0x27`) | 1 |
| Breadboard, jumper wires, capacitor across the servo supply | - |

| Signal | Arduino pin |
|---|---|
| Entry sensor TRIG / ECHO | 6 / 7 |
| Exit sensor TRIG / ECHO | 8 / 9 |
| Servo signal | 2 |
| LCD SDA / SCL | A4 / A5 |

## How it works

1. **Idle:** both sensors are polled; the LCD shows the free slots.
2. **Entry:** car at the entry sensor and at least one free slot -> gate opens. When the car has reached and then left the exit sensor, the gate closes and free slots decrease by 1. If the parking is full, the gate stays closed and the LCD says so.
3. **Exit:** car at the exit sensor first -> gate opens. When it has passed the entry sensor, the gate closes and free slots increase by 1.
4. **Safety:** if no car passes within 10 s, the gate closes without changing the count.

## Build and run

1. Install the **LiquidCrystal I2C** library (Library Manager in the Arduino IDE).
2. Open `parking_gate/parking_gate.ino`, select *Arduino Uno*, upload.
3. Tune `TOTAL_SLOTS`, `DETECT_CM` and the servo angles at the top of the sketch for your mechanics.

## Tests

The control logic is checked on a PC with scripted sensor readings (no hardware needed):

```bash
g++ -std=c++17 -I test/stubs -I parking_gate -x c++ test/sim_test.cpp -o sim && ./sim
```

Scenarios: no echo (no false detection), one car in and out, parking full, car that turns back (timeout).

## Status

- Control logic: tested in simulation (10 checks pass).
- Hardware test on a real board: **TODO - update this line after testing** (add a photo or video to `media/`).

## License

My code is released under the MIT License (see `LICENSE`). The tutorial it is inspired by remains the property of its author.
