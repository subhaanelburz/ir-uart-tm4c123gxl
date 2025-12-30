#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart7.h"

/*
 *  Since we want to use UART7, we need to check which GPIO pins it corresponds to in the data sheet
 *  This is on page 1351, and we can see that:
 *  PE0 = U7Rx
 *  PE1 = U7Tx
 *  so we can define the pin-masks as follows:
 *  Receiving end is PE0 so we must have a mask for bit 0, which would be 1 = 0000.0001
 *  Transmitting end is PE1 so we must have a mask for bit 1, which would be 2 = 0000.0010
 */

#define UART_RX_MASK 1  // Receiving end PE0, so activate bit 0, which is 1
#define UART_TX_MASK 2  // Transmitting end PE1, so activate bit 1, which is 2

void initUart7()
{
    // First we need to enable the clocks for UART7 and also GPIO Port E
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R7;    // 0x80 = 1000.0000 , which enables UART 7
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;    // 0x10 = 0001.0000 , which enables GPIO port E
    _delay_cycles(3);

    GPIO_PORTE_DR2R_R |= UART_TX_MASK;   // it defaults to 2mA, but the uart0 one had it so I might as well have it
                                        // because you need to enable current for transmitting end

    GPIO_PORTE_DEN_R |= UART_TX_MASK | UART_RX_MASK;    // enable the digital functions for the UART7 pins

    // Basically makes the GPIO pins PE0 and PE1 be controlled
    // by an alternate hardware function, which is UART7
    GPIO_PORTE_AFSEL_R |= UART_TX_MASK | UART_RX_MASK;

    // We also need to configure this register so that the GPIO pins can be controlled by UART7
    GPIO_PORTE_PCTL_R &= ~(GPIO_PCTL_PE1_M | GPIO_PCTL_PE0_M);      // we first clear the register with the mask
    GPIO_PORTE_PCTL_R |= GPIO_PCTL_PE1_U7TX | GPIO_PCTL_PE0_U7RX;   // now enable PE1 and PE0 as U7Tx and U7Rx

    UART7_CTL_R = 0;    // Turn off UART7 so we can configure it properly

    UART7_CC_R = UART_CC_CS_SYSCLK; // use system clock (40 MHz)

    // Set the Baud Rate to 1200
    // The formula for IBRD is (system clock) / (16 * baud_rate) = 2083.33
    // Then for FBRD, we take the factional part of the result so like 2083.XX
    // Then take the decimal, do (0.XXXX) * 64 + 0.5 = 21.8333
    UART7_IBRD_R = 2083;
    UART7_FBRD_R = 21;

    // Now we will configure the UART line control register
    // which will allow us to make it into 8E1, like 8 bit word length, parity, stop bit, FIFOs, etc.
    UART7_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_PEN | UART_LCRH_EPS | UART_LCRH_FEN;

    // Enable the UART, transmitter, and receiver
    UART7_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
}

// Set baud rate as function of instruction cycle frequency
// The init function will initialize it to 115200 baud, but
// this function is if you ever want to change the baud rate later on
// to any other value
void setUart7BaudRate(uint32_t baudRate, uint32_t fcyc)
{
    uint32_t divisorTimes128 = (fcyc * 8) / baudRate;   // calculate divisor (r) in units of 1/128,
                                                        // where r = fcyc / 16 * baudRate
    divisorTimes128 += 1;                               // add 1/128 to allow rounding
    UART7_CTL_R = 0;                                    // turn-off UART7 to allow safe programming
    UART7_IBRD_R = divisorTimes128 >> 7;                // set integer value to floor(r)
    UART7_FBRD_R = ((divisorTimes128) >> 1) & 63;       // set fractional value to round(fract(r)*64)
    UART7_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_PEN | UART_LCRH_EPS | UART_LCRH_FEN; // set it to 8E1
    UART7_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;        // Enable UART/transmitter/receiver
}

// Blocking function that writes a serial character when the UART buffer is not full
void putcUart7(char c)
{
    while (UART7_FR_R & UART_FR_TXFF);               // wait if uart7 tx fifo full
    // Writing to the UART7 data register
    UART7_DR_R = c;                                  // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart7(char* str)
{
    uint8_t i = 0;
    while (str[i] != '\0')
        putcUart7(str[i++]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart7()
{
    while (UART7_FR_R & UART_FR_RXFE);               // wait if uart7 rx fifo empty
    return UART7_DR_R & 0xFF;                        // get character from fifo
}

// Returns the status of the receive buffer
bool kbhitUart7()
{
    // if fifo is empty evaluates to !(1) = 0
    // if something in RX fifo !(0) = 1
    return !(UART7_FR_R & UART_FR_RXFE);
}
