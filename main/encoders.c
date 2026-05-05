#include "encoders.h"
#include "esp_log.h"
#include "pins.h"
#include "spi_helper.h"
#include "driver/spi_master.h"

spi_device_handle_t t_enc_handle, r_enc_handle, s_enc_handle, d_enc_handle;
static constexpr char TAG[] = "encoders";

void EncoderInit()
{
    ESP_LOGI(TAG, "Initializing Encoders...");
    SPI_SensorConfig(&t_enc_handle, T_ENC_CS);
    //SPI_SensorConfig(&r_enc_handle, R_ENC_CS);
    //SPI_SensorConfig(&s_enc_handle, S_ENC_CS);
    //SPI_SensorConfig(&d_enc_handle, D_ENC_CS);
}

void ReadEncoders(uint8_t *buff)
{
    SPI_Transact(t_enc_handle, buff);
}
