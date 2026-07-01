#pragma once

#include "bsp.h"
#include "stdbool.h"

typedef enum
{
    BSP_GPIO_LED1_B,
    BSP_GPIO_LED1_R,
    BSP_GPIO_LED2_B,
    BSP_GPIO_LED2_R,
    BSP_GPIO_NUM,
} bsp_gpio_t;

bsp_status_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value);
bsp_status_t bsp_gpio_toggle_pin(bsp_gpio_t gpio);
bool bsp_gpio_read_pin(bsp_gpio_t gpio);