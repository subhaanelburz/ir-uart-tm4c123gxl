#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "uart7.h"
#include "uart7_interrupt.h"


/*
 *  this code will basically enable an interrupt on UART7
 *  whenever it receives any data
 */


void init_uart7_rx_interrupt()
{
    // first you have to turn off the UART
    UART7_CTL_R &= ~(UART_CTL_UARTEN);

    // next we have to set it up so that the interrupt
    // happens when we receive anything
    UART7_IFLS_R &= ~(UART_IFLS_RX_M);  // clear the register first
    UART7_IFLS_R |= UART_IFLS_RX1_8;    // make it so that it trigger interrupt when
                                        // the RX fifo is 1/8th full

    // here we clear all of the flags so no interrupt happen immediately
    UART7_ICR_R = (UART_ICR_RXIC | UART_ICR_RTIC);

    // enable the interrupts for BOTH receive and receive time out
    UART7_IM_R |= (UART_IM_RXIM | UART_IM_RTIM);

    // now we must enable the interrupt and set the priority for NVIC
    // page 105: UART7 = Interrupt 63
    // Interrupt 63 is in NVIC_EN1_R
    // And it is the last bit
    NVIC_EN1_R |= 0x80000000;

    // Interrupt 63 is in NVIC_PRI15_R
    // first we clear old priorities of it
    NVIC_PRI15_R &= ~(NVIC_PRI15_INTD_M);

    // next we set it to highest priority 0
    NVIC_PRI15_R |= (0 << NVIC_PRI15_INTD_S);

    // now that the interrupt is enabled, we can enable the UART again
    UART7_CTL_R |= UART_CTL_UARTEN;
}
