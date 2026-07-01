/**
 * @file    dev_led.c
 * @brief   双色 LED 指示灯设备驱动
 * @details 负责控制系统级指示灯(LED1)和功率变换器模式指示灯(LED2)。
 *          实现了常亮、闪烁等效果，并提供统一的接口。
 */

#include "dev_led.h"

#include "bsp_gpio.h"

/**
 * @brief 当前的系统状态
 */
static dev_led_sys_state_t sys_state = DEV_LED_SYS_OFF;

/**
 * @brief 当前的变换器工作状态
 */
static dev_led_conv_state_t conv_state = DEV_LED_CONV_OFF;

/**
 * @brief 初始化 LED 设备
 * @details 关闭所有 LED，并将状态机复位为 OFF 状态。
 */
void Device_LED_Init(void)
{
    sys_state = DEV_LED_SYS_OFF;
    conv_state = DEV_LED_CONV_OFF;

    bsp_gpio_write_pin(BSP_GPIO_LED1_B, false);
    bsp_gpio_write_pin(BSP_GPIO_LED1_R, false);
    bsp_gpio_write_pin(BSP_GPIO_LED2_B, false);
    bsp_gpio_write_pin(BSP_GPIO_LED2_R, false);
}

/**
 * @brief 设置系统级指示灯状态 (LED1)
 * @param state 要设置的系统状态
 * @details 状态切换时会先熄灭当前 LED，防止颜色干扰。
 */
void Device_LED_SetSysState(dev_led_sys_state_t state)
{
    if (sys_state != state)
    {
        sys_state = state;
        /* 切换状态时先关闭 LED1 的所有颜色 */
        bsp_gpio_write_pin(BSP_GPIO_LED1_B, false);
        bsp_gpio_write_pin(BSP_GPIO_LED1_R, false);
    }
}

/**
 * @brief 设置变换器模式指示灯状态 (LED2)
 * @param state 要设置的变换器模式
 * @details 直接根据传入的工作模式点亮对应颜色的灯（红: Buck, 蓝: Boost, 紫: Buck-Boost）
 */
void Device_LED_SetConvState(dev_led_conv_state_t state)
{
    if (conv_state != state)
    {
        conv_state = state;
        switch (state)
        {
        case DEV_LED_CONV_OFF:
            bsp_gpio_write_pin(BSP_GPIO_LED2_B, false); /* 蓝灭 */
            bsp_gpio_write_pin(BSP_GPIO_LED2_R, false); /* 红灭 */
            break;
        case DEV_LED_CONV_BUCK:
            bsp_gpio_write_pin(BSP_GPIO_LED2_B, false); /* 蓝灭 */
            bsp_gpio_write_pin(BSP_GPIO_LED2_R, true);  /* 红亮 (Buck) */
            break;
        case DEV_LED_CONV_BOOST:
            bsp_gpio_write_pin(BSP_GPIO_LED2_B, true);  /* 蓝亮 (Boost) */
            bsp_gpio_write_pin(BSP_GPIO_LED2_R, false); /* 红灭 */
            break;
        case DEV_LED_CONV_BUCK_BOOST:
            bsp_gpio_write_pin(BSP_GPIO_LED2_B, true); /* 蓝亮 */
            bsp_gpio_write_pin(BSP_GPIO_LED2_R, true); /* 红亮 (紫, Buck-Boost) */
            break;
        }
    }
}

/**
 * @brief LED 驱动任务
 * @param tick_ms 当前系统运行毫秒数 (一般传入 HAL_GetTick())
 * @details 需要在主循环或定时器中周期调用，通过时间戳计算驱动 LED 闪烁效果。
 *          内部基准时间刻度为 100ms。
 */
void Device_LED_Task(uint32_t tick_ms)
{
    static uint32_t last_tick = 0;
    uint32_t dt = tick_ms - last_tick;

    /* 每 100ms 作为一个基础时间刻度进行判断 */
    if (dt >= 100)
    {
        last_tick = tick_ms;
        static uint8_t counter_100ms = 0;
        counter_100ms++;

        /* 针对 LED1 动态闪烁处理 */
        switch (sys_state)
        {
        case DEV_LED_SYS_NORMAL:
            /* 蓝灯慢闪 (1Hz): 500ms 亮, 500ms 灭 -> 每 5 个 100ms 翻转 */
            if (counter_100ms % 5 == 0)
            {
                bsp_gpio_toggle_pin(BSP_GPIO_LED1_B);
            }
            break;

        case DEV_LED_SYS_COMM:
            /* 蓝灯快闪 (5Hz): 100ms 亮, 100ms 灭 -> 每 1 个 100ms 翻转 */
            bsp_gpio_toggle_pin(BSP_GPIO_LED1_B);
            break;

        case DEV_LED_SYS_FAULT:
            /* 红灯常亮 */
            bsp_gpio_write_pin(BSP_GPIO_LED1_R, true);
            break;

        case DEV_LED_SYS_WARNING:
            /* 红灯慢闪 (1Hz) */
            if (counter_100ms % 5 == 0)
            {
                bsp_gpio_toggle_pin(BSP_GPIO_LED1_R);
            }
            break;

        case DEV_LED_SYS_CALIBRATION:
            /* 红蓝交替闪烁 (2.5Hz) -> 每 2 个 100ms 翻转 */
            if (counter_100ms % 2 == 0)
            {
                bool current_blue = bsp_gpio_read_pin(BSP_GPIO_LED1_B);
                bsp_gpio_write_pin(BSP_GPIO_LED1_B, !current_blue);
                bsp_gpio_write_pin(BSP_GPIO_LED1_R, current_blue); /* 交替亮灭 */
            }
            break;

        case DEV_LED_SYS_OFF:
        default:
            break;
        }
    }
}