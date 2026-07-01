#pragma once
#include "bsp.h"

typedef enum
{
    BSP_CAN_1,
    BSP_CAN_ERR,
} bsp_can_t;

typedef enum
{
    CAN_FORMAT_STD_DATA,
    CAN_FORMAT_EXT_DATA,
    CAN_FORMAT_STD_REMOTE,
    CAN_FORMAT_EXT_REMOTE,
} bsp_can_format_t;

void bsp_can_init(void);
bsp_status_t bsp_can_trans_packet(bsp_can_t can, uint8_t *data);
bsp_status_t bsp_can_get_msg(uint8_t *data, uint32_t *index);
