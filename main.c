#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "uart/uart.h"
#include "notes/notes.h"

///////////////////////////////
// defines
///////////////////////////////
#define DBG 				0

#define MAX_NUMBER_NOTES 	100
#define MIN_NUM_PLAYERS		2
#define MAX_NUM_PLAYERS		9
#define BTN_PRESS_TIMEOUT	5 * 1000	// in ms
#define DEBOUNCE_TIME 		250 		// debounce time in ms


///////////////////////////////
// PINS
///////////////////////////////
volatile char* DDRd = (char*)0x02A;
volatile char* PORTd = (char*)0x02B;
volatile char* PINd = (char*)0x029;

///////////////////////////////
// variables
///////////////////////////////
int num_players;
int num_active_players;
int cur_player;
int winner;
int cur_round;
int cur_number_of_notes;
volatile int time_between_notes;
volatile char note_pressed;
volatile unsigned long milliseconds;			// milliseconds since start
volatile unsigned long btn1_last_pressed;		// last time button 1 was pressed
volatile unsigned long btn2_last_pressed;		// last time button 2 was pressed

int players[MAX_NUM_PLAYERS];
char notes_sequence[MAX_NUMBER_NOTES];

volatile char watchdog_counter = 0;


///////////////////////////////
// Buttons setup
//
// Button 1:
//   5v  - 5v pin
//   GND - GND pin
//   Out - PIN2 (INT0)
//
// Button 2:
//   5v  - pin7
//   GND - GND pin
//   Out - PIN3 (INT1)
//
///////////////////////////////
void setup_buttons() {
    *DDRd &= ~(1 << DDD2);         // pin2 as input
    EICRA |= (1 << ISC01);         // set INT0 to trigger
    EIMSK |= (1 << INT0);          // Turns on INT0

    *DDRd &= ~(1 << DDD3);         // pin3 as input
    EICRA |= (1 << ISC11);         // set INT1 to trigger
    EIMSK |= (1 << INT1);          // Turns on INT1

    *DDRd |= (1 << PORTD7);        // pin7 as output
    *PORTd |= (1 << PORTD7);       // pin7 to send 5V to button2

    sei();
}

void enable_buttons() {
	EIMSK |= (1 << INT1) | (1 << INT0); 		// Allow INT0/INT1 interrupts
	
	btn1_last_pressed = 0;
	btn2_last_pressed = 0;
}

void disable_buttons() {
	EIMSK &= ~(1 << INT1);
	EIMSK &= ~(1 << INT0);
}

ISR(INT0_vect) {
    if (milliseconds - btn1_last_pressed > DEBOUNCE_TIME) {
        btn1_last_pressed = milliseconds;
        // printf("F");
        note_pressed = 'F';
        play_note(f6);
    }
}

ISR(INT1_vect) {
    if (milliseconds - btn2_last_pressed > DEBOUNCE_TIME) {
        btn2_last_pressed = milliseconds;
        // printf("C");
        note_pressed = 'C';
        play_note(c5);
    }
}


///////////////////////////////
// Watchdog setup
// Watchdog will be used to count time to user press button (5s).
// As we don't have a prescaler, we count 5 watchdog interruptions of 1s.
///////////////////////////////
//initialize watchdog
void WDT_Init(void) {
    //disable interrupts
    cli();

    //reset watchdog
    wdt_reset();

    //set up WDT interrupt
    WDTCSR |= (1<<WDCE) | (1 << WDE);

    // Set Watchdog settings: prescaler 1s and no reset
    WDTCSR = (1<<WDIE) | (0<<WDP3) | (1<<WDP2) | (1<<WDP1) | (0<<WDP0);

    watchdog_counter = 0;
}

void reset_watchdog_counter() {
	watchdog_counter = 0;
}

//Watchdog timeout ISR
ISR(WDT_vect) {
	watchdog_counter++;

  //   printf("Watch Dog Timer\n");
  //   if (watchdog_counter == 4) {
  //       // current player lost
  //       // watchdog_counter = 0;
  //       wdt_disable();
  //       printf("Watch Dog Timer - TIMEOUT\n");
  //   } else {
        // watchdog_counter++;
  //       printf("Watch Dog Timer - time %d \n", watchdog_counter);
		// wdt_reset();
  //   }
}

///////////////////////////////
// timer0 functions
///////////////////////////////
void config_timer0() {
	// cpu freq.: 16MHz - 16.000.000
  	// prescaler 64
  	// 16MHz / 64 = 250.000 cycles / sec.
  	// 1 cycle = 4us (4 micro sec.)
  	// config timer to count from 0 to 250 (1ms)
	TIMSK0 = (1 << TOIE0); // Enable the interrupt on overflow for Timer0
	TCCR0B = (1<<CS01) | (1<<CS00); // Set timer0 with /64 prescaler (TCCR0B = 0b00000011)
	TCNT0 = 0x00; // Set timer0 counter initial value to 0
	OCR0A = 0xFA; // Output Compare = 250

	milliseconds = 0;
    time_between_notes = 0;
}

// timer0 interrupt
// configured to count 1ms intervals
ISR(TIMER0_OVF_vect) {
	// Output Compare = 250
	OCR0A = 0xFA;
	
	// increment millis
	milliseconds++;
    time_between_notes++;
}

///////////////////////////////
// player functions
///////////////////////////////
int get_next_active_player() {
	int next;

	for (next=cur_player+1; next < num_players && !players[next]; next++) {}

	if (next >= num_players) {
		return -1;
	}
	return next;
}

int get_first_active_player() {
	char first_active = 0;

	int i;
	for (i=0; i<num_players; i++) {
		if (players[i]) {
			first_active = i;
			break;
		}
	}

	return first_active;
}

// set all players as active
void activate_players() {
    if (DBG) {
        printf("activate_players");
    }

    num_active_players = num_players;

    memset(players, 1, sizeof(int) * num_players);
}

void eliminate_player(int player_number) {
	players[player_number] = 0;
	num_active_players--;

	play_note(fx4);
}

///////////////////////////////
// notes sequence functions
///////////////////////////////
void clear_notes_sequence() {
    if (DBG) {
        printf("clear notes sequence");
    }

    cur_number_of_notes = 0;
    memset(notes_sequence, ' ', sizeof(char) * MAX_NUMBER_NOTES);
}

char get_next_note() {
	note_pressed = ' ';
	while (note_pressed != 'C' && note_pressed != 'F' && time_between_notes < BTN_PRESS_TIMEOUT) { }

	return note_pressed;
}

int timeout() {
	// more than 5 seconds ellapsed since last note
	if (time_between_notes >= BTN_PRESS_TIMEOUT) {
		return 1;
	}

	return 0;
}

/**
 * Check if note pressed is already in the notes sequence array.
 * If note is not in present, eliminate current player and return 0.
 * If note is present, return 1.
 * 
 * The number of notes pressed will be incremented.
 **/
int is_note_in_sequence(char note, int num_notes_pressed) {
	if (note != notes_sequence[num_notes_pressed]) {
		printf("\nIncorrect sequence! You have been eliminated!\n");
		eliminate_player(cur_player);

		return 0;
	}

	return 1;
}

///////////////////////////////
// round functions
///////////////////////////////
void inc_round() {
	cur_round++;
}

///////////////////////////////
// states functions
///////////////////////////////
void reset() {
    printf("Welcome to the follow the leader game! Please enter the number of players: [2-9]\n");
}

void read_num_players() {
    num_players = 0;
    while (num_players < MIN_NUM_PLAYERS || num_players > MAX_NUM_PLAYERS) {
        scanf("%d", &num_players);	
    }
    printf("%d players\n", num_players);
}

void round_reset() {
    cur_round = 1;
    winner = 0;

    clear_notes_sequence();
    activate_players();
}

void round_start() {
    printf("\nRound %d\n", cur_round);
    cur_player = get_first_active_player();
}


void round_turn() {

    enable_buttons();

    while (cur_player >= 0) {
        printf("Player %d\n", (cur_player+1));
        time_between_notes = 0;

        char note;
   
        if (cur_number_of_notes == 0) {
            note = get_next_note();

            // check timeout
            if (timeout()) {
            	printf("Time expired. You have been eliminated!\n");
				eliminate_player(cur_player);
            } else {
	            printf("%c\n", note);
	            notes_sequence[cur_number_of_notes++] = note;	
	            time_between_notes = 0;
            }

        } else {
			int num_notes_pressed = 0;
			while (num_notes_pressed < cur_number_of_notes) {
				// start timer to check timeout
				note = get_next_note();

				if (timeout()) {
					printf("Time expired. You have been eliminated!\n");
					eliminate_player(cur_player);
					
					break;
				}
				time_between_notes = 0;

				// When there's more than one note in the sequence and the 
            	// current note pressed is not in the sequence, eliminate player.
	            if (note != notes_sequence[num_notes_pressed]) {
	            	printf("\nIncorrect sequence! You have been eliminated!\n");
					
					eliminate_player(cur_player);
			
	 				break;	
				}
				num_notes_pressed++;

				printf("%c", note);
			}

			// If there's more than one note in sequence and
			// the current player was not eliminated, get one more note
			if (players[cur_player]) {
				note = get_next_note();

				if (timeout()) {
					printf("Time expired. You have been eliminated!\n");
					eliminate_player(cur_player);
				} else {
					printf("%c\n", note);
					notes_sequence[cur_number_of_notes++] = note;
					time_between_notes = 0;
				}
			}
		}

		if (num_active_players == 1) {
			// set global variable winner to indicate
			// that the game must be reset
			winner = get_first_active_player() + 1;
			printf("Player %d won\n", winner);

			cur_player = -1;
		} else {
			cur_player = get_next_active_player();
		}
	}

    disable_buttons();
    inc_round();
}

///////////////////////////////
// main
///////////////////////////////
int main(void) {
    uart_init();

    setup_buttons();
    enable_buttons();

    config_timer0();

    // IMPORTANT: notes uses TIMER2 to control how long note will ve played.
    init_notes();

  	// WDT_Init();

    sei();

    while (1) {
        reset();
        read_num_players();
        round_reset();

        do {
            round_start();
            round_turn();
        } while (!winner);
    }

    return 0;
}
