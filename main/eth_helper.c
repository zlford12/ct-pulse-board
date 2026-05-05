#include "eth_helper.h"
#include "esp_eth_phy_w5500.h"
#include "esp_eth_mac_w5500.h"
#include "esp_eth_netif_glue.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "pins.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "lwip/ip4_addr.h"

#   define ETH_HOST         SPI2_HOST

static constexpr char TAG[] = "ETH";

void EthernetConfig()
{
    // initialize SPI bus
    ESP_LOGI(TAG, "Configuring Ethernet...");
    gpio_install_isr_service(0);
    spi_bus_config_t buscfg = {
        .miso_io_num = ETH_MISO_PIN,
        .mosi_io_num = ETH_MOSI_PIN,
        .sclk_io_num = ETH_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(ETH_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .mode = 0,
        .clock_speed_hz = 12 * 1000 * 1000,
        .queue_size = 16,
        .spics_io_num = ETH_CS_PIN
    };

    // create ethernet config
    eth_w5500_config_t ethcfg = ETH_W5500_DEFAULT_CONFIG(ETH_HOST, &devcfg);
    ethcfg.int_gpio_num = ETH_INT_PIN;

    // create mac driver
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.rx_task_stack_size = 4096;
    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&ethcfg,&mac_config);

    // create phy driver
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.reset_gpio_num = ETH_RST_PIN;
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));

    uint8_t mac_addr[6] = {0};
    ESP_ERROR_CHECK(esp_read_mac(mac_addr, ESP_MAC_WIFI_STA));
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, mac_addr));

    // set static ip
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
    if (netif == NULL)
    {
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        netif = esp_netif_new(&cfg);
    }

    if (netif)
    {
        ESP_ERROR_CHECK(esp_netif_attach(netif, esp_eth_new_netif_glue(eth_handle)));

        esp_netif_dhcpc_stop(netif);
        esp_netif_ip_info_t ip_info;
        IP4_ADDR(&ip_info.ip, 192, 168, 22, 101);
        IP4_ADDR(&ip_info.gw, 192, 168, 22, 1);
        IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
        esp_netif_set_ip_info(netif, &ip_info);
    }

    // start driver
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}
