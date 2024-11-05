#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include <cstdint>

namespace Gpio {

// ================= Базовый класс GPIO ====================
class BaseGpio {
protected:
  const gpio_num_t _pin;      // номер пина
  bool _invert_logic = false; // инвертировать логику

public:
  BaseGpio(const gpio_num_t pin, const bool invert_logic)
      : _pin{pin}, _invert_logic{invert_logic} {}

  virtual esp_err_t init(void) = 0;
}; // class BaseGpio

// ================== Класс GPIO-выход =====================
class GpioOutput : public BaseGpio {
private:
  bool _level = false; // уровень выходного пина
  // Функция инициализации пина
  esp_err_t _init(const gpio_num_t pin, const bool invert_logic);

public:
  GpioOutput(const gpio_num_t pin, const bool invert_logic);

  GpioOutput(const gpio_num_t pin);

  // Необходимо инициализировать объект до использования.
  // Удобно если нужно позволить другому классу использовать этот объект.
  // TODO:
  // NOTE:
  // WARNING:
  // FIX: нужно подумать как возвращать
  GpioOutput(void);

  esp_err_t init(void);
  // esp_err_t init(const gpio_num_t pin);
  esp_err_t set_level(bool level);
  esp_err_t toggle(void);

}; // class GpioOutput

// ================== Класс GPIO-вход ======================
class GpioInput : public BaseGpio {
private:
  esp_err_t _init(const gpio_num_t pin, const bool invert_logic);

public:
  GpioInput(const gpio_num_t pin, const bool invert_logic);
  GpioInput(const gpio_num_t pin);
  GpioInput(void);

  esp_err_t init(void);
  // esp_err_t init(const gpio_num_t pin);
  uint8_t read(void);
}; // class GpioInput

} // namespace Gpio
