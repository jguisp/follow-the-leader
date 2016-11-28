#define F_CPU 16000000UL 
#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "uart/uart.h"

unsigned int tempo;
volatile char note;
volatile unsigned char playing = 0;
volatile int duration=0;

void play_note(int frequency_note) {
    note = frequency_note;
    TIMSK1 |= (1 << OCIE1A);
    sei();
}

ISR(TIMER2_COMPA_vect) {
    PORTB ^= 1;
}

ISR(TIMER1_COMPA_vect)
{

    if(duration>0) {
        duration--;
        if (duration == 0) {
            playing = 0;
            DDRB &=~(1);
            TIMSK1 &= ~(1 << OCIE1A);
        }
    } else {
        if (note>0) {
            duration = tempo;
            OCR2A = note;
            DDRB |= 1;
            playing = 1;
        }
    }
}

      
void init_notes(void)
{
    DDRB |= 1;				//set data direction reg bit 0 to output
    PORTB &=~(1);			//set buzzer output low

    TCCR1A = 0;
    TCCR1B |= (1<<WGM01) | (1<<CS01) | (1<<CS00) ;
    OCR1A = 125;
    TCNT1=0;
    TIMSK1 |= (1 << OCIE1A);

    TCCR2A= (1<<WGM21);
    TCCR2B=(1<<CS22);
    TCNT2=0;
    OCR2A=255;
    OCR2B=255;
    TIMSK2= (1<<OCIE2A);

    DDRB &= ~(1);
    tempo = 2;
    sei();
}
