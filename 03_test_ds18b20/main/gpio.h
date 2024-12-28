#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
// #include "esp_event.h"

namespace Gpio {

class Gpio {
protected:
  const gpio_num_t _pin; // Mask for the pin
  bool _invert_logic;    // Invert logic
  bool _level;           // Output level
  bool _is_output;

  // Initialization of the GPIO pin in output/input mode
  esp_err_t _mode_output(void);
  esp_err_t _mode_input(void);

public:
  // =================== Constructor ====================
  Gpio(const gpio_num_t pin, const bool invert_logic);
  Gpio(const gpio_num_t pin);

  // =================== GPIO-output ====================
  esp_err_t output(void);
  esp_err_t set_level(bool level);
  esp_err_t toggle(void);

  // =================== GPIO-input =====================
  esp_err_t input(void);
  uint8_t read(void);
}; // class Gpio

} // namespace Gpio
