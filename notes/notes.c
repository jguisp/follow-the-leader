#define F_CPU 16000000UL 
#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "uart/uart.h"

#define c4  239
#define f4  179
#define fx4 169

volatile int millis = 0;
volatile char playing = 0;
volatile unsigned char note;
static unsigned int duration=0;

unsigned char play_tune,*tune_array;
unsigned int tempo;

// notes uses timer2 to control how much time the note will be played.
ISR(TIMER2_COMPA_vect)
{
    millis++;
    if (millis == 1) {
        duration = tempo * 8;       // duration of note
        OCR2A = note;               // set note
        DDRB |= 1;                  // turn on buzzer
        PORTB |= 1;
        playing = 1;
        // printf("timer2 play %d \n", note);
    } else if (millis == 160) {
        DDRB &=~(1);                // turn off buzzer
        TIMSK2 &= ~(1<<OCIE2A);
        playing = 0;
    }

}

int is_playing() {
   return playing;
}

void init_buzzer(void)
{
    DDRB |= 1;				//set data direction reg bit 0 to output
    PORTB &=~(1);			//set buzzer output low
}

void init_timer2() {
    TCCR2A = 0; //tmr0 Clear Timer on Compare Match
    TCCR2B = _BV(WGM01)|_BV(CS01)|_BV(CS00);  //CTC mode with CPU clock/64 = 125 kHz
    OCR2A = 125;  //set for 1 ms interrupt
    TCNT2=0;
    millis = 0;
    TIMSK2 = (1<<OCIE2A);
    sei();
}

void play_note(int note_frequency) {
    note = note_frequency;
    duration = 0;
    init_timer2();
}

void init_notes() {
    init_buzzer();
    tempo=(210>>1);
    // sei();
}
