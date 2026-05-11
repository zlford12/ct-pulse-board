#include "scan.h"
#include <stdint.h>
#include <math.h>
#include "pins.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "socket_helper.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

#define MIN_PULSE_FREQ 1
#define MAX_PULSE_FREQ 120
#define SCAN_TIMEOUT 10000000

static constexpr char TAG[] = "scan";
const esp_timer_create_args_t pulse_timer_args = {
    .callback = &PulseTimer,
    .name = "pulse_timer"
};
const esp_timer_create_args_t scan_timer_args = {
    .callback = &ScanTimeout,
    .name = "scan_timeout"
};
esp_timer_handle_t pulse_timer;
esp_timer_handle_t scan_timeout;
static bool timeout_occurred = false;
static uint8_t pulse_state = 0;
static uint16_t pulse_count = 360;
static uint16_t remaining_pulses = 0;
static float pulse_freq = MIN_PULSE_FREQ;
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

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PULSE_START);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    ESP_ERROR_CHECK(esp_timer_create(&pulse_timer_args, &pulse_timer));
    ESP_ERROR_CHECK(esp_timer_create(&scan_timer_args, &scan_timeout));
}

void SetPulseFrequency(float new_freq)
{
    pulse_freq = fmaxf(MIN_PULSE_FREQ, fminf(new_freq, MAX_PULSE_FREQ));

    ESP_LOGI(TAG, "Pulse frequency updated to %f Hz", pulse_freq);
    char freq_str[16];
    sprintf(freq_str, "%f", pulse_freq);
    SendResponse(freq_str);
}

void SetPulseCount(uint16_t new_count)
{
    pulse_count = new_count;
    ESP_LOGI(TAG, "Pulse count updated to %d", pulse_count);
    char count_str[16];
    sprintf(count_str, "%d", pulse_count);
    SendResponse(count_str);
}

void RunScan()
{
    ESP_LOGI(TAG, "Running scan");
    SendResponse("scanning");

    timeout_occurred = false;
    ESP_ERROR_CHECK(esp_timer_start_once(scan_timeout, SCAN_TIMEOUT));
    while (gpio_get_level(PULSE_START) == 0)
    {
        esp_rom_delay_us(1);
        if (timeout_occurred)
        {
            return;
        }
    }

    ESP_LOGI(TAG, "Pulse Train Started");
    esp_timer_stop(scan_timeout);
    remaining_pulses = pulse_count;
    scan_task_hdl = xTaskGetCurrentTaskHandle();
    xTaskNotifyStateClear(scan_task_hdl);
    ESP_ERROR_CHECK(esp_timer_start_periodic(pulse_timer, 1000000 / (2 * pulse_freq)));
    xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
}

static void PulseTimer(void *arg)
{
    // create 50% duty cycle pulse
    pulse_state = !pulse_state;

    // panel trigger lags pulse_state
    esp_rom_delay_us(5);
    gpio_set_level(PANEL_TRIGGER, pulse_state);

    // decrement pulse count on rising edge
    if (pulse_state == 1)
    {
        remaining_pulses--;
    }
    else if (remaining_pulses == 0)
    {
        esp_timer_stop(pulse_timer);
        xTaskNotify(scan_task_hdl, 0, eNoAction);
        ESP_LOGI(TAG, "Scan complete");
        SendResponse("complete");
    }


}

static void ScanTimeout(void *arg)
{
    timeout_occurred = true;
    ESP_LOGI(TAG, "Scan timed out");
    SendResponse("timeout");
}
