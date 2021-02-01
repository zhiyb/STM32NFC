#include <device.h>
#include <macros.h>
#include "clocks.h"

extern uint32_t SystemCoreClock;
void SystemCoreClockUpdate(void);

uint32_t clkAHB()
{
	return 72000000;		// TODO calculate?
}

uint32_t clkAPB1()
{
	uint32_t div = (RCC->CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos;
	if (!(div & 0b100))
		return clkAHB();
	return clkAHB() / (2 << (div & 0b11));
}

uint32_t clkAPB2()
{
	uint32_t div = (RCC->CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos;
	if (!(div & 0b100))
		return clkAHB();
	return clkAHB() / (2 << (div & 0b11));
}

uint32_t clkTimer(uint32_t i)
{
	unsigned int apb1rate = (((RCC->CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos) & 0b100) ? 2 : 1;
	//unsigned int apb2rate = (((RCC->CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos) & 0b100) ? 2 : 1;
	switch (i) {
	// APB1 clocked
	case 4:
		return clkAPB1() * apb1rate;
	}
	return 0;
}

void rcc_init()
{
	// Reset the RCC clock configuration to the default reset state
	// Disable all RCC interrupts
	RCC->CIR = 0;
	// Enable HSI
	RCC->CR |= (16 << RCC_CR_HSITRIM_Pos) | RCC_CR_HSION_Msk;
	// Wait for HSI stable
	while (!(RCC->CR & RCC_CR_HSIRDY_Msk));
	// Switch to HSI as system clock
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_HSI;
	// Wait for HSI switch
	while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_HSI);
	// Enable HSE and HSI, disable PLL
	RCC->CR = (16 << RCC_CR_HSITRIM_Pos) | RCC_CR_HSION_Msk | RCC_CR_HSEON_Msk;
	// Configure PLL and clocks
	// USB clock divide by 1.5
	// PLL multiply by 9
	// Select HSE as PLL clock, No HSE divider for PLL
	// AHB not divided, APB1 divided by 2, APB2 not divided
	// Keep HSI as system clock
	RCC->CFGR = (0b111 << RCC_CFGR_PLLMULL_Pos) | RCC_CFGR_PLLSRC_Msk |
		    RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1 |
		    RCC_CFGR_SW_HSI;
	// Wait for HSE stable
	while (!(RCC->CR & RCC_CR_HSERDY_Msk));
	// Enable PLL
	RCC->CR |= RCC_CR_PLLON_Msk;
	// Set flash latency
	// Prefetch enable, no half cycle access, 2 wait states
	FLASH->ACR = FLASH_ACR_PRFTBE_Msk | (0b010 << FLASH_ACR_LATENCY_Pos);
	// Wait for PLL lock
	while (!(RCC->CR & RCC_CR_PLLRDY_Msk));
	// Switch to PLL as system clock
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
	// Wait for PLL switch
	while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL);
	// Keep HSI enabled for flash operations, HSE and PLL enabled
	RCC->CR = (16 << RCC_CR_HSITRIM_Pos) | RCC_CR_HSION_Msk | RCC_CR_HSEON_Msk | RCC_CR_PLLON_Msk;
}
