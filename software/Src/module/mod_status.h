#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "dev_buckboost.h"
#include "dev_led.h"

typedef struct
{
    float capEnergy_;

    float paside_;
    float refreePower_;
    float chassisPower_;
    float PowerLimit_;

    float realVBToVA_;

    float dcdc_mode_;

    bool enableCONV_;
    bool outputEnabled_;
    bool lowBattery_;
    uint8_t errorcode_;
} Module_Status;

static inline void __attribute__((always_inline)) Module_Status_UpdateLED(Module_Status *status, Device_BuckBoostMode_t mode)
{
    if (status == NULL)
    {
        return;
    }

    if (status->errorcode_ != 0U)
    {
        Device_LED_SetSysState(DEV_LED_SYS_FAULT);
    }
    else if (status->lowBattery_)
    {
        Device_LED_SetSysState(DEV_LED_SYS_WARNING);
    }
    else
    {
        Device_LED_SetSysState(DEV_LED_SYS_NORMAL);
    }

    if (!status->enableCONV_ || !status->outputEnabled_ || ((status->errorcode_ & 0x01U) != 0U))
    {
        Device_LED_SetConvState(DEV_LED_CONV_OFF);
        return;
    }

    switch (mode)
    {
    case BUCK:
        Device_LED_SetConvState(DEV_LED_CONV_BUCK);
        break;
    case BOOST:
        Device_LED_SetConvState(DEV_LED_CONV_BOOST);
        break;
    case BUCKBOOST:
    default:
        Device_LED_SetConvState(DEV_LED_CONV_BUCK_BOOST);
        break;
    }
}
