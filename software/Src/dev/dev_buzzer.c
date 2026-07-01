#include "dev_buzzer.h"

#include "bsp_hrtim.h"
#include "bsp_pwm.h"
#include "mod_errchecker.h"
#include "stm32g4xx_hal.h"

#define DEV_BUZZER_WARNING_MASK (ERROR_UNDER_VOLTAGE | ERROR_NO_POWER_INPUT)
#define DEV_BUZZER_FAULT_MASK (ERROR_SHORT_CIRCUIT | ERROR_PHASE_UNBALANCE)

void Device_Buzzer_Start() { bsp_pwm_start(BSP_PWM_BUZZER); }

void Device_Buzzer_Stop() { bsp_pwm_stop(BSP_PWM_BUZZER); }

void Device_Buzzer_Set(float freq, float duty_cycle)
{
    bsp_pwm_set_freq(BSP_PWM_BUZZER, freq);
    bsp_pwm_set_comp(BSP_PWM_BUZZER, duty_cycle);
}

void Device_Buzzer_Play(float freq, uint32_t duration_ms)
{
    Device_Buzzer_Set(freq, 0.5f);  // Default 50% duty cycle
    Device_Buzzer_Start();
    HAL_Delay(duration_ms);
    Device_Buzzer_Stop();
}

void Device_Buzzer_PowerOn()
{
    Device_Buzzer_Start();
    HAL_Delay(100);
    Device_Buzzer_Set(1046.50f * 0.65f, 0.5f);
    HAL_Delay(250);

    /* D5 */
    Device_Buzzer_Set(1174.66f * 0.65f, 0.5f);
    HAL_Delay(250);

    /* G5 */
    Device_Buzzer_Set(1567.98f * 0.65f, 0.5f);
    HAL_Delay(250);

    Device_Buzzer_Stop();
}

void Device_Buzzer_UpdateErrorCode(uint8_t errorcode, uint32_t now_ms)
{
    if ((errorcode & DEV_BUZZER_FAULT_MASK) != 0U)
    {
        Device_Buzzer_Set(1218.5f, 0.5f);
        if ((now_ms % 400U) < 200U)
        {
            Device_Buzzer_Start();
        }
        else
        {
            Device_Buzzer_Stop();
        }
        return;
    }

    static uint32_t t0 = 0U;

    if ((errorcode & DEV_BUZZER_WARNING_MASK) != 0U)
    {
        if (!t0)
            t0 = now_ms;
        if ((now_ms - t0) > 100000U)
        {
            Device_Buzzer_Stop();
            return;
        }

        Device_Buzzer_Set(780.0f, 0.5f);
        ((now_ms % 1000U) < 50U) ? Device_Buzzer_Start() : Device_Buzzer_Stop();
        return;
    }
    else
    {
        t0 = 0U;
    }

    Device_Buzzer_Stop();
}
