

#include "main.h"
#include "spiflash.h"
#include "boot.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
// memory organisation
// start at addr 0x0
// 4 x 8KBytes sectors
// 1 x 32kBytes sectors
// 30 x 64kBytes sectors
// 1 x 32kBytes sectors
// 4 x 8KBytes sectors
// total 40 sectors
//
//#define SST26VF016B_DEV_NAME "SST26VF016B"
//#define SST26VF016B_SIZE (2*1024*1024)
//#define SST26VF016B_SECTORS (40)
//#define SIZEOFSECTOR(X) X*1024/8
//
//#define LOG_SECTOR1	5
//#define LOG_SECTOR2	6
//
//uint32_t SECTOR_A_ADDR, SECTOR_B_ADDR;
//static uint8_t f_dat[32];
//#define MAX_MSG_SIZE 32
//#define format_sector(addr) {\
//								printf("\nformating Sector %x\n", addr);\
//								sFLASH_EraseSector(addr);\
//							}
//
//
//
//
//
//static uint32_t sst26vf016b_sectorNumberToAddr(uint32_t secNbr) {
//	// start at addr 0x0
//	// 4 x 8KBytes sectors  0-3
//	// 1 x 32kBytes sectors 4
//	// 30 x 64kBytes sectors 5-
//	// 1 x 32kBytes sectors
//	// 4 x 8KBytes sectors
//
//	uint32_t addr;
//
//	// 4 x 8KBytes sectors
//	if (secNbr < 4) {
//		return (8 * 1024) * secNbr;
//	}
//	addr = 32 * 1024;
//	secNbr -= 4;
//
//	// 1 x 32kBytes sectors
//	if (secNbr < 1) {
//		return addr;
//	}
//	addr += 32 * 1024;
//	secNbr -= 1;
//
//	// 30 x 64kBytes sectors
//	if (secNbr < 30) {
//		return addr + 64*1024*secNbr;
//	}
//	addr += 30*64*1024;
//	secNbr -= 30;
//
//	// 1 x 32kBytes sectors
//	if (secNbr < 1) {
//		return addr;
//	}
//	addr += 32 * 1024;
//	secNbr -= 1;
//
//	// 4 x 8KBytes sectors
//	if (secNbr < 4) {
//		return addr + (8 * 1024) * secNbr;
//	}
//	return SST26VF016B_SIZE;
//}
//
//
///************* Test codes - start *****************/
//uint8_t buff[8125];
//
////void mem_write(uint8_t *data,uint32_t index, len)
////{
////	for(uint32_t x=0;x<len;x++)
////	{
////		buff[index + x] = data[x];
////	}
////}
//
//
///************* Test codes - end *****************/
//
//
//
//
////memlog("voltage = %f", voltage);
////
////void mem_log(const char* format, ...)
////{
////    va_list args;
////    va_start(args, format);
////    char numStr[32]; // Common buffer for conversions
////    while (*format != '\0') {
////        if (*format == '%' && *(format + 1) != '\0') {
////            // Handle format specifiers
////            switch (*(format + 1)) {
////                case 'd':
////                    sprintf(numStr, "%d", va_arg(args, int));
////                    //puts(numStr);
////                    break;
////                case 'f':
////                    sprintf(numStr, "%f", va_arg(args, double));
////                    //puts(numStr);
////                    break;
////                case 's':
////                	sprintf(numStr, "%s", va_arg(args, char*));
////                   // puts();
////                    break;
////                default:
////                    // Just print the '%' character if the specifier is not recognized
////                    putchar('%');
////                    break;
////            }
////            format += 2;  // Move to the next character after the specifier
////        } else {
////            // Print regular characters
////        	sprintf(numStr, "%s", *format);
////            //putchar(*format);
////            format++;
////        }
////    }
////
////    va_end(args);
////}
///*
// * 			data_end_count = f_dat[j] == 0xFF ? data_end_count+1 : 0;
//			if(data_end_count == 4) goto END;
//
// *
// */
//
///* print data until last byte */
//void log_print_data(log_t* log)
//{
//	static uint8_t wdata[32];
//	uint32_t c,j;
//	for(c=0; c< log->sector_size; c+=32)
//	{
//		sFLASH_ReadBuffer(f_dat, log->activeSector + c, sizeof(f_dat));
//
//		for(j=0; j<sizeof(f_dat); j++)
//		{
//			//if(f_dat[j] == 0xFF) goto END;
//
//			putchar(f_dat[j]);
//			wdata[j]=f_dat[j];
//		}
//
//		//printf("\n%s",wdata);
//
//	}
//
//	END:
//
//	printf("\nAll bytes were read\n");
//}
//
//
//
//
////uint32_t log_checkspace(struct log_t* log)
////{
////
////}
////
////void log_erase_inactive_space(struct log_t* log)
////{
////
////}
//
//void format_log_Sectors(log_t* log)
//{
//	format_sector(SECTOR_A_ADDR);
//	format_sector(SECTOR_B_ADDR);
//
//	log->activeSector = SECTOR_A_ADDR;
//	log->index= 0;
//	log->sector_size = SIZEOFSECTOR(64);
//
//}
//
//
//void log_init(log_t* log)
//{
//	uint8_t mem_clear_count;
//	ReadFlashID();
//	/*
//		findout active sector and available space in that sector
//	*/
//	SECTOR_A_ADDR = sst26vf016b_sectorNumberToAddr(LOG_SECTOR1);
//	SECTOR_B_ADDR = sst26vf016b_sectorNumberToAddr(LOG_SECTOR2);
//
//	printf("Sector A Addr: 0x%x\nSector B Addr:0x%x\n",SECTOR_A_ADDR,SECTOR_B_ADDR);
//
//	log->activeSector =0;
//	log->sector_size = 0;
//	log->index=0;
//
//	uint32_t x,i,base_addr;
//	base_addr = SECTOR_A_ADDR;
//
//	RESCAN:
//	printf("Scanning Addr 0x%x\n",base_addr);
//	for(i=0; i< ( 8192 / sizeof(f_dat) ); i += sizeof(f_dat))
//	{
//		sFLASH_ReadBuffer(f_dat, base_addr + i, sizeof(f_dat));
//		mem_clear_count=0;
//		for(x=0; x<sizeof(f_dat); x++)
//		{
//
//			mem_clear_count = f_dat[x] == 0xFF ? mem_clear_count + 1:0;
//			if(mem_clear_count >= 4)
//			{
//				log->activeSector = base_addr;
//				log->index=i + (x-3);
//				log->sector_size = SIZEOFSECTOR(64);
//				goto SCANNED;
//			}
//		}
//	}
//
//	// UNSCANNED
//
//	if(base_addr == SECTOR_A_ADDR) {
//		base_addr = SECTOR_B_ADDR;
//		printf("Sector A is full\n");
//		goto RESCAN;
//	}
//
//	if(log->activeSector == 0)
//	{
//		// format both the sectors here
//		format_sector(SECTOR_A_ADDR);
//		format_sector(SECTOR_B_ADDR);
//
//	}
//
//	SCANNED:
//
//	printf("Active Scetor: 0x%x, Index : %ld\n", log->activeSector, log->index);
//
//
//
//}
//
//
//
//static void switch_sector(log_t* log)
//{
//	if(log->activeSector == SECTOR_A_ADDR)
//	{
//		// format SectorB Here
//		format_sector(SECTOR_B_ADDR);
//		log->activeSector = SECTOR_B_ADDR;
//	}
//	else
//	{
//		// format Sector A Here
//		format_sector(SECTOR_A_ADDR);
//		log->activeSector = SECTOR_A_ADDR;
//	}
//
//	log->index = 0;
//	log->sector_size = SIZEOFSECTOR(64);
//
//}
//
//void log_Message(log_t* log,char *tag, char *msg)
//{
//	memset(f_dat,0x00,sizeof(f_dat));
//	/*
//	uint32_t len = strlen(tag)+ strlen(msg) + 3; // including : and \n
//	if( (len % 4) > 0) len += 4- (len % 4);
//	if(len > MAX_MSG_SIZE) return;
//	sprintf((char *)f_dat,"$%s:%s\n",tag,msg);
//
//	if( log->index  > log->sector_size)
//	{
//		// add end delimeter here
//		// switch sectors here
//		switch_sector(log);
//	}
//	*/
//	//sFLASH_WriteBuffer((char *)f_dat, log->activeSector + log->index, len);
//	sprintf((char *)f_dat,"$%s:%s\n",tag,msg);
//	sFLASH_WriteBuffer((char *)f_dat, log->activeSector + 128, 32);
//	//log->index += len;
//
//}
//log_t sys_log;
//void test_log(void)
//{
//
//  log_init(&sys_log);
//  format_sector(SECTOR_B_ADDR);
//  //for(uint8_t x=0;x<10;x++)
//  {
//	  log_Message(&sys_log,"ERR", "Over voltage");
//  }
//  log_print_data(&sys_log);
//
//  while(1)
//  {
//	  ;
//  }
//}
