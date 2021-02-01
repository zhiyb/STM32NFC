#include <stdint.h>
#include <device.h>
#include <debug.h>
#include "common.h"

static char toHex(char c)
{
	return (c < 10 ? '0' : 'a' - 10) + c;
}

void uid_str(char *s)
{
	// Construct serial number string from device UID
	char *c = s;
	uint8_t *p = (void *)UID_BASE + 11u;
	for (uint32_t i = 12; i != 0; i--) {
		*c++ = toHex(*p >> 4u);
		*c++ = toHex(*p-- & 0x0f);
	}
	*c = 0;
}

void dump_hex(const void *pdata, uint32_t size)
{
	flushCache();
	//dbgprintf("\n");
	//dbgprintf(ESC_READ "[PN5180] EEPROM data:\n" ESC_DEBUG);
	printf(ESC_DEBUG);
	const uint8_t *p = pdata;
	for (uint32_t i = 0; i < size; i += 0x10) {
		if (i == 0) {
			printf("         ");
			for (uint32_t j = 0; j < 0x10; j++) {
				if (i + j >= size)
					break;
				printf("%02x ", (unsigned int)j);
			}
			printf("\n");
		}
		printf(ESC_DEBUG "%08x " ESC_DATA, (unsigned int)i);
		for (uint32_t j = 0; j < 0x10; j++) {
			uint32_t idx = i + j;
			if (idx >= size)
				break;
			//dbgprintf("%02x ", (unsigned int)idx);
			printf("%02x ", (unsigned int)p[idx]);
		}
		printf("\n");
		flushCache();
	}
	//dbgprintf("\n");
}
