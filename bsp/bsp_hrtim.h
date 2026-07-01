#pragma once

#include <stdbool.h>

#include "bsp.h"
#include "hrtim.h"
#include "stm32g4xx_hal_hrtim.h"

#define HRTIM_PERIOD 24000.0f  // defined on stm32cubemx

typedef enum
{
    BSP_HRTIM_Aalpha,
    BSP_HRTIM_Balpha,
    BSP_HRTIM_Abeta,
    BSP_HRTIM_Bbeta,
    BSP_HRTIM_Agamma,
    BSP_HRTIM_Bgamma,
    BSP_HRTIM_NUM
} bsp_hrtim_channel_t;

typedef struct
{
    uint32_t timer_id;
    uint32_t timer_idx;
    uint32_t output_1;
    uint32_t output_2;
} bsp_hrtim_config_t;

extern HRTIM_HandleTypeDef hhrtim1;
static const bsp_hrtim_config_t bsp_hrtim_map[BSP_HRTIM_NUM] = {
    [BSP_HRTIM_Aalpha] = {.timer_id = HRTIM_TIMERID_TIMER_B,
                          .timer_idx = HRTIM_TIMERINDEX_TIMER_B,
                          .output_1 = HRTIM_OUTPUT_TB1,
                          .output_2 = HRTIM_OUTPUT_TB2},
    [BSP_HRTIM_Balpha] = {.timer_id = HRTIM_TIMERID_TIMER_A,
                          .timer_idx = HRTIM_TIMERINDEX_TIMER_A,
                          .output_1 = HRTIM_OUTPUT_TA1,
                          .output_2 = HRTIM_OUTPUT_TA2},
    [BSP_HRTIM_Abeta] = {.timer_id = HRTIM_TIMERID_TIMER_F,
                         .timer_idx = HRTIM_TIMERINDEX_TIMER_F,
                         .output_1 = HRTIM_OUTPUT_TF1,
                         .output_2 = HRTIM_OUTPUT_TF2},
    [BSP_HRTIM_Bbeta] = {.timer_id = HRTIM_TIMERID_TIMER_E,
                         .timer_idx = HRTIM_TIMERINDEX_TIMER_E,
                         .output_1 = HRTIM_OUTPUT_TE1,
                         .output_2 = HRTIM_OUTPUT_TE2},
    [BSP_HRTIM_Agamma] = {.timer_id = HRTIM_TIMERID_TIMER_C,
                          .timer_idx = HRTIM_TIMERINDEX_TIMER_C,
                          .output_1 = HRTIM_OUTPUT_TC1,
                          .output_2 = HRTIM_OUTPUT_TC2},
    [BSP_HRTIM_Bgamma] = {.timer_id = HRTIM_TIMERID_TIMER_D,
                          .timer_idx = HRTIM_TIMERINDEX_TIMER_D,
                          .output_1 = HRTIM_OUTPUT_TD1,
                          .output_2 = HRTIM_OUTPUT_TD2},
};

#define BSP_HRTIM_ALL_TIMER_IDS                                                                                                     \
    (HRTIM_TIMERID_MASTER | bsp_hrtim_map[BSP_HRTIM_Aalpha].timer_id | bsp_hrtim_map[BSP_HRTIM_Balpha].timer_id |                   \
     bsp_hrtim_map[BSP_HRTIM_Abeta].timer_id | bsp_hrtim_map[BSP_HRTIM_Bbeta].timer_id | bsp_hrtim_map[BSP_HRTIM_Agamma].timer_id | \
     bsp_hrtim_map[BSP_HRTIM_Bgamma].timer_id)

#define BSP_HRTIM_ALL_OUTPUTS                                                                                                         \
    (bsp_hrtim_map[BSP_HRTIM_Aalpha].output_1 | bsp_hrtim_map[BSP_HRTIM_Aalpha].output_2 | bsp_hrtim_map[BSP_HRTIM_Balpha].output_1 | \
     bsp_hrtim_map[BSP_HRTIM_Balpha].output_2 | bsp_hrtim_map[BSP_HRTIM_Abeta].output_1 | bsp_hrtim_map[BSP_HRTIM_Abeta].output_2 |   \
     bsp_hrtim_map[BSP_HRTIM_Bbeta].output_1 | bsp_hrtim_map[BSP_HRTIM_Bbeta].output_2 | bsp_hrtim_map[BSP_HRTIM_Agamma].output_1 |   \
     bsp_hrtim_map[BSP_HRTIM_Agamma].output_2 | bsp_hrtim_map[BSP_HRTIM_Bgamma].output_1 | bsp_hrtim_map[BSP_HRTIM_Bgamma].output_2)

/**
 * @brief Start the output of all HRTIM channels.
 * @return bsp_status_t Status of the operation.
 */
static inline bsp_status_t __attribute__((always_inline)) bsp_hrtim_allch_start(void)
{
    HAL_HRTIM_WaveformCountStart(&hhrtim1, BSP_HRTIM_ALL_TIMER_IDS);
    HAL_Delay(10);  // 等待波形对齐（可能有效吧）
    HAL_HRTIM_WaveformOutputStart(&hhrtim1, BSP_HRTIM_ALL_OUTPUTS);
    return BSP_OK;
}

/**
 * @brief Start the output of all HRTIM channels.
 * @param ch The HRTIM channel to stop.
 * @return bsp_status_t Status of the operation.
 */
static inline bsp_status_t __attribute__((always_inline)) bsp_hrtim_allch_stop()
{
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, BSP_HRTIM_ALL_OUTPUTS);
    HAL_HRTIM_WaveformCountStop(&hhrtim1, BSP_HRTIM_ALL_TIMER_IDS);
    return BSP_OK;
}

/**
 * @brief Set the duty cycle for a specific HRTIM channel.
 * @param ch The HRTIM channel to configure.w
 * @param duty_cycle The duty cycle to set (from 0.0000～01f to 0.9999～f).
 * @return bsp_status_t Status of the operation.
 */
static inline bsp_status_t __attribute__((always_inline)) bsp_hrtim_set_comp(bsp_hrtim_channel_t ch, float duty_cycle)
{
    static HRTIM_CompareCfgTypeDef compare_config = {0};

    if (duty_cycle == 0)
    {
        compare_config.CompareValue = 0xFFDF;
        HAL_HRTIM_WaveformCompareConfig(&hhrtim1, bsp_hrtim_map[ch].timer_idx, HRTIM_COMPAREUNIT_1, &compare_config);
        HAL_HRTIM_WaveformCompareConfig(&hhrtim1, bsp_hrtim_map[ch].timer_idx, HRTIM_COMPAREUNIT_3, &compare_config);
        return BSP_OK;
    }

    compare_config.CompareValue = HRTIM_PERIOD / 2 * (1 - duty_cycle);
    HAL_HRTIM_WaveformCompareConfig(&hhrtim1, bsp_hrtim_map[ch].timer_idx, HRTIM_COMPAREUNIT_1, &compare_config);
    compare_config.CompareValue = HRTIM_PERIOD / 2 * (1 + duty_cycle);
    HAL_HRTIM_WaveformCompareConfig(&hhrtim1, bsp_hrtim_map[ch].timer_idx, HRTIM_COMPAREUNIT_3, &compare_config);
    return BSP_OK;
}