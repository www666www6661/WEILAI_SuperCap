#pragma once

#include "comp_filter.h"
#include "component.h"

/* PID参数 */
typedef struct PID_Param {
  float k;         /* 控制器增益，设置为1用于并行模式 */
  float p;         /* 比例项增益，设置为1用于标准形式 */
  float i;         /* 积分项增益 */
  float i_limit;   /* 积分项上限 */
  float out_limit; /* 输出绝对值限制 */
} Component_PID_Param;

// TODO:考虑是否优化掉这个last
typedef struct Last {
  float err;  /* 上次误差 */
  float k_fb; /* 上次反馈值 */
  float out;  /* 上次输出 */
} Component_PID_Last;

typedef struct Component_PID {
  Component_PID_Param param_;
  Component_PID_Last last_;
  float i_; /* 积分 */
} Component_PID;

void Component_PID_Init(Component_PID *pid, Component_PID_Param param_);

float Component_PID_Calculate(Component_PID *this, float sp, float fb,
                              float dt);
void Component_PID_SetK(Component_PID *this, float k);
void Component_PID_SetP(Component_PID *this, float p);
void Component_PID_SetI(Component_PID *this, float i);
void Component_PID_Reset(Component_PID *this);
