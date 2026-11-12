// Minimal host-side stand-in for Arduino.h, just enough to run the sketch logic.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#define F(x) x
#define INPUT 0
#define OUTPUT 1
#define HIGH 1
#define LOW 0
extern unsigned long fake_ms;
void sensors_at(unsigned long t_ms, int cm[2]);   // defined by the test
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline void delayMicroseconds(unsigned) {}
inline void delay(unsigned long ms) { fake_ms += ms; }
inline unsigned long millis() { return fake_ms; }
inline unsigned long pulseIn(uint8_t echo, uint8_t, unsigned long) {
  int cm[2]; sensors_at(fake_ms, cm);
  int v = (echo == 7) ? cm[0] : cm[1];
  return v < 0 ? 0 : (unsigned long)(v * 2 / 0.034);   // 0 = no echo (timeout)
}
struct SerialStub {
  void begin(int) {}
  void print(const char *s) { if (verbose) fputs(s, stdout); }
  void print(int v) { if (verbose) printf("%d", v); }
  void println(const char *s) { if (verbose) puts(s); }
  void println(int v) { if (verbose) printf("%d\n", v); }
  bool verbose = false;
};
extern SerialStub Serial;
