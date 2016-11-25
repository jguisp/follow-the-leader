#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "uart/uart.h"

///////////////////////////////
// defines
///////////////////////////////
#define MAX_NUMBER_NOTES 	100
#define MIN_NUM_PLAYERS		2
#define MAX_NUM_PLAYERS		9
#define DBG 				0


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
int timeout;

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

ISR (INT0_vect)
{
    printf("Interruption Button 1\n");
}

ISR (INT1_vect)
{
    printf("Interruption Button 2\n");
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
}

//Watchdog timeout ISR
ISR(WDT_vect) {
    printf("Watch Dog Timer\n");
    if (watchdog_counter == 4) {
        // current player lost
        watchdog_counter = 0;
        wdt_disable();
        printf("Watch Dog Timer - TIMEOUT\n");
    } else {
        watchdog_counter++;
        printf("Watch Dog Timer - time %d \n", watchdog_counter);
	wdt_reset();
    }
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
	// TODO: replace by button inputs
	char note = 'a';
	while (note != 'C' && note != 'F') {
		scanf("%c", &note);	
	}

	return note;
}

void print_notes_sequence() {
	printf("\n");
	
	int i;
	for(i=0; i<cur_number_of_notes; i++) {
		printf("%c", notes_sequence[i]);
	}
	
	printf("\n");
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

	while (cur_player >= 0) {
		printf("Player %d\n", (cur_player+1));

		char note;
		if (cur_number_of_notes == 0) {
			note = get_next_note();
			printf("%c\n", note);
			notes_sequence[cur_number_of_notes++] = note;
		} else {
			// wait for the sequence of notes or timeout
			char num_notes_pressed = 0;
			int eliminated=0;
			while (num_notes_pressed < cur_number_of_notes) {
				// start timer to check timeout
				note = get_next_note();
				printf("%c", note);

				if (note != notes_sequence[num_notes_pressed++]) {
					printf("\nIncorrect sequence! You have been eliminated!\n");
					eliminate_player(cur_player);
					eliminated=1;
			 		break;	
				}
			}

			if (!eliminated) {
				note = get_next_note();
				printf("%c\n", note);
				notes_sequence[cur_number_of_notes++] = note;
			}

			// more than 5 seconds between notes
			// if (timeout) {
			// 	printf("Time expired. You have been eliminated!");

			// 	eliminate_player(cur_player);

			// 	if (get_num_active_players() == 1) {
			// 		printf("Player %c won", get_next_active_player());
			// 	}
			// }
		}

		
		if (num_active_players == 1) {
			winner = get_first_active_player() + 1;
			printf("Player %d won\n", winner);
			cur_player = -1;
		} else {
			// print_notes_sequence();
			cur_player = get_next_active_player();;
		}
	}

	inc_round();
}

///////////////////////////////
// main
///////////////////////////////
int main(void) {
    uart_init();
//    setup_buttons();
//    WDT_Init();
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
