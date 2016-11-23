#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "uart/uart.h"

///////////////////////////////
// defines
///////////////////////////////
#define MAX_NUMBER_NOTES 100
#define DBG 0

///////////////////////////////
// variables
///////////////////////////////
unsigned char num_players;
unsigned char num_active_players;
unsigned char cur_player;
unsigned char* players = NULL;

unsigned int cur_round;

unsigned char* notes_sequence = NULL;
unsigned char cur_number_of_notes;

unsigned char timeout = 0;

///////////////////////////////
// player functions
///////////////////////////////
void read_num_players() {
	scanf("%c", &num_players);
	printf("%c players\n", num_players);
}

unsigned char get_num_active_players() {
	return num_active_players;
}

unsigned char get_next_active_player() {
	unsigned char next;

	for (next=cur_player+1; next < num_players && !players[next]; next++) {}

	if (next >= num_players) {
		return -1;
	}
	return next;
}

unsigned char get_first_active_player() {
	unsigned char first_active = 0;

	unsigned char* p = players;
	while (!(*p) && p++) {
		first_active++;
	}

	return first_active;
}

void init_players_list(int size) {
	if (DBG) {
		printf("init players list");	
	}

	players = (unsigned char*) malloc(sizeof(unsigned char) * size);
}

// set all players as active
void activate_players() {
	if (DBG) {
		printf("activate_players");
	}

	num_active_players = num_players;
	cur_player = 0;

	// memset(players, 1, sizeof(unsigned char) * num_players);
	int i;
	for (i=0; i<num_players; i++) {
		players[i] = 1;
	}
}

void destroy_players_list() {
	if (players) {
		free(players);
		players = NULL;
	}
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
	// memset(notes_sequence, ' ', sizeof(unsigned char) * MAX_NUMBER_NOTES);
	int i;
	for (i=0; i<num_players; i++) {
		notes_sequence[i] = ' ';
	}
}

void init_notes_sequence() {
	if (!notes_sequence) {
		if (DBG) {
			printf("init notes sequence");
		}

		notes_sequence = (unsigned char*) malloc(sizeof(unsigned char) * MAX_NUMBER_NOTES);
	} else {
		clear_notes_sequence();
	}
}

void destroy_notes_sequence() {
	if (notes_sequence) {
		free(notes_sequence);
		notes_sequence = NULL;
	}
}

unsigned char get_next_note() {
	volatile char note;
	scanf("%c", &note);

	return note;
}

void print_notes_sequence() {
	printf("\n");
	
	int i;
	for(i=0; i<cur_number_of_notes; i++) {
		printf("%c - ", notes_sequence[i]);
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

void state_num_players() {
	read_num_players();
	init_players_list(num_players);
}

void round_reset() {
	cur_round = 1;

	init_notes_sequence();
	activate_players();

	round_start();	
}

void round_start() {
	printf("Round %d\n", cur_round);

	cur_player = get_first_active_player();
}

void round_turn() {

	while (cur_player >= 0) {
		printf("Player %d \n", (cur_player+1));

		// start timer to check timeout
		
		volatile unsigned char note;
		if (cur_number_of_notes == 0) {
			note = get_next_note();
			notes_sequence[cur_number_of_notes++] = note;

		} else {
			// wait for the sequence of notes or timeout
			unsigned char notes_pressed = 0;
			while (notes_pressed < cur_number_of_notes+1) {
				note = get_next_note();
				printf("%c - ", note);

				if (notes_pressed == cur_number_of_notes) {
					notes_sequence[cur_number_of_notes++] = note;
					break;
				}

				if (notes_sequence[notes_pressed++] != note) {
					printf("Incorrect sequence! You have been eliminated!\n");
					eliminate_player(cur_player);
					
				 	break;
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
		}
	
		printf("Active Players %d \n", get_num_active_players());

		if (get_num_active_players() == 1) {
			printf("Player %c won", get_next_active_player());
		} else {
			print_notes_sequence();
			cur_player = get_next_active_player();
		}
	}

	inc_round();
}

///////////////////////////////
// main
///////////////////////////////
int main(void) {
    uart_init();

    sei();

	reset();
	state_num_players();
	round_reset();

    for ( ; ; ) {
    	round_start();

    	round_turn();
	}

    return 0;
}