/**
  ******************************************************************************
  * @file    stm32f10x_it.c
  * $Author: wdluo $
  * $Revision: 67 $
  * $Date:: 2012-08-15 19:00:29 +0800 #$
  * @brief   中断函数定义.
  ******************************************************************************
  * @attention
  *
  *<h3><center>&copy; Copyright 2009-2012, ViewTool</center>
  *<center><a href="http:\\www.viewtool.com">http://www.viewtool.com</a></center>
  *<center>All Rights Reserved</center></h3>
  * 
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_dma.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "usb_pwr.h"
#include "delay.h"
#include "uart.h"
#include "main.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            			Cortex-M3 处理器的相关服务函数                        */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}
/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	  TimingDelay_Decrement();
}
extern uint16_t Gu16Tim3Cnt;
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET ) /*检查TIM3更新中断发生与否*/
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update); /*清除TIMx更新中断标志 */
		Modetime++;
		g_msTick++;
		Keyin=Keyin_key(); 
		if(0==Keyin)
		{
			B_respond=0;
			Key_In_Timer=0;
		}
		else
		{
			Key_In_Timer++;
			if(0!=(Keyin^Keyvalue))
			{
				Keyvalue=Keyin;
				B_respond=0;
				Key_In_Timer=0;
			}
			else
			{
				if(0==B_respond)
				{
					if(30<Key_In_Timer) //30ms chong doi phim (TIM3 = 1ms)
					{
						switch(Keyvalue)
						{
							case Key_Left:
								B_Key_Left=1;
								break;

							case Key_Up:
								B_Key_Up=1;
								break;

							case Key_Enter:
								B_Key_Enter=1;
								break;
							
							case Key_Combine:
								B_Key_Combine=1;
							default: break;
						}
						B_respond=1;
					}
				}
			}
		}		
	}

}

extern uint8_t Gu8BufCrc[9];
extern void c_fading_process(void);
extern uint16_t LED_main_timer ;
uint8_t Gu8BreathFlag = 0,BreathMode = 0,BreathGama = 0,BreathRed = 0,Gu8TST1 = 0,Gu8T2T3 = 0,Gu8Base = 0,
BreathGreen = 0,BreathBlue = 0,Gu8StopFlag = 0;
uint16_t Gu8TS = 0,Gu8T1 = 0,Gu8T2 = 0,Gu8T3 = 0,Gu8Tim4Cnt = 0;
extern uint16_t crc_16_ibm(uint8_t *buf, uint8_t len);
uint8_t Gu8Tim4State = 0,Gu8Tim4State1 = 0,Gu8Tim4Flag = 0;
uint16_t Gu16TC = 0;
void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET ) /*检查TIM3更新中断发生与否*/
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update); /*清除TIMx更新中断标志 */


	}

}
/******************************************************************************/
/*                 STM32F10x 外设 中断 服务函数                               */
/*  增加需要的外设中断函数在下面。中断的函数名字都已经在startup_stm32f10x_xx.s*/
/*  的文件中定义好了，请参照它来写。                                          */
/******************************************************************************/
/**
  * @brief  USB中断处理函数
  * @note	该中断函数只有STM32F105和STM32F107系列芯片才有这个中断
  * @param  None
  * @retval None
  */
#ifndef STM32F10X_CL
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  USB_Istr();
}
#endif /* STM32F10X_CL */
/**
  * @brief  This function handles OTG WakeUp interrupt request.
  * @note	None
  * @param  None
  * @retval None
  */
void OTG_FS_WKUP_IRQHandler(void)
{
  /* Initiate external resume sequence (1 step) */
  Resume(RESUME_EXTERNAL);  

}
extern uint8_t Gu8DmaComplete ,Gu8RealLen;
void USART1_IRQHandler(void)                	//串口1中断服务程序
{ 
//	uint8_t num = 0;
	if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET) 
	{
		USART_ClearFlag(USART1,USART_FLAG_ORE);
		USART_ReceiveData(USART1);
	}
//	if(USART_GetFlagStatus(USART1, USART_FLAG_IDLE) != RESET) 
//	{
//		USART_ClearFlag(USART1,USART_FLAG_IDLE);//USART_IT_IDLE
//		num = USART1->SR;
//		num = USART1->DR; //?USART_IT_IDLE??
//		DMA_Cmd(DMA1_Channel5,DISABLE);    //??DMA
//		num = 64 -  DMA_GetCurrDataCounter(DMA1_Channel5);      //??????????  
//		Gu8RealLen = num;
//		DMA1_Channel5->CNDTR=64;       
//		DMA_Cmd(DMA1_Channel5,ENABLE); 	
//		Gu8DmaComplete = 1;
//		GPIO_WriteBit(GPIOB, GPIO_Pin_13, 
//			(BitAction)((1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_13))));//B
//	}
	if(USART_GetITStatus(USART1,USART_IT_RXNE)!=RESET) 
	{
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
//		USART1_Rec();
//		GPIO_WriteBit(GPIOB, GPIO_Pin_12, 
//					 (BitAction)((1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_12))));//Dectect
	} 
}
void USART2_IRQHandler(void)                	//串口2中断服务程序
{ 
	if(USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET) {
		USART_ClearFlag(USART2,USART_FLAG_ORE);
		USART_ReceiveData(USART2);
	}
	if(USART_GetITStatus(USART2,USART_IT_RXNE)!=RESET) {
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
		USART2_Rec();
	} 
}
void EXTI15_10_IRQHandler(void)
{
		EXTI_ClearITPendingBit(EXTI_Line12);

//		Delay(2);	
//		GPIO_WriteBit(GPIOB, GPIO_Pin_13, 
//					 (BitAction)((1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_13))));//Dectect
		EXTI_PB12_Disable(); 
//EXTI->IMR &= ~ (EXTI->LineX);
	Gu8ExtFlag = 1;

}

/**
  * @brief  This function handles USB-On-The-Go FS global interrupt request.
  * @note	None
  * @param  None
  * @retval None
  */
#ifdef STM32F10X_CL
void OTG_FS_IRQHandler(void)
{
  STM32_PCD_OTG_ISR_Handler(); 
}
#endif /* STM32F10X_CL */





/*********************************END OF FILE**********************************/
