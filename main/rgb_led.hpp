#pragma once

#include "esp_log.h"
#include "led_strip.h"

class RGBLed{

  public:

    RGBLed(uint8_t gpio_num);
    ~RGBLed();

    void blink(uint8_t led_state_on);

  private:

    const led_strip_config_t led_strip_config;
    const led_strip_rmt_config_t led_strip_rmt_config;
    led_strip_handle_t pStrip_a;

};
