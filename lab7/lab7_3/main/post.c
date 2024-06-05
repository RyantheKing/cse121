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

/* Constants that aren't configurable in menuconfig */
#define POST_SERVER "100.64.107.85"
#define POST_PORT "1234"

static const char *POST_TAG = "post_request";

static void http_post_task(char *location, int outside_temperature, float temperature, float humidity)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s;

    int err = getaddrinfo(POST_SERVER, POST_PORT, &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE(POST_TAG, "DNS lookup failed err=%d res=%p", err, res);
        return;
    }

    /* Code to print the resolved IP.

        Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(POST_TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE(POST_TAG, "... Failed to allocate socket.");
        freeaddrinfo(res);
        return;
    }
    ESP_LOGI(POST_TAG, "... allocated socket");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(POST_TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        return;
    }

    ESP_LOGI(POST_TAG, "... connected");
    freeaddrinfo(res);

    char REQUEST[1024];
    char JSON[128];
    sprintf(JSON, "{\"location\":\"%s\",\"outside_temp\":%d,\"sens_temp\":%.0f,\"sens_hum\":%.0f}", location, outside_temperature, temperature, humidity);
    int len = strlen(JSON);

    sprintf(REQUEST,
    "POST /post HTTP/1.1\r\n"
    "Host: "POST_SERVER":"POST_PORT"\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "\r\n"
    "%s\r\n", len, JSON);

    // const char *REQUEST = "POST /post HTTP/1.1\r\n"
    // "Host: 100.64.107.85:1234\r\n"
    // "Content-Type: application/json\r\n"
    // "Content-Length: 19\r\n"
    // "\r\n"
    // "{\"temp\":\"\r\n";

    if (write(s, REQUEST, strlen(REQUEST)) < 0) {
        ESP_LOGE(POST_TAG, "... socket send failed");
        close(s);
        return;
    }
    ESP_LOGI(POST_TAG, "... socket send success");

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
        ESP_LOGE(POST_TAG, "... failed to set socket receiving timeout");
        close(s);
        return;
    }
    ESP_LOGI(POST_TAG, "... set socket receiving timeout success");
    close(s);
}

// void app_main(void)
// {
//     ESP_ERROR_CHECK(nvs_flash_init());
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     ESP_ERROR_CHECK(i2c_master_init());
//     ESP_LOGI("SHTC3", "I2C initialized");

//     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
//      * Read "Establishing Wi-Fi or Ethernet Connection" section in
//      * examples/protocols/README.md for more information about this function.
//      */
//     ESP_ERROR_CHECK(example_connect());

//     xTaskCreate(&http_post_task, "http_post_task", 4096, NULL, 5, NULL);
// }
