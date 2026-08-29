#include "touch.h"

#include "hardware/irq.h"
#include "hardware/pio.h"
#include "touch.pio.h"

enum {
    TOUCH_FIRST_PIN = 2,
    TOUCH_FIRST_BANK_SIZE = 5,
    TOUCH_SECOND_PIN = 7,
    TOUCH_SECOND_BANK_SIZE = 5,
    TOUCH_THIRD_PIN = 12,
    TOUCH_THIRD_BANK_SIZE = 2,
};

static const float TOUCH_PIO_CLOCK_DIVIDER = 48.0f;

static volatile uint16_t touch_state;
static volatile bool touch_change_flag;

static void touch_irq_handler(void) {
    while (!pio_sm_is_rx_fifo_empty(pio0, 0)) {
        const uint32_t pins = pio_sm_get(pio0, 0) & 0x1fu;
        touch_state = (touch_state & (uint16_t)~0x001fu) | (uint16_t)pins;
        touch_change_flag = true;
    }

    while (!pio_sm_is_rx_fifo_empty(pio0, 1)) {
        const uint32_t pins = pio_sm_get(pio0, 1) & 0x1fu;
        touch_state = (touch_state & (uint16_t)~0x03e0u) | (uint16_t)(pins << 5);
        touch_change_flag = true;
    }

    while (!pio_sm_is_rx_fifo_empty(pio0, 2)) {
        const uint32_t pins = pio_sm_get(pio0, 2) & 0x03u;
        touch_state = (touch_state & (uint16_t)~0x0c00u) | (uint16_t)(pins << 10);
        touch_change_flag = true;
    }
}

void touch_init(void) {
    touch_state = 0;
    touch_change_flag = false;

    const uint offset = pio_add_program(pio0, &touch_program);
    touch_sm_init(pio0, 0, offset, TOUCH_FIRST_PIN, TOUCH_FIRST_BANK_SIZE,
                  TOUCH_PIO_CLOCK_DIVIDER);
    touch_sm_init(pio0, 1, offset, TOUCH_SECOND_PIN, TOUCH_SECOND_BANK_SIZE,
                  TOUCH_PIO_CLOCK_DIVIDER);
    touch_sm_init(pio0, 2, offset, TOUCH_THIRD_PIN, TOUCH_THIRD_BANK_SIZE,
                  TOUCH_PIO_CLOCK_DIVIDER);

    irq_set_exclusive_handler(PIO0_IRQ_0, touch_irq_handler);
    pio_set_irq0_source_mask_enabled(
        pio0,
        pis_sm0_rx_fifo_not_empty |
            pis_sm1_rx_fifo_not_empty |
            pis_sm2_rx_fifo_not_empty,
        true);
    irq_set_enabled(PIO0_IRQ_0, true);

    pio_sm_set_enabled(pio0, 0, true);
    pio_sm_set_enabled(pio0, 1, true);
    pio_sm_set_enabled(pio0, 2, true);
}

uint16_t touch_get_state(void) {
    return touch_state;
}

bool touch_state_changed(void) {
    const bool changed = touch_change_flag;
    touch_change_flag = false;
    return changed;
}
