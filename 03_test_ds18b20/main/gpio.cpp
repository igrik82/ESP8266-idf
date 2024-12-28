#include "gpio.h"
#include "esp_err.h"

namespace Gpio {
// Constructor by default for GPIO
Gpio::Gpio(const gpio_num_t pin, const bool invert_logic)
    : _pin{pin}, _invert_logic{invert_logic} {};
Gpio::Gpio(const gpio_num_t pin) : _pin{pin}, _invert_logic{false} {};

// ========================= GPIO-output ===============================

// Initialization of the GPIO pin in output mode
//
esp_err_t Gpio::_mode_output(void) {

  esp_err_t status{ESP_OK};

  // Configuration of the GPIO pin
  gpio_config_t cfg;
  cfg.pin_bit_mask = 1ULL << _pin;          // Bit mask for the pin
  cfg.mode = GPIO_MODE_OUTPUT;              // Mode output
  cfg.pull_up_en = GPIO_PULLUP_DISABLE;     // Disable pull-up resistor
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down resistor
  cfg.intr_type = GPIO_INTR_DISABLE;        // Disable interrupt

  status |= gpio_config(&cfg);
  _is_output = true;

  return status;
};

esp_err_t Gpio::output(void) { return _mode_output(); };

// Set the output level
esp_err_t Gpio::set_level(const bool level) {
  if (_is_output) {
    _level = _invert_logic ? !level : level;
    return gpio_set_level(_pin, _level);
  }
  return ESP_FAIL;
}

// Toggle the output level.
esp_err_t Gpio::toggle(void) {
  if (_is_output) {
    _level = !_level;
    return gpio_set_level(_pin, _level);
  }
  return ESP_FAIL;
}

// ========================= GPIO-input ===============================
esp_err_t Gpio::_mode_input(void) {

  esp_err_t status{ESP_OK};

  // Configuration of the GPIO pin.
  gpio_config_t cfg;
  cfg.pin_bit_mask = 1ULL << _pin;          // Bit mask for the pin
  cfg.mode = GPIO_MODE_INPUT;               // Mode input
  cfg.pull_up_en = GPIO_PULLUP_DISABLE;     // Disable pull-up resistor
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down resistor
  cfg.intr_type = GPIO_INTR_DISABLE;        // Disable interrupt

  status |= gpio_config(&cfg);
  _is_output = false;

  return status;
};

// Initialization of the GPIO pin in input mode
esp_err_t Gpio::input(void) { return _mode_input(); };

// Read the input level.
// If it's output, return ESP_FAIL.
uint8_t Gpio::read(void) {
  if (_is_output) {
    return ESP_FAIL;
  }
  return _invert_logic ? !gpio_get_level(_pin) : gpio_get_level(_pin);
};
} // namespace Gpio
