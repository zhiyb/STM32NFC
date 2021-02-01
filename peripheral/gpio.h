#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

void gpio_set_pn5180_rst(int lvl);
void gpio_set_pn5180_req(int lvl);

int gpio_get_pn3180_busy();
int gpio_get_pn3180_irq();

// Configure GPIOs
// Input with pull-down:
// PB13  ?            BUSY
// PB14  ?            GPIO
// PB15  ?            IRQ
// PA8   ?            AUX

#endif // GPIO_H
