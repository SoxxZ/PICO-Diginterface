#ifndef AUDIO_PDM_OUTPUT_H_
#define AUDIO_PDM_OUTPUT_H_

#include <stdint.h>
#include <stdbool.h>

#define PDM_OUTPUT_GPIO 18

int audio_pdm_output_init(uint32_t sample_rate);
void audio_pdm_output_set_sample_rate(uint32_t sample_rate);
void audio_pdm_output_write(int16_t sample);
bool audio_pdm_output_is_ready(void);
void audio_pdm_output_set_mute(bool mute);
void audio_pdm_output_set_volume(uint16_t vol);
void audio_pdm_output_set_tx_boost(bool boost);

#endif
