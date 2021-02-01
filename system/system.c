#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <escape.h>
#include <device.h>
#include <debug.h>
#include <system/systick.h>
#include <system/clocks.h>
#include <system/irq.h>
#include "system.h"

#ifdef USE_STDIO
#define STDOUT_BUFFER_SIZE	1024
#endif

// Vector table base address
extern const uint32_t g_pfnVectors[];

// Section addresses
extern uint8_t __bss_start__;
extern uint8_t __bss_end__;
extern uint8_t __data_load__;
extern uint8_t __data_start__;
extern uint8_t __data_end__;

// Main entry point
extern int main();

#ifdef USE_STDIO
// stdout buffer
static char stdout_buf[STDOUT_BUFFER_SIZE];
static uint32_t i_stdout_read = 0;
static volatile uint32_t i_stdout_write = 0;

static inline void debug_uart_init()
{
	// Configure GPIOs
	// PB6 TX USART1: Alternate function output push-pull, 2MHz
	// PB7 RX USART1: Input with pull-up
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN_Msk | RCC_APB2ENR_AFIOEN_Msk;
	GPIOB->CRL = (GPIOB->CRL & ~(GPIO_CRL_CNF6_Msk | GPIO_CRL_MODE6_Msk |
				     GPIO_CRL_CNF7_Msk | GPIO_CRL_MODE7_Msk)) |
		     ((0b10 << GPIO_CRL_CNF6_Pos) | (0b10 << GPIO_CRL_MODE6_Pos)) |
		     ((0b10 << GPIO_CRL_CNF7_Pos) | (0b00 << GPIO_CRL_MODE7_Pos));
	GPIOB->ODR |= GPIO_ODR_ODR7_Msk;
	AFIO->MAPR |= AFIO_MAPR_USART1_REMAP_Msk;

	// Enable USART1 clock
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN_Msk;
	// USART1 uses APB2 clock
	uint32_t clk = clkAPB2();
	// Debug UART baud rate 115200
	uint32_t baud = 115200;
	// Disable UART, 8-bit, N, 1 stop
	USART1->CR1 = 0;
	USART1->CR2 = 0;
	USART1->CR3 = 0;
	USART1->BRR = ((clk << 1) / baud + 1) >> 1;
	// Enable transmitter & receiver & UART
	USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static inline void debug_uart_putchar(int ch)
{
	while (!(USART1->SR & USART_SR_TXE));
	USART1->DR = ch;
}

static inline int debug_uart_getchar(void)
{
	while (!(USART1->SR & USART_SR_RXNE));
	return USART1->DR;
}

// Buffered STDIO
int fio_write(int file, char *ptr, int len)
{
	UNUSED(file);
#if DEBUG >= 6
	if (file != STDOUT_FILENO) {
		errno = ENOSYS;
		return -1;
	}
#endif
	// Update buffer pointer
	__disable_irq();
	uint32_t i = i_stdout_write;
	i_stdout_write = (i + len) & (STDOUT_BUFFER_SIZE - 1);
	__enable_irq();

	// Copy data to allocated space
	if (i + len >= STDOUT_BUFFER_SIZE) {
		uint32_t s = STDOUT_BUFFER_SIZE - i;
		memcpy(&stdout_buf[i], ptr, s);
		memcpy(&stdout_buf[0], ptr + s, len - s);
	} else {
		memcpy(&stdout_buf[i], ptr, len);
	}
	return len;
}

int fio_read(int file, char *ptr, int len)
{
	UNUSED(file);
	UNUSED(len);
#if 1
	flushCache();
	*ptr = debug_uart_getchar();
	return 1;
#if 0
	for (int i = 0; i < len; i++)
		ptr[i] = debug_uart_getchar();
	return len;
#endif
#else
	UNUSED(ptr);
	errno = ENOSYS;
	return -1;
#endif
}
#endif

// Reset entry point, initialise the system
void reset(void)
{
	// Configure the Vector Table location add offset address
	SCB->VTOR = (uint32_t)g_pfnVectors;

	// System initialisation
	NVIC_SetPriorityGrouping(NVIC_PRIORITY_GROUPING);
	rcc_init();
	systick_init(1000);
#ifdef USE_STDIO
	debug_uart_init();
#endif

	// Initialise data from flash to SRAM
	// TODO using DMA
	memcpy(&__data_start__, &__data_load__, &__data_end__ - &__data_start__);

	// Zero fill bss segment
	memset(&__bss_start__, 0, &__bss_end__ - &__bss_start__);

	// Start program
	asm(	"ldr sp, =__stack_end__\n\t"	// Set stack pointer
		"bl __libc_init_array\n\t"	// Call static constructors
		"bl main\n\t"			// Call the application's entry point
		: : "r"(main));
}

// Default handler for unexpected interrupts
void debug_handler()
{
	uint32_t ipsr = __get_IPSR();
	SCB_Type scb = *SCB;
	NVIC_Type nvic = *NVIC;
	printf(ESC_ERROR "\nUnexpected interrupt: %lu\n", ipsr);
	dbgbkpt();
	NVIC_SystemReset();
}

#if DEBUG > 5
#ifndef BOOTLOADER
void HardFault_Handler()
{
	SCB_Type scb = *SCB;
	printf(ESC_ERROR "\nHard fault: 0x%08lx\n", scb.HFSR);

#if DEBUG > 5
	printf("...\t%s%s%s\n",
	       scb.HFSR & SCB_HFSR_DEBUGEVT_Msk ? "Debug " : "",
	       scb.HFSR & SCB_HFSR_FORCED_Msk ? "Forced " : "",
	       scb.HFSR & SCB_HFSR_VECTTBL_Msk ? "Vector " : "");
	uint8_t mfsr = FIELD(scb.CFSR, SCB_CFSR_MEMFAULTSR);
	if (mfsr & 0x80) {	// Memory manage fault valid
		printf("Memory manage fault: 0x%02x\n", mfsr);
		printf("...\t%s%s%s%s\n",
		       mfsr & 0x10 ? "Entry " : "",
		       mfsr & 0x08 ? "Return " : "",
		       mfsr & 0x02 ? "Data " : "",
		       mfsr & 0x01 ? "Instruction " : "");
		printf("...\tAddress: 0x%08lx\n", scb.MMFAR);
	}
	uint8_t bfsr = FIELD(scb.CFSR, SCB_CFSR_BUSFAULTSR);
	if (bfsr & 0x80) {	// Bus fault valid
		printf("Bus fault: 0x%02x\n", bfsr);
		printf("...\t%s%s%s%s%s\n",
		       bfsr & 0x10 ? "Entry " : "",
		       bfsr & 0x08 ? "Return " : "",
		       bfsr & 0x04 ? "Imprecise " : "",
		       bfsr & 0x02 ? "Precise " : "",
		       bfsr & 0x01 ? "Instruction " : "");
		if (bfsr & 0x02)
			printf("...\tPrecise: 0x%08lx\n", scb.BFAR);
	}
	uint16_t ufsr = FIELD(scb.CFSR, SCB_CFSR_USGFAULTSR);
	if (ufsr) {	// Usage fault
		printf("Usage fault: 0x%04x\n", ufsr);
		printf("...\t%s%s%s%s%s%s\n",
		       ufsr & 0x0200 ? "Divide " : "",
		       ufsr & 0x0100 ? "Unaligned " : "",
		       ufsr & 0x0008 ? "Coprocessor " : "",
		       ufsr & 0x0004 ? "INVPC " : "",
		       ufsr & 0x0002 ? "INVSTATE " : "",
		       ufsr & 0x0001 ? "UNDEFINSTR " : "");
	}
#endif

	dbgbkpt();
	NVIC_SystemReset();
}
#endif
#endif

#ifdef USE_STDIO
static void system_debug_process()
{
	// Flush stdout buffer to debug uart
	while (i_stdout_read != i_stdout_write) {
		char c = stdout_buf[i_stdout_read];
		if (c == '\n')
			debug_uart_putchar('\r');
		debug_uart_putchar(c);
		i_stdout_read = (i_stdout_read + 1) & (STDOUT_BUFFER_SIZE - 1);
	}
}

IDLE_HANDLER() = &system_debug_process;
#endif

void flushCache()
{
#ifdef USE_STDIO
	fflush(stdout);
	system_debug_process();
#endif
}
