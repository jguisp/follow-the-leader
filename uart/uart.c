#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "circular_buffer/circular_buffer.h"
#include "uart.h"


///////////////////////////////
// defines
///////////////////////////////
#define USART_BAUDRATE 9600 // Defines the baud rate
#define F_CPU 16000000      // Defines CPU frequency
#define BAUD_PRESCALE (((F_CPU/(USART_BAUDRATE*16UL)))-1) // Calculate the prescale

#define BUF_SIZE 32

///////////////////////////////
// variables
///////////////////////////////
FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input  = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

circular_buffer_t* buffer = NULL;


///////////////////////////////
// functions
///////////////////////////////
void uart_init(void) {
	UCSR0B |= (1<<RXEN0) | (1 << TXEN0);					// enable transmitter
	UCSR0C |= (1<<UCSZ00) | (1<<UCSZ01);	// user 8-bit char size
	UBRR0H = (BAUD_PRESCALE >> 8);       	// set the Baud Rate Register High
	UBRR0L = BAUD_PRESCALE;              	// set the Baud Rate Register Low
	UCSR0B |= (1<<RXEN0)|(1<<RXCIE0);    // Enable reception and RC complete interrupt

	stdout = &uart_output;
	stdin = &uart_input;
	
	circular_buffer_init(&buffer, BUF_SIZE);
}

void wait_tx_ready() {
	while((UCSR0A & (1<<UDRE0)) == 0 ) {} // wait until the port is ready to be written to
}

int uart_putchar(char c, FILE *stream) {
	wait_tx_ready();
	UDR0 = c;

	return 0;
}

char uart_getchar(FILE *stream) {
	volatile char ch;
	while ( (circular_buffer_pop(buffer, &ch)) == -1) {}

	return ch;
}

// Interruption function
ISR(USART_RX_vect) {
    circular_buffer_push(buffer, UDR0);
}
