#include "temp.c"
#include "get_https.c"
#include "post.c"
#include "get_http.c"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "100.64.107.85"
#define WEB_PORT "1234"

static const char *POST_TAG = "post_request";

static void http_post_task(void *pvParameters)
{
    float temperature = 0;
    float humidity = 0;
    char REQUEST[1024];

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s;

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(POST_TAG, "DNS lookup failed err=%d res=%p", err, res);
            continue;
        }

        /* Code to print the resolved IP.

            Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(POST_TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(POST_TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            continue;
        }
        ESP_LOGI(POST_TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(POST_TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            continue;
        }

        ESP_LOGI(POST_TAG, "... connected");
        freeaddrinfo(res);

        if (power_up() != ESP_OK) {ESP_LOGE(TAG, "Could not power up the sensor"); continue;}
        vTaskDelay(12 / portTICK_PERIOD_MS); // Wait for the sensor to power up
        if (signal_readout() != ESP_OK) {ESP_LOGE(TAG, "Could not signal readout"); continue;}
        vTaskDelay(20 / portTICK_PERIOD_MS); // Wait for the sensor to read the data
        if (read_temperature(&temperature) != ESP_OK) {ESP_LOGE(TAG, "Could not read temperature"); continue;}
        if (read_humidity(&humidity) != ESP_OK) {ESP_LOGE(TAG, "Could not read humidity"); continue;}
        if (power_down() != ESP_OK) {ESP_LOGE(TAG, "Could not power down the sensor"); continue;}

        sprintf(REQUEST,
        "POST /post HTTP/1.1\r\n"
        "Host: "WEB_SERVER":"WEB_PORT"\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 24\r\n"
        "\r\n"
        "{\"temp\":\"%.0f\",\"hum\":\"%.0f\"}", temperature, humidity);

        // const char *REQUEST = "POST /post HTTP/1.1\r\n"
        // "Host: 100.64.107.85:1234\r\n"
        // "Content-Type: application/json\r\n"
        // "Content-Length: 19\r\n"
        // "\r\n"
        // "{\"temp\":\"\r\n";

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(POST_TAG, "... socket send failed");
            close(s);
            continue;
        }
        ESP_LOGI(POST_TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(POST_TAG, "... failed to set socket receiving timeout");
            close(s);
            continue;
        }
        ESP_LOGI(POST_TAG, "... set socket receiving timeout success");
        close(s);
        ESP_LOGI(TAG, "Starting again!");
    }
}

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

    xTaskCreate(&http_post_task, "http_post_task", 4096, NULL, 5, NULL);
}
