#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "ptt.h"

typedef struct {
    uint8_t pin;
    uint16_t threshold;
    uint32_t hold_us;
    uint32_t last_activity_us;
    bool asserted;
} ptt_channel_t;

static ptt_channel_t ptt_channels[] = {
    { PTT1_PIN, PTT_THRESHOLD_DEFAULT, PTT_HOLD_US_DEFAULT, 0, false },
    { PTT2_PIN, PTT_THRESHOLD_DEFAULT, PTT_HOLD_US_DEFAULT, 0, false },
};

static const uint8_t ptt_count = sizeof(ptt_channels) / sizeof(ptt_channels[0]);
static uint8_t ptt_asserted_count = 0;

static void ptt_assert(ptt_channel_t *ch) {
    if (!ch->asserted) {
        ch->asserted = true;
        gpio_put(ch->pin, 1);
        if (ptt_asserted_count++ == 0) {
            gpio_put(PTT_STATUS_LED_PIN, 1);
        }
    }
}

static void ptt_deassert(ptt_channel_t *ch) {
    if (ch->asserted) {
        ch->asserted = false;
        gpio_put(ch->pin, 0);
        if (--ptt_asserted_count == 0) {
            gpio_put(PTT_STATUS_LED_PIN, 0);
        }
    }
}

void ptt_init(void) {
    for (uint8_t i = 0; i < ptt_count; i++) {
        gpio_init(ptt_channels[i].pin);
        gpio_set_dir(ptt_channels[i].pin, GPIO_OUT);
        gpio_put(ptt_channels[i].pin, 0);
    }
    gpio_init(PTT_STATUS_LED_PIN);
    gpio_set_dir(PTT_STATUS_LED_PIN, GPIO_OUT);
    gpio_put(PTT_STATUS_LED_PIN, 0);
}

void ptt_process_sample(int16_t sample, uint32_t now_us) {
    for (uint8_t i = 0; i < ptt_count; i++) {
        ptt_channel_t *ch = &ptt_channels[i];

        if (sample < 0) sample = -sample;
        if ((uint16_t)sample >= ch->threshold) {
            ch->last_activity_us = now_us;
            ptt_assert(ch);
        } else if (ch->asserted) {
            uint32_t elapsed = now_us - ch->last_activity_us;
            if (elapsed >= ch->hold_us) {
                ptt_deassert(ch);
            }
        }
    }
}

void ptt_set_threshold(uint16_t threshold) {
    for (uint8_t i = 0; i < ptt_count; i++) {
        ptt_channels[i].threshold = threshold;
    }
}

void ptt_set_hold_us(uint32_t hold_us) {
    for (uint8_t i = 0; i < ptt_count; i++) {
        ptt_channels[i].hold_us = hold_us;
    }
}
