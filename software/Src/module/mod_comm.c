#include "mod_comm.h"

#include <string.h>

#include "SuperCap.h"
#include "bsp.h"
#include "fdcan.h"

extern SuperCap supercap;

void Module_Comm_Init(Module_Comm *this, Module_Status *status, Module_Conn *conn)
{
    memset(&this->rxdata, 0, sizeof(this->rxdata));
    memset(&this->txdata, 0, sizeof(this->txdata));
    this->status_ = status;
    this->conn_ = conn;
    bsp_can_init();
}

bsp_status_t temp1;

bsp_status_t Module_Comm_Transmit(Module_Comm *this)
{
    this->txdata.chassisPower = this->conn_->chassisPower_;
    this->txdata.refereePower = this->conn_->refereePower_;
    this->txdata.SuperCapOutputMx = this->conn_->SuperCapOutputMx_;
    this->txdata.OutPutCapability = this->conn_->OutPutCapability_;
    this->txdata.refereePowerLimit = this->conn_->refereePowerLimit_;

    return temp1 = bsp_can_trans_packet(BSP_CAN_1, (uint8_t *)&this->txdata);
}

void FDCAN1_IT0_IRQHandler(void)
{
    uint32_t rx_id = 0U;
    uint8_t raw_data[sizeof(RxData)] = {0};
    RxData rx_data = {0};

    if (__HAL_FDCAN_GET_FLAG(&hfdcan1, FDCAN_FLAG_RX_FIFO0_NEW_MESSAGE) == RESET)
    {
        return;
    }

    if (bsp_can_get_msg(raw_data, &rx_id) == BSP_OK)
    {
        if (rx_id == 0x061U)
        {
            memcpy(&rx_data, raw_data, sizeof(RxData));
            supercap.status_.enableCONV_ = (rx_data.enableCONV != 0U);
            supercap.status_.PowerLimit_ = (float)rx_data.refereePowerLimit;
        }
    }

    HAL_FDCAN_IRQHandler(&hfdcan1);
}