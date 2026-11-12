#pragma once
#include <cstdint>
#include <cstring>
struct LiquidCrystal_I2C {
  char rows[2][17] = {"", ""}; int r = 0;
  LiquidCrystal_I2C(uint8_t, uint8_t, uint8_t) {}
  void init() {} void backlight() {}
  void setCursor(int, int row) { r = row; }
  void print(const char *s) { strncpy(rows[r], s, 16); rows[r][16] = 0; }
};
