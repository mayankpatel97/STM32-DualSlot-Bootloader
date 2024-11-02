#ifndef EF_FLASH_H
#define EF_FLASH_H

#include "main.h"

/* SPI handle */
extern SPI_HandleTypeDef hspi1;

/* CS Pin configuration */
#define EF_CS_PIN               GPIO_PIN_4
#define EF_CS_PORT              GPIOA

/* Functions */
HAL_StatusTypeDef ef_Init(void);
uint16_t ef_ReadID(void);
HAL_StatusTypeDef ef_ReadData(uint32_t address, uint8_t* data, uint32_t size);
HAL_StatusTypeDef ef_WritePage(uint32_t address, uint8_t* data, uint16_t size);
HAL_StatusTypeDef ef_EraseSector(uint32_t address);
HAL_StatusTypeDef ef_EraseBlock(uint32_t address);
HAL_StatusTypeDef ef_EraseChip(void);

#endif // EF_FLASH_H
