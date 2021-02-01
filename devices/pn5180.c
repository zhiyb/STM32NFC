#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <device.h>
#include <debug.h>
#include <common.h>
#include <system/systick.h>
#include <peripheral/spi_master.h>
#include <peripheral/gpio.h>
#include "pn5180.h"
#include "pn5180_defs.h"
#include "nfc_defs.h"

static inline void pn5180_nss_assert()
{
	spi_master_nss(0);
}

static inline void pn5180_nss_deassert()
{
	spi_master_nss(1);
}

#if 0
// Recommendation for the BUSY line handling
static inline void pn5180_start()
{
}

static inline uint8_t pn5180_transfer(uint8_t v)
{
	// 1. Assert NSS to Low
	pn5180_nss_assert();
	// 2. Perform Data Exchange
	v = spi_master_transfer(v);
	// 3. Wait until BUSY is high
	while (!gpio_get_pn3180_busy());
	// 4. Deassert NSS
	pn5180_nss_deassert();
	// 5. Wait until BUSY is low
	while (gpio_get_pn3180_busy());
	return v;
}

static inline void pn5180_end()
{
}
#else
// Recommendation for the BUSY line handling
static inline void pn5180_start()
{
	// 1. Assert NSS to Low
	pn5180_nss_assert();
}

static inline uint8_t pn5180_transfer(uint8_t v)
{
	// 2. Perform Data Exchange
	v = spi_master_transfer(v);
	return v;
}

static inline void pn5180_end()
{
	// 3. Wait until BUSY is high
	while (!gpio_get_pn3180_busy());
	// 4. Deassert NSS
	pn5180_nss_deassert();
	// 5. Wait until BUSY is low
	while (gpio_get_pn3180_busy());
}
#endif

static const void *pn5180_transfer_n_const(const void *p, uint16_t len)
{
	const uint8_t *pc = (const uint8_t *)p;
	while (len--) {
		pn5180_transfer(*pc);
		pc++;
	}
	return p;
}

static void *pn5180_transfer_n(void *p, uint16_t len)
{
	uint8_t *pc = (uint8_t *)p;
	while (len--) {
		*pc = pn5180_transfer(*pc);
		pc++;
	}
	return p;
}

#if 0
// Write register, or mask
// cmd can be one of:
//     CMD_WRITE_REGISTER
//     CMD_WRITE_REGISTER_AND_MASK
//     CMD_WRITE_REGISTER_OR_MASK
static void pn5180_write_register(uint8_t cmd, uint32_t v)
{
	pn5180_transfer(cmd);
	pn5180_transfer_n(&v, 4);
}

// Read register
static uint32_t pn5180_read_register()
{
	uint32_t v;
	pn5180_transfer(CMD_READ_REGISTER);
	pn5180_transfer_n(&v, 4);
	return v;
}
#endif

// Read EEPROM
static void *pn5180_read_eeprom(uint8_t addr, uint8_t len, void *p)
{
	pn5180_start();
	pn5180_transfer(CMD_READ_EEPROM);
	pn5180_transfer(addr);
	pn5180_transfer(len);
	pn5180_end();

	//memset(p, 0xff, len);
	pn5180_start();
	pn5180_transfer_n(p, len);
	pn5180_end();
	return p;
}

// Read Register
static uint32_t pn5180_read_reg(uint8_t addr)
{
	pn5180_start();
	pn5180_transfer(CMD_READ_REGISTER);
	pn5180_transfer(addr);
	pn5180_end();

	uint32_t v = 0xffffffff;
	pn5180_start();
	pn5180_transfer_n(&v, sizeof(v));
	pn5180_end();
	return v;
}

// Write Register
static void pn5180_write_reg(uint8_t addr, uint32_t v)
{
	pn5180_start();
	pn5180_transfer(CMD_WRITE_REGISTER);
	pn5180_transfer(addr);
	pn5180_transfer_n(&v, sizeof(v));
	pn5180_end();
}

static void pn5180_transfer_nss(uint8_t *p, uint32_t size)
{
	pn5180_start();
	pn5180_transfer_n(p, size);
	pn5180_end();
}

static void pn5180_transfer_nss_const(const uint8_t *p, uint32_t size)
{
	pn5180_start();
	pn5180_transfer_n_const(p, size);
	pn5180_end();
}

void pn5180_read_die_id(uint8_t *p)
{
	pn5180_read_eeprom(EEPROM_DIE_ID, 16, p);
}

uint16_t pn5180_read_product_version()
{
	uint16_t v;
	pn5180_read_eeprom(EEPROM_PRODUCT_VERSION, 2, &v);
	return v;
}

uint16_t pn5180_read_firmware_version()
{
	uint16_t v;
	pn5180_read_eeprom(EEPROM_FIRMWARE_VERSION, 2, &v);
	return v;
}

void pn5180_init()
{
	union PACKED {
		struct PACKED {
			uint8_t die[16];
			uint16_t product;
			uint16_t firmware;
			uint16_t eeprom;
		};
		uint8_t raw[256];
	} eeprom;
	eeprom.raw[255] = 0;

	// Hard reset
	gpio_set_pn5180_rst(0);
	gpio_set_pn5180_req(0);
	systick_delay(1);	// At least 10us

	// Power ON
	gpio_set_pn5180_rst(1);
	systick_delay(5);	// Boot time typical 2.5ms

	printf(ESC_INIT "%lu\tpn5180: Init done\n", systick_cnt());

	pn5180_read_eeprom(0, 255, &eeprom);
	dbgprintf(ESC_INFO "[PN5180] Product version: " ESC_DATA "0x%04hx\n", eeprom.product);
	dbgprintf(ESC_INFO "[PN5180] Firmware version: " ESC_DATA "0x%04hx\n", eeprom.firmware);
	dbgprintf(ESC_INFO "[PN5180] EEPROM version: " ESC_DATA "0x%04hx\n", eeprom.eeprom);
	dump_hex(eeprom.raw, 255);

#if 0
	if (pn5180_update(eeprom.firmware) > 0) {
		pn5180_read_eeprom(0, 32, &eeprom);
		dbgprintf(ESC_INFO "[PN5180] Product version: " ESC_DATA "0x%04hx\n", eeprom.product);
		dbgprintf(ESC_INFO "[PN5180] Firmware version: " ESC_DATA "0x%04hx\n", eeprom.firmware);
		dbgprintf(ESC_INFO "[PN5180] EEPROM version: " ESC_DATA "0x%04hx\n", eeprom.eeprom);
	}
#endif

	// LOAD_RF_CONFIG, ISO 14443-A, 106 kbps
	// 1: Loads the ISO 14443 - 106 protocol into the RF registers
	// 1: sendSPI(0x11, 0x00, 0x80);
	{uint8_t d[] = {CMD_LOAD_RF_CONFIG, 0x00, 0x80};
	pn5180_transfer_nss(d, sizeof(d));}

	// Disable TX_CRC_ENABLE
	// 3: Switches the CRC extension off in Tx direction
	// 3: sendSPI(0x02, 0x19, 0xFE, 0xFF, 0xFF, 0xFF);
	{uint8_t d[] = {CMD_WRITE_REGISTER_AND_MASK, REG_CRC_TX_CONFIG, 0xFE, 0xFF, 0xFF, 0xFF};
	pn5180_transfer_nss(d, sizeof(d));}

	// Disable RX_CRC_ENABLE
	// 4: Switches the CRC extension off in Rx direction
	// 4: sendSPI(0x02, 0x12, 0xFE, 0xFF, 0xFF, 0xFF);
	{uint8_t d[] = {CMD_WRITE_REGISTER_AND_MASK, REG_CRC_RX_CONFIG, 0xFE, 0xFF, 0xFF, 0xFF};
	pn5180_transfer_nss(d, sizeof(d));}

	// Clear some IRQs
	// 5: Clears the interrupt register IRQ_STATUS
	// 5: sendSPI(0x00, 0x03, 0xFF, 0xFF, 0x0F, 0x00);
	{uint8_t d[] = {CMD_WRITE_REGISTER, REG_IRQ_CLEAR, 0xFF, 0xFF, 0x0F, 0x00};
	pn5180_transfer_nss(d, sizeof(d));}

	// Set to IDLE state
	// 6: Sets the PN5180 into IDLE state
	// 6: sendSPI(0x02, 0x00, 0xF8, 0xFF, 0xFF, 0xFF);
	{uint8_t d[] = {CMD_WRITE_REGISTER_AND_MASK, REG_SYSTEM_CONFIG, 0xF8, 0xFF, 0xFF, 0xFF};
	pn5180_transfer_nss(d, sizeof(d));}

	// Activate TRANSCEIVE routine
	// 7: Activates TRANSCEIVE routine
	// 7: sendSPI(0x01, 0x00, 0x03, 0x00, 0x00, 0x00);
	{uint8_t d[] = {CMD_WRITE_REGISTER_OR_MASK, REG_SYSTEM_CONFIG, 0x03, 0x00, 0x00, 0x00};
	pn5180_transfer_nss(d, sizeof(d));}

#if 0
	// Clear IRQ on STATUS read, active-high
	// TODO Reset after this
	{uint8_t d[] = {CMD_WRITE_EEPROM, EEPROM_IRQ_PIN_CONFIG, 0x03};
	pn5180_transfer_nss(d, sizeof(d));}
#endif

	// IRQ enable: TX_IRQ_EN, RX_IRQ_EN
	{uint8_t d[] = {CMD_WRITE_REGISTER, REG_IRQ_ENABLE, 0x03, 0x00, 0x00, 0x00};
	pn5180_transfer_nss(d, sizeof(d));}

#if 0
	// 9: Waits until a Card has responded via checking the IRQ_STATUS register
	// 9: waitForCardResponse();

	// 10: Reads the reception buffer. (ATQA)
	// 10: sendSPI(0x0A, 0x00);
	{uint8_t d[] = {CMD_READ_DATA, 0x00};
	pn5180_transfer_nss(d, sizeof(d));}

	// 11: Switches the RF field OFF.
	// 11: sendSPI(0x17, 0x00);
	{uint8_t d[] = {CMD_RF_OFF, 0x00};
	pn5180_transfer_nss(d, sizeof(d));}
#endif
}

INIT_DEV_HANDLER() = &pn5180_init;

static void pn5180_wait_rx()
{
	for (;;) {
		while (!gpio_get_pn3180_irq());
		// Process IRQ
		uint32_t irq = pn5180_read_reg(REG_IRQ_STATUS);
		// RX complete
		if (irq & 1)
			break;
	}
}

static void pn5180_send_data(const uint8_t *p, uint32_t size, uint8_t last_bits)
{
	pn5180_start();
	const uint8_t cmd[] = {CMD_SEND_DATA, last_bits};
	pn5180_transfer_n_const(cmd, sizeof(cmd));
	pn5180_transfer_n_const(p, size);
	pn5180_end();
	dbgprintf(ESC_WRITE "pn5180: TX size %lu (%x)\n", size, (unsigned int)last_bits);
	dump_hex(p, size);
}

static void pn5180_read_data(uint8_t *p)
{
	uint32_t rxstat = pn5180_read_reg(REG_RX_STATUS);
	uint32_t rxsize = rxstat & 0x1ff;
	dbgprintf(ESC_READ "pn5180: RX size %lu\n", rxsize);

	if (rxsize != 0) {
		// 10: Reads the reception buffer. (ATQA)
		// 10: sendSPI(0x0A, 0x00);
		{uint8_t d[] = {CMD_READ_DATA, 0x00};
		 pn5180_transfer_nss_const(d, sizeof(d));}
		if (p == 0) {
			uint8_t buf[512];
			pn5180_transfer_nss(buf, rxsize);
			dump_hex(buf, rxsize);
		} else {
			pn5180_transfer_nss(p, rxsize);
		}
		//dbgprintf(ESC_DEBUG "pn5180: RX size %lu, %02x, %02x\n", rxsize, (int)buf[0], (int)buf[1]);
	}
	//dbgprintf(ESC_DEBUG "pn5180: RX status 0x%08lx\n", rxstat);
}

void pn5180_tick()
{
	static const uint32_t interval = 1000;
	static uint32_t tick = 0;
	uint32_t ctick = systick_cnt();
	if (ctick - tick < interval)
		return;
	tick = ctick;

	dbgprintf(ESC_DEBUG "\npn5180: tick %lu\n", systick_cnt());

	// 3: Switches the CRC extension off in Tx direction
	// 3: sendSPI(0x02, 0x19, 0xFE, 0xFF, 0xFF, 0xFF);
	{uint8_t d[] = {CMD_WRITE_REGISTER_AND_MASK, REG_CRC_TX_CONFIG, 0xFE, 0xFF, 0xFF, 0xFF};
	pn5180_transfer_nss(d, sizeof(d));}

	// 4: Switches the CRC extension off in Rx direction
	// 4: sendSPI(0x02, 0x12, 0xFE, 0xFF, 0xFF, 0xFF);
	{uint8_t d[] = {CMD_WRITE_REGISTER_AND_MASK, REG_CRC_RX_CONFIG, 0xFE, 0xFF, 0xFF, 0xFF};
	pn5180_transfer_nss(d, sizeof(d));}

	// 2: Switches the RF field ON.
	// 2: sendSPI(0x16, 0x00);
	{static const uint8_t d[] = {CMD_RF_ON, 0x00};
	 pn5180_transfer_nss_const(d, sizeof(d));}

	// 8: Sends REQA command
	// 8: sendSPI(0x09, 0x07, 0x26);
	{static const uint8_t d[] = {NFC_REQA};
	 pn5180_send_data(d, sizeof(d), 0x07);}
	// RX complete, read buffer
	pn5180_wait_rx();
	pn5180_read_data(0);

	// Enable TX_CRC_ENABLE
	{static const uint8_t d[] = {CMD_WRITE_REGISTER_OR_MASK, REG_CRC_TX_CONFIG, 0x01, 0x00, 0x00, 0x00};
	 pn5180_transfer_nss_const(d, sizeof(d));}

	// Enable RX_CRC_ENABLE
	{static const uint8_t d[] = {CMD_WRITE_REGISTER_OR_MASK, REG_CRC_RX_CONFIG, 0x01, 0x00, 0x00, 0x00};
	 pn5180_transfer_nss_const(d, sizeof(d));}

	// Send READ to activate the tag
	{static const uint8_t d[] = {NFC_NTAG_READ, 0x00};
	 pn5180_send_data(d, sizeof(d), 0x00);}
	pn5180_wait_rx();
	pn5180_read_data(0);

	// GET_VERSION
	{static const uint8_t d[] = {NFC_NTAG_GET_VERSION};
	 pn5180_send_data(d, sizeof(d), 0x00);}
	pn5180_wait_rx();
	pn5180_read_data(0);

	// READ_SIG
	{static const uint8_t d[] = {NFC_NTAG_READ_SIG, 0x00};
	 pn5180_send_data(d, sizeof(d), 0x00);}
	pn5180_wait_rx();
	pn5180_read_data(0);

	// FAST_READ all data
	uint8_t buf[0x87 * 4];
	{static const uint8_t d[] = {NFC_NTAG_FAST_READ, 0x00, 0x3f};
	 pn5180_send_data(d, sizeof(d), 0x00);}
	pn5180_wait_rx();
	pn5180_read_data(&buf[0]);
	{static const uint8_t d[] = {NFC_NTAG_FAST_READ, 0x40, 0x7f};
	 pn5180_send_data(d, sizeof(d), 0x00);}
	pn5180_wait_rx();
	pn5180_read_data(&buf[0x40 * 4]);
	{static const uint8_t d[] = {NFC_NTAG_FAST_READ, 0x80, 0x86};
	 pn5180_send_data(d, sizeof(d), 0x00);}
	pn5180_wait_rx();
	pn5180_read_data(&buf[0x80 * 4]);

	dbgprintf(ESC_READ "pn5180: NTAG215 dump size %lu\n", (uint32_t)sizeof(buf));
	dump_hex(buf, sizeof(buf));

	// 11: Switches the RF field OFF.
	// 11: sendSPI(0x17, 0x00);
	{static const uint8_t d[] = {CMD_RF_OFF, 0x00};
	 pn5180_transfer_nss_const(d, sizeof(d));}
}

SYSTICK_HANDLER() = &pn5180_tick;

void pn5180_proc()
{
	static int debug = 0;
	if (!debug)
		return;

	static const uint8_t conv[256] = {
		['0'] = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		['a'] = 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
		['A'] = 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
	};

	dbgprintf(ESC_NORMAL);
	printf("> ");
	int rw = getchar();
	putchar(rw);
	putchar(' ');
	uint8_t b1 = getchar();
	putchar(b1);
	b1 = conv[b1];
	uint8_t b0 = getchar();
	putchar(b0);
	b0 = conv[b0];
	uint32_t addr = (b1 << 4) | b0;
	uint32_t v = 0;
	if (rw == 'w') {
		putchar(' ');
		for (int i = 0; i < 8; i++) {
			uint8_t b = getchar();
			putchar(b);
			b = conv[b];
			v = (v << 4) | b;
		}
		pn5180_write_reg(addr, v);
	}
	putchar('\n');
	v = pn5180_read_reg(addr);
	dbgprintf("%spn5180: reg" ESC_DATA " 0x%02lx 0x%08lx\n",
		  rw == 'w' ? ESC_WRITE : ESC_READ, addr, v);
}

IDLE_HANDLER() = &pn5180_proc;
