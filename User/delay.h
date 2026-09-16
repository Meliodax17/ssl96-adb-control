#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

void TimingDelay_Decrement(void);
void Delay(__IO uint32_t nTime);
void TIM3_Config(void);
void TIM4_Config(void);
void TIM4_Cfg(uint16_t u16Period,uint16_t u16Prescaler);
#endif /* __DELAY_H */

