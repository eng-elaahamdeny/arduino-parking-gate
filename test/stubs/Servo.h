#pragma once
#include <cstdint>
struct Servo { int pos = 0; void attach(int) {} void write(int p) { pos = p; } };
