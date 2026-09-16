#ifndef __SPI_H
#define __SPI_H

#include "stm32f10x.h"

#define SPI_FLASH_SPI                           SPI1
#define SPI_FLASH_SPI_CLK                       RCC_APB2Periph_SPI1
#define SPI_FLASH_SPI_SCK_PIN                   GPIO_Pin_5                  /* PA.05 */
#define SPI_FLASH_SPI_SCK_GPIO_PORT             GPIOA                       /* GPIOA */
#define SPI_FLASH_SPI_SCK_GPIO_CLK              RCC_APB2Periph_GPIOA
#define SPI_FLASH_SPI_MISO_PIN                  GPIO_Pin_6                  /* PA.06 */
#define SPI_FLASH_SPI_MISO_GPIO_PORT            GPIOA                       /* GPIOA */
#define SPI_FLASH_SPI_MISO_GPIO_CLK             RCC_APB2Periph_GPIOA
#define SPI_FLASH_SPI_MOSI_PIN                  GPIO_Pin_7                  /* PA.07 */
#define SPI_FLASH_SPI_MOSI_GPIO_PORT            GPIOA                       /* GPIOA */
#define SPI_FLASH_SPI_MOSI_GPIO_CLK             RCC_APB2Periph_GPIOA
#define SPI_FLASH_CS_PIN                        GPIO_Pin_4                  /* PC.04 */
#define SPI_FLASH_CS_GPIO_PORT                  GPIOA                       /* GPIOC */
#define SPI_FLASH_CS_GPIO_CLK                   RCC_APB2Periph_GPIOA


#define SPI_CS_LOW()       GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define SPI_CS_HIGH()      GPIO_SetBits(GPIOA, GPIO_Pin_4)

void SPI_GPIO_Init(void);
void SPI_Soft_GPIO_Init(void);
void SPI_Write(uint8_t addr,uint8_t dat);
u8 SPI_Read(u8 addr);
void SPI_SendHalfWord(u16 HalfWord);
void SPI_Init_3131(uint8_t u8FreqCase,uint8_t u8Mode);
void SPI_SendOnly(u8 byte);
void SPI_SendString(uint8_t * buf , u8 len);
u8 SPI_SendByte(u8 byte);
void SWSPI_Send(uint8_t u8Data1,uint32_t u8NOPcnt);
void SPI_Soft_GPIO_Init(void);
#endif /* __SPI_FLASH_H */

