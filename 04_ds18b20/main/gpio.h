#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_event.h" // IWYU pragma: keep
#include "esp_log.h" // IWYU pragma: keep

#define MASTER_RESET_PULSE_DURATION 490 // Reset time high. Reset time low.
#define RESPONSE_MAX_DURATION 60 // Presence detect high.
#define PRESENCE_PULSE_MAX_DURATION 240 // Presence detect low.
#define RECOVERY_DURATION 1 // Bus recovery time.
#define TIME_SLOT_START_DURATION 15 // Time slot start.
#define TIME_SLOT_DURATION 65 // Time slot.
#define ONE_BIT_DATA_DURATION 10 // Valid data duration.
#define ZERO_BIT_DATA_DURATION 65 // Valid data duration.

#define MATCH_ROM 0x55 // Match ROM command to address a specific 1-Wire device.
#define CONVERT_T 0x44 // Convert temperature command.
#define READ_ROM \
    0x33 // Read ROM command for read ROM on only one 1-Wire device.
#define READ_SCRATCHPAD 0xBE // Read Scratchpad command to read temperature.

#define esp_delay_us(x) os_delay_us(x) // Delay in microseconds max 65535 us

// static const char* TAG = "DS18B20"; // Tag for ESP_LOG

namespace OneWire {

class DS18B20 {
protected:
    // ================= Class variables ==================
    const gpio_num_t _pin; // Pin number
    bool _invert_logic { false }; // Invert logic
    gpio_config_t config; // Pin configuration
    bool _is_initialized { false };
    bool _level; // Output level
    bool _is_output;
    const char* TAG = "DS18B20";

    // Initialization of the GPIO pin in output/input mode
    esp_err_t _init_one_wire_gpio(void);

public:
    // =================== Constructor ====================
    DS18B20(const gpio_num_t pin);
    DS18B20(const gpio_num_t pin, const bool invert_logic);

    // ================ GPIO-mode & level +================
    esp_err_t pin_direction(gpio_mode_t direction);
    esp_err_t set_level(const bool level);
    uint8_t get_pin_level(void);

    // =================== 1-Wire =========================
    esp_err_t reset(void);
    void write_bit(uint8_t bit);
    void write_byte(uint8_t byte);
    uint8_t read_bit(void);
    uint8_t read_byte(void);
    esp_err_t readROM(void);
    esp_err_t match_rom(uint8_t (&address)[8]);
    esp_err_t get_temp(uint8_t (&address)[8], float& temperature);

}; // class Gpio

} // namespace OneWire
