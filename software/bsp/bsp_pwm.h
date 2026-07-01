#pragma once

#include "bsp.h"
// PWM通道
typedef enum
{
    BSP_PWM_BUZZER,
    BSP_PWM_ASK,
    BSP_PWM_NUM
} bsp_pwm_channel_t;

bsp_status_t bsp_pwm_start(bsp_pwm_channel_t ch);
bsp_status_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle);
bsp_status_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq);
bsp_status_t bsp_pwm_stop(bsp_pwm_channel_t ch);
bsp_status_t bsp_pwmn_start(bsp_pwm_channel_t ch);
bsp_status_t bsp_pwmn_stop(bsp_pwm_channel_t ch);