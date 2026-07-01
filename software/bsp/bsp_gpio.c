#include "bsp_gpio.h"

#include "main.h"
#include "stm32g4xx_hal_gpio.h"

typedef struct
{
    uint16_t pin;
    GPIO_TypeDef *gpio;
} bsp_gpio_map_t;

static const bsp_gpio_map_t bsp_gpio_map[BSP_GPIO_NUM] = {
    [BSP_GPIO_LED1_B] = {GPIO_PIN_2, GPIOE},
    [BSP_GPIO_LED1_R] = {GPIO_PIN_4, GPIOE},
    [BSP_GPIO_LED2_B] = {GPIO_PIN_3, GPIOE},
    [BSP_GPIO_LED2_R] = {GPIO_PIN_5, GPIOE},
};

/**
 * @brief Write a value to a specific GPIO pin.
 * @param gpio The GPIO pin to write to.
 * @param value The value to write (true for high, false for low).
 * @return bsp_status_t Status of the operation.
 */
inline bsp_status_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value)
{
    HAL_GPIO_WritePin(bsp_gpio_map[gpio].gpio, bsp_gpio_map[gpio].pin, value);
    return BSP_OK;
}

/**
 * @brief Toggle a specific GPIO pin.
 * @param gpio The GPIO pin to toggle.
 * @return bsp_status_t Status of the operation.
 */
inline bsp_status_t bsp_gpio_toggle_pin(bsp_gpio_t gpio)
{
    HAL_GPIO_TogglePin(bsp_gpio_map[gpio].gpio, bsp_gpio_map[gpio].pin);
    return BSP_OK;
}

/**
 * @brief Read the value of a specific GPIO pin.
 * @param gpio The GPIO pin to read from.
 * @return bool The value of the pin (true for high, false for low).
 */
inline bool bsp_gpio_read_pin(bsp_gpio_t gpio) { return HAL_GPIO_ReadPin(bsp_gpio_map[gpio].gpio, bsp_gpio_map[gpio].pin); }
