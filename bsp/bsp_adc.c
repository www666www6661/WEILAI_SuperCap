#include "bsp_adc.h"

#include "adc.h"
#include "bsp.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_adc.h"

#define BSP_ADC_BUF_DEPTH 4
#define BSP_ADC1_DMA_LEN 2
#define BSP_ADC2_DMA_LEN BSP_ADC_BUF_DEPTH

static inline void bsp_adc_dwt_profiler_init(void)
{
    static uint8_t dwt_inited = 0;

    if (dwt_inited != 0U)
        return;
}

/**
 * @brief Raw ADC buffer layout.
 *
 * [BSP_ADC_DEV_1]
 *   [0] ADC1 regular rank1 -> CH2 -> BSP_ADC_Ibeta
 *   [1] ADC1 regular rank2 -> CH8 -> BSP_ADC_NC
 *   [2] ADC1 injected JDR1 -> CH1 -> BSP_ADC_Ialpha
 *   [3] ADC1 injected JDR2 -> CH3 -> BSP_ADC_VA
 *
 * [BSP_ADC_DEV_2]
 *   [0] ADC2 regular rank1 -> BSP_ADC_VB
 *   [1] ADC2 regular rank2 -> BSP_ADC_Igamma
 *   [2] ADC2 regular rank3 -> BSP_ADC_IREF
 *   [3] ADC2 regular rank4 -> BSP_ADC_IA
 */
uint32_t bsp_adc_buf[BSP_ADC_DEV_NUM][BSP_ADC_BUF_DEPTH] = {0};

typedef struct
{
    bsp_adc_dev_t dev;
    uint32_t offset;
} bsp_adc_map_t;

/**
 * @brief BSP ADC channel map.
 */
const bsp_adc_map_t bsp_adc_map[BSP_ADC_NUM] = {
    [BSP_ADC_Ialpha] = {BSP_ADC_DEV_1, BSP_ADC1_BUF_Ialpha},
    [BSP_ADC_Ibeta] = {BSP_ADC_DEV_1, BSP_ADC1_BUF_Ibeta},
    [BSP_ADC_VA] = {BSP_ADC_DEV_1, BSP_ADC1_BUF_VA},
    [BSP_ADC_NC] = {BSP_ADC_DEV_1, BSP_ADC1_BUF_NC},
    [BSP_ADC_VB] = {BSP_ADC_DEV_2, BSP_ADC2_BUF_VB},
    [BSP_ADC_Igamma] = {BSP_ADC_DEV_2, BSP_ADC2_BUF_Igamma},
    [BSP_ADC_IREF] = {BSP_ADC_DEV_2, BSP_ADC2_BUF_IREF},
    [BSP_ADC_IA] = {BSP_ADC_DEV_2, BSP_ADC2_BUF_IA},
};

static inline __attribute__((always_inline)) ADC_HandleTypeDef *bsp_adc_get_handle(bsp_adc_dev_t dev)
{
    switch (dev)
    {
    case BSP_ADC_DEV_1:
        return &hadc1;
    case BSP_ADC_DEV_2:
        return &hadc2;
    default:
        return NULL;
    }
}

/**
 * @brief ADC Calibration
 *        only need be down twice here (adc1/2)
 * @param ch
 * @return bsp_status_t
 */
static bsp_status_t bsp_adc_cal(bsp_adc_channel_t ch)
{
    static uint8_t adc_calibrated[BSP_ADC_DEV_NUM] = {0};
    ADC_HandleTypeDef *adc = NULL;

    if (ch >= BSP_ADC_NUM)
    {
        return BSP_ERR;
    }

    adc = bsp_adc_get_handle(bsp_adc_map[ch].dev);

    if (adc == NULL)
    {
        return BSP_ERR;
    }

    if (adc_calibrated[bsp_adc_map[ch].dev] == 0)
    {
        adc_calibrated[bsp_adc_map[ch].dev] = 1;

        if (HAL_ADCEx_Calibration_Start(adc, ADC_SINGLE_ENDED) != HAL_OK)
        {
            adc_calibrated[bsp_adc_map[ch].dev] = 0;
            return BSP_ERR;
        }
    }
    else
    {
        return BSP_ERR;  // Repeatedly calibration error or Channel error
    }

    return BSP_OK;
}

/**
 * @brief ADC start
 *        only need be down twice here (adc1/2)
 * @param ch
 * @return bsp_status_t
 */
bsp_status_t bsp_adc_start(bsp_adc_channel_t ch)
{
    static uint32_t adc_start_status[BSP_ADC_DEV_NUM] = {0};

    if (ch >= BSP_ADC_NUM)
        return BSP_ERR;

    ADC_HandleTypeDef *adc = bsp_adc_get_handle(bsp_adc_map[ch].dev);

    if (adc == NULL)
        return BSP_ERR;

    if (adc_start_status[bsp_adc_map[ch].dev] == 0)
    {
        bsp_adc_dwt_profiler_init();

        if (bsp_adc_cal(ch) != BSP_OK)
            return BSP_ERR;

        HAL_Delay(100);

        if (bsp_adc_map[ch].dev == BSP_ADC_DEV_1)
        {
            /*
             * ADC1 contains both injected and regular groups.
             * Start both paths together on the first ADC1 request:
             * - injected group   : BSP_ADC_Ialpha / BSP_ADC_VA
             * - regular DMA group: BSP_ADC_Ibeta / BSP_ADC_NC
             */
            if (HAL_ADCEx_InjectedStart_IT(adc) != HAL_OK)
                return BSP_ERR;

            if (HAL_ADC_Start_DMA(adc, bsp_adc_buf[BSP_ADC_DEV_1], BSP_ADC1_DMA_LEN) != HAL_OK)
                return BSP_ERR;
        }
        else if (bsp_adc_map[ch].dev == BSP_ADC_DEV_2)
        {
            /* ADC2 only uses regular group conversion with DMA. */
            if (HAL_ADC_Start_DMA(adc, bsp_adc_buf[BSP_ADC_DEV_2], BSP_ADC2_DMA_LEN) != HAL_OK)
                return BSP_ERR;
        }
        else
        {
            return BSP_ERR;
        }

        adc_start_status[bsp_adc_map[ch].dev] = 1;
    }
    else
    {
        return BSP_ERR;  // Repeatedly start error or Channel error
    }

    return BSP_OK;
}

/**
 * @brief Update ADC buffer value.
 *
 * @param ch BSP ADC channel.
 * @param buf Output buffer.
 * @return bsp_status_t
 */
bsp_status_t bsp_adc_update(bsp_adc_channel_t ch, uint32_t *buf)
{
    if ((ch >= BSP_ADC_NUM) || (buf == NULL))
        return BSP_ERR;

    *buf = bsp_adc_buf[bsp_adc_map[ch].dev][bsp_adc_map[ch].offset];

    return BSP_OK;
}

/**
 * @brief Shared interrupt handler for ADC1 and ADC2.
 *
 * @details
 * Handles ADC1 injected end-of-sequence events and copies injected
 * conversion results from `JDR1`/`JDR2` into the exported BSP buffer.
 * It also clears ADC1 injected queue overflow related flags, then forwards
 * any remaining ADC1 pending interrupts to the HAL layer. ADC2 pending
 * interrupts are directly forwarded to the HAL handler.
 */
void ADC1_2_IRQHandler(void)
{
    uint32_t adc1_pending = ADC1->ISR & ADC1->IER;

    if ((adc1_pending & ADC_FLAG_JEOS) != 0U)
    {
        bsp_adc_buf[BSP_ADC_DEV_1][BSP_ADC1_BUF_Ialpha] = ADC1->JDR1;
        bsp_adc_buf[BSP_ADC_DEV_1][BSP_ADC1_BUF_VA] = ADC1->JDR2;

        ADC1->ISR = ADC_FLAG_JEOS | ADC_FLAG_JEOC;
        adc1_pending &= ~(ADC_FLAG_JEOS | ADC_FLAG_JEOC);
    }

    if ((adc1_pending & ADC_FLAG_JQOVF) != 0U)
    {
        ADC1->ISR = ADC_FLAG_JQOVF;
        adc1_pending &= ~ADC_FLAG_JQOVF;
    }

    if (adc1_pending != 0U)
        HAL_ADC_IRQHandler(&hadc1);

    if ((ADC2->ISR & ADC2->IER) != 0U)
        HAL_ADC_IRQHandler(&hadc2);
}