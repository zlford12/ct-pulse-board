#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "eth_helper.h"
#include "nvs.h"
#include "scan.h"
#include "socket_helper.h"
#include "wifi_helper.h"
#include "soc/rtc.h"

static constexpr char TAG[] = "main";

void app_main(void)
{
    WifiConfig();
    //EthernetConfig();
    SocketInit();
    ScanInit();

    while (1) {
        if (!WifiConnected())
        {
            WifiConnect();
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }

        soc_rtc_slow_clk_src_t rtc_clk_src = rtc_clk_slow_src_get();
        if (rtc_clk_src == SOC_RTC_SLOW_CLK_SRC_XTAL32K) {
            ESP_LOGI(TAG, "Using External 32kHz Crystal");
        }
        else if (rtc_clk_src == SOC_RTC_SLOW_CLK_SRC_DEFAULT) {
            ESP_LOGI(TAG, "Using Default Internal RC Oscillator");
        }

        char cmd[64] = "";
        SocketListen(cmd, sizeof(cmd));
        ESP_LOGI(TAG, "Command: %s", cmd);
        if (strcmp(cmd, "scan") == 0)
        {
            RunScan();
        }
        else if (strncmp(cmd, "setfreq ", 8) == 0)
        {
            uint16_t new_freq = atoi(cmd + 8);
            if (new_freq > 0)
            {
                SetPulseFrequency(new_freq);
            }
            else
            {
                ESP_LOGI(TAG, "Invalid frequency value");
                SendResponse("invalid");
            }
        }
        else if (strncmp(cmd, "setroi ", 7) == 0)
        {
            char* comma = strchr(cmd + 7, ',');
            if (comma != NULL)
            {
                uint32_t min_roi = (uint32_t)atoi(cmd + 7);
                uint32_t max_roi = (uint32_t)atoi(comma + 1);
                if (max_roi > min_roi)
                {
                    SetRoi(min_roi, max_roi);
                }
                else
                {
                    ESP_LOGI(TAG, "Invalid ROI values");
                    SendResponse("invalid");
                }
            }
            else
            {
                ESP_LOGI(TAG, "Invalid ROI format");
                SendResponse("invalid");
            }
        }
        else
        {
            ESP_LOGI(TAG, "Unknown command");
            SendResponse("invalid");
            continue;
        }

        SocketClose();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}