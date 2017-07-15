#ifndef _T3PWM
#define _T3PWM

#include <stdint.h>

// Initialize Timer 3 for variable resolution "Fast PWM" mode
void T3PWMInit(void);

// Set PWM period for Timer 3
void SetPWM3Period(uint16_t period);

// Set PWM pulse width for Timer 3 channel A - OCR3A
void SetPWM3A(uint16_t width);

// Set PWM pulse width for Timer 3 channel B - OCR3B
void SetPWM3B(uint16_t width);

// Set PWM pulse width for Timer 3 channel C - OCR3C
void SetPWM3C(uint16_t width);

#endif
