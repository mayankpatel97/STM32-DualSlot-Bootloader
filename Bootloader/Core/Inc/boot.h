#ifndef __BOOT__

#define __BOOT__

typedef struct
{
	uint32_t index,sector_size,activeSector;
}log_t;

uint8_t is_new_packet(void);
void process_packet(void);

#endif
