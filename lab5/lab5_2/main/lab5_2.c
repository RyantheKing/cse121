#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include <stdio.h>

#define ADC1_CHAN0 ADC_CHANNEL_4
#define ADC_ATTEN ADC_ATTEN_DB_12

static int adc_raw[2][10];

void app_main(void)
{
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHAN0, &config));

    while (1) {
        adc_oneshot_read(adc1_handle, ADC1_CHAN0, &adc_raw[0][0]);
        printf("%d\n", adc_raw[0][0]);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
