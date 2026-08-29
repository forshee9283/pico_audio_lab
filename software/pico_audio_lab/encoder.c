#include "encoder.h"

#include "pico/stdlib.h"

enum {
    ENCODER_PIN_A = 26,
    ENCODER_PIN_B = 27,
    ENCODER_BUTTON_PIN = 22,
    ENCODER_COUNTS_PER_DETENT = 4,
    BUTTON_DEBOUNCE_US = 20000,
    BUTTON_LONG_PRESS_US = 500000,
    EVENT_QUEUE_SIZE = 16,
};

static const int8_t quadrature_table[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0,
};

static encoder_event_t event_queue[EVENT_QUEUE_SIZE];
static uint8_t event_read_index;
static uint8_t event_write_index;

static uint8_t previous_ab;
static int8_t transition_count;
static int32_t encoder_position;

static bool button_state;
static bool button_candidate_state;
static bool long_press_reported;
static uint64_t button_candidate_since;
static uint64_t button_pressed_since;

static void queue_event(encoder_event_type_t type) {
    const uint8_t next_index = (event_write_index + 1u) % EVENT_QUEUE_SIZE;
    if (next_index == event_read_index) {
        return;
    }

    event_queue[event_write_index].type = type;
    event_queue[event_write_index].position = encoder_position;
    event_write_index = next_index;
}

static uint8_t read_encoder_pins(void) {
    return (uint8_t)((gpio_get(ENCODER_PIN_A) << 1) |
                     gpio_get(ENCODER_PIN_B));
}

static void update_rotation(void) {
    const uint8_t current_ab = read_encoder_pins();
    if (current_ab == previous_ab) {
        return;
    }

    const uint8_t transition = (previous_ab << 2) | current_ab;
    transition_count += quadrature_table[transition];
    previous_ab = current_ab;

    if (transition_count >= ENCODER_COUNTS_PER_DETENT) {
        transition_count = 0;
        ++encoder_position;
        queue_event(ENCODER_EVENT_CLOCKWISE);
    } else if (transition_count <= -ENCODER_COUNTS_PER_DETENT) {
        transition_count = 0;
        --encoder_position;
        queue_event(ENCODER_EVENT_COUNTERCLOCKWISE);
    }
}

static void update_button(uint64_t now) {
    const bool raw_pressed = !gpio_get(ENCODER_BUTTON_PIN);

    if (raw_pressed != button_candidate_state) {
        button_candidate_state = raw_pressed;
        button_candidate_since = now;
    }

    if (button_candidate_state != button_state &&
        now - button_candidate_since >= BUTTON_DEBOUNCE_US) {
        button_state = button_candidate_state;

        if (button_state) {
            button_pressed_since = now;
            long_press_reported = false;
        } else if (!long_press_reported) {
            queue_event(ENCODER_EVENT_BUTTON_SHORT_PRESS);
        }
    }

    if (button_state && !long_press_reported &&
        now - button_pressed_since >= BUTTON_LONG_PRESS_US) {
        long_press_reported = true;
        queue_event(ENCODER_EVENT_BUTTON_LONG_PRESS);
    }
}

void encoder_init(void) {
    gpio_init(ENCODER_PIN_A);
    gpio_set_dir(ENCODER_PIN_A, GPIO_IN);
    gpio_disable_pulls(ENCODER_PIN_A);

    gpio_init(ENCODER_PIN_B);
    gpio_set_dir(ENCODER_PIN_B, GPIO_IN);
    gpio_disable_pulls(ENCODER_PIN_B);

    gpio_init(ENCODER_BUTTON_PIN);
    gpio_set_dir(ENCODER_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_BUTTON_PIN);

    event_read_index = 0;
    event_write_index = 0;
    encoder_position = 0;
    transition_count = 0;
    previous_ab = read_encoder_pins();

    const uint64_t now = time_us_64();
    button_state = !gpio_get(ENCODER_BUTTON_PIN);
    button_candidate_state = button_state;
    button_candidate_since = now;
    button_pressed_since = now;
    long_press_reported = false;
}

void encoder_update(void) {
    update_rotation();
    update_button(time_us_64());
}

bool encoder_get_event(encoder_event_t *event) {
    if (event == NULL || event_read_index == event_write_index) {
        return false;
    }

    *event = event_queue[event_read_index];
    event_read_index = (event_read_index + 1u) % EVENT_QUEUE_SIZE;
    return true;
}

int32_t encoder_get_position(void) {
    return encoder_position;
}

bool encoder_button_is_pressed(void) {
    return button_state;
}

uint8_t encoder_get_raw_state(void) {
    return read_encoder_pins();
}

int8_t encoder_get_transition_count(void) {
    return transition_count;
}

void encoder_reset_position(void) {
    encoder_position = 0;
    transition_count = 0;
}
