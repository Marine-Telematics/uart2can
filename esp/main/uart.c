#include <driver/gpio.h>
#include <driver/uart.h>

#include "pinout.h"
#include "types.h"
#include "uart.h"

static const char TAG[] = "uart";

static const size_t      BUF_SIZE = 240;
static const i32         BAUDRATE = 115200;
static const uart_port_t PORT     = UART_NUM_1;

static const gpio_num_t TXD = UART_TXD_PIN;
static const gpio_num_t RXD = UART_RXD_PIN;

QueueHandle_t int_queue = NULL;

/// @brief Sends a byte array to the uart chip.
///
/// @param buffer Data to send.
/// @param size The stream size.
///
/// @return true If the values could be sent, false otherwise.
bool uart_send(const byte *buffer, size_t size)
{
    const size_t ret = uart_write_bytes(PORT, buffer, size);
    if (ret != size)
    {
        ESP_LOGI(TAG, "failed to send %zu bytes (%zu)", size, ret);
        return false;
    }

    const esp_err_t err = uart_wait_tx_done(PORT, pdMS_TO_TICKS(30));
    if (err != ESP_OK)
    {
        const char *name = esp_err_to_name(err);
        ESP_LOGE(TAG, "failed for wait to send %zu bytes (%s)", size, name);

        return false;
    }

    return true;
}

/// @brief Tries to read a stream of bytes from the uart.
///
/// @param buffer[out] Buffer to write to.
/// @param size The maximum write size(in bytes).
/// @param timeout The maximum await time(in RTOS Ticks).
///
/// @return 32 bit integer with the read size.
i32 uart_read(byte *buffer, size_t size, TickType_t timeout)
{
    i32 nbytes = uart_read_bytes(PORT, buffer, size, timeout);
    if (nbytes != 0 && nbytes != -1)
    {
        uart_flush_input(PORT);

        return nbytes;
    }

    return 0;
}

/// @brief Wipes out the uart input FIFO.
void uart_flush_rxd()
{
    uart_flush_input(PORT);
}

/// @brief Getter for the event queue.
QueueHandle_t uart_queue()
{
    return int_queue;
}

/// @brief Initializes the rs232 communication.
///
/// @return ESP_OK On success.
esp_err_t uart_initialize()
{
    uart_config_t uart_config       = {0};
    uart_config.baud_rate           = BAUDRATE;
    uart_config.data_bits           = UART_DATA_8_BITS;
    uart_config.parity              = UART_PARITY_DISABLE;
    uart_config.stop_bits           = UART_STOP_BITS_1;
    uart_config.flow_ctrl           = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 122;
    uart_config.source_clk          = UART_SCLK_APB;

    QueueHandle_t *queue = &int_queue;

    esp_err_t ret = uart_driver_install(PORT, BUF_SIZE, BUF_SIZE, 5, queue, 0);
    if (ret != ESP_OK)
    {
        const char *ename = esp_err_to_name(ret);
        ESP_LOGE(TAG, "Failed to install uart driver : %s", ename);

        return ret;
    }

    ret = uart_param_config(PORT, &uart_config);
    if (ret != ESP_OK)
    {
        const char *ename = esp_err_to_name(ret);
        ESP_LOGE(TAG, "Failed to configure uart driver parameters : %s", ename);

        return ret;
    }

    ret = uart_set_pin(PORT, TXD, RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK)
    {
        const char *ename = esp_err_to_name(ret);
        ESP_LOGE(TAG, "Failed to configure uart driver pinout : %s", ename);

        return ret;
    }

    ret = uart_set_mode(PORT, UART_MODE_UART);
    if (ret != ESP_OK)
    {
        const char *ename = esp_err_to_name(ret);
        ESP_LOGE(TAG, "Failed to configure uart mode to RS232 : %s", ename);

        return ret;
    }

    return ESP_OK;
}
