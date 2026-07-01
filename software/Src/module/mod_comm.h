#pragma once

#include <stdbool.h>

#include "bsp_can.h"
#include "mod_conn.h"
#include "mod_status.h"

typedef struct TxData
{
    uint8_t refereePowerLimit;
    uint16_t chassisPower;
    uint16_t refereePower;
    uint16_t SuperCapOutputMx;
    uint8_t OutPutCapability;
} __attribute__((packed)) Txdata;

typedef struct RxData
{
    uint8_t enableCONV : 1;
    uint8_t resv : 1;
    uint8_t resv0 : 1;
    uint8_t resv1 : 1;
    uint8_t resv2 : 1;
    uint8_t resv3 : 1;
    uint16_t refereePowerLimit;
    uint16_t resv4;
    uint8_t resv5;
    int16_t resv6;
} __attribute__((packed)) RxData;

typedef struct
{
    RxData rxdata;
    Txdata txdata;
    Module_Status *status_;
    Module_Conn *conn_;
} Module_Comm;

void Module_Comm_Init(Module_Comm *this, Module_Status *status, Module_Conn *conn);
bsp_status_t Module_Comm_Transmit(Module_Comm *this);
