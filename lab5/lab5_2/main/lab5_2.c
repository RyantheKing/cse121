#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

#define ADC1_CHAN0 ADC_CHANNEL_2
#define ADC_ATTEN ADC_ATTEN_DB_12

// morse code constants
#define BASE 50
#define DOT (BASE - BASE*0.8) * 1000
#define DASH (BASE*3 - BASE*0.8) * 1000
#define SYMBOL_SPACE (BASE - BASE*0.8) * 1000
#define LETTER_SPACE (BASE*3 - BASE*0.8) * 1000
#define WORD_SPACE (BASE*7 - BASE*0.8) * 1000
#define NEWLINE (BASE*14 - BASE*0.8) * 1000

int THRESHOLD = 0;

const char *morse_code[] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
    "..-", "...-", ".--", "-..-", "-.--", "--..",
    "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
};

void app_main(void)
{
    int reading = 0;
    bool light_on = false;
    bool pause = true;
    int switch_time = 0;
    int prev_time = 0;
    int light_gap = 0;
    int index = 0;
    char morse_code_str[10] = {0};

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

    // configure threshhold
    for (int i = 0; i < 300; i++) {
        adc_oneshot_read(adc1_handle, ADC1_CHAN0, &reading);
        if (reading > THRESHOLD) {
            THRESHOLD = reading;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    THRESHOLD += 50;
    printf("THRESHOLD: %d\n", THRESHOLD);

    while (1) {
        adc_oneshot_read(adc1_handle, ADC1_CHAN0, &reading);

        switch_time = esp_timer_get_time();
        light_gap = switch_time - prev_time;
        if (!pause && light_gap > NEWLINE) {
            // translate morse code
            for (int i = 0; i < 26; i++) {
                if (strcmp(morse_code[i], morse_code_str) == 0) {
                    printf("%c", 'A' + i);
                    break;
                }
            }
            for (int i = 26; i < 36; i++) {
                if (strcmp(morse_code[i], morse_code_str) == 0) {
                    printf("%d", i - 26);
                    break;
                }
            }
            memset(morse_code_str, 0, sizeof(morse_code_str));
            index = 0;
            printf("\n");
            memset(morse_code_str, 0, sizeof(morse_code_str));
            index = 0;
            pause = true;
        }
        else if (reading > THRESHOLD) {
            if (!light_on) {
                light_on = true;
                if (!pause) {
                    if (light_gap > LETTER_SPACE) {
                        // translate morse code
                        for (int i = 0; i < 26; i++) {
                            if (strcmp(morse_code[i], morse_code_str) == 0) {
                                printf("%c", 'A' + i);
                                break;
                            }
                        }
                        for (int i = 26; i < 36; i++) {
                            if (strcmp(morse_code[i], morse_code_str) == 0) {
                                printf("%d", i - 26);
                                break;
                            }
                        }
                        memset(morse_code_str, 0, sizeof(morse_code_str));
                        index = 0;
                    }
                    if (light_gap > WORD_SPACE) {
                        printf(" ");
                    }
                    prev_time = switch_time;
                } else {
                    pause = false;
                    prev_time = switch_time;
                }
            }
        } else {
            if (light_on) {
                light_on = false;
                if (light_gap > DASH) {
                    // add to morse code
                    strcat(morse_code_str, "-");
                } else if (light_gap > DOT) {
                    // add to morse code
                    strcat(morse_code_str, ".");
                }
                index++;
                prev_time = switch_time;
            }
        }
        // printf("%d\n", reading);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
