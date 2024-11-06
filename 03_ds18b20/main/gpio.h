#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

namespace Gpio {

class Gpio {
protected:
  const gpio_num_t _pin; // номер пина
  bool _invert_logic;    // инвертировать логику
  bool _level;           // уровень выходного пина
  bool _is_output;

  // Функция инициализации пина
  esp_err_t _mode_output(void);
  esp_err_t _mode_input(void);

public:
  // =================== Конструктор ====================
  Gpio(const gpio_num_t pin, const bool invert_logic);
  Gpio(const gpio_num_t pin);

  // =================== GPIO-выход =====================
  esp_err_t output(void);
  esp_err_t set_level(bool level);
  esp_err_t toggle(void);

  // ================== GPIO-вход =======================
  esp_err_t input(void);
  uint8_t read(void);
}; // class Gpio

} // namespace Gpio
