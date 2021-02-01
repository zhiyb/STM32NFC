#ifndef DEFINES_H
#define DEFINES_H

#include <stdint.h>

#define USB_VID			0x0483
#define USB_PID			0x5750
#define USB_RELEASE		SWVER
#define USB_MANUFACTURER	"zhiyb"
#define USB_PRODUCT_NAME	"USB Mix"

// 25 bytes space (null-terminated) required
void uid_str(char *s);
void dump_hex(const void *pdata, uint32_t size);

#endif // DEFINES_H
