DFU
=====================
typedef enum {
    DFU_START,
	DFU_ERASE_MEM,
    DFU_HEADER,
    DFU_DATA,
    DFU_END,
    DFU_UPDATE_FW,
    DFU_JUMP_TO_APP
} dfu_stat;

packet format

0x55 CMD LEN PAYLOAD CRC16 0xAA

two slots 

start packet
0x55 0x01 0x00 0xF0 0xB8 0xAA

header packet 

total fw size[4B], fw crc[2B], fw ver[4B]

header[1] cmd[1] len[1] total fw size[4B], fw crc[2B], fw ver[4B] crc[2] footer[1]

