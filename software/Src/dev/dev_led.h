/**
 * @file dev_led.h
 * @author concon
 * @brief 双色 LED 指示灯设备驱动头文件，负责控制系统级指示灯(LED1)和功率变换器模式指示灯(LED2)。
 * @version 1.0
 * @date 2026-04-09
 *
 *
 */

#pragma once
#include "stdint.h"

/**
 * @brief 系统状态定义 (对应 LED1)
 *
 * 用于指示系统整体的运行状态、故障警告等。
 */
typedef enum
{
    DEV_LED_SYS_NORMAL = 0,  /* 蓝灯慢闪 (系统待机/正常运行) */
    DEV_LED_SYS_COMM,        /*蓝灯快闪 (数据通信中) */
    DEV_LED_SYS_FAULT,       /*红灯常亮 (发生严重硬件故障) */
    DEV_LED_SYS_WARNING,     /*红灯闪烁 (系统警告, 如电量低) */
    DEV_LED_SYS_CALIBRATION, /*红蓝交替闪烁 (标定/固件升级模式) */
    DEV_LED_SYS_OFF          /*熄灭 (系统关闭) */
} dev_led_sys_state_t;

/**
 * @brief 变换器工作模式定义 (对应 LED2)
 *
 * 用于指示功率电路(Buck/Boost变换器)的当前工作模式。
 */
typedef enum
{
    DEV_LED_CONV_OFF = 0,   /*熄灭 (变换器关闭/空闲) */
    DEV_LED_CONV_BUCK,      /*红灯常亮 (Buck 降压充电模式) */
    DEV_LED_CONV_BOOST,     /*蓝灯常亮 (Boost 升压放电模式) */
    DEV_LED_CONV_BUCK_BOOST /*红蓝双亮呈紫色 (Buck-Boost 模式) */
} dev_led_conv_state_t;

void Device_LED_Init(void);
void Device_LED_SetSysState(dev_led_sys_state_t state);
void Device_LED_SetConvState(dev_led_conv_state_t state);
void Device_LED_Task(uint32_t tick_ms);