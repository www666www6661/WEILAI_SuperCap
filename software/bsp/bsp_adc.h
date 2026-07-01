#pragma once

#include "bsp.h"

/**
 * @brief ADC1 sampling split after removing injected queue reload.
 *
 * injected group:
 *   trigger : HRTIM TRG4
 *   channel : CH1, CH3
 *   export  : BSP_ADC_Ialpha, BSP_ADC_VA
 *
 * regular group:
 *   trigger : ADC1 regular external trigger configured in CubeMX
 *   channel : CH2, CH8
 *   export  : BSP_ADC_Ibeta, BSP_ADC_NC
 */

typedef enum
{
    BSP_ADC_DEV_1 = 0,
    BSP_ADC_DEV_2,
    BSP_ADC_DEV_NUM
} bsp_adc_dev_t;

typedef enum
{
    BSP_ADC1_BUF_Ibeta = 0,
    BSP_ADC1_BUF_NC,
    BSP_ADC1_BUF_Ialpha,
    BSP_ADC1_BUF_VA,
} bsp_adc1_buf_t;

typedef enum
{
    BSP_ADC2_BUF_VB = 0,
    BSP_ADC2_BUF_Igamma,
    BSP_ADC2_BUF_IREF,
    BSP_ADC2_BUF_IA,
} bsp_adc2_buf_t;

typedef enum
{
    BSP_ADC_Ialpha,
    BSP_ADC_Ibeta,
    BSP_ADC_VA,
    BSP_ADC_NC,
    BSP_ADC_VB,
    BSP_ADC_Igamma,
    BSP_ADC_IREF,
    BSP_ADC_IA,
    BSP_ADC_NUM
} bsp_adc_channel_t;

bsp_status_t bsp_adc_start(bsp_adc_channel_t ch);
bsp_status_t bsp_adc_update(bsp_adc_channel_t ch, uint32_t *buf);