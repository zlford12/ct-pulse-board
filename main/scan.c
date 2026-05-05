#include "scan.h"
#include <stdint.h>
#include "pins.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "socket_helper.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

#define MIN_PULSE_FREQ 1
#define MAX_PULSE_FREQ 4
#define MAX_LINE_SIZE 256
#define ROI_TIMEOUT 10000000

static constexpr char TAG[] = "scan";
const esp_timer_create_args_t pulse_timer_args = {
    .callback = &PulseTimer,
    .name = "pulse_timer"
};
const esp_timer_create_args_t roi_timer_args = {
    .callback = &RoiTimeout,
    .name = "roi_timeout"
};
esp_timer_handle_t pulse_timer;
esp_timer_handle_t roi_timeout;
static uint8_t pulse_state = 0;
static uint32_t position;
static uint32_t line_buffer[MAX_LINE_SIZE];
static uint32_t line_buffer_size = 0;
static uint32_t roi_min = 2080;
static uint32_t roi_max = 2100;
volatile bool read_encoders = false;
volatile bool acquire = false;
static TaskHandle_t scan_task_hdl;

void ScanInit()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PANEL_TRIGGER);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_drive_capability(PANEL_TRIGGER, GPIO_DRIVE_CAP_3);

    ESP_ERROR_CHECK(esp_timer_create(&pulse_timer_args, &pulse_timer));
    ESP_ERROR_CHECK(esp_timer_create(&roi_timer_args, &roi_timeout));
    ESP_ERROR_CHECK(esp_timer_start_periodic(pulse_timer, 1000000 / (2 * MIN_PULSE_FREQ)));
}

void RunScan()
{
    ESP_LOGI(TAG, "Running scan");

    scan_task_hdl = xTaskGetCurrentTaskHandle();
    xTaskNotifyStateClear(scan_task_hdl);
    line_buffer_size = 0;
    read_encoders = true;
    ESP_ERROR_CHECK(esp_timer_start_once(roi_timeout, ROI_TIMEOUT));
    xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

    ESP_LOGI(TAG, "Scan finished, %d frames captured", line_buffer_size);
    SendFrames(line_buffer, line_buffer_size * sizeof(uint32_t));
}

void SetPulseFrequency(uint16_t new_freq)
{
    if (new_freq <= MAX_PULSE_FREQ && new_freq >= MIN_PULSE_FREQ)
    {
        ESP_ERROR_CHECK(esp_timer_stop(pulse_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(pulse_timer, 1000000 / (2 * new_freq)));
        ESP_LOGI(TAG, "Pulse frequency updated to %d Hz", new_freq);

        char freq_str[16];
        sprintf(freq_str, "%d", new_freq);
        SendResponse(freq_str);
    }
    else
    {
        ESP_LOGI(TAG, "Pulse frequency %d Hz is out of range (%d - %d Hz)", new_freq, MIN_PULSE_FREQ, MAX_PULSE_FREQ);
        SendResponse("invalid");
    }
}

void SetRoi(uint32_t min, uint32_t max)
{
    roi_min = min;
    roi_max = max;

    char roi_str[32];
    sprintf(roi_str, "%u,%u", (unsigned int)min, (unsigned int)max);
    SendResponse(roi_str);
}

static void PulseTimer(void *arg)
{
    // create 50% duty cycle pulse
    pulse_state = !pulse_state;

    // linac/lda trigger lags pulse_state
    esp_rom_delay_us(5);
    gpio_set_level(PANEL_TRIGGER, acquire && pulse_state);

    if (!pulse_state)
    {
        return;
    }

    bool acquire_cache = acquire;

    if (acquire_cache)
    {
        if (esp_timer_is_active(roi_timeout))
        {
            esp_timer_stop(roi_timeout);
        }
        if (line_buffer_size < MAX_LINE_SIZE)
        {
            line_buffer[line_buffer_size++] = position;
            if (acquire)
            {
                return;
            }
        }
        else
        {
            acquire = false;
        }

        read_encoders = false;
        xTaskNotify(scan_task_hdl, 0, eNoAction);
    }
}

static void RoiTimeout(void *arg)
{
    read_encoders = false;
    acquire = false;
    xTaskNotify(scan_task_hdl, 0, eNoAction);
    SendResponse("timeout");
}
