#pragma once

// 功率编码：p*64 + 16384，量程-256W~+768W，分辨率0.015625W
#define POWER_WATT_TO_U16(p) ((uint16_t)((p) * 64.0f + 16384.0f))
#define POWER_U16_TO_WATT(u) (((float)(u) - 16384.0f) / 64.0f)

typedef struct
{
    uint16_t chassisPower_;
    uint16_t refereePower_;
    uint16_t SuperCapOutputMx_;
    uint8_t OutPutCapability_;
    uint8_t refereePowerLimit_;
    uint8_t errcode_;
} Module_Conn;
