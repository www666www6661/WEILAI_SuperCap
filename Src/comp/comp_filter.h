/*
  各类滤波器。
*/

#pragma once

#include "component.h"

#ifndef M_2PI
#define M_2PI 6.283185f
#endif

/* 一阶数字低通滤波器 */
typedef struct {
  float cut_freq_;
  float last_out_;
  float last_k_;
  float last_t_;
  bool initialized_;
} LowPassFilter;

/* 二阶巴特沃斯低通滤波器 */
typedef struct {
  float cutoff_freq_; /* 截止频率 */

  float a1_;
  float a2_;

  float b0_;
  float b1_;
  float b2_;

  float delay_element_1_;
  float delay_element_2_;
} LowPassFilter2p;

static inline void __attribute__((always_inline))
LowPassFilter_Init(LowPassFilter *lpf, float cut_freq) {
  lpf->cut_freq_ = cut_freq;
  lpf->last_out_ = 0;
  lpf->last_k_ = 0;
  lpf->last_t_ = 0;
  lpf->initialized_ = false;
}

static inline float __attribute__((always_inline))
LowPassFilter_Apply(LowPassFilter *lpf, float sample, float dt) {
  if (lpf->initialized_ == false) {
    lpf->last_out_ = sample;
    lpf->initialized_ = true;
    return sample; // 第一帧直接返回原值，不滤波
  }

#define DT_STATIC

#include "comp_filter.h"
#include <math.h>
#include <stdbool.h>

#ifdef DT_STATIC
  float k = 0;
  if (lpf->last_t_ == dt) { // 第一次时，last_t_是初始化值0
    k = lpf->last_k_;       // 高速定dt系统规避除法
  } else if (lpf->last_t_ != dt) {
    k = 2 * M_2PI * lpf->cut_freq_ * dt;
    k = k / (1 + k);
    lpf->last_k_ = k;
    lpf->last_t_ = dt;
  }
#else
  float k = 2 * M_2PI * lpf->cut_freq_ * dt;
  k = k / (1 + k);
#endif

  float out = k * sample + (1 - k) * lpf->last_out_;
  lpf->last_out_ = out;

  return out;
}

static inline void __attribute__((always_inline))
LowPassFilter_Reset(LowPassFilter *lpf, float sample) {
  lpf->last_out_ = sample;
}

static inline void __attribute__((always_inline))
LowPassFilter2p_Init(LowPassFilter2p *lpf, float sample_freq,
                     float cutoff_freq) {
  lpf->cutoff_freq_ = cutoff_freq;
  lpf->delay_element_1_ = 0.0f;
  lpf->delay_element_2_ = 0.0f;

  if (lpf->cutoff_freq_ <= 0.0f) {
    /* no filtering */
    lpf->b0_ = 1.0f;
    lpf->b1_ = 0.0f;
    lpf->b2_ = 0.0f;

    lpf->a1_ = 0.0f;
    lpf->a2_ = 0.0f;

    return;
  }
  const float FR = sample_freq / lpf->cutoff_freq_;
  const float OHM = tanf(M_PI / FR);
  const float C = 1.0f + 2.0f * cosf(M_PI / 4.0f) * OHM + OHM * OHM;

  lpf->b0_ = OHM * OHM / C;
  lpf->b1_ = 2.0f * lpf->b0_;
  lpf->b2_ = lpf->b0_;

  lpf->a1_ = 2.0f * (OHM * OHM - 1.0f) / C;
  lpf->a2_ = (1.0f - 2.0f * cosf(M_PI / 4.0f) * OHM + OHM * OHM) / C;
}

static inline float __attribute__((always_inline))
LowPassFilter2p_Apply(LowPassFilter2p *lpf, float sample) {
  /* do the filtering */
  float delay_element_0 = sample - lpf->delay_element_1_ * lpf->a1_ -
                          lpf->delay_element_2_ * lpf->a2_;

  if (isinf(delay_element_0)) {
    /* don't allow bad values to propagate via the filter */
    delay_element_0 = sample;
  }

  const float OUTPUT = delay_element_0 * lpf->b0_ +
                       lpf->delay_element_1_ * lpf->b1_ +
                       lpf->delay_element_2_ * lpf->b2_;

  lpf->delay_element_2_ = lpf->delay_element_1_;
  lpf->delay_element_1_ = delay_element_0;

  /* return the value. Should be no need to check limits */
  return OUTPUT;
}

static inline float __attribute__((always_inline))
LowPassFilter2p_Reset(LowPassFilter2p *lpf, float sample) {
  const float DVAL = sample / (lpf->b0_ + lpf->b1_ + lpf->b2_);

  if (isfinite(DVAL)) {
    lpf->delay_element_1_ = DVAL;
    lpf->delay_element_2_ = DVAL;

  } else {
    lpf->delay_element_1_ = sample;
    lpf->delay_element_2_ = sample;
  }

  return LowPassFilter2p_Apply(lpf, sample);
}
