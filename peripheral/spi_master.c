#include <stdio.h>
#include <string.h>
#include <device.h>
#include <debug.h>
#include <system/systick.h>
#include <system/clocks.h>
#include <system/irq.h>
#include "spi_master.h"

#define DEBUG_PRINT	5
#define DEBUG_CHECK	5

#define SPI			SPI1
#define DMA			DMA1
#define DMA_CHANNEL_TX		DMA1_Channel3
#define DMA_CHANNEL_TX_IRQ	DMA1_Channel3_IRQn
#define DMA_CHANNEL_RX		DMA1_Channel2
#define DMA_CHANNEL_RX_IRQ	DMA1_Channel2_IRQn

static struct {
} data = {};

static void spi_master_init()
{
	// Configure GPIOs
	// PA4   CS SPI1: Output push-pull, 10MHz
	// PA5  SCK SPI1: Alternate function output push-pull, 10MHz
	// PA6 MISO SPI1: Floating input
	// PA7 MOSI SPI1: Alternate function output push-pull, 10MHz
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN_Msk | RCC_APB2ENR_AFIOEN_Msk;
	GPIOA->CRL = (GPIOA->CRL & ~(GPIO_CRL_CNF4_Msk | GPIO_CRL_MODE4_Msk |
				     GPIO_CRL_CNF5_Msk | GPIO_CRL_MODE5_Msk |
				     GPIO_CRL_CNF6_Msk | GPIO_CRL_MODE6_Msk |
				     GPIO_CRL_CNF7_Msk | GPIO_CRL_MODE7_Msk)) |
		     ((0b00 << GPIO_CRL_CNF4_Pos) | (0b01 << GPIO_CRL_MODE4_Pos)) |
		     ((0b10 << GPIO_CRL_CNF5_Pos) | (0b01 << GPIO_CRL_MODE5_Pos)) |
		     ((0b01 << GPIO_CRL_CNF6_Pos) | (0b00 << GPIO_CRL_MODE6_Pos)) |
		     ((0b10 << GPIO_CRL_CNF7_Pos) | (0b01 << GPIO_CRL_MODE7_Pos));
	// De-select SPI
	GPIOA->BSRR = GPIO_BSRR_BS4_Msk;
	// Disable SPI1 remap
	AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG_Msk) & ~AFIO_MAPR_SPI1_REMAP_Msk;

	// Enable SPI1 clock
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN_Msk;
	// Configure SPI
	// SPI mode, I2S disabled
	SPI->I2SCFGR = 0;
	// 2-line unidirectional, output enabled, CRC disabled, 8-bit data
	// SSM set NSS to 1, MSB first, SPI disabled, baud rate div by 16, master mode
	// CPOL = 0, CPHA = 0
	SPI->CR1 = SPI_CR1_SSM_Msk | SPI_CR1_SSI_Msk | (0b011 << SPI_CR1_BR_Pos) | SPI_CR1_MSTR_Msk;
	// SS output disable, TX DMA disabled, RX DMA disabled
	SPI->CR2 = 0;	// SPI_CR2_TXDMAEN_Msk;
	// Enable SPI
	SPI->CR1 |= SPI_CR1_SPE_Msk;

	// TODO Init DMA

	// Configure GPIOs
	// PB0   WP: Input pull-up
	// PB1  RST: Input pull-up
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN_Msk;
	GPIOB->CRL = (GPIOB->CRL & ~(GPIO_CRL_CNF0_Msk | GPIO_CRL_MODE0_Msk |
				     GPIO_CRL_CNF1_Msk | GPIO_CRL_MODE1_Msk)) |
		     ((0b10 << GPIO_CRL_CNF0_Pos) | (0b00 << GPIO_CRL_MODE0_Pos)) |
		     ((0b10 << GPIO_CRL_CNF1_Pos) | (0b00 << GPIO_CRL_MODE1_Pos));
	// Disable RST and WP
	GPIOB->BSRR = GPIO_BSRR_BS0_Msk | GPIO_BSRR_BS1_Msk;

#if DEBUG >= DEBUG_PRINT
	printf(ESC_INIT "%lu\tspi: Init done\n", systick_cnt());
#endif
}

INIT_HANDLER() = &spi_master_init;

void spi_master_nss(int lvl)
{
	if (lvl)
		GPIOA->BSRR = GPIO_BSRR_BS4_Msk;
	else
		GPIOA->BSRR = GPIO_BSRR_BR4_Msk;
}

uint8_t spi_master_transfer(uint8_t v)
{
	while (!(SPI->SR & SPI_SR_TXE_Msk));
	SPI->DR = v;
	while (!(SPI->SR & SPI_SR_RXNE_Msk));
	return SPI->DR;
}

void spi_master_write(const uint8_t *p, unsigned int len)
{
	while (len--)
		spi_master_transfer(*p++);
}

void spi_master_transfer_buf(uint8_t *p, unsigned int len)
{
	for (; len--; p++)
		*p = spi_master_transfer(*p);
}

#if 0
static void spi_master_process()
{
#if DEBUG >= DEBUG_CHECK
	AFIO_TypeDef *afio = AFIO;
	SPI_TypeDef *spi = SPI;
	DMA_TypeDef *dma = DMA;
	DMA_Channel_TypeDef *dmarx = DMA_CHANNEL_RX;
	DMA_Channel_TypeDef *dmatx = DMA_CHANNEL_TX;
#endif
	static int pwup = 0;

	if (!pwup) {
		// Select SPI
		GPIOA->BSRR = GPIO_BSRR_BR4_Msk;
		// Release power-down and read device ID
		spi_write((const uint8_t []){0xab, 0, 0, 0}, 4);
		uint8_t id = spi_transfer(0);
		// De-select SPI
		GPIOA->BSRR = GPIO_BSRR_BS4_Msk;
		// Delay 1.,8us
		printf(ESC_READ "%lu\tflash: Read ID 0x%02x\n", systick_cnt(), id);

		// Select SPI
		GPIOA->BSRR = GPIO_BSRR_BR4_Msk;
		// Read JEDEC ID
		spi_transfer(0x9f);
		uint8_t jedec[3];
		jedec[0] = spi_transfer(0);
		jedec[1] = spi_transfer(0);
		jedec[2] = spi_transfer(0);
		// De-select SPI
		GPIOA->BSRR = GPIO_BSRR_BS4_Msk;
		printf(ESC_READ "%lu\tflash: JEDEC ID 0x%02x, 0x%02x, 0x%02x\n",
		       systick_cnt(), jedec[0], jedec[1], jedec[2]);

		// Select SPI
		GPIOA->BSRR = GPIO_BSRR_BR4_Msk;
		// Read Unique ID
		spi_write((const uint8_t []){0x4b, 0, 0, 0, 0}, 5);
		uint8_t uid[8];
		spi_transfer_buf(uid, sizeof(uid));
		// De-select SPI
		GPIOA->BSRR = GPIO_BSRR_BS4_Msk;
		printf(ESC_READ "%lu\tflash: Unique ID 0x%02x", systick_cnt(), uid[0]);
		for (unsigned int i = 1; i < sizeof(uid); i++)
			printf(", 0x%02x", uid[i]);
		printf("\n");

		//pwup = 1;
	}
}

RTC_SECOND_HANDLER() = &spi_master_process;
#endif
