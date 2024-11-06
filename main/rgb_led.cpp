#include "esp_log.h"
#include "led_strip.h"

#include "rgb_led.hpp"

static const char *TAG = "pedalboard:led";

RGBLed::RGBLed(uint8_t gpio_num):
led_strip_config{
    .strip_gpio_num = gpio_num,  // GPIO18 on SAOLA-1 devboard
    .max_leds = 1,  // 1 LED only on SAOLA-1 devboard
    .led_model = LED_MODEL_WS2812, // LED strip model, it determines the bit timing
    .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color component format is G-R-B
    .flags = {
        .invert_out = false, // don't invert the output signal
    }
},
led_strip_rmt_config{
    .clk_src = RMT_CLK_SRC_DEFAULT,    // different clock source can lead to different power consumption
    .resolution_hz = 10 * 1000 * 1000, // RMT counter clock frequency: 10MHz
    .mem_block_symbols = 64,           // the memory size of each RMT channel, in words (4 bytes)
    .flags = {
        .with_dma = false, // DMA feature is available on chips like ESP32-S3/P4
    }
}
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&led_strip_config, &led_strip_rmt_config, &pStrip_a));
    /* Set all LED off to clear all pixels */
    led_strip_clear(pStrip_a);
}

RGBLed::~RGBLed(){
    ESP_ERROR_CHECK(led_strip_del(pStrip_a));
}

void RGBLed::blink(uint8_t led_state_on)
{
    /* If the addressable LED is enabled */
    if (led_state_on) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(pStrip_a, 0, 15, 15, 15);
        /* Refresh the strip to send data */
        led_strip_refresh(pStrip_a);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(pStrip_a);
    }
}
