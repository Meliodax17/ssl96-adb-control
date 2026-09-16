#include "stm32f10x.h"
#include "uart.h"
#include "main.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_dma.h"
//static void USART1_NVIC_Configuration(void)
//{
//	NVIC_InitTypeDef NVIC_InitStructure; 
//	/* Configure the NVIC Preemption Priority Bits */  
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
//	
//	/* Enable the USARTy Interrupt */
//	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;	 
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
//}


/*
 * 函数名：USART1_Config
 * 描述  ：USART1 GPIO 配置,工作模式配置。9600 8-N-1
 * 输入  ：无
 * 输出  : 无
 * 调用  ：外部调用
 */
void UART_DMA_Config(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar, u32 Direction,u32 u32length)
{
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA_CHx);   
	DMA_InitStructure.DMA_PeripheralBaseAddr = cpar;  
	DMA_InitStructure.DMA_MemoryBaseAddr = cmar;  
	DMA_InitStructure.DMA_DIR = Direction;
	DMA_InitStructure.DMA_BufferSize = u32length; //Default 15 Gu8DMABufferLen
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; 
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  
	DMA_Init(DMA_CHx, &DMA_InitStructure); 
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE); 
	
//	DMA_Cmd(DMA1_Channel5, ENABLE); 	
} 
void USART1_IO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_USART1|RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_USART1,ENABLE);
	/* USART1 GPIO config */
	/* Configure USART1 Tx (PA.9) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);    
	/* Configure USART1 Rx (PA.10) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
}	

void USART1_Config(u32 BaudRate,u8 WordLength,u8 StopBits,u8 Parity)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure; 
//	DMA_InitTypeDef DMA_InitStructure;
//	USART_Cmd(USART1, DISABLE); 
//	USART_DeInit(USART1);
	/* config USART1 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_USART1|RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_USART1,ENABLE);
	/* USART1 GPIO config */
	/* Configure USART1 Tx (PA.9) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);    
	/* Configure USART1 Rx (PA.10) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	USART1_IO_Config();  
	/* UART1 mode config */
	USART_InitStructure.USART_BaudRate = BaudRate;
	if(Parity==1)
	{
		USART_InitStructure.USART_Parity = USART_Parity_Even ;
		if(WordLength==8)
		{
			USART_InitStructure.USART_WordLength = USART_WordLength_9b;
		}
		else{
			USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		}
	}
	else if(Parity==2)
	{
		USART_InitStructure.USART_Parity = USART_Parity_Odd ;
		if(WordLength==8)
		{
			USART_InitStructure.USART_WordLength = USART_WordLength_9b;
		}
		else
		{
			USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		}
	}
	else
	{
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	}
	
	if(StopBits == 1)//1 stop bit 1
	{
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
	}
	else if(StopBits == 2)
	{
		USART_InitStructure.USART_StopBits = USART_StopBits_2;
	}
	
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	
	USART_ClearFlag(USART1, USART_FLAG_TC);
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE); 			

	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);    

//	DMA_Cmd(DMA1_Channel5, DISABLE);
//	DMA_ClearFlag(DMA1_FLAG_TC5);                  
//  DMA_ClearITPendingBit(DMA1_IT_TC5);   
//	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
//	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, DISABLE); 
//	DMA_StructInit(&DMA_InitStructure); 
	
	UART_DMA_Config(DMA1_Channel5,(u32)&USART1->DR,(u32)USART1_Rx_Buf,DMA_DIR_PeripheralSRC,Gu8DMABufferLen); 	
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	

  DMA_ClearFlag(DMA1_FLAG_TC5);
	DMA_ClearITPendingBit(DMA1_IT_GL5);	
  DMA_ClearITPendingBit(DMA1_IT_TC5);             
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE); 
	
	USART_Cmd(USART1, ENABLE); 		
	DMA_Cmd(DMA1_Channel5, ENABLE); 
}
extern uint8_t Gu8DmaComplete;
void DMA1_Channel5_IRQHandler()
{
//	DMA_ClearITPendingBit(DMA1_IT_GL5);
	if(DMA_GetITStatus(DMA1_IT_TC5) != RESET)
	{
		DMA_Cmd(DMA1_Channel5, DISABLE);		
		DMA_ClearITPendingBit(DMA1_IT_TC5);
		DMA_ClearITPendingBit(DMA1_IT_GL5);
 		DMA1_Channel5->CNDTR = Gu8DMABufferLen;   	
		DMA_ClearFlag(DMA1_FLAG_TC5); 		
		Gu8DmaComplete = 1;	
		DMA_Cmd(DMA1_Channel5, ENABLE);//Repeat Enable
//		GPIO_WriteBit(GPIOB, GPIO_Pin_13, 
//			(BitAction)((1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_13))));//B	
	}
}
                    
   
void Uart1_PutChar(u8 ch)
{
//	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE); 			
	/* Write a character to the USART */	

	USART_SendData(USART1, (u8) ch);
	
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void Uart1_PutString(uint8_t * buf , u8 len)
{
	uint8_t i=0;
	for(i=0;i<len;i++)
	{
		Uart1_PutChar(*buf);
		Uart2_PutChar(*buf++);
	}
}
//&&(USART1_Rx_Buf[2]==0x03)

void USART1_Rec(void)
{
	USART1_Rx_Buf[USART1_RxCnt]= USART_ReceiveData(USART1);
	USART1_RxCnt++;
//	GPIO_WriteBit(GPIOB, GPIO_Pin_13, 
//				 (BitAction)((1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_13))));//B
//	if(Gu8data[2] != 0)
//	{
//		USART1_RxCnt = 0;
//	}
//	if(USART1_RxCnt == (Gu8ReadCnt + 5)) //判断数据是否满足要求 
//	{
//		USART1_HaveMes = 1;	  //接收成功			
//	}
//	if(USART1_RxCnt>(Gu8ReadCnt+5))
//	{
//		USART1_RxCnt = 0;
//		USART1_HaveMes = 0;	  //接收失败	 
//	}
}

void USART2_IO(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* USART1 GPIO config */
	/* Configure USART1 Tx (PA.9) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);    
	/* Configure USART1 Rx (PA.10) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);		
}
void USART2_Config(u32 BaudRate,u8 WordLength,u8 StopBits,u8 Parity)
{

	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure; 
//	USART_DeInit(USART2);
	/* config USART1 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	USART2_IO();	  
	/* UART1 mode config */
	USART_InitStructure.USART_BaudRate = BaudRate;
	if(Parity==1)
	{
		USART_InitStructure.USART_Parity = USART_Parity_Even ;
		if(WordLength==8)
		{
			USART_InitStructure.USART_WordLength = USART_WordLength_9b;
		}
		else
		{
			USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		}
	}
	else if(Parity==2)
	{
		USART_InitStructure.USART_Parity = USART_Parity_Odd ;
		if(WordLength==8)
		{
			USART_InitStructure.USART_WordLength = USART_WordLength_9b;
		}
		else
		{
			USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		}
	}
	else
	{
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	}
	if(StopBits==2)
	{
		USART_InitStructure.USART_StopBits = USART_StopBits_2;
	}
	else
	{
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
	}
	
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
	
//	USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);

//	/*USART1 NVIC config*/

//	/* Configure the NVIC Preemption Priority Bits */  
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
//	
//	/* Enable the USARTy Interrupt */
//	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;	 
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
//	USART_Cmd(USART2, ENABLE);	
	USART_ITConfig(USART2, USART_IT_RXNE, DISABLE); 			

	USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);    

//	DMA_Cmd(DMA1_Channel5, DISABLE);
//	DMA_ClearFlag(DMA1_FLAG_TC5);                  
//  DMA_ClearITPendingBit(DMA1_IT_TC5);   
//	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
//	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, DISABLE); 
//	DMA_StructInit(&DMA_InitStructure); 
	
	UART_DMA_Config(DMA1_Channel6,(u32)&USART2->DR,(u32)USART2_Rx_Buf,DMA_DIR_PeripheralSRC,11); 	
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	

  DMA_ClearFlag(DMA1_FLAG_TC6);
	DMA_ClearITPendingBit(DMA1_IT_GL6);	
  DMA_ClearITPendingBit(DMA1_IT_TC6);             
	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE); 
	
	USART_Cmd(USART2, ENABLE); 		
	DMA_Cmd(DMA1_Channel6, ENABLE); 
}


void DMA1_Channel6_IRQHandler()
{

	if(DMA_GetITStatus(DMA1_IT_TC6) != RESET)
	{
		DMA_Cmd(DMA1_Channel6, DISABLE);		
		DMA_ClearITPendingBit(DMA1_IT_TC6);
		DMA_ClearITPendingBit(DMA1_IT_GL6);
 		DMA1_Channel6->CNDTR = 11;   	
		DMA_ClearFlag(DMA1_FLAG_TC6); 		
		USART2_HaveMes = 1;	
		DMA_Cmd(DMA1_Channel6, ENABLE);//Repeat Enable
//		GPIO_WriteBit(GPIOB, GPIO_Pin_13, 
//			(BitAction)((1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_13))));//B	
	}
}
void Uart2_PutChar(u8 ch)
{
	/* Write a character to the USART */	
	USART_SendData(USART2, (u8) ch);
	
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}
void Uart2_PutString(uint8_t * buf , u16 len)
{
	uint16_t i=0;
	for(i=0;i<len;i++)
	{
		Uart2_PutChar(*buf++);
	}
}


//&&(USART1_Rx_Buf[2]==0x03)
void USART2_Rec(void)
{
	USART2_Rx_Buf[USART2_RxCnt]= USART_ReceiveData(USART2);
//				  GPIO_WriteBit(GPIOB, GPIO_Pin_12, 
//							(BitAction)((1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_12))));//B						

	USART2_RxCnt++;
	if(USART2_RxCnt == 11)//判断数据是否满足要求 
	{
		USART2_HaveMes = 1;	  //接收成功			
	}
	if(USART2_RxCnt > 11)
	{
		USART2_RxCnt = 0;
		USART2_HaveMes = 0;	  //接收失败	 
	}
//		if(Gu8data[2] != 0)
//		{
//			USART2_RxCnt = 0;
//		}
//		if(USART2_RxCnt == (Gu8ReadCnt+4+5)) //判断数据是否满足要求 
//		{
//			USART2_HaveMes = 1;	  //接收成功			
//		}
}
//STM32F1 as Master:LIN数据场校验函数
uint8_t Lin_Checksum_Calculate(uint8_t *buf,uint8_t u8Cnt)
{
	uint16_t u16SumData=0;
	uint8_t u8ChecksumData=0;
	uint8_t i;

	for(i=0;i<u8Cnt;i++)
	{
		u16SumData=buf[i]+u16SumData;
		if(u16SumData>0xff)//大于等于256，-255+1
		{
			u16SumData=(u16SumData&0x00ff)+1;
		}
	}
	u8ChecksumData=0xff-u16SumData;
	return(u8ChecksumData);
}
//P0P1奇偶校验，LIN PID
uint8_t Lin_ID_Convert(uint8_t id)
{
  uint8_t p_p0,p_p1;
  uint8_t p_id0,p_id1,p_id2,p_id3,p_id4,p_id5;
  uint8_t p_id=0;
  p_id0=id&0x01;
  p_id1=(id&0x02)>>1;
  p_id2=(id&0x04)>>2;
  p_id3=(id&0x08)>>3;
  p_id4=(id&0x10)>>4;
  p_id5=(id&0x20)>>5;
	
	p_p0 = (p_id0^p_id1^p_id2^p_id4);
	p_p1 = ~(p_id1^p_id3^p_id4^p_id5);
	
	if(p_p1==0xfe){
		p_p1=0;
	}else if(p_p1==0xff){
		p_p1=1;
	}	 
  p_id=(p_p1<<7)|(p_p0<<6)|id;
  return(p_id);
}
//void USART1_LinMaster(uint8_t pid,uint8_t *u8buf,uint8_t u8Cnt)
//{
//	uint8_t PID;//check_sum
//	PID=Lin_ID_Convert(pid);//PID为高俩位增加校验的pid，pid上位机不需要处理，由下位机统一处理
//	USART_SendBreak(USART1);
//  Uart1_PutChar(0x55);
//	Uart1_PutChar(PID);
//	USART1_RxCnt=0;
//	if((pid==0x30)||(pid==0x20)||(pid==0x21)||(pid==0x22)||(pid==0x23)||(pid==0x24)||(pid==0x25)
//		||(pid==0x26)||(pid==0x27)||(pid==0x28)||(pid==0x3d)||(pid==0x11)
//	||(pid==0x29)){//需从机返回数据给上位机
//		USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
////						GPIO_WriteBit(GPIOB, GPIO_Pin_14, 
////					   (BitAction)((1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_14))));	
//	}else{
//		Uart1_PutString(u8buf , u8Cnt);
////		check_sum=Lin_Checksum_Calculate(u8buf,u8Cnt);
//		Uart1_PutChar(Gu8data[14]);	//下位机统一增加校验场
//	}
//}
static void PB12_NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  
  /* 配置P[A|B|C|D|E]0为中断源 */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}



void EXTI_PB12_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_DeInit();
	/* config the extiline(PB13) clock and AFIO clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);

	/* config the NVIC(PB13) */
	PB12_NVIC_Configuration();

	/* EXTI line gpio config(PB12) */	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;       
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	 // 上拉开漏输入
  GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* EXTI line(PB13) mode config */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource12);
  EXTI_InitStructure.EXTI_Line = EXTI_Line12;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //下降沿中断
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure); 
}
void EXTI_PB12_Disable(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_DeInit();
	/* config the extiline(PB13) clock and AFIO clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);

	/* config the NVIC(PB13) */
	PB12_NVIC_Configuration();

	/* EXTI line gpio config(PB12) */	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;       
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	 // 上拉开漏输入
  GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* EXTI line(PB13) mode config */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource12);
  EXTI_InitStructure.EXTI_Line = EXTI_Line12;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //下降沿中断
  EXTI_InitStructure.EXTI_LineCmd = DISABLE;
  EXTI_Init(&EXTI_InitStructure); 
}

