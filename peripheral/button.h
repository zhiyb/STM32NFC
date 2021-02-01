#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <list.h>

enum {	ButtonGeneral = 1 << 5,
	ButtonOrientation = 1 << 8};

// Button state change callback
typedef void (*const button_handler_t)(uint16_t btn);
#define BUTTON_HANDLER()	LIST_ITEM(button, button_handler_t)

#endif // BUTTON_H
