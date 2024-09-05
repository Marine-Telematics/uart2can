#include <driver/twai.h>
#include <soc/gpio_num.h>

#include "can.h"
#include "pinout.h"

static const char TAG[] = "can";

static const gpio_num_t TX_PIN = CAN_TX_PIN;
static const gpio_num_t RX_PIN = CAN_RX_PIN;

static twai_handle_t bus_handle = {};

/// @brief Receives messages from the CAN network.
///
/// @param message Reference to the message buffer.
/// @param wait_time Time to await in system ticks
///
/// @returns True if a message could be read.
bool can_receive(CAN_Message *message, TickType_t wait_time)
{
    const esp_err_t ret = twai_receive_v2(bus_handle, message, wait_time);
    return ret == ESP_OK;
};

/// @brief Transmits messages to the CAN network.
///
/// @param message Reference to the message buffer.
/// @param wait_time Time to await in system ticks
///
/// @returns True if the message could be transmited.
bool can_transmit(const CAN_Message *message, TickType_t wait_time)
{
    const esp_err_t ret = twai_transmit_v2(bus_handle, message, wait_time);
    return ret == ESP_OK;
}

void can_initialize(i32 controller_id)
{
    twai_general_config_t g_config = {0};
    g_config.controller_id         = controller_id;
    g_config.mode                  = TWAI_MODE_NORMAL;
    g_config.tx_io                 = TX_PIN;
    g_config.rx_io                 = RX_PIN;
    g_config.clkout_io             = TWAI_IO_UNUSED;
    g_config.bus_off_io            = TWAI_IO_UNUSED;
    g_config.tx_queue_len          = 5;
    g_config.rx_queue_len          = 5;
    g_config.alerts_enabled        = TWAI_ALERT_NONE;
    g_config.clkout_divider        = 0;
    g_config.intr_flags            = ESP_INTR_FLAG_SHARED;

    const twai_timing_config_t T_CONFIG = TWAI_TIMING_CONFIG_250KBITS();

    twai_filter_config_t f_config = {0};
    f_config.acceptance_code      = 0;
    f_config.acceptance_mask      = 0xFFFFFFFF;
    f_config.single_filter        = true;

    twai_handle_t *handle = &bus_handle;

    esp_err_t ret =
        twai_driver_install_v2(&g_config, &T_CONFIG, &f_config, handle);
    if (ret != ESP_OK)
    {
        const char *ename = esp_err_to_name(ret);
        ESP_LOGE(TAG, "failed to intall BUS driver : %s", ename);

        return;
    }

    ret = twai_start_v2(bus_handle);
    if (ret != ESP_OK)
    {
        const char *ename = esp_err_to_name(ret);
        ESP_LOGE(TAG, "failed to start the can bus : %s", ename);

        return;
    }
}
