#include "SuperCap.h"

#include <stdint.h>
#include <sys/cdefs.h>

#include "bsp_adc.h"
#include "bsp_time.h"
#include "dev_buckboost.h"
#include "dev_buzzer.h"
#include "dev_led.h"
#include "iwdg.h"
#include "mod_errchecker.h"
#include "mod_powerctrl.h"
#include "mod_status.h"
#include "tim.h"

SuperCap supercap;

#define CPU_FREQ_HZ (170000000UL)
#define HRTIM_ISR_CYCLE_BUDGET ((uint32_t)(DT * (float)CPU_FREQ_HZ + 0.5f))

void SuperCap_Init(SuperCap *this, SuperCap_Param param)
{
    bsp_time_hs_start();
    bsp_time_ls_start();
    Module_Sampler_Init(&supercap.sampler_, param.sampler);
    Module_ErrChecker_Init(&supercap.errchk_, param.errchk);
    Module_PowerCtrl_Init(&this->powerctrl_, param.powerctrl);
    Module_Comm_Init(&this->comm_, &this->status_, &this->conn_);
    this->heartbeat_ = 0U;
    Device_LED_Init();
    Device_LED_SetSysState(DEV_LED_SYS_NORMAL);
    //  1. 开启 CoreDebug 中的 TRCENA 位，允许使用跟踪组件
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // 2. 将 DWT 计数器清零
    DWT->CYCCNT = 0;
    // 3. 开启 DWT 控制寄存器中的 CYCCNTENA 位，开始计数
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

#define DT (24000.0f * 8.0f / (170000000.0f * 32.0f))  // HRTIM MREP实际控制周期，约35.294us / 28.333kHz

void SuperCap_Start()
{
    /* clang-format off */
SuperCap_Param param_ = {
    .sampler = {
        .dt = DT, // HRTIM MREP actual loop rate: about 28.333kHz
        /* CALIBRATION_BEGIN */
        .vaside = {
            .adc_channel = BSP_ADC_VA,
            .k = 0.0073428809f,
            .b = (-0.0676202320f),
            .cutoff_freq = 250.0000000000f
        },
        .vbside = {
            .adc_channel = BSP_ADC_VB,
            .k = 0.0072455170f,
            .b = (-0.0428535364f),
            .cutoff_freq = 250.0000000000f
        },
        .iaside = {
            .adc_channel = BSP_ADC_IA,
            .k = 0.0170487785f,
            .b = (-34.8319823082f),
            .cutoff_freq = 600.0000000000f
        },
        .ialpha = {
            .adc_channel = BSP_ADC_Ialpha,
            .k = (-0.0169824765f),
            .b = 34.6869424325f,
            .cutoff_freq = 600.0000000000f
        },
        .ibeta = {
            .adc_channel = BSP_ADC_Ibeta,
            .k = (-0.0173633552f),
            .b = 35.4643917976f,
            .cutoff_freq = 600.0000000000f
        },
        .igamma = {
            .adc_channel = BSP_ADC_Igamma,
            .k = (-0.0167286818f),
            .b = 34.1624723030f,
            .cutoff_freq = 600.0000000000f
        },
        .iRefree = {
            .adc_channel = BSP_ADC_IREF,
            .k = 0.0138746097f,
            .b = (-28.2941082940f),
            .cutoff_freq = 250.0000000000f
        }
        /* CALIBRATION_END */

    },
    .powerctrl = {
        .dt = DT, // HRTIM MREP actual loop rate: about 28.333kHz
        .sampler_ = &(supercap.sampler_),
        .status_ = &(supercap.status_),
        .conn_ = &(supercap.conn_),
        .default_base_referee_power = 45.0f,
        .referee_power_margin = 2.0f,
        .referee_light_load_ratio = 0.6f,
        .share_gain = 0.4f,
        .share_limit = 4.f,
        .cap_chargestop_voltage = 28.6f,
        .cap_chargeresume_voltage = 28.0f,
        .pRefree_cutoff_freq = 120.0f,
        .ialpha = {
            .k = 0.1f,
            .p = 0.12f,
            .i = 3.9f,
            .i_limit = 0.9f,
            .out_limit = 0.99f
        },
        .ibeta = {
            .k = 0.1f,
            .p = 0.12f,
            .i = 3.9f,
            .i_limit = 0.9f,
            .out_limit = 0.99f
        },
        .igamma = {
            .k = 0.1f,
            .p = 0.12f,
            .i = 3.9f,
            .i_limit = 0.9f,
            .out_limit = 0.99f
        },
        .preferee = {
            .k = 5.0f,
            .p = 10.1f,
            .i = 17.5f,
            .i_limit = 800.0f,
            .out_limit = 800.0f
        },
        .buckboost = {
            .CAP_CUTOFF_VOLTAGE = 6.3f,
            .CAP_MAX_VOLTAGE = 28.8f,
            .CAP_NORMAL_VOLTAGE = 18.0f,
            .CAP_IOUT_MAX = 27.5f,
            .CAP_IOUT_MIN = 0.1f,
            .I_LIMIT = 27.5f,
            .BAT_VOLTAGE_MIN = 19.0f
        }
    },
    .errchk = {
        .UNDER_VOLTAGE = 18.0f,
        .NO_POWER_INPUT_VOLTAGE = 12.0f,
        .WARNING_DEBOUNCE_CNT = 80U,
        .SHORT_CIRCUIT_VOLTAGE = 4.0f,
        .SHORT_CIRCUIT_CURRENT = 40.0f,
        .PHASE_SHARE_DIFF = 1.5f,
        .PHASE_SHARE_DEBOUNCE_CNT = 180U,
        .sampler = &(supercap.sampler_),
        .conn = &(supercap.conn_),
        .status = &(supercap.status_),
    }
};
/*clang-format on*/

    SuperCap_Init(&supercap,param_);
    Device_Buzzer_PowerOn();
}

inline void __attribute__((always_inline))  SuperCap_control(){
  
    Module_Sampler_Update(&(supercap.sampler_));
    Module_ErrChecker_ShortChk(&(supercap.errchk_));
    Module_ErrChecker_PhaseShareChk(&(supercap.errchk_));
    Module_PowerCtrl_Control(&(supercap.powerctrl_));

}

void SuperCap_BackgroundTask(void)
{
    Module_Status_UpdateLED(&(supercap.status_), Device_BuckBoost_GetMode(&(supercap.powerctrl_.buckboost_)));
    Device_Buzzer_UpdateErrorCode(supercap.status_.errorcode_, HAL_GetTick());
    Device_LED_Task(HAL_GetTick());
}

static inline void __attribute__((always_inline)) SuperCap_HeartbeatCheck(void)
{
    static uint32_t last_heartbeat = 0U;
    static uint32_t ALLt = 0;

    if (supercap.heartbeat_ != last_heartbeat)
    {
        last_heartbeat = supercap.heartbeat_;
        HAL_IWDG_Refresh(&hiwdg);
        ALLt = 0;
        return;
    }

    /* 超时阈值硬编码为 100：若 TIM2 连续 100 次检测到 heartbeat 未变化，则停止喂狗 */
    if (ALLt < 100U)
    {
        ALLt++;
        HAL_IWDG_Refresh(&hiwdg);
    }
}

static inline void __attribute__((always_inline)) SuperCap_CommTask(void)
{
    Module_Comm_Transmit(&supercap.comm_);
}

volatile uint32_t ALLt = 0;
volatile uint32_t supercap_irq_cycles_last = 0;
volatile uint32_t supercap_irq_cycles_max = 0;
volatile uint32_t supercap_irq_load_permille_last = 0;
volatile uint32_t supercap_irq_load_permille_max = 0;
/**
 * @brief HRTIM MREP control cycle, about 28.333kHz; pid/pwm update/short check
 *
 */
void HRTIM1_Master_IRQHandler(void) {

    uint32_t cycle_start = DWT->CYCCNT;

    __HAL_HRTIM_MASTER_CLEAR_IT(&hhrtim1, HRTIM_MASTER_IT_MREP);
    supercap.heartbeat_++;
    SuperCap_control();


  if (__HAL_HRTIM_MASTER_GET_FLAG(&hhrtim1, HRTIM_MASTER_FLAG_MREP) !=
      RESET) // blocking detected
  {
    ALLt++;
    __HAL_HRTIM_MASTER_CLEAR_IT(&hhrtim1,HRTIM_MASTER_IT_MREP); // stall the loop 
  }

  uint32_t cycle_cost = DWT->CYCCNT - cycle_start;
  supercap_irq_cycles_last = cycle_cost;
  if (cycle_cost > supercap_irq_cycles_max) {
    supercap_irq_cycles_max = cycle_cost;
  }

  supercap_irq_load_permille_last =
      (uint32_t)(((uint64_t)cycle_cost * 1000ULL) / HRTIM_ISR_CYCLE_BUDGET);
  supercap_irq_load_permille_max =
      (uint32_t)(((uint64_t)supercap_irq_cycles_max * 1000ULL) / HRTIM_ISR_CYCLE_BUDGET);
}

/**
 * @brief 1khz control
 *
 */
void TIM2_IRQHandler(void) { 
    static uint32_t comm_divider = 0U;

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    SuperCap_HeartbeatCheck();
    Module_ErrChecker_WarningChk(&(supercap.errchk_));

    comm_divider++;
    if (comm_divider >= 5U)
    {
        comm_divider = 0U;
        SuperCap_CommTask();
    }
 }
