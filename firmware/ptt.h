#ifndef PTT_H_
#define PTT_H_

#include <stdint.h>
#include <stdbool.h>

#define PTT1_PIN    22
#define PTT2_PIN    24

#define PTT_THRESHOLD_DEFAULT   16
#define PTT_HOLD_US_DEFAULT     20000

#define PTT_STATUS_LED_PIN      12

void ptt_init(void);
void ptt_process_sample(int16_t sample, uint32_t now_us);
void ptt_set_threshold(uint16_t threshold);
void ptt_set_hold_us(uint32_t hold_us);

#endif
