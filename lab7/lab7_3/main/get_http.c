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

/* Constants that aren't configurable in menuconfig */
#define LOCAL_WEB_SERVER "100.64.107.85"
#define LOCAL_WEB_PORT "1234"
#define LOCAL_WEB_PATH "/location"

static const char *LOCAL_GET_TAG = "local_get";

static const char *REQUEST = "GET " LOCAL_WEB_PATH " HTTP/1.1\r\n"
    "Host: "LOCAL_WEB_SERVER":"LOCAL_WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static void get_location(char *location)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    int err = getaddrinfo(LOCAL_WEB_SERVER, LOCAL_WEB_PORT, &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE(LOCAL_GET_TAG, "DNS lookup failed err=%d res=%p", err, res);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return;
    }

    /* Code to print the resolved IP.

        Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(LOCAL_GET_TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE(LOCAL_GET_TAG, "... Failed to allocate socket.");
        freeaddrinfo(res);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return;
    }
    ESP_LOGI(LOCAL_GET_TAG, "... allocated socket");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(LOCAL_GET_TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        return;
    }

    ESP_LOGI(LOCAL_GET_TAG, "... connected");
    freeaddrinfo(res);

    if (write(s, REQUEST, strlen(REQUEST)) < 0) {
        ESP_LOGE(LOCAL_GET_TAG, "... socket send failed");
        close(s);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        return;
    }
    ESP_LOGI(LOCAL_GET_TAG, "... socket send success");

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
        ESP_LOGE(LOCAL_GET_TAG, "... failed to set socket receiving timeout");
        close(s);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        return;
    }
    ESP_LOGI(LOCAL_GET_TAG, "... set socket receiving timeout success");

    /* Read HTTP response */
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);
        // put in location string
        if (r > 0) {
            strcpy(location, recv_buf);
        }
    } while(r > 0);
    strcat(location, "\0");

    ESP_LOGI(LOCAL_GET_TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);
    return;
}

// void app_main(void)
// {
//     ESP_ERROR_CHECK( nvs_flash_init() );
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
//      * Read "Establishing Wi-Fi or Ethernet Connection" section in
//      * examples/protocols/README.md for more information about this function.
//      */
//     ESP_ERROR_CHECK(example_connect());

//     char location[256] = "";

//     get_location(location);
//     printf("Location: %s\n", location);
// }
