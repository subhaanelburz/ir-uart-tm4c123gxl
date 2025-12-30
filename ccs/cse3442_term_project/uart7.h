// Copied format of uart0.h

#ifndef UART7_H_
#define UART7_H_

#include <stdint.h>
#include <stdbool.h>

// Subroutines
void initUart7();
void setUart7BaudRate(uint32_t baudRate, uint32_t fcyc);
void putcUart7(char c);
void putsUart7(char* str);
char getcUart7();
bool kbhitUart7();

#endif
