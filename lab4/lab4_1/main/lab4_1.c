#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define I2C_MASTER_SCL_IO 8
#define I2C_MASTER_SDA_IO 10
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 1000000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

#define IMU_SENSOR_ADDR 0x68
#define GYRO_XOUT_H 0x0B

static const char *TAG = "ICM-42670-P";

static esp_err_t i2c_master_init() {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static esp_err_t write_register(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, IMU_SENSOR_ADDR << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t read_register(uint8_t reg, uint8_t *data, size_t len) {
    if (len == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, IMU_SENSOR_ADDR  << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, IMU_SENSOR_ADDR << 1 | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void gyroscope_task(void *arg) {
    int16_t x = 0;
    int16_t y = 0;

    int16_t diff_x = 0;
    int16_t diff_y = 0;

    float velocityX = 0, velocityY = 0;
    float accelX = 0, accelY = 0;
    float prevAccelX = 0, prevAccelY = 0;

    int sampling_rate = 10;
    float dt = sampling_rate / 1000.0;

    // direction string
    char dir[11];
    
    uint8_t data[6];

    if (read_register(GYRO_XOUT_H, data, 6) == ESP_OK) {
            diff_x = (int16_t)((data[0] << 8) | data[1]);
            diff_y = (int16_t)((data[2] << 8) | data[3]);
            // diff_z = (int16_t)((data[4] << 8) | data[5]);
        }

    while (1) {
        if (read_register(GYRO_XOUT_H, data, 6) == ESP_OK) {
            x = (int16_t)((data[0] << 8) | data[1]);
            y = (int16_t)((data[2] << 8) | data[3]);
            // z = (int16_t)((data[4] << 8) | data[5]);
            
            accelX = x - diff_x;
            accelY = y - diff_y;

            if (fabs(accelX) < 100) {
                accelX = 0;
            }
            if (fabs(accelY) < 100) {
                accelY = 0;
            }
            velocityX += (accelX + prevAccelX) / 2 * dt;
            velocityY += (accelY + prevAccelY) / 2 * dt;
            prevAccelX = accelX;
            prevAccelY = accelY;

            if (fabs(accelX) < 150) velocityX *= 0.9;
            if (fabs(accelY) < 150) velocityY *= 0.9;

            if (velocityX > 100) {
                strcpy(dir, "RIGHT ");
            } else if (velocityX < -100) {
                strcpy(dir, "LEFT  ");
            } else {
                strcpy(dir, "      ");
            }
            if (velocityY > 100) {
                strcat(dir, "UP  ");
            } else if (velocityY < -100) {
                strcat(dir, "DOWN");
            } else {
                strcat(dir, "    ");
            }

            ESP_LOGI(TAG, "%s", dir);
        }
        vTaskDelay(pdMS_TO_TICKS(sampling_rate));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(i2c_master_init());
    write_register(0x1F, 0x03);
    // write_register(0x20, 0x66);
    // write_register(0x23, 0x37);
    write_register(0x21, 0x66);
    xTaskCreate(gyroscope_task, "gyroscope_task", 2048, NULL, 5, NULL);
}