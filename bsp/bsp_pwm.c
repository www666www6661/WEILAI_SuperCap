#include "bsp_pwm.h"

#include <assert.h>

#include "bsp.h"
#include "main.h"
#include "stm32g4xx.h"
#include "stm32g4xx_hal_cortex.h"
#include "stm32g4xx_hal_tim.h"

/**
 * @brief TIM1 计数时钟频率 (Hz)
 * @note TIM_CLK = PCLK2 / (Prescaler + 1)
 *       PCLK2 = 170MHz (SystemCoreClock)
 *       Prescaler = 16
 *       TIM_CLK = 170,000,000 / (16 + 1) = 10,000,000 Hz
 */
#define BUZZER_TIM_CLK_HZ 10000000.0f

extern TIM_HandleTypeDef htim1;  // buzzer & TIM1_CH3N

typedef struct
{
    void *tim;
    uint32_t channel;
} bsp_pwm_config_t;

static bsp_pwm_config_t bsp_pwm_map[BSP_PWM_NUM] = {
    [BSP_PWM_BUZZER] = {&htim1, TIM_CHANNEL_2},
    [BSP_PWM_ASK] = {&htim1, TIM_CHANNEL_3},
};

/**
 * @brief Start PWM output on a specific channel.
 * @param ch The PWM channel to start.
 * @return bsp_status_t Status of the operation.
 * two mode reguler/multi(hrtim)
 */
bsp_status_t bsp_pwm_start(bsp_pwm_channel_t ch)
{
    HAL_TIM_PWM_Start(bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel);
    return BSP_OK;
}

/**
 * @brief Start complementary PWM output on a specific channel.
 * @param ch The PWM channel to start.
 * @return bsp_status_t Status of the operation.
 */
bsp_status_t bsp_pwmn_start(bsp_pwm_channel_t ch)
{
    HAL_TIMEx_PWMN_Start(bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel);
    return BSP_OK;
}

/**
 * @brief Set the duty cycle for a specific PWM channel.
 * @param ch The PWM channel to configure.
 * @param duty_cycle The duty cycle to set (from 0.0f to 1.0f).
 * @return bsp_status_t Status of the operation.
 */
bsp_status_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle)
{
    if (duty_cycle > 1.0f)
    {
        duty_cycle = 1.0f;
    }
    if (duty_cycle < 0.0f)
    {
        duty_cycle = 0.f;
    }

    uint16_t pulse = (uint16_t)(duty_cycle * (float)__HAL_TIM_GET_AUTORELOAD((TIM_HandleTypeDef *)bsp_pwm_map[ch].tim));
    __HAL_TIM_SET_COMPARE((TIM_HandleTypeDef *)bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel, pulse);
    return BSP_OK;
}

/**
 * @brief Set the frequency for a specific PWM channel.
 * @param ch The PWM channel to configure.
 * @param freq The frequency to set in Hz.
 * @return bsp_status_t Status of the operation.
 */
bsp_status_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq)
{
    TIM_HandleTypeDef *tim = (TIM_HandleTypeDef *)bsp_pwm_map[ch].tim;
    uint32_t reload = (uint32_t)(BUZZER_TIM_CLK_HZ / freq);

    if (reload > 0)
    {
        __HAL_TIM_SET_AUTORELOAD(tim, reload - 1);
    }
    else
    {
        return BSP_ERR;
    }
    return BSP_OK;
}

/**
 * @brief shutdown specific pwm channel
 *
 * @param ch pwm_channel
 * @return bsp_status_t
 */
bsp_status_t bsp_pwm_stop(bsp_pwm_channel_t ch)
{
    HAL_TIM_PWM_Stop(bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel);
    return BSP_OK;
}

bsp_status_t bsp_pwmn_stop(bsp_pwm_channel_t ch)
{
    HAL_TIMEx_PWMN_Stop(bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel);
    return BSP_OK;
}
