#ifndef __SPIFLASH__
#define __SPIFLASH__


void spiflash_ChipErase(void);
void spiflash_WriteEnable(void);
void spiflash_WritePage(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite);
void spiflash_EraseSector(uint32_t SectorAddr);
void spiflash_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead);
void spiflash_WriteBuffer(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite);
void spiflash_GlobalBlockProtectionUnlock(void);

#endif
