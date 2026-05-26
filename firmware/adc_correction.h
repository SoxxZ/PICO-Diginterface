#ifndef ADC_CORRECTION_H_
#define ADC_CORRECTION_H_

#include <stdint.h>

static inline uint16_t adc_correct(uint16_t d) {
    if (d >= 4081) return 4095;
    else if (d >= 3711) return d + 14;
    else if (d >= 3584) return d + 15;
    else if (d == 3583) return 3593;
    else if (d >= 3455) return d + 5;
    else if (d >= 2943) return d + 6;
    else if (d >= 2560) return d + 7;
    else if (d == 2559) return 2562;
    else if (d >= 2431) return d - 2;
    else if (d >= 2048) return d - 1;
    else if (d == 2047) return 2047;
    else if (d >= 1664) return d + 1;
    else if (d >= 1536) return d + 2;
    else if (d == 1535) return 1532;
    else if (d >= 1407) return d - 7;
    else if (d >= 895)  return d - 6;
    else if (d >= 512)  return d - 5;
    else if (d == 511)  return 501;
    else if (d >= 16)   return d - 15;
    else return 0;
}

#endif
