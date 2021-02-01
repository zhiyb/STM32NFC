#ifndef DEVICE_H
#define DEVICE_H

#include <stm32f1xx.h>
#include <list.h>

// Common system level lists
typedef void (*const basic_handler_t)();
#define INIT_HANDLER()		LIST_ITEM(init, basic_handler_t)
#define INIT_DEV_HANDLER()	LIST_ITEM(init_dev, basic_handler_t)
#define IDLE_HANDLER()		LIST_ITEM(idle, basic_handler_t)

#define FIELD(r, f)		(((r) & (f##_Msk)) >> (f##_Pos))

#endif // DEVICE_H
