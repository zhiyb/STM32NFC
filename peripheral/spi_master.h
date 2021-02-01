#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include <stdint.h>
#include <list.h>

void spi_master_nss(int lvl);
uint8_t spi_master_transfer(uint8_t v);
void spi_master_write(const uint8_t *p, unsigned int len);
void spi_master_transfer_buf(uint8_t *p, unsigned int len);

#endif // SPI_MASTER_H
