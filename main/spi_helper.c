#include "spi_helper.h"
#include <string.h>
#include "pins.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/spi_master.h"

// SPI Config
#   define ESP_HOST         SPI3_HOST

static constexpr char TAG[] = "SPI";
esp_err_t spi_ret;

void SPI_BusConfig()
{
    // Initialize SPI bus
    ESP_LOGI(TAG, "Initializing bus SPI%d...", ESP_HOST + 1);
    spi_bus_config_t const bus_config = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_ret = spi_bus_initialize(ESP_HOST, &bus_config, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(spi_ret);
}

void SPI_SensorConfig(spi_device_handle_t *handle, int cs_pin)
{
    // Initialize SPI device
    ESP_LOGI(TAG, "Adding SPI Device on GPIO%d...", cs_pin);
    spi_device_interface_config_t const dev_config = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .clock_speed_hz = 10000000,
        .duty_cycle_pos = 128,
        .mode = 0,
        .spics_io_num = cs_pin,
        .queue_size = 3
    };

    spi_ret = spi_bus_add_device(ESP_HOST, &dev_config, handle);
    ESP_ERROR_CHECK(spi_ret);

    // Configure Temperature Sensor
    ESP_LOGI(TAG, "Configuring Temperature Sensor...");
    spi_transaction_t trans = {0};

    uint8_t constexpr tx_data[2] = {0x74, 0x27};
    trans.tx_buffer = tx_data;
    trans.rx_buffer = NULL;
    trans.length = sizeof(tx_data)*8;
    //ESP_LOGI(TAG, "Transaction Size: %d bits", trans.length);

    spi_ret = spi_device_transmit(*handle, &trans);
    ESP_ERROR_CHECK(spi_ret);
}

void SPI_Transact(spi_device_handle_t handle, uint8_t *buff)
{
    spi_transaction_t trans = {0};

    uint8_t constexpr tx_data[6] = {0xF7, 0, 0, 0, 0, 0};
    trans.tx_buffer = tx_data;
    trans.rx_buffer = buff;
    trans.length = sizeof(tx_data)*8;

    spi_ret = spi_device_transmit(handle, &trans);
    ESP_ERROR_CHECK(spi_ret);
}
