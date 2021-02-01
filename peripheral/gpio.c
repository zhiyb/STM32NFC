#include <stdio.h>
#include <device.h>
#include <debug.h>
#include <system/irq.h>
#include <system/systick.h>
#include "gpio.h"

#define DEBUG_PRINT	5
#define DEBUG_POWER	1

static void gpio_init()
{
	// Configure GPIOs
	// Low speed outputs:
	// PB12  Active-low   RST
	// PA9   Active-high  REQ
	// Input with pull-down:
	// PB13  ?            BUSY
	// PB14  ?            GPIO
	// PB15  ?            IRQ
	// PA8   ?            AUX
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN_Msk | RCC_APB2ENR_IOPBEN_Msk;
	GPIOA->BSRR = GPIO_BSRR_BR8_Msk | GPIO_BSRR_BR9_Msk;
	GPIOB->BSRR = GPIO_BSRR_BR12_Msk | GPIO_BSRR_BR13_Msk |
		      GPIO_BSRR_BR14_Msk | GPIO_BSRR_BR15_Msk;
	GPIOA->CRH = (GPIOA->CRH & ~((GPIO_CRH_CNF8_Msk | GPIO_CRH_MODE8_Msk) |
				     (GPIO_CRH_CNF9_Msk | GPIO_CRH_MODE9_Msk))) |
		     ((0b10 << GPIO_CRH_CNF8_Pos) | (0b00 << GPIO_CRH_MODE8_Pos)) |
		     ((0b00 << GPIO_CRH_CNF9_Pos) | (0b10 << GPIO_CRH_MODE9_Pos));
	GPIOB->CRH = (GPIOB->CRH & ~((GPIO_CRH_CNF12_Msk | GPIO_CRH_MODE12_Msk) |
				     (GPIO_CRH_CNF13_Msk | GPIO_CRH_MODE13_Msk) |
				     (GPIO_CRH_CNF14_Msk | GPIO_CRH_MODE14_Msk) |
				     (GPIO_CRH_CNF15_Msk | GPIO_CRH_MODE15_Msk))) |
		     ((0b00 << GPIO_CRH_CNF12_Pos) | (0b10 << GPIO_CRH_MODE12_Pos)) |
		     ((0b10 << GPIO_CRH_CNF13_Pos) | (0b00 << GPIO_CRH_MODE13_Pos)) |
		     ((0b10 << GPIO_CRH_CNF14_Pos) | (0b00 << GPIO_CRH_MODE14_Pos)) |
		     ((0b10 << GPIO_CRH_CNF15_Pos) | (0b00 << GPIO_CRH_MODE15_Pos));

#if DEBUG >= DEBUG_PRINT
	printf(ESC_INIT "%lu\tgpio: Init done\n", systick_cnt());
#endif
}

INIT_HANDLER() = &gpio_init;

void gpio_set_pn5180_rst(int lvl)
{
	// PB12  Active-low   RST
	if (lvl)
		GPIOB->BSRR = GPIO_BSRR_BS12_Msk;
	else
		GPIOB->BSRR = GPIO_BSRR_BR12_Msk;
}

void gpio_set_pn5180_req(int lvl)
{
	// PA9   Active-high  REQ
	if (lvl)
		GPIOA->BSRR = GPIO_BSRR_BS9_Msk;
	else
		GPIOA->BSRR = GPIO_BSRR_BR9_Msk;
}

int gpio_get_pn3180_busy()
{
	// PB13  ?            BUSY
	return GPIOB->IDR & GPIO_IDR_IDR13_Msk;
}

int gpio_get_pn3180_irq()
{
	// PB15  ?            IRQ
	return GPIOB->IDR & GPIO_IDR_IDR15_Msk;
}
