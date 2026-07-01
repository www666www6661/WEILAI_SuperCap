#pragma once

#include "comp_pid.h"
#include "dev_buckboost.h"
#include "mod_conn.h"
#include "mod_samplemanager.h"
#include "mod_status.h"

typedef struct
{
    float dt;

    Module_Status *status_;
    Module_Conn *conn_;
    Module_SampleManager *sampler_;

    float default_base_referee_power;

    float referee_power_margin;
    float referee_light_load_ratio;

    float share_gain;
    float share_limit;

    float cap_chargestop_voltage;
    float cap_chargeresume_voltage;

    float pRefree_cutoff_freq;

    Component_PID_Param vbside;
    Component_PID_Param ialpha;
    Component_PID_Param ibeta;
    Component_PID_Param igamma;
    Component_PID_Param preferee;

    Device_BuckBoost_Param buckboost;

} Module_PowerCtrl_Param;

typedef struct
{
    float dt;

    float pRefree_setpoint_;
    float paside_setpoint_;
    float iaside_setpoint_;
    float iphase_setpoint_;

    Component_PID PID_ialpha_;
    Component_PID PID_ibeta_;
    Component_PID PID_igamma_;
    Component_PID PID_pRefree_;
    Component_PID PID_energy_;
    LowPassFilter pRefree_Filter_;

    Device_BuckBoost buckboost_;

    Module_PowerCtrl_Param param_;

    Module_Status *status_;
    Module_Conn *conn_;
    Module_SampleManager *sampler_;
} Module_PowerCtrl;

void Module_PowerCtrl_Init(Module_PowerCtrl *this, Module_PowerCtrl_Param param);
void Module_PowerCtrl_Control(Module_PowerCtrl *this);