#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <string.h>

#include "can.h"
#include "uart.h"

static const char TAG[] = "main";

/// @brief Prints out the formated CAN::Message into a string.
///
/// @param message Pointer to the message object.
/// @param str Pointer to a buffer string.
/// @param n sizeof(str)
void snprintf_can_message(const CAN_Message *message, char *str, size_t n)
{
    char msg[n + 1];
    memset(msg, '\0', n + 1);

    i32 j = 0;

    for (i32 i = 0; i < message->data_length_code; i++)
    {
        if (i == (message->data_length_code - 1))
        {
            snprintf(msg + j, sizeof(msg) - j, "%02X", message->data[i]);
            j += 2;
        }
        else
        {
            snprintf(msg + j, sizeof(msg) - j, "%02X ", message->data[i]);
            j += 3;
        }
    }

    msg[j] = '\0';

    snprintf(
        str, n, "CAN: id 0x%04lX  data %u [%s]", message->identifier,
        message->data_length_code, msg);
}

/// @brief Prints out the formated UART buffer into a string.
///
/// @param data Pointer to the buffer.
/// @param data_size size of the data.
/// @param str Pointer to a buffer string.
/// @param n sizeof(str)
void snprintf_uart_message(const byte *data, i32 data_size, char *str, size_t n)
{
    char msg[n + 1];
    memset(msg, '\0', n + 1);

    i32 j = 0;

    for (i32 i = 0; i < data_size; i++)
    {
        if (i == (data_size - 1))
        {
            snprintf(msg + j, sizeof(msg) - j, "%02X", data[i]);
            j += 2;
        }
        else
        {
            snprintf(msg + j, sizeof(msg) - j, "%02X ", data[i]);
            j += 3;
        }
    }

    msg[j] = '\0';

    snprintf(str, n, "UART: data %u [%s]", data_size, msg);
}

/// @brief Handles UART events.
///
/// @param event Pointer to the received event.
void handle_uart_received(const uart_event_t *event)
{
    static byte buffer[2 * sizeof(CAN_Message)];
    i32         recv_size = uart_read(buffer, sizeof(buffer), pdMS_TO_TICKS(1));

    if (event->size != recv_size)
    {
        ESP_LOGE(TAG, "void handle_uart_received -> event->size != recv_size");
        return;
    }

    CAN_Message message = {0};

    i32 index = 0;
    message.identifier |= (u32)(buffer[index++]) << 24;
    message.identifier |= (u32)(buffer[index++]) << 16;
    message.identifier |= (u32)(buffer[index++]) << 8;
    message.identifier |= (u32)(buffer[index++]) << 0;

    message.data_length_code = buffer[index++];

    for (i32 i = 0; i < message.data_length_code; ++i)
    {
        message.data[i] = buffer[index++];
    }

    const bool sent = can_transmit(&message, pdMS_TO_TICKS(2));
    if (!sent)
    {
        ESP_LOGE(TAG, "Failed to transmit CAN message");
    }

    static char from[150] = {0};
    static char to[150]   = {0};

    snprintf_uart_message(buffer, recv_size, from, sizeof(from));
    snprintf_can_message(&message, to, sizeof(to));

    ESP_LOGI(TAG, "sent from %s -> %s", from, to);
}

/// @brief Handles CAN message received.
///
/// @param message Pointer to the received message.
void handle_can_received(const CAN_Message *message)
{
    static byte buffer[2 * sizeof(CAN_Message)];

    i32 index       = 0;
    buffer[index++] = (byte)(message->identifier >> 24);
    buffer[index++] = (byte)(message->identifier >> 16);
    buffer[index++] = (byte)(message->identifier >> 8);
    buffer[index++] = (byte)(message->identifier >> 0);

    buffer[index++] = message->data_length_code;

    for (i32 i = 0; i < message->data_length_code; ++i)
    {
        buffer[index++] = message->data[i];
    }

    const size_t tx_size = index;
    const bool   sent    = uart_send(buffer, tx_size);
    if (!sent)
    {
        ESP_LOGE(TAG, "Failed to transmit UART message");
    }

    static char from[150] = {0};
    static char to[150]   = {0};

    snprintf_can_message(message, from, sizeof(from));
    snprintf_uart_message(buffer, tx_size, to, sizeof(to));

    ESP_LOGI(TAG, "sent from %s -> %s", from, to);
}

void app_main(void)
{
    can_initialize(0);
    uart_initialize();

    QueueHandle_t queue = uart_queue();

    for (;;)
    {
        uart_event_t     event = {0};
        const BaseType_t received =
            xQueueReceive(queue, &event, pdMS_TO_TICKS(5));
        if (received == pdTRUE)
        {
            switch (event.type)
            {
                case UART_DATA:
                    handle_uart_received(&event);
                    break;

                case UART_BREAK:
                    // do nothing
                    break;

                case UART_BUFFER_FULL:
                case UART_FIFO_OVF:
                    uart_flush_rxd();
                    break;

                default:
                    ESP_LOGW(TAG, "UART event type: %d", event.type);
                    break;
            }
        }

        CAN_Message message  = {0};
        const bool  can_rcvd = can_receive(&message, pdMS_TO_TICKS(2));
        if (can_rcvd)
        {
            handle_can_received(&message);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
