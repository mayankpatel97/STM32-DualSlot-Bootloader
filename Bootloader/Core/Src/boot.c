

#include "main.h"
#include "spiflash.h"
#include "boot.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <usbd_cdc_if.h>  // Include the USB CDC interface header

#define PACKET_START_MARKER 0xAA
#define PACKET_END_MARKER 0xBB
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
	DFU_START = 1,
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
	uint32_t fw_crc;
	uint32_t fw_ver;
}__attribute__((packed)) fw_info_t;

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
uint32_t fw_crc;
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
	resp.cmd = (uint8_t)cmd;
	resp.len = 0x01;
	resp.data = (uint8_t)response_code;
	resp.crc = calculate_crc16((uint8_t *)&resp,resp.len + 2 );
	resp.end_byte = PACKET_END_MARKER;
	CDC_Transmit_FS((uint8_t *)&resp,resp.len + 6);
}

void process_packet(void)
{
    // Verify CRC
    uint16_t calculated_crc = calculate_crc16(&payload[0], payload_len);  // CMD + LEN + PAYLOAD
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
			ef_ChipErase();
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
			send_response_packet(DFU_DATA,received_fw_len > total_fw_len ? DFU_ERROR : DFU_OK);
			break;
		case DFU_END:
			uint8_t crc_check_ok=0;
			if(received_fw_len == total_fw_len)
			{
				/* perform a crc check here */

			}


			send_response_packet(DFU_END,received_fw_len == total_fw_len && crc_check_ok? DFU_OK:DFU_ERROR);
			break;
		case DFU_UPDATE_FW:
			uint8_t checksumMatched=0;
			/* copy firmware from flash memory to mcu mem */
			UpdateFirmware();
			/* perform checksum here */
			send_response_packet(DFU_UPDATE_FW,checksumMatched ? DFU_OK:DFU_ERROR);
			break;
		case DFU_JUMP_TO_APP:
			break;

		default:
			break;
	}
}

void writeDataChunk(uint8_t *dat, uint16_t length)
{
	//ef_WriteBuffer(dat, FW_ADDR_SPIFLASH + fw_index, length);
	fw_index += length;
}

void UpdateFirmware(void)
{

}
