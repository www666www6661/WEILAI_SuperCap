#pragma once

#include "mod_comm.h"
#include "mod_conn.h"
#include "mod_errchecker.h"
#include "mod_powerctrl.h"
#include "mod_samplemanager.h"

typedef struct
{
    Module_SampleManager_Param sampler;
    Module_PowerCtrl_Param powerctrl;
    Module_ErrChecker_Param errchk;

} SuperCap_Param;

typedef struct
{
    Module_Status status_;
    Module_Conn conn_;
    Module_Comm comm_;
    Module_SampleManager sampler_;
    Module_PowerCtrl powerctrl_;
    Module_ErrChecker errchk_;
    volatile uint32_t heartbeat_;
} SuperCap;

void SuperCap_Start(void);
void SuperCap_control(void);
void SuperCap_BackgroundTask(void);
void HRTIM1_Master_IRQHandler(void);
void TIM2_IRQHandler(void);
