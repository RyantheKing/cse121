#include <stdio.h>
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO 8
#define I2C_MASTER_SDA_IO 10
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define SHTC3_SENSOR_ADDR 0x70
#define WRITE_BIT I2C_MASTER_WRITE
#define READ_BIT I2C_MASTER_READ
#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0
#define ACK_VAL 0x0
#define NACK_VAL 0x1

static const char *TAG = "SHTC3";

static esp_err_t i2c_master_init()
{
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

static esp_err_t power_up() {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x35, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x17, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t power_down() {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0xB0, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x98, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t signal_readout() {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x7C, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0xA2, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t read_humidity(float *humidity) {
    uint8_t discard[3];
    uint8_t data[3];

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, discard, 3, ACK_VAL);
    i2c_master_read(cmd, data, 3, ACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t humidity_raw = (data[0] << 8) | data[1];
    *humidity = 100 * ((float)humidity_raw / 65535.0f);

    return ESP_OK;
}

static esp_err_t read_temperature(float *temperature) {
    uint8_t data[3];

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, data, 3, ACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t temp_raw = (data[0] << 8) | data[1];
    *temperature = -45 + 175 * ((float)temp_raw / 65535.0f);

    return ESP_OK;
}

void app_main(void)
{
    float temperature = 0;
    float humidity = 0;
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized");

    while (true) {
        esp_err_t ret = power_up();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Could not power up the sensor: %d", ret);
            continue;
        }

        vTaskDelay(12 / portTICK_PERIOD_MS); // Wait for the sensor to power up

        ret = signal_readout();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Could not signal readout: %d", ret);
            continue;
        }

        vTaskDelay(20 / portTICK_PERIOD_MS); // Wait for the sensor to read the data
        
        ret = read_temperature(&temperature);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Could not read temperature: %d", ret);
            continue;
        }

        // Notes: Reading only 3 bytes would be impossible. You could take a RH first and T first measurement according
        // to the datasheet, but measurements take 20ms, and so it would be ridiculous to poll the sensor twice.
        // You could also read 3 bytes for temperature and then read humidity, but then the temperature function is required
        // as you cannot skip bytes in the I2C buffer. This defeats the purpose of the seperated functions.
        // I believe my method is best for speed and modularity.
        ret = read_humidity(&humidity);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Could not read humidity: %d", ret);
            continue;
        }

        printf("Temperature is %.0fC (or %.0fF) with a %.0f%% humidity\n", temperature, temperature * 9 / 5 + 32, humidity);

        ret = power_down();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Could not power down the sensor: %d", ret);
            continue;
        }
        
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
