#include "driver/i2c.h"
#include "esp_log.h"
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
#define WRITE_BIT I2C_MASTER_WRITE
#define READ_BIT I2C_MASTER_READ
#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0
#define ACK_VAL I2C_MASTER_ACK
#define NACK_VAL I2C_MASTER_NACK

static const char *TAG = "ICM-42670-P";

static esp_err_t i2c_master_init() {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;
    
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static esp_err_t write_register(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, IMU_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t read_gyro(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t data[6];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, IMU_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x0B, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, IMU_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, data, 6, ACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    *x = (int16_t)((data[0] << 8) | data[1]);
    *y = (int16_t)((data[2] << 8) | data[3]);
    *z = (int16_t)((data[4] << 8) | data[5]);

    return ret;
}

void calibrate_accelerometer(int16_t *x, int16_t *y, int16_t *z) {
    read_gyro(x, y, z);
}

void app_main(void)
{
    int16_t x = 0;
    int16_t y = 0;
    int16_t z = 0;

    int16_t diff_x = 0;
    int16_t diff_y = 0;
    int16_t diff_z = 0;

    float velocityX = 0, velocityY = 0;
    float accelX = 0, accelY = 0;
    float prevAccelX = 0, prevAccelY = 0;

    int sampling_rate = 10;
    float dt = sampling_rate / 1000.0;

    // direction string
    char dir[11];

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_ERROR_CHECK(write_register(0x1F, 0x03));
    ESP_ERROR_CHECK(write_register(0x21, 0x66));

    calibrate_accelerometer(&diff_x, &diff_y, &diff_z);
    
    while (true) {
        ESP_ERROR_CHECK(read_gyro(&x, &y, &z));
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

        // velocityX *= 0.9;
        // velocityY *= 0.9;

        prevAccelX = accelX;
        prevAccelY = accelY;

        if (fabs(accelX) < 200) velocityX *= 0.9;
        if (fabs(accelY) < 200) velocityY *= 0.9;

        // printf("Velocity X: %.3f, Velocity Y: %.3f\n", accelX, accelY);

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

        vTaskDelay(sampling_rate / portTICK_PERIOD_MS);
    }
}
