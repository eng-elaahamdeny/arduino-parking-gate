// Host-side logic test: scripted sensor readings drive the real sketch code.
// Build: g++ -I test/stubs -I parking_gate -x c++ test/sim_test.cpp -o sim && ./sim
#include "Arduino.h"
#include "../parking_gate/parking_gate.ino"

unsigned long fake_ms = 0;
SerialStub Serial;

struct Window { unsigned long a, b; int sensor; };      // sensor 0=entry 1=exit
static Window script[64]; static int nscript = 0;
static void car(int sensor, unsigned long a, unsigned long b) { script[nscript++] = {a, b, sensor}; }
void sensors_at(unsigned long t, int cm[2]) {
  cm[0] = cm[1] = -1;                                   // default: no echo at all
  for (int i = 0; i < nscript; i++)
    if (t >= script[i].a && t < script[i].b) cm[script[i].sensor] = 5;
}

static int failures = 0;
static void expect(const char *name, int got, int want) {
  printf("%-52s got=%d want=%d  %s\n", name, got, want, got == want ? "OK" : "FAIL");
  if (got != want) failures++;
}
static void run_until(unsigned long t) { while (fake_ms < t) loop(); }

int main() {
  setup();

  run_until(5000);
  expect("no echo for 5 s -> no false detection (slots)", freeSlots, 4);
  expect("no echo for 5 s -> gate still closed", gate.pos, GATE_CLOSED);

  car(0, 6000, 8000); car(1, 9000, 10500);             // one car enters
  run_until(20000);
  expect("1 car enters -> free slots", freeSlots, 3);
  expect("gate closed after entry", gate.pos, GATE_CLOSED);

  car(1, 21000, 23000); car(0, 24000, 25500);          // that car leaves
  run_until(35000);
  expect("car leaves -> free slots", freeSlots, 4);

  unsigned long t = 40000;
  for (int i = 0; i < 4; i++) { car(0, t, t + 1500); car(1, t + 2500, t + 4000); t += 15000; }
  run_until(t);
  expect("4 cars enter -> free slots", freeSlots, 0);

  car(0, t + 1000, t + 3000);                          // 5th car at entry
  run_until(t + 6000);
  expect("5th car refused -> gate stays closed", gate.pos, GATE_CLOSED);
  expect("5th car refused -> free slots", freeSlots, 0);

  t += 20000;
  car(1, t, t + 1500);                                 // car at exit side, then reverses away
  run_until(t + 6000 + PASS_TIMEOUT_MS);
  expect("car turns back (timeout) -> free slots unchanged", freeSlots, 0);
  expect("timeout -> gate closed again", gate.pos, GATE_CLOSED);

  printf(failures ? "\n%d test(s) FAILED\n" : "\nAll tests passed\n", failures);
  return failures ? 1 : 0;
}
