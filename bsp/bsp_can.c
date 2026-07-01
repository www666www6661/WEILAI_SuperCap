#include "bsp_can.h"

#include <string.h>

#include "bsp.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_fdcan.h"

#define CAN_DEV (&hfdcan1)

extern FDCAN_HandleTypeDef hfdcan1;

static uint32_t s_last_tx_request = 0U;

/**
 * @brief can_filter_init can_it enable can_start
 *
 */
void bsp_can_init(void)
{
    FDCAN_FilterTypeDef can_filter = {0};

    can_filter.IdType = FDCAN_STANDARD_ID;
    can_filter.FilterIndex = 0;
    can_filter.FilterType = FDCAN_FILTER_MASK;
    can_filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    can_filter.FilterID1 = 0x061;
    can_filter.FilterID2 = 0x7FF;  // exact match for 0x061

    HAL_FDCAN_ConfigFilter(CAN_DEV, &can_filter);

    // Reject all other standard and extended IDs
    HAL_FDCAN_ConfigGlobalFilter(CAN_DEV, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

    HAL_FDCAN_ActivateNotification(CAN_DEV, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_Start(CAN_DEV);
}

/**
 * @brief trans a std can packet(add to mailbox)
 *
 * @param can can_channel
 * @param data trans_data_buf
 * @return bsp_status_t
 */
bsp_status_t bsp_can_trans_packet(bsp_can_t can, uint8_t *data)
{
    if (can != BSP_CAN_1)
    {
        return BSP_ERR;
    }

    if (hfdcan1.Instance->PSR & FDCAN_PSR_BO_Msk)
    {
        hfdcan1.Instance->CCCR &= ~FDCAN_CCCR_INIT;
    }

    FDCAN_TxHeaderTypeDef header = {0};
    header.Identifier = 0x051;
    header.IdType = FDCAN_STANDARD_ID;
    header.TxFrameType = FDCAN_DATA_FRAME;
    header.DataLength = FDCAN_DLC_BYTES_8;
    header.ErrorStateIndicator = FDCAN_ESI_PASSIVE;
    header.BitRateSwitch = FDCAN_BRS_OFF;
    header.FDFormat = FDCAN_CLASSIC_CAN;
    header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    header.MessageMarker = 0;

    if (HAL_FDCAN_GetTxFifoFreeLevel(CAN_DEV) == 0U)
    {
        uint32_t pending = s_last_tx_request;

        if (pending == 0U)
        {
            pending = HAL_FDCAN_GetLatestTxFifoQRequestBuffer(CAN_DEV);
        }

        if (pending != 0U)
        {
            HAL_FDCAN_AbortTxRequest(CAN_DEV, pending);
        }
    }

    if (HAL_FDCAN_AddMessageToTxFifoQ(CAN_DEV, &header, data) == HAL_OK)
    {
        s_last_tx_request = HAL_FDCAN_GetLatestTxFifoQRequestBuffer(CAN_DEV);
        return BSP_OK;
    }

    return BSP_ERR;
}

/**
 * @brief get can message (0x61)
 *
 * @param data receive_buffer
 * @param index can_header_index
 * @return bsp_status_t
 */
bsp_status_t bsp_can_get_msg(uint8_t *data, uint32_t *index)
{
    FDCAN_RxHeaderTypeDef rx_header = {0};

    if (HAL_FDCAN_GetRxMessage(CAN_DEV, FDCAN_RX_FIFO0, &rx_header, data) == HAL_OK)
    {
        *index = rx_header.Identifier;
        return BSP_OK;
    }

    return BSP_ERR;
}
