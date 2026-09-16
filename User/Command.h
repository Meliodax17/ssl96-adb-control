/* Define to prevent recursive inclusion ------------------------------------ */ 
#ifndef __COMMAND_H 
#define __COMMAND_H 
#include "stm32f10x.h"
//   I2C  器件地址
#define Addr_GND_GND  0xA0
#define Addr_GND_SCL  0xA2
#define Addr_GND_SDA  0xA4
#define Addr_GND_VCC  0xA6
#define Addr_SCL_GND  0xA8
#define Addr_SCL_SCL  0xAA
#define Addr_SCL_SDA  0xAC
#define Addr_SCL_VCC  0xAE
#define Addr_SDA_GND  0xB0
#define Addr_SDA_SCL  0xB2
#define Addr_SDA_SDA  0xB4
#define Addr_SDA_VCC  0xB6
#define Addr_VCC_GND  0xB8
#define Addr_VCC_SCL  0xBA
#define Addr_VCC_SDA  0xBC
#define Addr_VCC_VCC  0xAE

void Software_Shutdown(void);//测软的静态电流
void Test_I2C_Adress(void);//测试I2C器件的所有地址
void Test_All_On_Off(void);//测试I2C器件的所有地址 
void delay_ms(uint32_t t);
void IC_Write_Pro(void);
void IC_WriteBuff_Pro(void);
void IC_Read_Pro(void);
void SPI_Write_Pro(void);
void SPI_Read_Pro(void);
void Custom_1(void);
void Custom_2(void);
void Custom_3(void);
void Custom_4(void);
// void Custom_5(void);
void Device_Detect(void);
#endif 
