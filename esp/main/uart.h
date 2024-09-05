#ifndef UART_H
#define UART_H

#include <freertos/FreeRTOS.h>

#include "types.h"

bool uart_send(const byte *buffer, size_t size);
i32  uart_read(byte *buffer, size_t size, TickType_t timeout);

void uart_flush_rxd();

QueueHandle_t uart_queue();

esp_err_t uart_initialize();

#endif
