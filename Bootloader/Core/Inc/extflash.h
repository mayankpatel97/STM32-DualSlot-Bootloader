#ifndef __EXTFLASH__
#define __EXTFLASH__

void ef_ChipErase(void);
void ef_WriteEnable(void);
void ef_WritePage(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite);
void ef_EraseSector(uint32_t SectorAddr);
void ef_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead);
void ef_WriteBuffer(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite);
void ef_GlobalBlockProtectionUnlock(void);

#endif
