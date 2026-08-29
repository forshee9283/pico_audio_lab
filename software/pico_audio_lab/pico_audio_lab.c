
#include <stdio.h>
#include <string.h>
#include "audio_pwm.h"
#include "encoder.h"
#include "pam8302.h"
#include "pico/stdlib.h"
#include "touch.h"

#define MIDI_TX 0
#define MIDI_RX 1
#define SW1 2
#define SW2 3
#define SW3 4
#define SW4 5
#define SW5 6
#define SW6 7
#define SW7 8
#define SW8 9
#define SW9 10
#define SW10 11
#define SW11 12
#define SW12 13
#define AUDIO_3 16
#define AUDIO_4 17
#define LED_1 18
#define LED_2 19
#define LED_3 20
#define LED_4 21
#define LED_PIN PICO_DEFAULT_LED_PIN

static bool watch_encoder = true;
static bool watch_encoder_raw;

static void print_encoder_status(void) {
    const uint8_t raw = encoder_get_raw_state();
    printf("encoder: position=%ld button=%s A=%u B=%u raw=%u transitions=%d\n",
           (long)encoder_get_position(),
           encoder_button_is_pressed() ? "pressed" : "released",
           (raw >> 1) & 1u,
           raw & 1u,
           raw,
           encoder_get_transition_count());
}

static void print_console_help(void) {
    printf("Commands:\n");
    printf("  help           Show this list\n");
    printf("  encoder        Show encoder and button state\n");
    printf("  encoder reset  Reset encoder position to zero\n");
    printf("  encoder raw on|off  Trace every raw A/B change\n");
    printf("  watch on|off   Enable or disable encoder event output\n");
    printf("  touch          Show the latest touch bit mask\n");
    printf("  amp on|off|status  Control the PAM8302 amplifier\n");
    printf("  tone <Hz>|off  Set the differential PWM test tone\n");
    printf("  level <0-100>  Set test-tone level percentage\n");
    printf("  audio          Show audio path status\n");
}

static void print_audio_status(void) {
    printf("audio: amp=%s tone=%lu Hz level=%u%% carrier=%lu Hz\n",
           pam8302_is_enabled() ? "on" : "off",
           (unsigned long)audio_pwm_get_test_tone(),
           audio_pwm_get_level_percent(),
           (unsigned long)audio_pwm_get_carrier_rate());
}

static void run_console_command(const char *command) {
    unsigned int value;

    if (strcmp(command, "help") == 0) {
        print_console_help();
    } else if (strcmp(command, "encoder") == 0) {
        print_encoder_status();
    } else if (strcmp(command, "encoder reset") == 0) {
        encoder_reset_position();
        printf("encoder: position reset\n");
    } else if (strcmp(command, "encoder raw on") == 0) {
        watch_encoder_raw = true;
        printf("encoder raw watch: on\n");
    } else if (strcmp(command, "encoder raw off") == 0) {
        watch_encoder_raw = false;
        printf("encoder raw watch: off\n");
    } else if (strcmp(command, "watch on") == 0) {
        watch_encoder = true;
        printf("encoder watch: on\n");
    } else if (strcmp(command, "watch off") == 0) {
        watch_encoder = false;
        printf("encoder watch: off\n");
    } else if (strcmp(command, "touch") == 0) {
        printf("touch: 0x%03x\n", touch_get_state());
    } else if (strcmp(command, "amp on") == 0) {
        pam8302_set_enabled(true);
        printf("amplifier: on\n");
    } else if (strcmp(command, "amp off") == 0) {
        pam8302_set_enabled(false);
        printf("amplifier: shutdown\n");
    } else if (strcmp(command, "amp status") == 0 ||
               strcmp(command, "audio") == 0) {
        print_audio_status();
    } else if (strcmp(command, "tone off") == 0) {
        audio_pwm_set_test_tone(0);
        printf("tone: off\n");
    } else if (sscanf(command, "tone %u", &value) == 1) {
        if (value < 20u || value > 20000u) {
            printf("tone: frequency must be 20-20000 Hz\n");
        } else {
            audio_pwm_set_test_tone(value);
            printf("tone: %u Hz\n", value);
        }
    } else if (sscanf(command, "level %u", &value) == 1) {
        if (value > 100u) {
            printf("level: value must be 0-100\n");
        } else {
            audio_pwm_set_level_percent((uint8_t)value);
            printf("level: %u%%\n", value);
        }
    } else if (command[0] != '\0') {
        printf("Unknown command: %s\n", command);
        printf("Type 'help' for available commands.\n");
    }
}

static void update_console(void) {
    static char command[64];
    static size_t length;

    int character;
    while ((character = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        if (character == '\r' || character == '\n') {
            if (length != 0) {
                command[length] = '\0';
                printf("\n");
                run_console_command(command);
                length = 0;
                printf("> ");
            }
        } else if ((character == '\b' || character == 127) && length != 0) {
            --length;
        } else if (character >= 32 && character <= 126 &&
                   length < sizeof(command) - 1) {
            command[length++] = (char)character;
        }
    }
}

static void report_encoder_events(void) {
    encoder_event_t event;
    while (encoder_get_event(&event)) {
        if (!watch_encoder) {
            continue;
        }

        switch (event.type) {
            case ENCODER_EVENT_CLOCKWISE:
                printf("\rencoder: clockwise, position=%ld\n> ",
                       (long)event.position);
                break;
            case ENCODER_EVENT_COUNTERCLOCKWISE:
                printf("\rencoder: counterclockwise, position=%ld\n> ",
                       (long)event.position);
                break;
            case ENCODER_EVENT_BUTTON_SHORT_PRESS:
                printf("\rencoder button: short press\n> ");
                break;
            case ENCODER_EVENT_BUTTON_LONG_PRESS:
                printf("\rencoder button: long press\n> ");
                break;
            case ENCODER_EVENT_NONE:
                break;
        }
    }
}

static void report_raw_encoder_changes(void) {
    static uint8_t previous_raw = 0xffu;
    const uint8_t raw = encoder_get_raw_state();

    if (raw == previous_raw) {
        return;
    }

    previous_raw = raw;
    if (watch_encoder_raw) {
        printf("\rencoder raw: A=%u B=%u state=%u transitions=%d\n> ",
               (raw >> 1) & 1u,
               raw & 1u,
               raw,
               encoder_get_transition_count());
    }
}

void displayKeyboard(uint16_t buttons) {
    printf("    _   _       _   _   _  \n");
    printf("   |%c| |%c|     |%c| |%c| |%c| \n",
        (buttons & 0b000000000010) ? 219 : 255,
        (buttons & 0b000000001000) ? 219 : 255,
        (buttons & 0b000001000000) ? 219 : 255,
        (buttons & 0b000100000000) ? 219 : 255,
        (buttons & 0b010000000000) ? 219 : 255
    );
    printf("  _   _   _   _   _   _   _ \n");
    printf(" |%c| |%c| |%c| |%c| |%c| |%c| |%c|\n\n",
        (buttons & 0b000000000001) ? 219 : 255,
        (buttons & 0b000000000100) ? 219 : 255,
        (buttons & 0b000000010000) ? 219 : 255,
        (buttons & 0b000000100000) ? 219 : 255,
        (buttons & 0b000010000000) ? 219 : 255,
        (buttons & 0b001000000000) ? 219 : 255,
        (buttons & 0b100000000000) ? 219 : 255
    );
}

int main(){
    stdio_init_all();
//setup IO
    gpio_init(LED_1);
    gpio_set_dir(LED_1, GPIO_OUT);
    gpio_init(LED_2);
    gpio_set_dir(LED_2, GPIO_OUT);
    gpio_init(LED_3);
    gpio_set_dir(LED_3, GPIO_OUT);
    gpio_init(LED_4);
    gpio_set_dir(LED_4, GPIO_OUT);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    touch_init();
    encoder_init();
    pam8302_init();
    audio_pwm_init();

    printf("\nPico Audio Lab hardware test\n");
    print_console_help();
    printf("> ");

    while (true){
        encoder_update();
        report_raw_encoder_changes();
        report_encoder_events();
        update_console();

        const uint16_t touch_state = touch_get_state();
        gpio_put (LED_1, touch_state&1);
        gpio_put (LED_2, (touch_state>>1)&1);
        gpio_put (LED_3, (touch_state>>2)&1);
        gpio_put (LED_4, (touch_state>>3)&1);
        gpio_put (LED_PIN, touch_state ? 1 : 0);//Light if any key is touched
        sleep_ms(1);
    }
    return 0;
}
