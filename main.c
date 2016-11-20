#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart/uart.h"

void repeat_char() {
    volatile char ch;

	scanf("%c", &ch);
	printf("char read: %c\n", ch);
}

int main(void) {
    uart_init();

    sei();

    for ( ; ; ) {
    	repeat_char();
	}

    return 0;
}