#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"
#include "gpio.h"

class Main final {
public:
    esp_err_t setup(void);
    void loop(void);
    Gpio::Gpio led { GPIO_NUM_2, true };
};
