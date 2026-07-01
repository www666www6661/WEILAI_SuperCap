/**
 * @file dev_sampler.h
 * @author concon
 * @brief
 * "adc计算得出的数据往往与真实数据之间存在误差，而且基本表现为线性误差"--ENTERPRIZE_RM2024-SuperCap-开源报告。
 *        所以直接将adc_sum与真实数据直接使用一个线性方程转换,
 *        即 voltage_/currrent_ = k * sum_ + b,并使用一阶线性滤波平稳adc数值
 */
#pragma once

#include <stdint.h>

#include "bsp_adc.h"
#include "comp_filter.h"
#include "stm32g4xx.h"
#include "stm32g4xx_hal.h"

typedef struct
{
    bsp_adc_channel_t adc_channel;
    float k;
    float b;
    float cutoff_freq;  // 滤波截止频率
} Device_Sampler_Param;

typedef struct
{
    Device_Sampler_Param param_;
    float adc_val_;  // 滤波后的ADC值
    float voltage_;
    LowPassFilter lpf_;  // 滤波器
} Device_Volt_Sampler;

typedef struct
{
    Device_Sampler_Param param_;
    float adc_val_;  // 滤波后的ADC值
    float current_;
    LowPassFilter lpf_;  // 滤波器
} Device_Current_Sampler;

/**
 * @brief Initializes the voltage sampler.
 * @param this Pointer to the Device_Volt_Sampler instance.
 * @param param Initialization paraabsmeters.
 */
static inline void Device_Volt_Sampler_Init(Device_Volt_Sampler *this, Device_Sampler_Param param)
{
    this->param_ = param;
    LowPassFilter_Init(&(this->lpf_), this->param_.cutoff_freq);
    bsp_adc_start(this->param_.adc_channel);

    this->adc_val_ = 0;
    this->voltage_ = 0;
}

/**
 * @brief Initializes the current sampler.
 * @param this Pointer to the Device_Current_Sampler instance.
 * @param param Initialization parameters.
 */
static inline void Device_Current_Sampler_Init(Device_Current_Sampler *this, Device_Sampler_Param param)
{
    this->param_ = param;
    LowPassFilter_Init(&(this->lpf_), this->param_.cutoff_freq);
    bsp_adc_start(this->param_.adc_channel);

    this->adc_val_ = 0;
    this->current_ = 0;
}

/**
 * @brief Gets the latest voltage value.
 * @param this Pointer to the Device_Volt_Sampler instance.
 * @param dt Sampling interval in seconds.
 * @return The calculated voltage value.
 */
static inline float __attribute__((always_inline)) Device_Sampler_GetVoltage(Device_Volt_Sampler *this, float dt)
{
    uint32_t raw_val = 0;
    bsp_adc_update(this->param_.adc_channel, &raw_val);

    this->adc_val_ = LowPassFilter_Apply(&(this->lpf_), (float)raw_val, dt);

    return this->voltage_ = this->param_.k * this->adc_val_ + this->param_.b;
}

/**
 * @brief Gets the latest current value.
 * @param this Pointer to the Device_Current_Sampler instance.
 * @param dt Sampling interval in seconds.
 * @return The calculated current value.
 */
static inline float __attribute__((always_inline)) Device_Sampler_GetCurrrent(Device_Current_Sampler *this, float dt)
{
    uint32_t raw_val = 0;

    bsp_adc_update(this->param_.adc_channel, &raw_val);
    this->adc_val_ = LowPassFilter_Apply(&(this->lpf_), (float)raw_val, dt);
    return this->current_ = this->param_.k * this->adc_val_ + this->param_.b;
}