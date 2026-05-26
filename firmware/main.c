#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "audio_pdm_output.h"
#include "adc_input.h"
#include "ptt.h"

static const uint32_t sample_rates[] = {48000};
static uint32_t current_sample_rate = 48000;

enum {
    VOLUME_CTRL_0_DB = 0,
    VOLUME_CTRL_50_DB = 12800,
};

static uint32_t blink_interval_ms = 250;

static int8_t mute[2];
static int16_t volume[2];

static int16_t spk_buf[48];
static volatile int spk_data_size = 0;

static void audio_task(void);
static void led_blinking_task(void);
static void cdc_task(void);

int main(void) {
    board_init();
    stdio_init_all();

    audio_pdm_output_init(48000);
    adc_input_init();
    ptt_init();

    uart_init(uart1, 4800);
    gpio_set_function(4, GPIO_FUNC_UART);
    gpio_set_function(5, GPIO_FUNC_UART);
    gpio_set_outover(4, GPIO_OVERRIDE_INVERT);
    gpio_set_inover(5, GPIO_OVERRIDE_INVERT);
    gpio_pull_up(5);
    uart_set_format(uart1, 8, 2, UART_PARITY_NONE);

    tusb_init();

    while (1) {
        tud_task();
        cdc_task();
        audio_task();
        led_blinking_task();
    }
}

void tud_mount_cb(void) {
    blink_interval_ms = 1000;
}

void tud_umount_cb(void) {
    blink_interval_ms = 250;
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    blink_interval_ms = 2500;
}

void tud_resume_cb(void) {
    blink_interval_ms = tud_mounted() ? 1000 : 250;
}

static bool tud_audio_clock_get_request(uint8_t rhport, audio_control_request_t const *request) {
    if (request->bEntityID != UAC2_ENTITY_CLOCK) return false;

    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ) {
        if (request->bRequest == AUDIO_CS_REQ_CUR) {
            audio_control_cur_4_t curf = { (int32_t)tu_htole32(current_sample_rate) };
            return tud_audio_buffer_and_schedule_control_xfer(rhport,
                (tusb_control_request_t const *)request, &curf, sizeof(curf));
        } else if (request->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_4_n_t(1) rangef = {
                .wNumSubRanges = tu_htole16(1),
                .subrange[0] = { .bMin = 48000, .bMax = 48000, .bRes = 0 }
            };
            return tud_audio_buffer_and_schedule_control_xfer(rhport,
                (tusb_control_request_t const *)request, &rangef, sizeof(rangef));
        }
    } else if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID &&
               request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_1_t cur_valid = { .bCur = 1 };
        return tud_audio_buffer_and_schedule_control_xfer(rhport,
            (tusb_control_request_t const *)request, &cur_valid, sizeof(cur_valid));
    }
    return false;
}

static bool tud_audio_clock_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf) {
    (void)rhport;
    if (request->bEntityID != UAC2_ENTITY_CLOCK) return false;
    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ &&
        request->bRequest == AUDIO_CS_REQ_CUR) {
        current_sample_rate = (uint32_t)((audio_control_cur_4_t const *)buf)->bCur;
        return true;
    }
    return false;
}

static bool tud_audio_feature_unit_get_request(uint8_t rhport, audio_control_request_t const *request) {
    if (request->bEntityID != UAC2_ENTITY_SPK_FEATURE_UNIT) return false;

    if (request->bControlSelector == AUDIO_FU_CTRL_MUTE && request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_1_t mute1 = { .bCur = mute[request->bChannelNumber] };
        return tud_audio_buffer_and_schedule_control_xfer(rhport,
            (tusb_control_request_t const *)request, &mute1, sizeof(mute1));
    } else if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME) {
        if (request->bRequest == AUDIO_CS_REQ_CUR) {
            audio_control_cur_2_t cur_vol = { .bCur = tu_htole16(volume[request->bChannelNumber]) };
            return tud_audio_buffer_and_schedule_control_xfer(rhport,
                (tusb_control_request_t const *)request, &cur_vol, sizeof(cur_vol));
        } else if (request->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_2_n_t(1) range_vol = {
                .wNumSubRanges = tu_htole16(1),
                .subrange[0] = { .bMin = tu_htole16(-VOLUME_CTRL_50_DB),
                                 .bMax = tu_htole16(VOLUME_CTRL_0_DB),
                                 .bRes = tu_htole16(256) }
            };
            return tud_audio_buffer_and_schedule_control_xfer(rhport,
                (tusb_control_request_t const *)request, &range_vol, sizeof(range_vol));
        }
    }
    return false;
}

static bool tud_audio_feature_unit_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf) {
    (void)rhport;
    if (request->bEntityID != UAC2_ENTITY_SPK_FEATURE_UNIT) return false;
    if (request->bRequest != AUDIO_CS_REQ_CUR) return false;

    if (request->bControlSelector == AUDIO_FU_CTRL_MUTE) {
        mute[request->bChannelNumber] = ((audio_control_cur_1_t const *)buf)->bCur;
        return true;
    } else if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME) {
        volume[request->bChannelNumber] = ((audio_control_cur_2_t const *)buf)->bCur;
        return true;
    }
    return false;
}

bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    audio_control_request_t const *request = (audio_control_request_t const *)p_request;

    if (request->bEntityID == UAC2_ENTITY_CLOCK)
        return tud_audio_clock_get_request(rhport, request);
    if (request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT)
        return tud_audio_feature_unit_get_request(rhport, request);
    return false;
}

bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf) {
    audio_control_request_t const *request = (audio_control_request_t const *)p_request;

    if (request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT)
        return tud_audio_feature_unit_set_request(rhport, request, buf);
    if (request->bEntityID == UAC2_ENTITY_CLOCK)
        return tud_audio_clock_set_request(rhport, request, buf);
    return false;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;
    uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    if (ITF_NUM_AUDIO_STREAMING_SPK == itf)
        blink_interval_ms = 1000;
    return true;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;
    uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

    if (ITF_NUM_AUDIO_STREAMING_SPK == itf && alt != 0)
        blink_interval_ms = 0;

    spk_data_size = 0;
    return true;
}

bool tud_audio_rx_done_pre_read_cb(uint8_t rhport, uint16_t n_bytes_received,
                                   uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting) {
    (void)rhport;
    (void)func_id;
    (void)ep_out;
    (void)cur_alt_setting;

    spk_data_size = tud_audio_read(spk_buf, n_bytes_received);
    return true;
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t func_id,
                                   uint8_t ep_in, uint8_t cur_alt_setting) {
    (void)rhport;
    (void)func_id;
    (void)ep_in;
    (void)cur_alt_setting;
    return true;
}

void audio_task(void) {
    if (spk_data_size) {
        int16_t *src = spk_buf;
        int count = spk_data_size / 2;
        uint32_t now = time_us_32();
        for (int i = 0; i < count; i++) {
            audio_pdm_output_write(src[i]);
            ptt_process_sample(src[i], now);
        }
        spk_data_size = 0;
    }
    while (audio_pdm_output_is_ready())
        audio_pdm_output_write(0);
}

static void cdc_task(void) {
    if (tud_cdc_connected()) {
        uint8_t buf[64];
        uint32_t count = tud_cdc_read(buf, sizeof(buf));
        if (count > 0) {
            uart_write_blocking(uart1, buf, count);
        }

        count = 0;
        while (uart_is_readable(uart1) && count < sizeof(buf)) {
            buf[count++] = uart_getc(uart1);
        }
        if (count > 0) {
            tud_cdc_write(buf, count);
            tud_cdc_write_flush();
        }
    }
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding) {
    (void)itf;
    (void)p_line_coding;
}

void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms) {
    (void)itf;
    (void)duration_ms;
}

void led_blinking_task(void) {
    if (blink_interval_ms == 0) {
        board_led_write(true);
        return;
    }
    static uint32_t start_ms = 0;
    static bool led_state = false;

    if (board_millis() - start_ms < blink_interval_ms) return;
    start_ms += blink_interval_ms;

    board_led_write(led_state);
    led_state = !led_state;
}
