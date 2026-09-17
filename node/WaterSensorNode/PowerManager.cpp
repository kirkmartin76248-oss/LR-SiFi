#include "PowerManager.h"
#include "Config.h"
#include <esp_sleep.h>
#include <driver/rtc_io.h>

void goToDeepSleep() {
  esp_sleep_disable_ext1_wakeup_io(1ULL << PIN_RADIO_DIO1);
  esp_sleep_enable_ext1_wakeup_io(1ULL << PIN_RADIO_DIO1, ESP_EXT1_WAKEUP_ANY_HIGH);
  rtc_gpio_init((gpio_num_t)PIN_RADIO_DIO1);
  rtc_gpio_pulldown_dis((gpio_num_t)PIN_RADIO_DIO1);
  rtc_gpio_pullup_dis((gpio_num_t)PIN_RADIO_DIO1);
  esp_deep_sleep_start();
}
