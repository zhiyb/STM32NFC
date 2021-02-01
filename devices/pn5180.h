#ifndef IO_H
#define IO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pn5180_init();
int pn5180_update(uint16_t fwver);

void pn5180_read_die_id(uint8_t *p);
uint16_t pn5180_read_product_version();
uint16_t pn5180_read_firmware_version();

#ifdef __cplusplus
}
#endif

#endif // IO_H
