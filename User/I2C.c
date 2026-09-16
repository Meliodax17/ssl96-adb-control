#include "stm32f10x.h"
/*================================= */
    
/* Private typedef -----------------------------------------------------------*/ 
/* Private define ------------------------------------------------------------*/ 
#define I2C_Speed              100000 
#define I2C1_SLAVE_ADDRESS7    0xC0 
#define I2C_PageSize           256 

/* Private macro -------------------------------------------------------------*/ 
/* Private variables ---------------------------------------------------------*/ 
vu8 FRAM_ADDRESS; 
extern uint8_t data[64];
/* Private function prototypes -----------------------------------------------*/ 

void I2C_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA, ENABLE);
//  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_10MHz;
//  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_OD;
//  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;
//  GPIO_Init(GPIOB, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_OD;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_14|GPIO_Pin_15;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_Init(GPIOB, &GPIO_InitStructure);//IO2
	
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_Init(GPIOB, &GPIO_InitStructure);//IO1
	
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
  GPIO_Init(GPIOB, &GPIO_InitStructure);//KEY1/KEY2

	GPIO_SetBits(GPIOB, GPIO_Pin_8);//LIN ENABLE
	GPIO_SetBits(GPIOC, GPIO_Pin_13);//Green
	GPIO_SetBits(GPIOC, GPIO_Pin_15);//Blue
	GPIO_ResetBits(GPIOC, GPIO_Pin_14);//电源指示灯 Red,

	
	GPIO_ResetBits(GPIOB, GPIO_Pin_13);
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);

}

void PIN_GPIO_Config_MISO(GPIOMode_TypeDef num1)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = num1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);				// PC1,输入时不用设置速率	
	MISO_L;
}
/**/ 
void I2C_delay(void)
{	
   int i=2; //????????	,??????5????
   while(i) 
   { 
     i--; 
   } 
}
/*********************************************************************************************************/
int I2C_Start(void)
{
	SDA_H;
	SCL_H;
	I2C_delay();
	if(!SDA_read)
	{
		return FALSE1;	//SDA低时忙
	}
	SDA_L;
	I2C_delay();
	if(SDA_read)
	{
	return FALSE1;	//SDA为高时出错
	}
	SCL_L;
	I2C_delay();
	return TRUE1;
}
/*********************************************************************************************************/
void I2C_Stop(void)
{
	SCL_L;
	I2C_delay();
	SDA_L;
	I2C_delay();
	SCL_H;
	I2C_delay();
	SDA_H;
	I2C_delay();
}
/*********************************************************************************************************/
void I2C_Ack(void)
{	
	SCL_L;
	I2C_delay();
	SDA_L;
	I2C_delay();
	SCL_H;
	I2C_delay();
	SCL_L;
	I2C_delay();
}
/*********************************************************************************************************/
void I2C_NoAck(void)
{	
	SCL_L;
	I2C_delay();
	SDA_H;
	I2C_delay();
	SCL_H;
	I2C_delay();
	SCL_L;
	I2C_delay();
}
/*********************************************************************************************************/
int I2C_WaitAck(void) 	 //返回值=1 有ack，0没有ack
{
	SCL_L;
	I2C_delay();
	SDA_H;			
	I2C_delay();
	SCL_H;
	I2C_delay();
	if(SDA_read)
	{
		
      SCL_L;
		  //LEDB_L ;
      return FALSE1;
	}	

	return TRUE1;
}
/*********************************************************************************************************/
void I2C_SendByte(int SendByte) // 从高到低发送//
{
    int i=8;
    while(i--)
    {
        SCL_L;
        I2C_delay();
      if(SendByte&0x80)
        SDA_H;  
      else 
        SDA_L;   
        SendByte<<=1;
        I2C_delay();
	      SCL_H;
        I2C_delay();
    }
    SCL_L;
}
/*********************************************************************************************************/
uint8_t I2C_ReceiveByte(void)  //从高到低接收//
{ 
    uint8_t i=8;
    uint8_t ReceiveByte=0;

    SDA_H;				
    while(i--)
    {
      ReceiveByte<<=1;      
      SCL_L;
      I2C_delay();
	    SCL_H;
      I2C_delay();	
      if(SDA_read)
      {
        ReceiveByte|=0x01;
      }
    }
    SCL_L;
    return ReceiveByte;
}
/*********************************************************************************************************/
//I2C test

uint8_t I2C_Test(uint8_t DeviceAddress)
{		
	  //I2C_Start();
    if(!I2C_Start())return FALSE1;
    I2C_SendByte(DeviceAddress & 0xFE);//
    if(I2C_WaitAck())
    {I2C_Stop(); return TRUE1;}
    else
		{I2C_Stop(); return FALSE1;}	 
    //Systick_Delay_1ms(10);
}


/*********************************************************************************************************/
//I2C 写一个byte，器件地址，寄存器地址，数据

uint8_t I2C_WriteByte(int DeviceAddress, int WriteAddress, int SendByte)
{		
	  //I2C_Start();
    if(!I2C_Start())return FALSE1;
    I2C_SendByte(DeviceAddress & 0xFE);//
    if(!I2C_WaitAck())
    {
			I2C_Stop(); 
			return FALSE1;
		}
    I2C_SendByte(WriteAddress);   //
    I2C_WaitAck();	
    I2C_SendByte(SendByte);
    I2C_WaitAck();   
    I2C_Stop(); 
    //Systick_Delay_1ms(10);
    return TRUE1;
}

/*********************************************************************************************************/
//I2C 写一个byte，寄存器地址，数据

uint8_t I2C_Write2Byte(int WriteAddress, int SendByte)
{		
	if(!I2C_Start())return FALSE1;
	I2C_SendByte(WriteAddress);   
	if(!I2C_WaitAck())
  {
		I2C_Stop(); 
		return FALSE1;
	}	
	I2C_SendByte(SendByte);
	I2C_WaitAck();   
	I2C_Stop(); 
	return TRUE1;
}

/*********************************************************************************************************/
//I2C 连续写
//器件地址，待写入寄存器起始地址，待写入数据数长度，数据数组地址  
uint8_t I2C_WriteBuffer(uint8_t* pBuffer, int length, int DeviceAddress, int WriteAddress )
{   
	  //I2C_Start();
    if(!I2C_Start())return FALSE1;
    I2C_SendByte(DeviceAddress & 0xFE);//???? 
    if(!I2C_WaitAck()){I2C_Stop(); return FALSE1;}
    I2C_SendByte(WriteAddress);   //??????????      
	  I2C_WaitAck();	  
		while(length--)
		{
		  I2C_SendByte(*pBuffer);
		  I2C_WaitAck();
          pBuffer++;
		}
	  I2C_Stop();
	  //Systick_Delay_1ms(10);
	  return TRUE1;
}
/*********************************************************************************************************/

// I2C 读一个byte
uint8_t I2C_ReadByte(int DeviceAddress,int ReadAddress)
{		
    uint8_t rec;
		//I2C_Start();
    if(!I2C_Start())return FALSE1;
    I2C_SendByte(DeviceAddress & 0xFE);// 
    if(!I2C_WaitAck()){I2C_Stop(); return FALSE1;}
    I2C_SendByte(ReadAddress);        //     
    I2C_WaitAck();
    I2C_Stop();                       //selectable
    I2C_Start();                      
    I2C_SendByte(DeviceAddress | 0x01);
    I2C_WaitAck();
    rec=I2C_ReceiveByte();
    I2C_WaitAck();
    I2C_Stop();
    return rec;
}


/*********************************************************************************************************/

// I2C 连续读一串数据
uint8_t I2C_ReadBuffer(uint8_t* pBuffer,   int length,     int ReadAddress,  int DeviceAddress)
{		
    if(length == 0)
    return FALSE1;
	//	I2C_Start();
    if(!I2C_Start())return FALSE1;
    I2C_SendByte(DeviceAddress & 0xFE);//???? 
    if(!I2C_WaitAck()){I2C_Stop(); return FALSE1;}
    I2C_SendByte(ReadAddress);        //??????????      
    I2C_WaitAck();
    I2C_Stop();                       //selectable
    I2C_Start();                      
    I2C_SendByte(DeviceAddress | 0x01);
    I2C_WaitAck();
    while(length)
    {
      *pBuffer = I2C_ReceiveByte();
      if(length == 1)I2C_NoAck();
      else I2C_Ack(); 
      pBuffer++;
      length--;
    }
    I2C_Stop();
    return TRUE1;
}
