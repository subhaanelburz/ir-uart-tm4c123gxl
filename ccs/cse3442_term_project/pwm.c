#include <stdint.h>
#include "pwm.h"
#include "tm4c123gh6pm.h"
#include "wait.h"

#define PB6 0x40 // 0100.0000 = bit 6 is ON

// GPIO Port B is where the PWM signal is coming from
void initPWM()
{
    // initialize clocks for GPIO port B and PWM module 0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; // R1 is port B
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0; // R0 is PWM module 0

    _delay_cycles(3);  // delay after setting clocks

    // pin PB6 will be used in alternate function mode for PWM module 0
    // table on page 658 explains that PWM must set the AFSEL high, ODR low, DEN high

    GPIO_PORTB_AFSEL_R |= PB6; // set alternate function select
    GPIO_PORTB_ODR_R &= ~PB6; // clear open drain select, idk why but data-sheet says to
    GPIO_PORTB_DEN_R |= PB6; // set digital functions

    GPIO_PORTB_AMSEL_R &= ~PB6; // unsure why he clears the analog mode select, idk if necessary

    // next we must configure the PCTL register properly, as table on page 1351
    // PB6 maps to M0PWM0

    GPIO_PORTB_PCTL_R &= ~GPIO_PCTL_PB6_M; // first we clear everything for PB6
    GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB6_M0PWM0; // then we set PB6 to map to M0PWM0

    // next we have to clear the PWM clock
    SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M; // value is E = 1110, ~E = [000]1, which sets the PWM divisor to default /2

    // we enable the PWM clock and make it Sys-clock / 4 = 40 MHz / 4 = 10 MHz clock for PWM
    SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_4;

    // before modifying the PWM0 values, we turn it off
    PWM0_CTL_R = 0;

    // we will make it so that when we load the starting counter value,
    // it will start a new cycle which make it go high, and then
    // the state will go low once the counter hits the CMPA value
    PWM0_0_GENA_R = PWM_0_GENA_ACTLOAD_M | PWM_0_GENA_ACTCMPAD_ZERO; // determines when signal go high/low

    // Now we will put the reload value for a 38 KHz output signal
    // Calculated as follows: PWM Clock Source / desired clock
    // 10,000,000 / 38,000 = 263.16
    PWM0_0_LOAD_R = 263 - 1;

    PWM0_0_CMPA_R = 131; // default duty cycle set to 50% so its equally on/off
                         // this is calculated by doing the load * (1 - desired_duty_cycle)
                         // so 263 * ( 1 - 0.5 ) = 131.5, so it will turn off once counter counts down to 131.5

    // after modifying the values we update PWM0 to be synced
    PWM0_CTL_R |= PWM_CTL_GLOBALSYNC0;

    PWM0_ENABLE_R |= PWM_ENABLE_PWM0EN; // enable the M0PWM0 output signal to pin PB6
    PWM0_0_CTL_R |= PWM_0_CTL_ENABLE; // enable PWM module 0, generator 0

    PWM0_ENABLE_R |= PWM_ENABLE_PWM0EN; // enable the M0PWM0 output signal to pin PB6
}
