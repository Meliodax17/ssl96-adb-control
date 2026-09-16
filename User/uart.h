#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"


uint8_t Lin_ID_Convert(uint8_t id);
uint8_t Lin_Checksum_Calculate(uint8_t *buf,uint8_t u8Cnt);
void USART1_LinMaster(uint8_t pid,uint8_t *u8buf,uint8_t u8Cnt);
void USART2_IO(void);
void USART2_Config(u32 BaudRate,u8 WordLength,u8 StopBits,u8 Parity);
void Uart2_PutString(uint8_t * buf , u16 len);
void Uart2_PutChar(u8 ch);
void USART2_Rec(void);

void USART1_Config(u32 BaudRate,u8 WordLength,u8 StopBits,u8 Parity);
void Uart1_PutString(uint8_t * buf , u8 len);
void Uart1_PutChar(u8 ch);
void USART1_Rec(void);
void EXTI_PB12_Config(void);
void EXTI_PB12_Disable(void);
void USART1_IO_Config(void);
void UART_DMA_Config(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar, u32 Direction,u32 u32length);
#endif /* __UART_H */


