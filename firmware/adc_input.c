#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "tusb.h"
#include "adc_input.h"
#include "adc_correction.h"

static uint16_t adc_buf0[ADC_BUF_SAMPLES];
static uint16_t adc_buf1[ADC_BUF_SAMPLES];
static int16_t adc_converted[ADC_BUF_SAMPLES];
static int adc_dma_chan0, adc_dma_chan1;

#define ADC_OUT_SAMPLES (ADC_BUF_SAMPLES / ADC_OVERSAMPLE)

static void adc_dma_irq(void) {
    if (dma_hw->ints0 & (1u << adc_dma_chan0)) {
        dma_hw->ints0 = 1u << adc_dma_chan0;
        int o = 0;
        for (int i = 0; i < ADC_BUF_SAMPLES; i += ADC_OVERSAMPLE) {
            uint32_t sum = 0;
            for (int j = 0; j < ADC_OVERSAMPLE; j++)
                sum += adc_buf0[i + j];
            uint16_t avg = (uint16_t)(sum / ADC_OVERSAMPLE);
            adc_converted[o++] = (int16_t)(((int)avg - 2048) * 16);
        }
        tud_audio_write((uint8_t *)adc_converted, ADC_OUT_SAMPLES * 2);
        dma_channel_set_write_addr(adc_dma_chan0, adc_buf0, false);
        dma_channel_set_trans_count(adc_dma_chan0, ADC_BUF_SAMPLES, false);
    }
    if (dma_hw->ints0 & (1u << adc_dma_chan1)) {
        dma_hw->ints0 = 1u << adc_dma_chan1;
        int o = 0;
        for (int i = 0; i < ADC_BUF_SAMPLES; i += ADC_OVERSAMPLE) {
            uint32_t sum = 0;
            for (int j = 0; j < ADC_OVERSAMPLE; j++)
                sum += adc_buf1[i + j];
            uint16_t avg = (uint16_t)(sum / ADC_OVERSAMPLE);
            adc_converted[o++] = (int16_t)(((int)avg - 2048) * 16);
        }
        tud_audio_write((uint8_t *)adc_converted, ADC_OUT_SAMPLES * 2);
        dma_channel_set_write_addr(adc_dma_chan1, adc_buf1, false);
        dma_channel_set_trans_count(adc_dma_chan1, ADC_BUF_SAMPLES, false);
    }
}

void adc_input_init(void) {
    adc_init();
    sleep_us(10);
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_INPUT);
    adc_fifo_setup(true, true, 1, false, false);

    float div = (float)clock_get_hz(clk_adc) / (float)(ADC_SAMPLE_RATE * ADC_OVERSAMPLE) - 1.0f;
    adc_set_clkdiv(div);

    adc_dma_chan0 = dma_claim_unused_channel(true);
    adc_dma_chan1 = dma_claim_unused_channel(true);

    dma_channel_config dc0 = dma_channel_get_default_config(adc_dma_chan0);
    channel_config_set_transfer_data_size(&dc0, DMA_SIZE_16);
    channel_config_set_read_increment(&dc0, false);
    channel_config_set_write_increment(&dc0, true);
    channel_config_set_dreq(&dc0, DREQ_ADC);
    channel_config_set_chain_to(&dc0, adc_dma_chan1);
    dma_channel_configure(adc_dma_chan0, &dc0, adc_buf0, &adc_hw->fifo, ADC_BUF_SAMPLES, false);

    dma_channel_config dc1 = dma_channel_get_default_config(adc_dma_chan1);
    channel_config_set_transfer_data_size(&dc1, DMA_SIZE_16);
    channel_config_set_read_increment(&dc1, false);
    channel_config_set_write_increment(&dc1, true);
    channel_config_set_dreq(&dc1, DREQ_ADC);
    channel_config_set_chain_to(&dc1, adc_dma_chan0);
    dma_channel_configure(adc_dma_chan1, &dc1, adc_buf1, &adc_hw->fifo, ADC_BUF_SAMPLES, false);

    dma_channel_set_irq0_enabled(adc_dma_chan0, true);
    dma_channel_set_irq0_enabled(adc_dma_chan1, true);
    irq_set_exclusive_handler(DMA_IRQ_0, adc_dma_irq);
    irq_set_enabled(DMA_IRQ_0, true);

    adc_run(true);
    dma_channel_start(adc_dma_chan0);
}

const int16_t *adc_input_read(void) {
    return adc_converted;
}
