#include "stm32f10x.h"
extern uint8_t data[64];
/***************************************************
函数功能：打开和关闭软件shutdown
参数个数:data[4]
参数说明：data[4]为0 sorftware shutdown，为1  normal operation
返回：无
***************************************/
void delay_ms(uint32_t t)
{
	uint8_t tt;
	for(;t>0;t--)
	{
		for(tt=255;tt>0;tt--);
		for(tt=255;tt>0;tt--);
		for(tt=255;tt>0;tt--);
		for(tt=255;tt>0;tt--);
		for(tt=255;tt>0;tt--);
		for(tt=255;tt>0;tt--);
		for(tt=255;tt>0;tt--);
		for(tt=30;tt>0;tt--);
	}
}
void Software_Shutdown()//测软的静态电流，测试的时候器件地址都是Addr_GND_GND
{
	uint8_t i;
  I2C_WriteByte(Addr_GND_GND,0xFE,0xc5);//unlock FDh
	I2C_WriteByte(Addr_GND_GND,0xFD,0x00);//写第一个页
// 	for(i=0;i<24;i++)
	I2C_WriteByte(Addr_GND_GND,0,0x01);//打开一排灯做指示作用
	
	I2C_WriteByte(Addr_GND_GND,0xFE,0xc5);//unlock FDh
	I2C_WriteByte(Addr_GND_GND,0xFD,0x01);//写第二个页
	for(i=0;i<192;i++)
	I2C_WriteByte(Addr_GND_GND,i,0xFF);//写第一排灯的PWM值
	
	I2C_WriteByte(Addr_GND_GND,0xFE,0xc5);//unlock FDh
	I2C_WriteByte(Addr_GND_GND,0xFD,0x03);//写第四个页
	if(data[5]>0)
	I2C_WriteByte(Addr_GND_GND,0x00,0x01);//normal operation 
	else
	I2C_WriteByte(Addr_GND_GND,0x00,0x00);//sorftware shutdown
	
  I2C_WriteByte(Addr_GND_GND,0x01,data[6]);//设置总电流
}


/***************************************************
函数功能：测试I2C器件地址
参数个数:data[5],data[6]
参数说明：data[5]选择器件的地址，data[6]改变PWM的值
返回：无
***************************************/
void Test_I2C_Adress(void)//测试I2C器件的所有地址
{
	uint8_t i,address=0;
	switch(data[5])
	{
         case 0 : address= Addr_GND_GND;  break;
		     case 1 : address= Addr_GND_SCL ;  break;
		     case 2 : address= Addr_GND_SDA;  break;
		     case 3 : address= Addr_GND_VCC;  break;
		     case 4 : address= Addr_SCL_GND;  break;
		     case 5 : address= Addr_SCL_SCL;  break;
		     case 6 : address= Addr_SCL_SDA;  break;
		     case 7 : address= Addr_SCL_VCC;  break;
		     case 8 : address= Addr_SDA_GND;  break;
		     case 9 : address= Addr_SDA_SCL;  break;
		     case 10 : address= Addr_SDA_SDA;  break;
		     case 11 : address= Addr_SDA_VCC;  break;
		     case 12 : address= Addr_VCC_GND;  break;
		     case 13 : address= Addr_VCC_SCL;  break;
		     case 14 : address= Addr_VCC_SDA;  break;
		     case 15 : address= Addr_VCC_VCC;  break;
		default:break;
  }
  I2C_WriteByte(address,0xFE,0xc5);//unlock FDh
	I2C_WriteByte(address,0xFD,0x00);//写第一个页	
	I2C_WriteByte(address,0x00,0xff);//打开一排灯做指示作用
	
	I2C_WriteByte(address,0xFE,0xc5);//unlock FDh
	I2C_WriteByte(address,0xFD,0x01);//写第二个页
	for(i=0;i<8;i++)
	I2C_WriteByte(address,i,data[6]);   //写第一排灯的PWM值
	
	I2C_WriteByte(address,0xFE,0xc5);//unlock FDh
	I2C_WriteByte(address,0xFD,0x03);//写第四个页
	I2C_WriteByte(address,0x00,0x01);//normal operation
	I2C_WriteByte(address,0x01,0xFF);//设置总电流
}
/***************************************************
函数功能：打开和关闭所有的灯,PWM值
参数个数: data[5]，data[6]
参数说明：data[5] 大于0开启所有的灯。为0，关闭所有的灯，data[6]值
返回：无
***************************************/
void Test_All_On_Off(void)//测软的静态电流，测试的时候器件地址都是Addr_GND_GND
{
	uint8_t i;
	I2C_WriteByte(Addr_GND_GND,0xFE,0xc5);//unlock FDh
	I2C_WriteByte(Addr_GND_GND,0xFD,0x03);//写第四个页
	I2C_WriteByte(Addr_GND_GND,0x00,0x00);//software shutdown
	
  I2C_WriteByte(Addr_GND_GND,0xFE,0xc5);//unlock FDh
	I2C_WriteByte(Addr_GND_GND,0xFD,0x00);//写第一个页	
	if(data[5]>0)
	{
		for(i=0;i<24;i++)
		I2C_WriteByte(Addr_GND_GND,i,0xff);//打开所有灯的开关
	} 
	else
	{
		for(i=0;i<24;i++)
		I2C_WriteByte(Addr_GND_GND,i,0x00);//关闭所有的灯
	}
	I2C_WriteByte(Addr_GND_GND,0xFE,0xc5);//unlock FDh
	I2C_WriteByte(Addr_GND_GND,0xFD,0x01);//写第二个页
	
	for(i=0;i<192;i++)
	I2C_WriteByte(Addr_GND_GND,i,data[6]);//打开所有灯的PWM

	
	I2C_WriteByte(Addr_GND_GND,0xFE,0xc5);//unlock FDh
	I2C_WriteByte(Addr_GND_GND,0xFD,0x03);//写第四个页
	
	I2C_WriteByte(Addr_GND_GND,0x00,0x01);//normal operation
	
  I2C_WriteByte(Addr_GND_GND,0x01,data[7]);//设置总电流
}

void IC_Write_Pro(void)
{
	u8 i,j = 0,flag = 0,succeed = 1;
	if (data[62] == 0x02)
	{
		I2C_Write2Byte(data[4], data[5]);
	}
	else
	{
		if (data[62]>2 && data[62]%2==1)
		{
			for (i=0;i<(data[62]-1)/2;i++)
			{
				flag = I2C_WriteByte(data[4],data[5+j],data[6+j]);
				if (flag != 1)
				{
					succeed = flag;
					break;
				}				
				j += 2;
			}	
		} 
	}	
	data[4] = succeed;
}

void IC_WriteBuff_Pro(void)
{
	if (data[62]-2 > 0)
	{
		data[4] = I2C_WriteBuffer(&data[6], data[62]-2, data[4], data[5]);
	}
}

void IC_Read_Pro(void)
{
	if (data[62] == 0x02)
	{
		data[4] = I2C_ReadByte(data[4],data[5]);
		data[62] = 1;
	}	
}

void SPI_Write_Pro(void)
{
	u8 i,j = 0;
	if (data[62]>1 && data[62]%2==0)
	{
		for (i=0;i<data[62]/2;i++)
		{
			SPI_Write(data[4+j], data[5+j]);
			j += 2;
		}			
	}
}

void SPI_Read_Pro(void)
{
	if (data[62] == 0x01)
	{
		data[4] = SPI_Read(data[4]);
	}
}

void Custom_1(void)
{
	switch(data[4])//function selection
	{
	 case 1: Software_Shutdown();break;//
	 case 2: Test_I2C_Adress();break;
	 case 3: Test_All_On_Off();break;
	 default:break;
	}
}

void Custom_2(void)
{
	data[5] = 7;//Test
}

void Custom_3(void)
{
	data[5] = 8;//Test
}

void Custom_4(void)
{
	data[5] = 9;//Test
}

// void Custom_5(void)
// {
// 	data[5] = 10;//Test
// }

void Device_Detect(void)
{
	u8 i;
	for (i=0;i<255;i+=2)
	{
		if (I2C_WriteByte(i,0x00,0x00) == 1)
			break;
	}	
	data[2] = 2;
	
	if (i == 0)
	{
		data[4] = 1;
	}
	else
	{
		data[4] = i;
	}
	LEDB_H;
}
