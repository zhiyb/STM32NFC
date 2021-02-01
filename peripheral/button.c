#include <stdio.h>
#include <device.h>
#include <debug.h>
#include <system/systick.h>
#include <system/irq.h>
#include <peripheral/rtc.h>
#include "button.h"

#define DEBOUNCING_MS	50

#define DEBUG_PRINT	5
#define DEBUG_CHECK	5

LIST(button, button_handler_t);

static struct {
	volatile uint16_t state, debouncing;
	volatile uint32_t tick[16];
} data = {0};

static inline uint16_t button_read();

static void button_init()
{
	// Configure GPIOs
	// PA8: Input pull-up
	// PB5: Input pull-up
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN_Msk | RCC_APB2ENR_IOPBEN_Msk | RCC_APB2ENR_AFIOEN_Msk;
	GPIOA->CRH = (GPIOA->CRH & ~(GPIO_CRH_CNF8_Msk | GPIO_CRH_MODE8_Msk)) |
		     ((0b10 << GPIO_CRH_CNF8_Pos) | (0b00 << GPIO_CRH_MODE8_Pos));
	GPIOB->CRL = (GPIOB->CRL & ~(GPIO_CRL_CNF5_Msk | GPIO_CRL_MODE5_Msk)) |
		     ((0b10 << GPIO_CRL_CNF5_Pos) | (0b00 << GPIO_CRL_MODE5_Pos));
	GPIOA->BSRR = GPIO_BSRR_BS8_Msk;
	GPIOB->BSRR = GPIO_BSRR_BS5_Msk;

	// Configure interrupts
	AFIO->EXTICR[1] |= AFIO_EXTICR2_EXTI5_PB;
	AFIO->EXTICR[2] |= AFIO_EXTICR3_EXTI8_PA;
	EXTI->RTSR |= EXTI_RTSR_TR5_Msk | EXTI_RTSR_TR8_Msk;
	EXTI->FTSR |= EXTI_FTSR_TR5_Msk | EXTI_FTSR_TR8_Msk;
	EXTI->PR |= EXTI_PR_PR5_Msk | EXTI_PR_PR8_Msk;

	uint32_t pg = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(EXTI9_5_IRQn, NVIC_EncodePriority(pg, NVIC_PRIORITY_BUTTON, 0));
	NVIC_EnableIRQ(EXTI9_5_IRQn);

	// Update button states
	data.state = button_read();

	// Enable interrupts
	EXTI->IMR |= EXTI_IMR_MR5_Msk | EXTI_IMR_MR8_Msk;

#if DEBUG >= DEBUG_PRINT
	printf(ESC_INIT "%lu\tbutton: Init done\n", systick_cnt());
#endif
}

INIT_HANDLER() = &button_init;

static inline uint16_t button_read()
{
	return (GPIOA->IDR & GPIO_IDR_IDR8_Msk) | (GPIOB->IDR & GPIO_IDR_IDR5_Msk);
}

void EXTI9_5_IRQHandler()
{
	uint16_t pr = EXTI->PR;
	uint32_t tick = systick_cnt();

	// Debouncing pins
	uint16_t sw = EXTI->SWIER;
	// Activated pins
	uint16_t hw = pr & ~sw;

	// Disable detection for activated pins, enable for debouncing pins
	EXTI->FTSR = (EXTI->FTSR & ~hw) | sw;
	EXTI->RTSR = (EXTI->RTSR & ~hw) | sw;
	// Reset pending IRQ states
	// If debouncing pin states changed unexpectedly, it should be caught by PR
	EXTI->PR = pr;
	//NVIC_ClearPendingIRQ(EXTI9_5_IRQn);

	// Toggle activated pins, update debouncing pins
	uint16_t state = data.state ^ hw;
	if (sw)
		state = (state & ~sw) | (button_read() & sw);
	data.state = state;

	// Debouncing ticks
	data.debouncing = (data.debouncing & ~sw) | hw;
	for (unsigned int i = 0; hw != 0; hw >>= 1, i++)
		if (hw & 1)
			data.tick[i] = tick;
}

static void button_tick(uint32_t tick)
{
	uint16_t db = data.debouncing;
	if (db) {
		// Find pins with expired debouncing timer
		uint16_t sw = 0;
		for (unsigned int i = 0; db != 0; db >>= 1, i++)
			if ((db & 1) && tick - data.tick[i] >= DEBOUNCING_MS)
				sw |= 1 << i;
		EXTI->SWIER = sw;
	}

	static uint16_t pstate = 0;
	uint16_t state = data.state;
	if (state != pstate) {
		pstate = state;
		// Button state callbacks
		LIST_ITERATE(button, button_handler_t, pfunc)
			(*pfunc)(state);
#if DEBUG >= DEBUG_CHECK
		printf(ESC_DEBUG "%lu\tbutton: State: 0x%04x\n", systick_cnt(), state);
#endif
	}
}

SYSTICK_HANDLER() = &button_tick;
