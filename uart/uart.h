#ifndef _UART_H_
#define _UART_H_

#include <stdio.h>

int uart_putchar(char c, FILE *stream);
char uart_getchar(FILE *stream);
void uart_init(void);

#endif /* _UART_H_ */