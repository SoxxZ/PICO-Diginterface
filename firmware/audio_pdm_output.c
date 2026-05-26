#include "audio_pdm_output.h"
#include "pdm_output.pio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

static PIO pdm_pio = pio1;
static uint pdm_sm;

static volatile bool muted = false;
static volatile bool tx_boost = false;
static volatile uint16_t volume = 65535;

static int32_t buff[2];

static uint32_t sdm_o2_os32(int16_t sig) {
    uint32_t out = 0;
    int32_t d = -32767 - sig;
    for (int j = 0; j < 32; j++) {
        int32_t etmp = d + 2 * buff[0] - buff[1];
        buff[1] = buff[0];
        buff[0] = etmp;
        if (etmp < 0) {
            buff[0] += 65534;
            out |= (1u << j);
        }
    }
    return out;
}

int audio_pdm_output_init(uint32_t sample_rate) {
    (void)sample_rate;

    pdm_sm = pio_claim_unused_sm(pdm_pio, true);
    uint offset = pio_add_program(pdm_pio, &pdm_output_program);

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset, offset);
    sm_config_set_out_pins(&c, PDM_OUTPUT_GPIO, 1);
    sm_config_set_out_shift(&c, true, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, clock_get_hz(clk_sys) / (48000.0f * 32.0f));

    pio_gpio_init(pdm_pio, PDM_OUTPUT_GPIO);
    pio_sm_set_consecutive_pindirs(pdm_pio, pdm_sm, PDM_OUTPUT_GPIO, 1, true);
    pio_sm_init(pdm_pio, pdm_sm, offset, &c);
    pio_sm_set_enabled(pdm_pio, pdm_sm, true);

    buff[0] = buff[1] = 0;
    muted = false;
    tx_boost = false;
    volume = 65535;

    return 0;
}

void audio_pdm_output_set_sample_rate(uint32_t sr) { (void)sr; }

void audio_pdm_output_write(int16_t sample) {
    if (muted) sample = 0;
    sample = (int16_t)(((int32_t)sample * volume) >> 16);
    if (tx_boost) {
        int32_t boosted = (int32_t)sample * 2;
        if (boosted > 32767) boosted = 32767;
        if (boosted < -32768) boosted = -32768;
        sample = (int16_t)boosted;
    }

    uint32_t pdm_word = sdm_o2_os32(sample);
    while (pio_sm_is_tx_fifo_full(pdm_pio, pdm_sm)) {}
    pdm_pio->txf[pdm_sm] = pdm_word;
}

bool audio_pdm_output_is_ready(void) {
    return !pio_sm_is_tx_fifo_full(pdm_pio, pdm_sm);
}

void audio_pdm_output_set_mute(bool m) {
    muted = m;
}

void audio_pdm_output_set_volume(uint16_t vol) {
    volume = vol;
}

void audio_pdm_output_set_tx_boost(bool boost) {
    tx_boost = boost;
}
