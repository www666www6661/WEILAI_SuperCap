#pragma once
#include <stdbool.h>

#include "bsp_hrtim.h"
#include "comp_utils.h"

typedef enum
{
    PHAS_ALPHA,
    PHAS_BETA,
    PHAS_GAMMA,
    PHAS_NUM
} Device_BuckBoost_Phase_t;

typedef enum
{
    BUCK,
    BOOST,
    BUCKBOOST,
} Device_BuckBoostMode_t;

typedef struct
{
    float CAP_IOUT_MAX;
    float CAP_CUTOFF_VOLTAGE;
    float CAP_IOUT_MIN;
    float CAP_NORMAL_VOLTAGE;
    float CAP_MAX_VOLTAGE;
    float I_LIMIT;
    float BAT_VOLTAGE_MIN;
} Device_BuckBoost_Param;

typedef struct
{
    bsp_hrtim_channel_t chA;
    bsp_hrtim_channel_t chB;
} Device_BuckBoost_Phase;

typedef struct
{
    Device_BuckBoost_Phase phase[PHAS_NUM];
    Device_BuckBoostMode_t mode_;
} Device_BuckBoost;

static inline void Device_BuckBoost_Init(Device_BuckBoost *this, Device_BuckBoostMode_t mode)
{
    this->phase[PHAS_ALPHA].chA = BSP_HRTIM_Aalpha;
    this->phase[PHAS_ALPHA].chB = BSP_HRTIM_Balpha;

    this->phase[PHAS_BETA].chA = BSP_HRTIM_Abeta;
    this->phase[PHAS_BETA].chB = BSP_HRTIM_Bbeta;

    this->phase[PHAS_GAMMA].chA = BSP_HRTIM_Agamma;
    this->phase[PHAS_GAMMA].chB = BSP_HRTIM_Bgamma;

    this->mode_ = mode;
}

static inline void __attribute__((always_inline)) Device_BuckBoost_Disable() { bsp_hrtim_allch_stop(); }

static inline void __attribute__((always_inline)) Device_BuckBoost_Enable() { bsp_hrtim_allch_start(); }

static inline Device_BuckBoostMode_t __attribute__((always_inline)) Device_BuckBoost_UpdateMode(Device_BuckBoost *this, float VBToVA)
{
    Device_BuckBoostMode_t mode = this->mode_;

    if (mode == BUCKBOOST)
    {
        if (VBToVA < 0.90f || VBToVA > 1.10f)  // BUCKBOOST退出区稍宽，避免模式抖动
            mode = (VBToVA < 1.0f) ? BUCK : BOOST;
    }
    else
    {
        if (VBToVA > 0.97f && VBToVA < 1.03f)  // 只在接近1:1时进入BUCKBOOST
        {
            mode = BUCKBOOST;
        }
        else
        {
            mode = (VBToVA < 1.0f) ? BUCK : BOOST;
        }
    }

    this->mode_ = mode;
    return mode;
}

static inline Device_BuckBoostMode_t __attribute__((always_inline)) Device_BuckBoost_GetMode(Device_BuckBoost *this) { return this->mode_; }

static inline void __attribute__((always_inline)) Device_BuckBoost_SetPWM(Device_BuckBoost *this,
                                                                          Device_BuckBoost_Phase_t phase,
                                                                          float dutyA,
                                                                          float dutyB)
{
    bsp_hrtim_set_comp(this->phase[phase].chA, dutyA);
    bsp_hrtim_set_comp(this->phase[phase].chB, dutyB);
}

static inline void __attribute__((always_inline)) Device_BuckBoost_UpdatePWMPhase(Device_BuckBoost *this,
                                                                                  Device_BuckBoost_Phase_t phase,
                                                                                  float VBToVA)
{
    float dutyA = 0.0f;
    float dutyB = 0.0f;

    const float duty_max = 0.97f;        // duty上限
    const float duty_base = 0.96f;       // BUCK/BOOST固定边占空比
    const float buckboost_gain = 0.33f;  // BUCKBOOST系数，避免x≈1时占空比过高

    if (this->mode_ == BUCKBOOST)
    {
        VBToVA = CLAMP(VBToVA, 0.73f, 1.37f);  // 保证BUCKBOOST两边都不易饱和
        dutyA = buckboost_gain * (VBToVA + 1.0f);
        dutyB = buckboost_gain * (1.0f / VBToVA + 1.0f);
    }
    else if (this->mode_ == BUCK)
    {
        VBToVA = CLAMP(VBToVA, 0.10f, duty_max / duty_base);  // BUCK线性区限幅
        dutyA = duty_base * VBToVA;
        dutyB = duty_base;
    }
    else if (this->mode_ == BOOST)
    {
        VBToVA = CLAMP(VBToVA, duty_base / duty_max, 2.3f);  // BOOST线性区限幅
        dutyA = duty_base;
        dutyB = duty_base / VBToVA;
    }

    dutyA = CLAMP(dutyA, 0.0f, duty_max);
    dutyB = CLAMP(dutyB, 0.0f, duty_max);

    Device_BuckBoost_SetPWM(this, phase, dutyA, dutyB);
}

static inline void __attribute__((always_inline)) Device_BuckBoost_UpdatePWM(Device_BuckBoost *this,
                                                                             float alpha_VBToVA,
                                                                             float beta_VBToVA,
                                                                             float gamma_VBToVA)
{
    Device_BuckBoost_UpdatePWMPhase(this, PHAS_ALPHA, alpha_VBToVA);
    Device_BuckBoost_UpdatePWMPhase(this, PHAS_BETA, beta_VBToVA);
    Device_BuckBoost_UpdatePWMPhase(this, PHAS_GAMMA, gamma_VBToVA);
}

static inline void __attribute__((always_inline)) Device_BuckBoost_CutOff(Device_BuckBoost *this)
{
    Device_BuckBoost_SetPWM(this, PHAS_ALPHA, 0.0f, 0.0f);
    Device_BuckBoost_SetPWM(this, PHAS_BETA, 0.0f, 0.0f);
    Device_BuckBoost_SetPWM(this, PHAS_GAMMA, 0.0f, 0.0f);
}
