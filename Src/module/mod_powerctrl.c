#include "mod_powerctrl.h"

#include <math.h>
#include <sys/cdefs.h>

#include "comp_pid.h"
#include "comp_utils.h"
#include "dev_buckboost.h"
#include "mod_status.h"

void Module_PowerCtrl_Init(Module_PowerCtrl *this, Module_PowerCtrl_Param param)
{
    this->param_ = param;

    this->dt = this->param_.dt;

    this->sampler_ = this->param_.sampler_;
    this->status_ = this->param_.status_;
    this->conn_ = this->param_.conn_;

    this->pRefree_setpoint_ = this->param_.default_base_referee_power;
    this->status_->PowerLimit_ = this->pRefree_setpoint_;
    this->conn_->refereePowerLimit_ = (uint8_t)(this->pRefree_setpoint_);

    LowPassFilter_Init(&this->pRefree_Filter_, this->param_.pRefree_cutoff_freq);

    Component_PID_Init(&(this->PID_pRefree_), this->param_.preferee);

    Component_PID_Init(&(this->PID_ialpha_), this->param_.ialpha);
    Component_PID_Init(&(this->PID_ibeta_), this->param_.ibeta);
    Component_PID_Init(&(this->PID_igamma_), this->param_.igamma);

    {
        float VbtoVa = MAX(this->sampler_->vbside_.voltage_, 0.1f) / MAX(this->sampler_->vaside_.voltage_, 0.1f);
        Device_BuckBoostMode_t init_mode = (VbtoVa < 0.97f) ? BUCK : ((VbtoVa > 1.03f) ? BOOST : BUCKBOOST);
        Device_BuckBoost_Init(&(this->buckboost_), init_mode);
    }

    this->status_->enableCONV_ = true;
    this->status_->outputEnabled_ = true;

    Device_BuckBoost_Enable();
}

static inline void __attribute__((always_inline)) Module_PowerCtrl_SetRefereePowerLimit(Module_PowerCtrl *this, float limit)
{
    if (limit < 0.0f)
    {
        limit = 0.0f;
    }

    this->pRefree_setpoint_ = limit;
    this->conn_->refereePowerLimit_ = (uint8_t)(this->pRefree_setpoint_);

    Component_PID_Reset(&(this->PID_pRefree_));
}

volatile float temp = 0;
volatile float power_limit_a_to_b;
volatile float power_limit_b_to_a;
// volatile float temp_Powerinput;
//  volatile float temp_Poweroutput;

void Module_PowerCtrl_Control(Module_PowerCtrl *this)
{
    static bool cap_charge_blocked = false;

    if (this->status_->PowerLimit_ != this->pRefree_setpoint_)
    {
        Module_PowerCtrl_SetRefereePowerLimit(this, this->status_->PowerLimit_);
    }

    float raw_referee_power = this->sampler_->vaside_.voltage_ * this->sampler_->iRefree_.current_;
    float referee_power = LowPassFilter_Apply(&this->pRefree_Filter_, raw_referee_power, this->dt);
    float chassis_power = referee_power - this->sampler_->vaside_.voltage_ * this->sampler_->iaside_.current_;

    this->conn_->chassisPower_ = POWER_WATT_TO_U16(chassis_power);
    this->conn_->refereePower_ = POWER_WATT_TO_U16(referee_power);

    float paside = this->sampler_->vaside_.voltage_ * this->sampler_->iaside_.current_;

    static bool cutoff_active = false;

    if (cutoff_active && (this->sampler_->vaside_.voltage_ > this->param_.buckboost.BAT_VOLTAGE_MIN + 1.0f))
    {
        cutoff_active = false;
    }
    else if ((!cutoff_active) && this->sampler_->vaside_.voltage_ < this->param_.buckboost.BAT_VOLTAGE_MIN)
    {
        cutoff_active = true;
    }

    if (!cutoff_active && this->status_->enableCONV_ && this->status_->outputEnabled_)
    {
        // ========================================================================

        float VbtoVa = MAX(this->sampler_->vbside_.voltage_, 0.1f) / MAX(this->sampler_->vaside_.voltage_, 0.1f);

        this->status_->chassisPower_ = chassis_power;
        this->status_->refreePower_ = referee_power;
        this->status_->paside_ = paside;

        if (cap_charge_blocked)
        {
            if (this->sampler_->vbside_.voltage_ < this->param_.cap_chargeresume_voltage)
            {
                cap_charge_blocked = false;
            }
        }
        else if (this->sampler_->vbside_.voltage_ > this->param_.cap_chargestop_voltage)
        {
            cap_charge_blocked = true;
            Component_PID_Reset(&(this->PID_pRefree_));
        }

        this->paside_setpoint_ = Component_PID_Calculate(&(this->PID_pRefree_), this->pRefree_setpoint_ - 2.0f, referee_power + 2.5f, this->dt);

        float cap_out_ilimit;
        float cap_in_ilimit = this->param_.buckboost.I_LIMIT;
        if (this->sampler_->vbside_.voltage_ < this->param_.buckboost.CAP_CUTOFF_VOLTAGE)
        {
            cap_out_ilimit = this->param_.buckboost.CAP_IOUT_MIN;
            cap_in_ilimit = 4.0f;
        }
        else if (this->sampler_->vbside_.voltage_ > this->param_.buckboost.CAP_NORMAL_VOLTAGE)
        {
            cap_out_ilimit = this->param_.buckboost.CAP_IOUT_MAX;
        }
        else
        {
            cap_out_ilimit =
                this->param_.buckboost.CAP_IOUT_MIN + (this->param_.buckboost.CAP_IOUT_MAX - this->param_.buckboost.CAP_IOUT_MIN) *
                                                          (this->sampler_->vbside_.voltage_ - this->param_.buckboost.CAP_CUTOFF_VOLTAGE) /
                                                          (this->param_.buckboost.CAP_NORMAL_VOLTAGE - this->param_.buckboost.CAP_CUTOFF_VOLTAGE);
            clampf(&cap_out_ilimit, this->param_.buckboost.CAP_IOUT_MIN, this->param_.buckboost.CAP_IOUT_MAX);
        }

        power_limit_a_to_b = MIN(this->param_.buckboost.I_LIMIT * this->sampler_->vaside_.voltage_, cap_in_ilimit * this->sampler_->vbside_.voltage_);
        power_limit_b_to_a =
            MAX(-1 * this->param_.buckboost.I_LIMIT * this->sampler_->vaside_.voltage_, -1 * cap_out_ilimit * this->sampler_->vbside_.voltage_);

        if (this->paside_setpoint_ > 0.0f && this->sampler_->vbside_.voltage_ > this->param_.buckboost.CAP_MAX_VOLTAGE * 0.9f)
        {
            float taper_scale =
                (this->param_.buckboost.CAP_MAX_VOLTAGE - this->sampler_->vbside_.voltage_) / (this->param_.buckboost.CAP_MAX_VOLTAGE * 0.1f);
            clampf(&taper_scale, 0.0f, 1.0f);

            float vbside_power_limit = power_limit_a_to_b * taper_scale;

            vbside_power_limit = CLAMP(vbside_power_limit, 0.0f, power_limit_a_to_b);
            this->paside_setpoint_ = MIN(this->paside_setpoint_, vbside_power_limit);
        }

        if (this->paside_setpoint_ < power_limit_b_to_a)
        {
            this->paside_setpoint_ = power_limit_b_to_a;
        }
        else if (this->paside_setpoint_ > power_limit_a_to_b)
        {
            this->paside_setpoint_ = power_limit_a_to_b;
        }

        /*满充停止*/
        if (cap_charge_blocked && this->paside_setpoint_ > 0.0f)
        {
            this->paside_setpoint_ = 0.0f;
        }

        this->conn_->SuperCapOutputMx_ = (uint16_t)(-power_limit_b_to_a);
        this->conn_->OutPutCapability_ =
            (uint8_t)(255.0f *
                      abs_clampf((this->conn_->SuperCapOutputMx_ - this->param_.buckboost.CAP_CUTOFF_VOLTAGE * this->param_.buckboost.CAP_IOUT_MIN) /
                                     (this->sampler_->vaside_.voltage_ * this->param_.buckboost.CAP_IOUT_MAX -
                                      this->param_.buckboost.CAP_CUTOFF_VOLTAGE * this->param_.buckboost.CAP_IOUT_MIN),
                                 1.0f));

        this->iaside_setpoint_ = abs_clampf(this->paside_setpoint_ / MAX(this->sampler_->vaside_.voltage_, 0.1f), this->param_.buckboost.I_LIMIT);

        // =========================================================================

        float k_phase_map = 1.0f;
        if (VbtoVa <= 0.97f)
        {
            k_phase_map = 1.0f / VbtoVa;
        }
        else if (VbtoVa >= 1.03f)
        {
            k_phase_map = 1.0f;
        }
        else
        {
            float k_buck = 1.0f / VbtoVa;
            float blend = (VbtoVa - 0.97f) / (1.03f - 0.97f);
            clampf(&blend, 0.0f, 1.0f);
            k_phase_map = k_buck + blend * (1.0f - k_buck);
        }

        this->iphase_setpoint_ = k_phase_map * this->iaside_setpoint_;

        // 前馈比率 (V_out / V_in)
        float volt_feedforward = VbtoVa;

        // 计算三相均流参考
        float ibase_setpoint = this->iphase_setpoint_ / 3.0f;
        float iavg = (this->sampler_->ialpha_.current_ + this->sampler_->ibeta_.current_ + this->sampler_->igamma_.current_) / 3.0f;

        float ialpha_share = abs_clampf(this->param_.share_gain * (iavg - this->sampler_->ialpha_.current_), this->param_.share_limit);
        float ibeta_share = abs_clampf(this->param_.share_gain * (iavg - this->sampler_->ibeta_.current_), this->param_.share_limit);
        float igamma_share = abs_clampf(this->param_.share_gain * (iavg - this->sampler_->igamma_.current_), this->param_.share_limit);

        float alpha_cmd =
            volt_feedforward *
            (1 + Component_PID_Calculate(&(this->PID_ialpha_), ibase_setpoint + ialpha_share, this->sampler_->ialpha_.current_, this->dt));

        float beta_cmd = volt_feedforward *
                         (1 + Component_PID_Calculate(&(this->PID_ibeta_), ibase_setpoint + ibeta_share, this->sampler_->ibeta_.current_, this->dt));

        float gamma_cmd =
            volt_feedforward *
            (1 + Component_PID_Calculate(&(this->PID_igamma_), ibase_setpoint + igamma_share, this->sampler_->igamma_.current_, this->dt));

        // 模式判断用三相cmd最大值，避免某相已越过边界但模式仍用均值判断
        // 原来用 avg_cmd，导致模式切换边界附近三相时序错乱，可能会引发环流和驱动器损坏
        float max_cmd = MAX(alpha_cmd, MAX(beta_cmd, gamma_cmd));
        Device_BuckBoost_UpdateMode(&(this->buckboost_), max_cmd);

        // 模式切换时三相强制同步一个周期，避免切换瞬间各相状态不一致
        static Device_BuckBoostMode_t last_mode = BUCKBOOST;
        Device_BuckBoostMode_t cur_mode = Device_BuckBoost_GetMode(&(this->buckboost_));
        if (cur_mode != last_mode)
        {
            // 模式刚切换：用三相平均值同步输出一个周期，等待时序稳定
            float sync_cmd = (alpha_cmd + beta_cmd + gamma_cmd) / 3.0f;
            Device_BuckBoost_UpdatePWM(&(this->buckboost_), sync_cmd, sync_cmd, sync_cmd);
            last_mode = cur_mode;
            return;
        }
        last_mode = cur_mode;

        Device_BuckBoost_UpdatePWM(&(this->buckboost_), alpha_cmd, beta_cmd, gamma_cmd);
    }
    else
    {
        this->status_->chassisPower_ = chassis_power;
        this->status_->refreePower_ = referee_power;
        this->status_->paside_ = paside;

        // A侧掉电/欠压、外部禁止使能，或内部保护禁止时，禁止超级电容向A侧反向供电
        this->paside_setpoint_ = 0.0f;
        this->iaside_setpoint_ = 0.0f;
        this->iphase_setpoint_ = 0.0f;

        this->conn_->SuperCapOutputMx_ = 0U;
        this->conn_->OutPutCapability_ = 0U;

        LowPassFilter_Reset(&this->pRefree_Filter_, raw_referee_power);

        Component_PID_Reset(&(this->PID_ialpha_));
        Component_PID_Reset(&(this->PID_ibeta_));
        Component_PID_Reset(&(this->PID_igamma_));
        Component_PID_Reset(&(this->PID_pRefree_));

        Device_BuckBoost_CutOff(&(this->buckboost_));
    }
}
