#ifndef ENCODER_H
#define ENCODER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ENCODER_EVENT_NONE = 0,
    ENCODER_EVENT_CLOCKWISE,
    ENCODER_EVENT_COUNTERCLOCKWISE,
    ENCODER_EVENT_BUTTON_SHORT_PRESS,
    ENCODER_EVENT_BUTTON_LONG_PRESS,
} encoder_event_type_t;

typedef struct {
    encoder_event_type_t type;
    int32_t position;
} encoder_event_t;

void encoder_init(void);

// Call frequently from the main loop. A 1 ms interval is recommended.
void encoder_update(void);

// Retrieve the oldest pending semantic event.
bool encoder_get_event(encoder_event_t *event);

int32_t encoder_get_position(void);
bool encoder_button_is_pressed(void);
uint8_t encoder_get_raw_state(void);
int8_t encoder_get_transition_count(void);
void encoder_reset_position(void);

#endif
