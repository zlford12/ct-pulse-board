#ifndef MAESTRO_SPI_HELPER_H
#define MAESTRO_SPI_HELPER_H

#include <stdint.h>
#include "driver/spi_master.h"

void SPI_BusConfig();
void SPI_SensorConfig(spi_device_handle_t *handle, int cs_pin);
void SPI_Transact(spi_device_handle_t handle, uint8_t *buff);

#endif