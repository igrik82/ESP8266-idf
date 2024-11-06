#include "gpio.h"
#include "esp_err.h"

namespace Gpio {
// Конструктор по умолчанию для GPIO
Gpio::Gpio(const gpio_num_t pin, const bool invert_logic)
    : _pin{pin}, _invert_logic{invert_logic} {};
Gpio::Gpio(const gpio_num_t pin) : _pin{pin}, _invert_logic{false} {};

// ========================= Реализация GPIO-выход ====================

// Функция инициализации пина
esp_err_t Gpio::_mode_output(void) {

  esp_err_t status{ESP_OK};

  // Конфигурируем пин
  gpio_config_t cfg;
  cfg.pin_bit_mask = 1ULL << _pin; // Выставляем битовую маску пина
  cfg.mode = GPIO_MODE_OUTPUT; // Режим выхода
  cfg.pull_up_en = GPIO_PULLUP_DISABLE; // Отключаем подтягивающий резистор
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE; // Отключаем резистор на землю
  cfg.intr_type = GPIO_INTR_DISABLE; // Отключаем прерывания

  status |= gpio_config(&cfg);
  _is_output = true;

  return status;
};

esp_err_t Gpio::output(void) { return _mode_output(); };

// Установить уровень выходного пина
esp_err_t Gpio::set_level(const bool level) {
  if (_is_output) {
    _level = _invert_logic ? !level : level;
    return gpio_set_level(_pin, _level);
  }
  return ESP_FAIL;
}

// Переключить уровень выходного пина
esp_err_t Gpio::toggle(void) {
  if (_is_output) {
    _level = !_level;
    return gpio_set_level(_pin, _level);
  }
  return ESP_FAIL;
}

// ========================= Реализация GPIO-вход =====================
esp_err_t Gpio::_mode_input(void) {

  esp_err_t status{ESP_OK};

  // Конфигурируем пин
  gpio_config_t cfg;
  cfg.pin_bit_mask = 1ULL << _pin; // Выставляем битовую маску пина
  cfg.mode = GPIO_MODE_INPUT; // Режим выхода
  cfg.pull_up_en = GPIO_PULLUP_DISABLE; // Отключаем подтягивающий резистор
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE; // Отключаем резистор на землю
  cfg.intr_type = GPIO_INTR_DISABLE; // Отключаем прерывания

  status |= gpio_config(&cfg);
  _is_output = false;

  return status;
};

// Инициализация пина в режиме GPIO-входа
esp_err_t Gpio::input(void) { return _mode_input(); };

// Чтение уровня входного пина
uint8_t Gpio::read(void) {
  // WARNING: Нет проверки на состояние выходного пина
  return _invert_logic ? !gpio_get_level(_pin) : gpio_get_level(_pin);
};
} // namespace Gpio
