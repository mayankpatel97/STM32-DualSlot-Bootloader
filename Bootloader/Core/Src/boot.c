

#include "main.h"
#include "spiflash.h"
#include "boot.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <usbd_cdc_if.h>  // Include the USB CDC interface header

#define PACKET_START_MARKER 0x55
#define PACKET_END_MARKER 0xAA
#define MAX_PACKET_SIZE 256
#define FW_ADDR_SPIFLASH (uint32_t) 0x00000

typedef enum {
    WAIT_FOR_START,
    READ_CMD,
    READ_LEN,
    READ_PAYLOAD,
    READ_CRC1,
    READ_CRC2,
    WAIT_FOR_END
} PacketState;

typedef enum {
    DFU_START=1,
	DFU_ERASE_MEM,
    DFU_HEADER,
    DFU_DATA,
    DFU_END,
    DFU_UPDATE_FW,
    DFU_JUMP_TO_APP
} dfu_stat_t;

typedef enum
{
	DFU_OK,
	DFU_ERROR
}RESP_CODE;

typedef struct
{
	uint8_t start_byte;
	uint8_t cmd;
	uint8_t len;
	RESP_CODE data;
	uint16_t crc;
	uint8_t end_byte;
}__attribute__((packed)) rep_packet_t;

typedef struct
{
	uint32_t total_fw_size;
	uint16_t fw_crc;
	uint32_t fw_ver;
}fw_info_t;

PacketState packet_state = WAIT_FOR_START;
uint8_t packet[MAX_PACKET_SIZE];
uint8_t payload[MAX_PACKET_SIZE - 6];
uint16_t packet_index = 0;
uint16_t payload_index = 0;
uint8_t cmd = 0;
uint8_t len = 0;
uint16_t received_crc = 0;
dfu_stat_t dfu_stat;
uint32_t total_fw_len,received_fw_len;
uint32_t fw_ver;
uint16_t fw_crc;
uint16_t payload_len,packet_len;
uint32_t fw_index;

void process_packet(void);
void writeDataChunk(uint8_t *dat, uint16_t length);
void UpdateFirmware(void);

uint16_t calculate_crc16(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    uint16_t poly = 0x2024;

    for (uint16_t i = 0; i < length; i++) {
        crc ^= (data[i] << 8);
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ poly;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}


void USB_VCP_ReceiveCallback(uint8_t* Buf, uint32_t *Len)
{
    for (uint32_t i = 0; i < *Len; i++) {
        uint8_t byte = Buf[i];

        switch (packet_state) {
            case WAIT_FOR_START:
                if (byte == PACKET_START_MARKER) {
                    packet_state = READ_CMD;
                    packet_index = 0;
                    payload_index = 0;
                    packet[packet_index++] = byte;
                }
                break;

            case READ_CMD:
                cmd = byte;
                packet[packet_index++] = byte;
                packet_state = READ_LEN;
                break;

            case READ_LEN:
                len = byte;
                packet[packet_index++] = byte;
                /*
                if (len > 0 && len <= (MAX_PACKET_SIZE - 6)) {
                    packet_state = READ_PAYLOAD;
                } else {
                    packet_state = WAIT_FOR_START; // Invalid length, reset
                }
                */
                if(len == 0) packet_state = READ_CRC1;
                else 		 packet_state = READ_PAYLOAD;
                break;

            case READ_PAYLOAD:
                payload[payload_index++] = byte;
                packet[packet_index++] = byte;
                if (payload_index == len) {
                    packet_state = READ_CRC1;
                }
                break;

            case READ_CRC1:
                received_crc = (byte << 8);
                packet[packet_index++] = byte;
                packet_state = READ_CRC2;
                break;

            case READ_CRC2:
                received_crc |= byte;
                packet[packet_index++] = byte;
                packet_state = WAIT_FOR_END;
                break;

            case WAIT_FOR_END:
                if (byte == PACKET_END_MARKER) {
                    packet[packet_index++] = byte;

                    payload_len = payload_index;
                    packet_len = packet_index;
                    process_packet();  // Validate and process the packet
                }
                packet_state = WAIT_FOR_START;
                break;
        }
    }
}


static void send_response_packet(dfu_stat_t cmd, RESP_CODE response_code)
{
	rep_packet_t resp;
	resp.start_byte = PACKET_START_MARKER;
	resp.cmd = DFU_START;
	resp.len = 0x01;
	resp.data = response_code;
	resp.crc = calculate_crc16((uint8_t *)&resp,resp.len + 2 );
	resp.end_byte = PACKET_END_MARKER;
	CDC_Transmit_FS((uint8_t *)&resp,resp.len + 6);
}

void process_packet(void)
{
    // Verify CRC
    uint16_t calculated_crc = calculate_crc16(packet + 1, len + 2);  // CMD + LEN + PAYLOAD
    if (received_crc != calculated_crc) {
    	printf("CRC check failed! Received: 0x%04X, Calculated: 0x%04X\n", received_crc, calculated_crc);
    }
	// Packet is valid, process it
	printf("Packet received successfully!\n");
	printf("Command: 0x%02X\n", cmd);
	printf("Payload Length: %d\n", len);
	printf("Payload: ");
	for (int i = 0; i < len; i++) {
		printf("0x%02X ", payload[i]);
	}
	printf("\n");
	/* process packet here */
	switch(cmd)
	{
		case DFU_START:
			printf("Start packet!\n");
			send_response_packet(DFU_START,DFU_OK);
			dfu_stat = DFU_ERASE_MEM;
			break;

		case DFU_ERASE_MEM:
			/* erase flash sectors */
			spiflash_ChipErase();
			send_response_packet(DFU_ERASE_MEM,DFU_OK);
			break;
		case DFU_HEADER:

			/*header[1] cmd[1] len[1] total fw size[4B], fw crc[2B], fw ver[4B] crc[2] footer[1] */
			fw_info_t fw_info;
			memcpy((uint8_t *)&fw_info, &payload, sizeof(fw_info_t));
			received_fw_len =0;
			total_fw_len = fw_info.total_fw_size;
			fw_ver		 = fw_info.fw_ver;
			fw_crc		 = fw_info.fw_crc;
			dfu_stat = DFU_DATA;
			send_response_packet(DFU_HEADER,DFU_OK);
			break;
		case DFU_DATA:
			/* write payload in to memory */

			writeDataChunk(payload, payload_len);
			received_fw_len += payload_len;
			send_response_packet(DFU_START,received_fw_len > total_fw_len ? DFU_ERROR : DFU_OK);
			break;
		case DFU_END:
			uint8_t crc_check_ok=0;
			if(received_fw_len == total_fw_len)
			{
				/* perform a crc check here */

			}


			send_response_packet(DFU_DATA,received_fw_len == total_fw_len && crc_check_ok? DFU_OK:DFU_ERROR);
			break;
		case DFU_UPDATE_FW:
			uint8_t checksumMatched=0;
			/* copy firmware from flash memory to mcu mem */
			UpdateFirmware();
			/* perform checksum here */
			send_response_packet(DFU_DATA,checksumMatched ? DFU_OK:DFU_ERROR);
			break;
		case DFU_JUMP_TO_APP:
			break;

		default:
			break;
	}
}

void writeDataChunk(uint8_t *dat, uint16_t length)
{
	spiflash_WriteBuffer(dat, FW_ADDR_SPIFLASH + fw_index, length);
	fw_index += length;
}

void UpdateFirmware(void)
{

}

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
