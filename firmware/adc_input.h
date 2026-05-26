#ifndef ADC_INPUT_H_
#define ADC_INPUT_H_

#include <stdint.h>

#define ADC_PIN         26
#define ADC_INPUT       0

#define ADC_SAMPLE_RATE 48000
#define ADC_OVERSAMPLE 8
#define ADC_BUF_SAMPLES (ADC_SAMPLE_RATE / 1000 * ADC_OVERSAMPLE) /* 384 */

void adc_input_init(void);
const int16_t *adc_input_read(void);

#endif
