
/*
 * ef Flash Memory Driver for STM32
 * Total Size: 8MB (64Mbit)
 * Page Size: 256 bytes
 * Sector Size: 4KB (4,096 bytes)
 * Block Size: 64KB (65,536 bytes)
 *
 * Memory Organization:
 * - Total Capacity: 64 Mbit / 8 MByte
 * - Number of Pages: 32,768 (each 256 bytes)
 * - Number of Sectors: 2,048 (each 4KB)
 * - Number of Blocks: 128 (each 64KB)
 *
 * Memory Map Layout:
 * - Address Range: Start at 0x000000, End at 0x7FFFFF
 * - Block Level Breakdown:
 *   Block 0 : 0x000000 - 0x00FFFF (64KB)
 *   Block 1 : 0x010000 - 0x01FFFF (64KB)
 *   ...
 *   Block 127: 0x7F0000 - 0x7FFFFF (64KB)
 *
 * - Sector Level Breakdown (within each block):
 *   Sector 0 : 0x000000 - 0x000FFF (4KB)
 *   Sector 1 : 0x001000 - 0x001FFF (4KB)
 *   ...
 *   Sector 15: 0x00F000 - 0x00FFFF (4KB)
 */

/* Include the header file */
#include "exflash.h"

/* ef Commands */
#define EF_WRITE_ENABLE         0x06
#define EF_WRITE_DISABLE        0x04
#define EF_READ_STATUS_REG1     0x05
#define EF_READ_DATA            0x03
#define EF_PAGE_PROGRAM         0x02
#define EF_SECTOR_ERASE         0x20
#define EF_BLOCK_ERASE          0xD8
#define EF_CHIP_ERASE           0xC7
#define EF_READ_ID              0x90

/* Status Register Bits */
#define EF_BUSY_BIT             0x01

/* Default timeout */
#define EF_TIMEOUT              1000

/* CS Pin configuration - modify as per your setup */
#define EF_CS_PIN               GPIO_PIN_4
#define EF_CS_PORT              GPIOA

/* Private functions */
static void ef_CS_Low(void) {
    HAL_GPIO_WritePin(EF_CS_PORT, EF_CS_PIN, GPIO_PIN_RESET);
}

static void ef_CS_High(void) {
    HAL_GPIO_WritePin(EF_CS_PORT, EF_CS_PIN, GPIO_PIN_SET);
}

static uint8_t ef_WaitForReady(uint32_t timeout) {
    uint32_t timer = HAL_GetTick();
    uint8_t status;

    do {
        ef_CS_Low();
        uint8_t cmd = EF_READ_STATUS_REG1;
        HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
        HAL_SPI_Receive(&hspi1, &status, 1, 100);
        ef_CS_High();

        if (!(status & EF_BUSY_BIT)) {
            return HAL_OK;
        }
    } while ((HAL_GetTick() - timer) < timeout);

    return HAL_TIMEOUT;
}

/* Initialize ef */
HAL_StatusTypeDef ef_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = EF_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(EF_CS_PORT, &GPIO_InitStruct);

    ef_CS_High();

    return HAL_OK;
}

/* Read Device ID */
uint16_t ef_ReadID(void) {
    uint8_t cmd = EF_READ_ID;
    uint8_t id[2];

    ef_CS_Low();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Transmit(&hspi1, (uint8_t[]){0x00, 0x00, 0x00}, 3, 100);
    HAL_SPI_Receive(&hspi1, id, 2, 100);
    ef_CS_High();

    return (id[0] << 8) | id[1];
}

/* Enable Write Operations */
static void ef_WriteEnable(void) {
    uint8_t cmd = EF_WRITE_ENABLE;

    ef_CS_Low();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    ef_CS_High();
}

/* Read Data */
HAL_StatusTypeDef ef_ReadData(uint32_t address, uint8_t* data, uint32_t size) {
    uint8_t cmd[4] = {
        EF_READ_DATA,
        (address >> 16) & 0xFF,
        (address >> 8) & 0xFF,
        address & 0xFF
    };

    ef_CS_Low();
    HAL_SPI_Transmit(&hspi1, cmd, 4, 100);
    HAL_StatusTypeDef status = HAL_SPI_Receive(&hspi1, data, size, 1000);
    ef_CS_High();

    return status;
}

/* Write Page (up to 256 bytes) */
HAL_StatusTypeDef ef_WritePage(uint32_t address, uint8_t* data, uint16_t size) {
    if (size > 256) return HAL_ERROR;

    ef_WriteEnable();

    uint8_t cmd[4] = {
        EF_PAGE_PROGRAM,
        (address >> 16) & 0xFF,
        (address >> 8) & 0xFF,
        address & 0xFF
    };

    ef_CS_Low();
    HAL_SPI_Transmit(&hspi1, cmd, 4, 100);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, data, size, 1000);
    ef_CS_High();

    return (ef_WaitForReady(EF_TIMEOUT) == HAL_OK) ? status : HAL_TIMEOUT;
}

/* Erase Sector (4KB) */
HAL_StatusTypeDef ef_EraseSector(uint32_t address) {
    ef_WriteEnable();

    uint8_t cmd[4] = {
        EF_SECTOR_ERASE,
        (address >> 16) & 0xFF,
        (address >> 8) & 0xFF,
        address & 0xFF
    };

    ef_CS_Low();
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, cmd, 4, 100);
    ef_CS_High();

    return (ef_WaitForReady(EF_TIMEOUT) == HAL_OK) ? status : HAL_TIMEOUT;
}

/* Erase Block (64KB) */
HAL_StatusTypeDef ef_EraseBlock(uint32_t address) {
    ef_WriteEnable();

    uint8_t cmd[4] = {
        EF_BLOCK_ERASE,
        (address >> 16) & 0xFF,
        (address >> 8) & 0xFF,
        address & 0xFF
    };

    ef_CS_Low();
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, cmd, 4, 100);
    ef_CS_High();

    return (ef_WaitForReady(EF_TIMEOUT) == HAL_OK) ? status : HAL_TIMEOUT;
}

/* Erase Entire Chip */
HAL_StatusTypeDef ef_EraseChip(void) {
    ef_WriteEnable();

    uint8_t cmd = EF_CHIP_ERASE;

    ef_CS_Low();
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    ef_CS_High();

    return (ef_WaitForReady(30000) == HAL_OK) ? status : HAL_TIMEOUT;
}
