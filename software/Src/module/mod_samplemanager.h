#pragma once

#include "comp_utils.h"
#include "dev_sampler.h"

typedef struct
{
    float dt;

    Device_Sampler_Param vaside;
    Device_Sampler_Param vbside;

    Device_Sampler_Param iaside;
    Device_Sampler_Param ialpha;
    Device_Sampler_Param ibeta;
    Device_Sampler_Param igamma;

    Device_Sampler_Param iRefree;
} Module_SampleManager_Param;

typedef struct
{
    float dt;

    Device_Volt_Sampler vaside_;
    Device_Volt_Sampler vbside_;

    Device_Current_Sampler iaside_;
    Device_Current_Sampler ialpha_;
    Device_Current_Sampler ibeta_;
    Device_Current_Sampler igamma_;

    Device_Current_Sampler iRefree_;

    Module_SampleManager_Param param_;
} Module_SampleManager;

static inline void __attribute__((always_inline)) Module_Sampler_Init(Module_SampleManager *this, Module_SampleManager_Param param)
{
    this->param_ = param;

    this->dt = this->param_.dt;

    Device_Volt_Sampler_Init(&this->vaside_, this->param_.vaside);
    Device_Volt_Sampler_Init(&this->vbside_, this->param_.vbside);

    Device_Current_Sampler_Init(&this->iaside_, this->param_.iaside);
    Device_Current_Sampler_Init(&this->ialpha_, this->param_.ialpha);
    Device_Current_Sampler_Init(&this->ibeta_, this->param_.ibeta);
    Device_Current_Sampler_Init(&this->igamma_, this->param_.igamma);

    Device_Current_Sampler_Init(&this->iRefree_, this->param_.iRefree);
}

/**
 * @brief This function can only read correct data when the ADC sampling cycle
 * is started.
 *
 * @param this
 */
static inline void __attribute__((always_inline)) Module_Sampler_Update(Module_SampleManager *this)
{
    Device_Sampler_GetVoltage(&(this->vaside_), this->dt);
    Device_Sampler_GetVoltage(&(this->vbside_), this->dt);

    Device_Sampler_GetCurrrent(&(this->iaside_), this->dt);
    Device_Sampler_GetCurrrent(&(this->ialpha_), this->dt);
    Device_Sampler_GetCurrrent(&(this->ibeta_), this->dt);
    Device_Sampler_GetCurrrent(&(this->igamma_), this->dt);

    Device_Sampler_GetCurrrent(&(this->iRefree_), this->dt);
}
