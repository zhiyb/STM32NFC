#include <device.h>
#include <debug.h>
#include <list.h>
#include <system/clocks.h>
#include <system/irq.h>
#include "systick.h"

LIST(systick_irq, systick_handler_t);
LIST(systick, systick_handler_t);

static volatile uint32_t cnt;

void systick_init(uint32_t hz)
{
	cnt = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
	SysTick->LOAD = (((clkAHB() << 1) / hz + 1) >> 1) - 1;
	// Configure interrupt priority
	uint32_t pg = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(pg, NVIC_PRIORITY_SYSTICK, 0));
	// Enable SysTick
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

uint32_t systick_cnt()
{
	return cnt;
}

void systick_delay(uint32_t cycles)
{
	uint32_t c = cnt;
	cycles += 1;
	while (cnt - c < cycles)
		__WFI();
}

void SysTick_Handler()
{
	uint32_t c = ++cnt;
	LIST_ITERATE(systick_irq, systick_handler_t, p) (*p)(c);
}

static void systick_process()
{
	static uint32_t pcnt = 0;
	uint32_t c = cnt;
	if (pcnt != c) {
		pcnt = c;
		LIST_ITERATE(systick, systick_handler_t, p) (*p)(c);
	}
}

IDLE_HANDLER() = &systick_process;
