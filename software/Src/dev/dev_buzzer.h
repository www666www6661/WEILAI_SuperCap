#pragma once
#include "stdint.h"

void Device_Buzzer_Start(void);
void Device_Buzzer_Stop(void);
void Device_Buzzer_Set(float freq, float duty_cycle);
void Device_Buzzer_PowerOn(void);
void Device_Buzzer_Play(float freq, uint32_t duration_ms);
void Device_Buzzer_UpdateErrorCode(uint8_t errorcode, uint32_t now_ms);
