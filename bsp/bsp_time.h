/**
 * @file mod_time.h
 * @brief 一个与底层硬件配置高度耦合的模块,安排中断的启动和调整
 *
 *
 */
#pragma once
#include "hrtim.h"
#include "stm32g4xx.h"
#include "tim.h"

static inline void __attribute__((always_inline)) bsp_time_hs_start()
{
    if (!(hhrtim1.Instance->sMasterRegs.MDIER & HRTIM_MDIER_MREPIE))
        __HAL_HRTIM_MASTER_ENABLE_IT(&hhrtim1, HRTIM_MASTER_IT_MREP);
}

static inline void __attribute__((always_inline)) bsp_time_ls_start()
{
    if (!(htim2.Instance->DIER & TIM_DIER_UIE))
        HAL_TIM_Base_Start_IT(&htim2);
}