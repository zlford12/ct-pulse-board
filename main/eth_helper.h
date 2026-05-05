#ifndef MAESTRO_ETH_HELPER_H
#define MAESTRO_ETH_HELPER_H
#include <stdint.h>

#include "esp_event_base.h"

void EthernetConfig();
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

#endif