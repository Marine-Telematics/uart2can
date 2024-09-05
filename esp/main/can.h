#ifndef CAN_H
#define CAN_H

#include <driver/twai.h>

#include "types.h"

typedef twai_message_t CAN_Message;

bool can_receive(CAN_Message *message, TickType_t wait_time);
bool can_transmit(const CAN_Message *message, TickType_t wait_time);

void can_initialize(i32 controller_id);

#endif
