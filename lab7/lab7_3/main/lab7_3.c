#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"
#include <stdio.h>

#include "get_https.c"
#include "post.c"
#include "get_http.c"

#include "temp.c"

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("SHTC3", "I2C initialized");

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    char location[256] = "";
    int outside_temperature = 0;
    float sensor_temperature = 0;
    float sensor_humidity = 0;

    while (1) {
        // get location from local server
        get_location(location);

        // get temperature from external server
        temp_request(location, &outside_temperature);
        // temp_request();

        // get temperature from sensors
        if (power_up() != ESP_OK) {ESP_LOGE(TAG, "Could not power up the sensor"); ;}
        vTaskDelay(12 / portTICK_PERIOD_MS); // Wait for the sensor to power up
        if (signal_readout() != ESP_OK) {ESP_LOGE(TAG, "Could not signal readout"); ;}
        vTaskDelay(20 / portTICK_PERIOD_MS); // Wait for the sensor to read the data
        if (read_temperature(&sensor_temperature) != ESP_OK) {ESP_LOGE(TAG, "Could not read temperature"); ;}
        if (read_humidity(&sensor_humidity) != ESP_OK) {ESP_LOGE(TAG, "Could not read humidity"); ;}
        if (power_down() != ESP_OK) {ESP_LOGE(TAG, "Could not power down the sensor"); ;}

        // Post the data to the server
        http_post_task(location, outside_temperature, sensor_temperature, sensor_humidity);

        printf("Location: %s\n", location);
        printf("Location Temperature: %d\n", outside_temperature);
        printf("Sensor Temperature: %0.f\n", sensor_temperature);
        printf("Sensor Humidity: %0.f\n", sensor_humidity);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
