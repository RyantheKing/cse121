#include "esp_cpu.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "temp.c"

#define TRIGGER_GPIO 20
#define ECHO_GPIO 9

void send_trigger_pulse() {
    gpio_set_level(TRIGGER_GPIO, 1);
    ets_delay_us(10);
    gpio_set_level(TRIGGER_GPIO, 0);
}

uint32_t measure_echo_pulse() {
    while (gpio_get_level(ECHO_GPIO) == 0) {}
    uint32_t start = esp_cpu_get_cycle_count();
    while (gpio_get_level(ECHO_GPIO) == 1) {}
    uint32_t end = esp_cpu_get_cycle_count();
    return end - start;
}

float calculate_distance(uint32_t pulse_duration, float temperature) {
    return pulse_duration / 160 * (331.3 + 0.606 * temperature) / 10000 / 2;
}

float calibrate_sensor() {
    float known_close_distance = 10;
    float known_far_distance = 20;
    float measured_close_distance = 0;
    float measured_far_distance = 0;
    float distance_sum = 0;
    float distance = 0;
    float temperature = 0;
    float pulse_duration = 0;

    distance_sum = 0;
    if (power_up() != ESP_OK) {ESP_LOGE(TAG, "Could not power up the sensor"); return 0.0;}
    vTaskDelay(12 / portTICK_PERIOD_MS); // Wait for the sensor to power up
    if (signal_readout() != ESP_OK) {ESP_LOGE(TAG, "Could not signal readout"); return 0.0;}
    vTaskDelay(20 / portTICK_PERIOD_MS); // Wait for the sensor to read the data
    if (read_temperature(&temperature) != ESP_OK) {ESP_LOGE(TAG, "Could not read temperature"); return 0.0;}
    if (power_down() != ESP_OK) {ESP_LOGE(TAG, "Could not power down the sensor"); return 0.0;}

    printf("5 seconds to place at 10cm mark\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    for (int i = 0; i < 100; i++) {
        send_trigger_pulse();
        pulse_duration = measure_echo_pulse();
        distance = calculate_distance(pulse_duration, temperature);
        distance_sum += distance;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    measured_close_distance = distance_sum / 100;
    printf("Measured: %.1f\n", measured_close_distance);
    printf("5 seconds to place at 20cm mark\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    distance_sum = 0;
    for (int i = 0; i < 100; i++) {
        send_trigger_pulse();
        pulse_duration = measure_echo_pulse();
        distance = calculate_distance(pulse_duration, temperature);
        distance_sum += distance;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    measured_far_distance = distance_sum / 100;
    printf("Measured: %.1f\n", measured_far_distance);

    return (known_far_distance - known_close_distance) / (measured_far_distance - measured_close_distance);
}

void app_main(void)
{
    float distance_sum = 0;
    float temperature = 0;
    float slope = 0;
    float distance = 0;
    uint32_t pulse_duration = 0;

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("SHTC3", "I2C initialized");

    esp_rom_gpio_pad_select_gpio(TRIGGER_GPIO);
    gpio_set_direction(TRIGGER_GPIO, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(ECHO_GPIO);
    gpio_set_direction(ECHO_GPIO, GPIO_MODE_INPUT);

    slope = calibrate_sensor();
    printf("Calibration slope: %.4f\n", slope);

    while (1) {
        distance_sum = 0;
        if (power_up() != ESP_OK) {ESP_LOGE(TAG, "Could not power up the sensor"); continue;}
        vTaskDelay(12 / portTICK_PERIOD_MS); // Wait for the sensor to power up
        if (signal_readout() != ESP_OK) {ESP_LOGE(TAG, "Could not signal readout"); continue;}
        vTaskDelay(20 / portTICK_PERIOD_MS); // Wait for the sensor to read the data
        if (read_temperature(&temperature) != ESP_OK) {ESP_LOGE(TAG, "Could not read temperature"); continue;}
        if (power_down() != ESP_OK) {ESP_LOGE(TAG, "Could not power down the sensor"); continue;}

        for (int i = 0; i < 100; i++) {
            send_trigger_pulse();
            pulse_duration = measure_echo_pulse();
            distance = calculate_distance(pulse_duration, temperature);
            distance_sum += distance;
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        printf("Distance: %.1f cm at %.0fC\n", distance_sum / 100 * slope, temperature);
    }
}
