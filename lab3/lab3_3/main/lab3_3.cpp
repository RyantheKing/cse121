#include <cstring>
#include "driver/i2c.h"
#include "esp_log.h"

#include "DFRobot_LCD.h"
#include "temp.c"

extern "C" void app_main(void)
{
    float temperature = 0;
    float humidity = 0;
    char temperature_str[10];
    char humidity_str[10];

    DFRobot_LCD lcd(16, 2, LCD_ADDRESS, RGB_ADDRESS);
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("SHTC3", "I2C initialized");

    while (true) {
        if (power_up() != ESP_OK) {ESP_LOGE(TAG, "Could not power up the sensor"); continue;}
        vTaskDelay(12 / portTICK_PERIOD_MS); // Wait for the sensor to power up
        if (signal_readout() != ESP_OK) {ESP_LOGE(TAG, "Could not signal readout"); continue;}
        vTaskDelay(20 / portTICK_PERIOD_MS); // Wait for the sensor to read the data
        if (read_temperature(&temperature) != ESP_OK) {ESP_LOGE(TAG, "Could not read temperature"); continue;}
        if (read_humidity(&humidity) != ESP_OK) {ESP_LOGE(TAG, "Could not read humidity"); continue;}
        if (power_down() != ESP_OK) {ESP_LOGE(TAG, "Could not power down the sensor"); continue;}

        snprintf(temperature_str, 10, "Temp: %.0fC", temperature);
        snprintf(humidity_str, 10, "Hum : %.0f%%", humidity);

        lcd.init();
        lcd.setRGB(0, 255, 0);
        lcd.printstr(temperature_str);
        lcd.setCursor(0, 1);
        lcd.printstr(humidity_str);

        // delay 1 second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
