#pragma once
#include <math.h>
#include <stdint.h>

#include "dev_buckboost.h"
#include "mod_conn.h"
#include "mod_samplemanager.h"
#include "mod_status.h"

#define ERROR_UNDER_VOLTAGE 0b00000001    // WARNING
#define ERROR_SHORT_CIRCUIT 0b00001000    // FAULT
#define ERROR_NO_POWER_INPUT 0b00100000   // WARING
#define ERROR_PHASE_UNBALANCE 0b10000000  // FAULT

#define ERROR_SET(code, bit) ((code) |= (bit))
#define ERROR_CLEAR(code, bit) ((code) &= (uint8_t)(~(bit)))
#define ERROR_IS_SET(code, bit) (((code) & (bit)) != 0U)

typedef struct
{
    float UNDER_VOLTAGE;
    float NO_POWER_INPUT_VOLTAGE;
    uint32_t WARNING_DEBOUNCE_CNT;
    uint32_t SHORT_CIRCUIT_VOLTAGE;
    uint32_t SHORT_CIRCUIT_CURRENT;
    float PHASE_SHARE_DIFF;
    uint32_t PHASE_SHARE_DEBOUNCE_CNT;

    Module_Status *status;
    Module_Conn *conn;
    Module_SampleManager *sampler;

} Module_ErrChecker_Param;

typedef struct
{
    uint32_t under_voltage_cnt_;
    uint32_t no_power_input_cnt_;
    uint32_t short_circuit_cnt_;
    uint32_t phase_share_cnt_;

    Module_ErrChecker_Param param_;
    Module_Status *status_;
    Module_Conn *conn_;
    Module_SampleManager *sampler_;
} Module_ErrChecker;

static inline void __attribute__((always_inline)) Module_ErrChecker_Init(Module_ErrChecker *this, Module_ErrChecker_Param param)
{
    this->param_ = param;
    this->status_ = param.status;
    this->conn_ = param.conn;
    this->sampler_ = param.sampler;

    this->under_voltage_cnt_ = 0U;
    this->no_power_input_cnt_ = 0U;
    this->short_circuit_cnt_ = 0U;
    this->phase_share_cnt_ = 0U;
    this->status_->errorcode_ = 0U;
    this->status_->lowBattery_ = false;
    this->conn_->errcode_ = 0U;
}

static inline void __attribute__((always_inline)) Module_ErrChecker_WarningChk(Module_ErrChecker *this)
{
    float vaside = this->sampler_->vaside_.voltage_;

    if (vaside < this->param_.UNDER_VOLTAGE)
    {
        if (this->under_voltage_cnt_ < this->param_.WARNING_DEBOUNCE_CNT)
        {
            this->under_voltage_cnt_++;
        }
    }
    else
    {
        this->under_voltage_cnt_ = 0U;
    }

    if (vaside < this->param_.NO_POWER_INPUT_VOLTAGE)
    {
        if (this->no_power_input_cnt_ < this->param_.WARNING_DEBOUNCE_CNT)
        {
            this->no_power_input_cnt_++;
        }
    }
    else
    {
        this->no_power_input_cnt_ = 0U;
    }

    if (this->under_voltage_cnt_ >= this->param_.WARNING_DEBOUNCE_CNT)
    {
        ERROR_SET(this->status_->errorcode_, ERROR_UNDER_VOLTAGE);
        this->status_->lowBattery_ = true;
    }
    else
    {
        ERROR_CLEAR(this->status_->errorcode_, ERROR_UNDER_VOLTAGE);
        this->status_->lowBattery_ = false;
    }

    if (this->no_power_input_cnt_ >= this->param_.WARNING_DEBOUNCE_CNT)
    {
        ERROR_SET(this->status_->errorcode_, ERROR_NO_POWER_INPUT);
    }
    else
    {
        ERROR_CLEAR(this->status_->errorcode_, ERROR_NO_POWER_INPUT);
    }

    this->conn_->errcode_ = this->status_->errorcode_;
}

static inline void __attribute__((always_inline)) Module_ErrChecker_ShortChk(Module_ErrChecker *this)
{
    float vaside = this->sampler_->vaside_.voltage_;
    float iaside = this->sampler_->iaside_.current_;

    if ((vaside < (float)this->param_.SHORT_CIRCUIT_VOLTAGE) && (fabsf(iaside) > (float)this->param_.SHORT_CIRCUIT_CURRENT))
    {
        ERROR_SET(this->status_->errorcode_, ERROR_SHORT_CIRCUIT);

        if (this->short_circuit_cnt_ < 80U)
        {
            this->short_circuit_cnt_++;
        }

        if (this->short_circuit_cnt_ >= 80U)
        {
            this->short_circuit_cnt_ = 80U;
            this->status_->outputEnabled_ = false;
            Device_BuckBoost_Disable();
        }
    }
    else
    {
        this->short_circuit_cnt_ = 0U;
        ERROR_CLEAR(this->status_->errorcode_, ERROR_SHORT_CIRCUIT);
    }
    this->conn_->errcode_ = this->status_->errorcode_;
}

static inline void __attribute__((always_inline)) Module_ErrChecker_PhaseShareChk(Module_ErrChecker *this)
{
    float ialpha = this->sampler_->ialpha_.current_;
    float ibeta = this->sampler_->ibeta_.current_;
    float igamma = this->sampler_->igamma_.current_;
    float iavg = (ialpha + ibeta + igamma) / 3.0f;

    bool unbalanced = (fabsf(ialpha - iavg) > this->param_.PHASE_SHARE_DIFF) || (fabsf(ibeta - iavg) > this->param_.PHASE_SHARE_DIFF) ||
                      (fabsf(igamma - iavg) > this->param_.PHASE_SHARE_DIFF);

    if (unbalanced)
    {
        if (this->phase_share_cnt_ < this->param_.PHASE_SHARE_DEBOUNCE_CNT)
        {
            this->phase_share_cnt_++;
        }

        if (this->phase_share_cnt_ >= this->param_.PHASE_SHARE_DEBOUNCE_CNT)
        {
            this->status_->outputEnabled_ = false;
            Device_BuckBoost_Disable();
            ERROR_SET(this->status_->errorcode_, ERROR_PHASE_UNBALANCE);
        }
    }
    else
    {
        this->phase_share_cnt_ = 0U;
        ERROR_CLEAR(this->status_->errorcode_, ERROR_PHASE_UNBALANCE);
    }
    this->conn_->errcode_ = this->status_->errorcode_;
}
