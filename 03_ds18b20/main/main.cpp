#include "main.h"
#include "esp_log.h"
#define CONFIG_LOG_SET_LEVEL ESP_LOG_VERBOSE
#define LOG_TAG "MAIN"
#define pdMILISECONDS pdMS_TO_TICKS

static Main my_main;

extern "C" void app_main(void) {
  ESP_ERROR_CHECK(my_main.setup());

  while (true) {
    my_main.loop();
  }
}

esp_err_t Main::setup(void) {
  ESP_LOGI(LOG_TAG, "Setup ok!");
  esp_err_t status{ESP_OK};

  led.init();
  return status;
}

void Main::loop(void) {
  ESP_LOGI(LOG_TAG, "LED on");
  led.set_level(true);
  vTaskDelay(pdMILISECONDS(1000));

  ESP_LOGI(LOG_TAG, "LED off");
  led.set_level(false);
  vTaskDelay(pdMILISECONDS(1000));

  // ESP_LOGI(LOG_TAG, "LED toggle");
  // led.toggle();
  // vTaskDelay(pdMILISECONDS(1000));
}
