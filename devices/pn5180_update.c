#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <device.h>
#include <debug.h>
#include <system/systick.h>
#include <peripheral/spi_master.h>
#include <peripheral/gpio.h>
#include "pn5180.h"
#include "pn5180_defs.h"

#include "PN5180Firmware_4.1.h"

#define SFU_DIR_DOWNLOAD		0x7f
#define SFU_DIR_UPLOAD			0xff

#define SFU_CMD_RESET			0xF0
#define SFU_CMD_GET_VERSION		0xF1
#define SFU_CMD_GET_SESSION_STATE	0xF2
#define SFU_CMD_GET_DIE_ID		0xF4
#define SFU_CMD_CHECK_INTEGRITY		0xE0
#define SFU_CMD_SECURE_WRITE		0xC0
#define SFU_CMD_READ			0xA2

#define SFU_CRC_INIT			0xffff
#define SFU_CRC_POLY			0x1021

union {
	uint16_t crc;
	struct PACKED {
		uint8_t low;
		uint8_t high;
	} f;
} crc;

static inline void crc16_init()
{
	crc.crc = SFU_CRC_INIT;
}

static void crc16_update(uint8_t v)
{
	crc.f.high ^= v;
	for (uint8_t i = 0; i < 8; i++) {
		uint8_t x = crc.f.high & 0x80;
		crc.crc <<= 1;
		if (x)
			crc.crc ^= SFU_CRC_POLY;
	}
}

static inline uint16_t crc16_finish()
{
	return crc.crc;
}

typedef struct PACKED {
	uint16_t len;
	uint8_t cmd;
	union PACKED {
		const uint8_t data[0];
		const void *p;
	};
} chunk_t;

static inline void pn5180_sfu_nss_assert()
{
	//printf(ESC_MSG "SPI start: ");
	spi_master_nss(0);
}

static inline void pn5180_nss_sfu_deassert()
{
	//putchar('\n');
	spi_master_nss(1);
}

static inline void pn5180_sfu_mosi(uint8_t v)
{
	// Perform Data Exchange
	spi_master_transfer(v);
	crc16_update(v);
	//printf(ESC_WRITE "%02x,", v);
}

static inline uint8_t pn5180_sfu_miso()
{
	// Perform Data Exchange
	uint8_t v = spi_master_transfer(0);
	crc16_update(v);
	//printf(ESC_READ "%02x,", v);
	return v;
}

static void pn5180_sfu_write(uint16_t header, const uint8_t *p)
{
	// Wait until BUSY is low
	while (gpio_get_pn3180_busy());
	pn5180_sfu_nss_assert();
	pn5180_sfu_mosi(SFU_DIR_DOWNLOAD);
	crc16_init();
	pn5180_sfu_mosi(header >> 8);
	pn5180_sfu_mosi(header & 0xff);
	uint16_t len = header & 0x03ff;
	while (len--)
		pn5180_sfu_mosi(*p++);
	uint16_t crc = crc16_finish();
	pn5180_sfu_mosi(crc >> 8);
	pn5180_sfu_mosi(crc & 0xff);
	pn5180_nss_sfu_deassert();
}

static uint8_t pn5180_sfu_read(uint16_t *header, uint8_t *p)
{
	// Wait until BUSY is high
	while (!gpio_get_pn3180_busy());
	pn5180_sfu_nss_assert();
	pn5180_sfu_mosi(SFU_DIR_UPLOAD);
	crc16_init();
	*header  = pn5180_sfu_miso() << 8;
	*header |= pn5180_sfu_miso();
	uint16_t len = *header & 0x03ff;
	while (len--)
		*p++ = pn5180_sfu_miso();
	pn5180_sfu_miso();
	pn5180_sfu_miso();
	pn5180_nss_sfu_deassert();
	return crc16_finish() != 0;
}

static uint8_t pn5180_sfu_get(uint8_t op, uint8_t *p, uint16_t *len)
{
	uint8_t payload[4] = {op, 0x00, 0x00, 0x00};
	pn5180_sfu_write(4, payload);
	return pn5180_sfu_read(len, p);
}

static uint16_t pn5180_sfu_write_fragment(const uint8_t *p, uint16_t *len, int fragment)
{
	if (fragment)
		*len |= 1 << 10;
	pn5180_sfu_write(*len, p);
	uint8_t buf[4];
	*len = sizeof(buf);
	uint16_t stat = pn5180_sfu_read(len, buf);
	stat = (stat << 8) | buf[0];
	if (stat != 0)
		printf(ESC_ERROR "[PN5180] Command error: " ESC_DATA "0x%04hx\n", stat);
	return stat;
}

static uint16_t pn5180_sfu_write_chunk(chunk_t c)
{
#if 1
	// Write last fragment
	uint16_t len = c.len;
	if (pn5180_sfu_write_fragment(c.p, &len, 0) != 0)
		return -1;
#else
	static const int fsize = 256;
	// Write complete fragments
	while (c.len >= fsize) {
		c.len -= fsize;
		uint16_t len = fsize;
		if (pn5180_sfu_write_fragment(c.p, &len, c.len != 0) != 0)
			return -1;
		c.p += fsize;
	}

	// Write last fragment
	if (c.len != 0) {
		uint16_t len = c.len;
		if (pn5180_sfu_write_fragment(c.p, &len, 0) != 0)
			return -1;
		c.len = 0;
	}
#endif

	return 0;
}

static const chunk_t *pn5180_sfu_read_header(const chunk_t *start, chunk_t *chunk)
{
	chunk->len = start->len;
	chunk->len = (chunk->len >> 8) | (chunk->len << 8);
	chunk->cmd = start->cmd;
	chunk->p = &start->cmd;
	return (chunk_t *)((uint8_t *)start + chunk->len + 2);
}

static int pn5180_print(const char *msg, uint8_t op)
{
	uint8_t data[128];
	uint16_t size = sizeof(data);
	uint8_t crc = pn5180_sfu_get(op, data, &size);
	if (crc != 0) {
		printf(ESC_ERROR "[PN5180] %s CRC error: " ESC_DATA "%hu\n", msg, (uint16_t)crc);
		return 1;
	} else {
		printf(ESC_INFO "[PN5180] %s: " ESC_DATA, msg);
		for (uint8_t i = 0; i < size; i++)
			printf("%02hx", (uint16_t)data[i]);
		putchar('\n');
		return 0;
	}
}

int pn5180_update(uint16_t fwver)
{
	int err = 0;
	if (fwver == _fw_ver)
		return err;

	// The PN5180 can be used for firmware update as follows:
	// 2. Reset
	gpio_set_pn5180_rst(0);
	// 1. Set DWL_REQ pin to high
	gpio_set_pn5180_req(1);
	systick_delay(1);	// At least 10us
	// 3. The PN1580 boots in download mode
	gpio_set_pn5180_rst(1);
	systick_delay(5);	// Boot time typical 2.5ms

	if (pn5180_print("Version", SFU_CMD_GET_VERSION) != 0) {
		err = -1;
		goto ret;
	}
	if (pn5180_print("Session State",  SFU_CMD_GET_SESSION_STATE) != 0) {
		err = -1;
		goto ret;
	}
	if (pn5180_print("Die ID",  SFU_CMD_GET_DIE_ID) != 0) {
		err = -1;
		goto ret;
	}

	// 4. Download new firmware version
	uint16_t i = 0;
	const chunk_t *p = (const chunk_t *)_fw;
	while ((void *)p != _fw + sizeof(_fw)) {
		chunk_t c;
		p = pn5180_sfu_read_header(p, &c);

		if (i == 0) {
			uint16_t ver = 0;
			memcpy(&ver, c.p + 4, 2);
			printf(ESC_INFO "[PN5180] Update firmware version: " ESC_DATA "0x%04hx\n", ver);
		}

		printf(ESC_WRITE "[PN5180] Writing chunk " ESC_DATA "%hu" ESC_WRITE " of size " ESC_DATA "%hu\n", i, c.len);
		flushCache();

		if (pn5180_sfu_write_chunk(c) != 0) {
			err = -1;
			goto ret;
		}
		i++;
	}
	// 5. Execute the check integrity command to verify the successful update
	// (The CheckIntegrity command cannot be called while a download session is open)
	if (pn5180_print("Check Integrity",  SFU_CMD_CHECK_INTEGRITY) != 0) {
		err = -1;
		goto ret;
	}
	err = 1;

ret:
	// 6. Reset the PN5180
	gpio_set_pn5180_rst(0);
	gpio_set_pn5180_req(0);
	systick_delay(1);	// At least 10us
	// 7. The device starts in NFC operation mode
	gpio_set_pn5180_rst(1);
	systick_delay(5);	// Boot time typical 2.5ms

	return err;
}
