#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>
#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*const systick_handler_t)(uint32_t cnt);

void systick_init(uint32_t hz);
uint32_t systick_cnt();
void systick_delay(uint32_t cycles);

// Register a systick handler
#define SYSTICK_IRQ_HANDLER()	LIST_ITEM(systick_irq, systick_handler_t)
#define SYSTICK_HANDLER()	LIST_ITEM(systick, systick_handler_t)

#ifdef __cplusplus
}
#endif

#endif // SYSTICK_H
