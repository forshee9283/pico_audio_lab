#ifndef TOUCH_H
#define TOUCH_H

#include <stdbool.h>
#include <stdint.h>

// Initialize the capacitive-touch PIO program and its interrupt handler.
void touch_init(void);

// Return the latest state of the twelve touch inputs. Bit 0 corresponds to
// GPIO 2 and bit 11 corresponds to GPIO 13.
uint16_t touch_get_state(void);

// Return true once after the PIO reports a changed state.
bool touch_state_changed(void);

#endif
