#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

#define LED_GPIO_PIN GPIO_NUM_7
#define BLINK_GPIO GPIO_NUM_2

static led_strip_handle_t led_strip;

static void configure_led() {
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
}

void blink_task(void *pvParameter) {
    esp_rom_gpio_pad_select_gpio(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    configure_led();

    while (true) {
        gpio_set_level(LED_GPIO_PIN, 0);
        led_strip_clear(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(LED_GPIO_PIN, 1);
        led_strip_set_pixel(led_strip, 0, 64, 0, 0);
        led_strip_refresh(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    xTaskCreate(&blink_task, "blink_task", 2048, NULL, 5, NULL);
}