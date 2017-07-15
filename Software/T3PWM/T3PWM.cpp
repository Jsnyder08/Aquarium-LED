#include "Arduino.h"
#include "T3PWM.h"

//------------------------------------------------------------------------------

void T3PWMInit(void)
{
    uint8_t sreg = SREG;

    // Power up timer 3

    cli();
    PRR0 &= ~_BV(PRTIM3);    // Clear bit to enable

    // Ensure that all Timer 3 interrupts are DISABLED

    TIMSK3 = 0;     // Disable all timer 3 interrupts
    TCCR3B = 0;     // Stop counter until init done

    // When timer is set up for WGM mode 14 (Fast PWM, ICR sets PWM period),
    // the input capture register sets the terminal count for the timer.
    // Set it to 16-bit MAXINT for full 16-bit resolution.

    ICR3 = 0xFFFE;

    // Set compare values (PWM period) to minimum for now

    OCR3A = 0;
    OCR3B = 0;
    OCR3C = 0;

    // Configure timer 3 output mode - Timer 3 control register A (TCCR3A)
    // Reset counter and clear interrupt flags

    TCNT3 = 0;      // Reset counter
    TIFR3 = 0xFF;   // Clears all interrupt flags (writing 1's clears them)

    // COM3n1,COM3n0 = 1,0 to force output pin low on compare match.
    //   Set high when timer hits max count.
    // Set COM3n1,COM3n0 = 1,1 to set output HIGH on compare match and LOW
    //   at max count.
    // WGM3n1,WGM3n0 = 1,0 - these are the two low bits of the 4-bit timer mode.
    // Timer will use mode 14 (1110) to select Fast PWM using ICR3 to specify
    //   max count. 

    // "Blow" mode (high = ON): Output low on compare match
    TCCR3A = _BV(COM3A1) | _BV(COM3B1) | _BV(COM3C1) | _BV(WGM31);
    // "Suck" mode (low = ON): Output high on compare match (use with FETs)
//  TCCR1A = _BV(COM3A1) | _BV(COM3A0) | _BV(COM3B1) | _BV(COM3B0) | _BV(COM3C1) | _BV(COM3C0) | _BV(WGM31);

    // Configure timer 3 counting mode - Timer 3 control register B (TCCR3B)
    // Waveform generation mode 14 (1110), Fast PWM with ICR3 setting max count
    //   WGM33,WGM32 = 11
    // Select maximum rate clock (clkio/1), CS32,CS31,CS30 = 001.
    //   Counter will start running when this register is set.
    // Input capture not used, ICNC3 = 0 and ICES3 = 0 

    TCCR3B = _BV(WGM33) | _BV(WGM32) | _BV(CS30);

    // Timer 3 control register C (TCCR3C) is used to force output compares
    // and is not used in PWM modes.

//  TCCR3C = 0;

    // Configure timer 3 compare pins as outputs

    PORTC &= ~0x70;	// "Blow" mode (high = ON): PC4,5,6 low
//  PORTC |= 0x70;  // "Suck" mode (low = ON):  PC4,5,6 high
    DDRC |= 0x70;   // PC6/OC3A, PC5/OC3B, PC4/OC3C

    SREG = sreg;
}

//-----------------------------------------------------------------------------
// Set Timer 3 PWM pulse period

void SetPWM3Period(uint16_t period)
{
    uint8_t sreg = SREG;

    cli();
    ICR3 = period;
    SREG = sreg;
}

//-----------------------------------------------------------------------------
// Set PWM pulse width for Timer 3 channel A

void SetPWM3A(uint16_t width)
{
    uint8_t sreg = SREG;

    cli();
    OCR3A = width;
    SREG = sreg;
}
    
//-----------------------------------------------------------------------------
// Set PWM pulse width for Timer 3 channel B

void SetPWM3B(uint16_t width)
{
    uint8_t sreg = SREG;

    cli();
    OCR3B = width;
    SREG = sreg;
}
    
//-----------------------------------------------------------------------------
// Set PWM pulse width for Timer 3 channel C

void SetPWM3C(uint16_t width)
{
    uint8_t sreg = SREG;

    cli();
    OCR3C = width;
    SREG = sreg;
}