#include "exflash.h"
#include "main.h"
#include "stdio.h"

extern SPI_HandleTypeDef hspi2;
#define FLASH_SPI		hspi2
uint8_t fraction,mfg_id,dev_type,dev_id;
unsigned char block_protection_10[18]; /* global array to store block_protection data */
uint8_t busyMask = 0;
uint8_t pRxData[256];

#define FLASH_CS(x) HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, x ? GPIO_PIN_SET: GPIO_PIN_RESET)

static uint8_t ef_transfer(uint8_t tx)
{
	uint8_t txdat [] = {tx,0x00};
	HAL_SPI_TransmitReceive(&FLASH_SPI, &txdat[0], pRxData, 1, 500);
	return pRxData[0];
}

void ef_writeEnable(void)
{
    FLASH_CS(0);
    ef_transfer(0x06);
    FLASH_CS(1);
}

void ef_readid(void)
{
	FLASH_CS(1);
	HAL_Delay(12);
	FLASH_CS(0);
	ef_transfer(0x9F);
    mfg_id = ef_transfer(0x00);
    dev_type = ef_transfer(0x00);
    dev_id = ef_transfer(0x00);
    FLASH_CS(1);
}

uint8_t ef_status(void) {
    unsigned int flashStatus = 0x00;
    FLASH_CS(0);
    ef_transfer(0x05);
    flashStatus = ef_transfer(0x00);
    FLASH_CS(1);
    return flashStatus;
}

void ef_ClearBlockProtectSST1(void) {
    uint8_t flashStatus1 = 0x00;
    FLASH_CS(0);
    ef_transfer(0x05);
    flashStatus1 = ef_transfer(0x00);
    FLASH_CS(1);

    //modify flashstatus to clear protect bits
    flashStatus1 &= 0xC3;
    FLASH_CS(0);
    ef_transfer(0x50);
    FLASH_CS(1);
    FLASH_CS(0);
    ef_transfer(0x01);
    ef_transfer(flashStatus1);
    FLASH_CS(1);
}

void ClearBlockProtect(void) {
    unsigned char i = 0;
    if (dev_type == 0x25) {
        ef_ClearBlockProtectSST1();
        return;
    }
    block_protection_10[0] = 0x00;
    block_protection_10[1] = 0x00;
    block_protection_10[2] = 0x00;
    block_protection_10[3] = 0x00;
    block_protection_10[4] = 0x00;
    block_protection_10[5] = 0x00;
    block_protection_10[6] = 0x00;
    block_protection_10[7] = 0x00;
    block_protection_10[8] = 0x00;
    block_protection_10[9] = 0x00;
    block_protection_10[10] = 0x00;
    block_protection_10[11] = 0x00;
    block_protection_10[12] = 0x00;
    block_protection_10[13] = 0x00;
    block_protection_10[14] = 0x00;
    block_protection_10[15] = 0x00;
    block_protection_10[16] = 0x00;
    block_protection_10[17] = 0x00;
    ef_writeEnable();
    FLASH_CS(0);
    ef_transfer(0x42);
    for (i = 18; i > 0; i--) {ef_transfer(block_protection_10[i - 1]);}
    FLASH_CS(1);

    //Florian's way for WINBOND
    if ((mfg_id == FLASH_ID) && (dev_type == FLASH_DevType) && (dev_id == FLASH_DevID)) {
        ef_writeEnable();
        FLASH_CS(0);
        ef_transfer(0x01);
        ef_transfer(0x00);
        FLASH_CS(1);
        while (ef_status() & busyMask);
    }
}

void ef_ChipErase(void) {
    ClearBlockProtect();
    ef_writeEnable();
    FLASH_CS(0);
    ef_transfer(0xC7);
    FLASH_CS(1);
}
/**
  * @brief  Enables the write access to the FLASH.
  * @param  None
  * @retval None
  */
void ef_WriteEnable(void)
{
	uint8_t txData[1];

	txData[0] = FLASH_CMD_WREN;

	/* Select the FLASH: Chip Select low */
	FLASH_CS(0);

	/* Send "Write Enable" instruction */
	HAL_SPI_Transmit(&FLASH_SPI, txData, sizeof(txData), HAL_MAX_DELAY);

	/* Deselect the FLASH: Chip Select high */
	FLASH_CS(1);
}

/**
  * @brief  Polls the status of the Write In Progress (WIP) flag in the FLASH's
  *         status register and loop until write operation has completed.
  * @param  None
  * @retval None
  */
void ef_WaitForWriteEnd(void)
{
	uint8_t txData[1];
	uint8_t rxData[1];

	txData[0] = FLASH_CMD_RDSR;

	/* Select the FLASH: Chip Select low */
	FLASH_CS(0);

	/* Send "Read Status Register" instruction */
	HAL_SPI_Transmit(&FLASH_SPI, txData, sizeof(txData), HAL_MAX_DELAY);

	/* Loop as long as the memory is busy with a write cycle */
	do
	{
		/* Receive "Read Status Register" value */
		HAL_SPI_Receive(&FLASH_SPI, rxData, sizeof(rxData), HAL_MAX_DELAY);
	}
	while ((rxData[0] & FLASH_WIP_MASK) == FLASH_WIP_MASK); /* Write in progress */

	/* Deselect the FLASH: Chip Select high */
	FLASH_CS(1);
}
/**
  * @brief  Writes more than one byte to the FLASH with a single WRITE cycle
  *         (Page WRITE sequence).
  * @note   The number of byte can't exceed the FLASH page size.
  * @param  pBuffer: pointer to the buffer containing the data to be written
  *         to the FLASH.
  * @param  WriteAddr: FLASH's internal address to write to.
  * @param  NumByteToWrite: number of bytes to write to the FLASH, must be equal
  *         or less than "FLASH_PAGESIZE" value.
  * @retval None
  */
void ef_WritePage(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
	uint8_t txData[4];

	txData[0] = FLASH_CMD_PP;
	txData[1] = (WriteAddr & 0xFF0000) >> 16;	/* high nibble address byte to write to */
	txData[2] = (WriteAddr & 0xFF00) >> 8;		/* medium nibble address byte to write to */
	txData[3] = WriteAddr & 0xFF;				/* low nibble address byte to write to */

	/* Enable the write access to the FLASH */
	ef_WriteEnable();

	/* Select the FLASH: Chip Select low */
	FLASH_CS(0);

	/* Send "Write to Memory " instruction */
	HAL_SPI_Transmit(&FLASH_SPI, txData, sizeof(txData), HAL_MAX_DELAY);

	/* Send data */
	HAL_SPI_Transmit(&FLASH_SPI, pBuffer, NumByteToWrite, HAL_MAX_DELAY);

	/* Deselect the FLASH: Chip Select high */
	FLASH_CS(1);

	/* Wait the end of Flash writing */
	ef_WaitForWriteEnd();
}


/**
  * @brief  Erases the specified FLASH sector.
  * @param  SectorAddr: address of the sector to erase.
  * @retval None
  */
void ef_EraseSector(uint32_t SectorAddr)
{
	uint8_t txData[4];

	txData[0] = FLASH_CMD_SE;
	txData[1] = (SectorAddr & 0xFF0000) >> 16;	/* high nibble address byte to write to */
	txData[2] = (SectorAddr & 0xFF00) >> 8;		/* medium nibble address byte to write to */
	txData[3] = SectorAddr & 0xFF;				/* low nibble address byte to write to */

	/* Send write enable instruction */
	ef_WriteEnable();

	/* Select the FLASH: Chip Select low */
	FLASH_CS(0);

	/* Send Sector Erase instruction */
	HAL_SPI_Transmit(&FLASH_SPI, txData, sizeof(txData), HAL_MAX_DELAY);

	/* Deselect the FLASH: Chip Select high */
	FLASH_CS(1);

	/*!< Wait the end of Flash writing */
	ef_WaitForWriteEnd();


}


/**
  * @brief  Reads a block of data from the FLASH.
  * @param  pBuffer: pointer to the buffer that receives the data read from the FLASH.
  * @param  ReadAddr: FLASH's internal address to read from.
  * @param  NumByteToRead: number of bytes to read from the FLASH.
  * @retval None
  */
void ef_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead)
{
	uint8_t txData[4];

	txData[0] = FLASH_CMD_READ;
	txData[1] = (ReadAddr & 0xFF0000) >> 16;	/* high nibble address byte to write to */
	txData[2] = (ReadAddr & 0xFF00) >> 8;		/* medium nibble address byte to write to */
	txData[3] = ReadAddr & 0xFF;				/* low nibble address byte to write to */

	/* Select the FLASH: Chip Select low */
	FLASH_CS(0);

	/* Send "Read from Memory " instruction */
	HAL_SPI_Transmit(&FLASH_SPI, txData, sizeof(txData), HAL_MAX_DELAY);

	/* Read data from the FLASH */
	HAL_SPI_Receive(&FLASH_SPI, pBuffer, NumByteToRead, HAL_MAX_DELAY);

	/* Deselect the FLASH: Chip Select high */
	FLASH_CS(1);
}


/**
  * @brief  Writes block of data to the FLASH. In this function, the number of
  *         WRITE cycles are reduced, using Page WRITE sequence.
  * @param  pBuffer: pointer to the buffer containing the data to be written
  *         to the FLASH.
  * @param  WriteAddr: FLASH's internal address to write to.
  * @param  NumByteToWrite: number of bytes to write to the FLASH.
  * @retval None
  */
void ef_WriteBuffer(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
	uint32_t NumOfPage = 0;
	uint32_t NumOfSingle = 0;
	uint32_t Addr = 0;
	uint32_t count = 0;
	uint32_t temp = 0;

	Addr = WriteAddr % FLASH_SPI_PAGE_SIZE;
	count = FLASH_SPI_PAGE_SIZE - Addr;
	NumOfPage =  NumByteToWrite / FLASH_SPI_PAGE_SIZE;
	NumOfSingle = NumByteToWrite % FLASH_SPI_PAGE_SIZE;

	if (Addr == 0) /* WriteAddr is FLASH_PAGESIZE aligned  */
	{
		if (NumOfPage == 0) /* NumByteToWrite < FLASH_PAGESIZE */
		{
			ef_WritePage(pBuffer, WriteAddr, NumByteToWrite);
		}
		else /* NumByteToWrite > FLASH_PAGESIZE */
		{
			while (NumOfPage--)
			{
				ef_WritePage(pBuffer, WriteAddr, FLASH_SPI_PAGE_SIZE);
				WriteAddr +=  FLASH_SPI_PAGE_SIZE;
				pBuffer += FLASH_SPI_PAGE_SIZE;
			}

			ef_WritePage(pBuffer, WriteAddr, NumOfSingle);
		}
	}
	else /* WriteAddr is not FLASH_PAGESIZE aligned  */
	{
		if (NumOfPage == 0) /* NumByteToWrite < FLASH_PAGESIZE */
		{
			if (NumOfSingle > count) /* (NumByteToWrite + WriteAddr) > FLASH_PAGESIZE */
			{
				temp = NumOfSingle - count;

				ef_WritePage(pBuffer, WriteAddr, count);
				WriteAddr +=  count;
				pBuffer += count;

				ef_WritePage(pBuffer, WriteAddr, temp);
			}
			else
			{
				ef_WritePage(pBuffer, WriteAddr, NumByteToWrite);
			}
		}
		else /* NumByteToWrite > FLASH_PAGESIZE */
		{
			NumByteToWrite -= count;
			NumOfPage =  NumByteToWrite / FLASH_SPI_PAGE_SIZE;
			NumOfSingle = NumByteToWrite % FLASH_SPI_PAGE_SIZE;

			ef_WritePage(pBuffer, WriteAddr, count);
			WriteAddr +=  count;
			pBuffer += count;

			while (NumOfPage--)
			{
				ef_WritePage(pBuffer, WriteAddr, FLASH_SPI_PAGE_SIZE);
				WriteAddr +=  FLASH_SPI_PAGE_SIZE;
				pBuffer += FLASH_SPI_PAGE_SIZE;
			}

			if (NumOfSingle != 0)
			{
				ef_WritePage(pBuffer, WriteAddr, NumOfSingle);
			}
		}
	}
}

/**
  * @brief  Send Global Block Register Unlock command
  * @param  None
  * @retval None
  */
void ef_GlobalBlockProtectionUnlock(void)
{
	uint8_t txData[1];

	txData[0] = FLASH_CMD_ULBPR;

	/* Send write enable instruction */
	ef_WriteEnable();

	/* Select the FLASH: Chip Select low */
	FLASH_CS(0);

	/* Send Bulk Erase instruction  */
	HAL_SPI_Transmit(&FLASH_SPI, txData, sizeof(txData), HAL_MAX_DELAY);

	/* Deselect the FLASH: Chip Select high */
	FLASH_CS(1);

}


