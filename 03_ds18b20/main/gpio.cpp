#include "gpio.h"
#include "esp_err.h"

namespace Gpio {
// ========================= Реализация класса GPIO-выход ====================

// Внутренний метод инициализация пина с указанием инверсной логики
esp_err_t GpioOutput::_init(void) {

  esp_err_t status{ESP_OK};

  // Конфигурируем пин
  gpio_config_t cfg;
  cfg.pin_bit_mask = 1ULL << _pin; // Выставляем битовую маску пина
  cfg.mode = GPIO_MODE_OUTPUT; // Режим выхода
  cfg.pull_up_en = GPIO_PULLUP_DISABLE; // Отключаем подтягивающий резистор
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE; // Отключаем резистор на землю
  cfg.intr_type = GPIO_INTR_DISABLE; // Отключаем прерывания

  status |= gpio_config(&cfg);

  return status;
}
// Конструктор по умолчанию для GPIO-выхода с указанием инверсной логики
GpioOutput::GpioOutput(const gpio_num_t pin, const bool invert_logic)
    : BaseGpio(pin, invert_logic) {
  _init();
}

// Конструктор по умолчанию для GPIO-выхода без указания инверсной логики
GpioOutput::GpioOutput(const gpio_num_t pin) : BaseGpio(pin, false) { _init(); }

// Инициализация пина с указанием инверсной логики
esp_err_t GpioOutput::init(void) { return _init(); }

// Установить уровень выходного пина
esp_err_t GpioOutput::set_level(const bool level) {
  _level = _invert_logic ? !level : level;
  return gpio_set_level(_pin, _level);
}

// Переключить уровень выходного пина
esp_err_t GpioOutput::toggle(void) {
  _level = !_level;
  return gpio_set_level(_pin, _level);
}

// ========================= Реализация класса GPIO-вход ====================

} // namespace Gpio
