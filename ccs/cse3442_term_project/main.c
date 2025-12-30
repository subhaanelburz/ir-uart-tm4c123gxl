#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "wait.h"
#include "uart0.h"
#include "uart7.h"
#include "uart7_interrupt.h"
#include "common_terminal_interface.h"
#include "strings.h"
#include "pwm.h"

// #define DEBUG

// bit banded alias for on-board blue LED
#define BLUE_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC - 0x40000000)*32 + 2*4))) //PF2
#define BLUE_LED_MASK 0x04 // 0000.0100 = bit 2 for PF2

/*
 * U0Rx: PA0
 * U0Tx: PA1
 *
 * U7Rx: PE0
 * U7Tx: PE1
 *
 * M0PWM0: PB6
*/

volatile uint32_t LED_off_timer = 0;

void SysTick_Handler(void)
{
    if (LED_off_timer > 0)
    {
        LED_off_timer--; // count down 1 ms
    }
    if (!LED_off_timer) // if the timer has counted down to 0
    {
        BLUE_LED = 0; // turn the blue led off after 500 ms
    }
}

void Uart7_Rx_Handler(void)
{
    BLUE_LED = 1;
    LED_off_timer = 500; // keep LED on for 500 ms

    // first we clear the interrupt since we are in the handler now
    UART7_ICR_R = (UART_ICR_RXIC | UART_ICR_RTIC);

    char temp_char;
    static bool start = true; // flag variable to determine if start of message

    while ( !(UART7_FR_R & UART_FR_RXFE) ) // loop when the FIFO is not empty
    {
        if (start) // determine if it is the start of a message to print intro to it
        {
            putsUart0("\r\nUART7 RX (IR) Message: ");
            start = false;
        }

        temp_char = getcUart7(); // read and store each character from the terminal
        putcUart0(temp_char);    // then print out the character into the terminal

        if ( !(temp_char) ) // make a new line if it is the last character which is 0
        {
            putsUart0("\r\n");
            start = true;
        }
    }
}

int main(void)
{
    initSystemClockTo40Mhz();

    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5; // enable clocks for GPIO Port F
    _delay_cycles(3);

    GPIO_PORTF_DIR_R |= BLUE_LED_MASK;  // make the blue LED an output
    GPIO_PORTF_DEN_R |= BLUE_LED_MASK;   // enable digital functions for blue LED

    NVIC_ST_RELOAD_R = 3999; // Set RELOAD for 1 ms

    NVIC_ST_CURRENT_R = 0x0; // Clear Current

    NVIC_ST_CTRL_R &= ~NVIC_ST_CTRL_CLK_SRC; // Set Control to a 4 MHZ Clock (16 MHz PIOSC / 4)
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE | NVIC_ST_CTRL_INTEN; // enable counter and interrupt

    // Initialize UART0, the clocks, all other registers, and set it to 115200 baud, 8N1
    initUart0();

    // Initialize UART7, the clocks, all other registers, and set it to 300 baud, 8E1
    initUart7();

    // Now call this function to enable interrupts whenever we receive something
    init_uart7_rx_interrupt();

    // Initialize PWM signal to 38 KHz on PB6
    initPWM();

    // create variable of struct USER_DATA, you can see it in common_terminal_interface.h
    USER_DATA input;

    // create variable which will store the original message
    char og_msg[MAX_CHARS + 1];

    putsUart0("UART7 (IR) baud rate set to 1200 \r\n");
    putsUart0("Command: baud <rate> \r\n");
    putsUart0("<rate> = 300, 1200, 2400, 4800 \r\n\r\n");

    putsUart0("Command: send <message> \r\n");
    putsUart0("<message> limited to 64 characters \r\n\r\n");

    while(1)
    {
        // PC UART transmits terminal input to the receiving FIFO of the UART0 on TM4C board
        // so UART0 "gets" the string from its receiving FIFO and stores it into the input data
        getsUart0(&input);

        // before parsing, I save the original message so entire thing can be sent after
        str_cpy(og_msg, input.buffer);

        // After we get the input string, we need to parse / tokenize it into subfields
        parseFields(&input);

        bool valid = false;

        if (isCommand(&input, "send", 1))
        {
            // here we update the message to include only the message
            // that will be sent using the IR transmitter
            char *IR_msg = &og_msg[input.fieldPosition[1]];

#ifdef DEBUG
        putsUart0(IR_msg);
        putsUart0("\r\n");
#endif

            uint32_t length = str_len(IR_msg);

            // limit messages to 64 bytes
            if (length <= 64)
            {
                putsUart7(IR_msg);
                putcUart7(0);
                valid = true;
            }
        }

        if ( isCommand(&input, "baud", 1) )
        {
            uint32_t baud = getFieldInteger(&input, 1);
            char* baud_str = getFieldString(&input, 1);

            if ( (baud == 300) || (baud == 1200) || (baud == 2400) || (baud == 4800) )
            {
                setUart7BaudRate(baud, 40000000);
                putsUart0("\r\nUART7 (IR) baud rate set to ");
                putsUart0(baud_str);
                putsUart0("\r\n");
                valid = true;
            }
        }

        if(!valid)
        {
            putsUart0("\r\nInvalid command\r\n");
        }
    }

    // return 0;
}
