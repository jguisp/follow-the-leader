#include <avr/io.h>
#include <stdlib.h>

#include "circular_buffer.h"

void circular_buffer_init(circular_buffer_t **c, volatile uint8_t size) {
    circular_buffer_t* cb = (circular_buffer_t*) malloc (sizeof(circular_buffer_t));

    cb->buffer = (char *) malloc (sizeof(char) * size);
	cb->head = 0;
	cb->tail = 0;
	cb->maxLen = size;

    *c = cb;
}

int circular_buffer_push(circular_buffer_t *c, volatile char data) {
    int next = (c->head + 1) % c->maxLen;
 
    // buffer is full
    if (next == c->tail)
        return -1;
 
    c->buffer[c->head] = data;
    c->head = next;
    return 0;
}
 
int circular_buffer_pop(circular_buffer_t *c, volatile char *data) {
    // when head and tail are equals, buffer is empty
    if (c->head == c->tail)
        return -1;
 
    *data = c->buffer[c->tail];
    c->buffer[c->tail] = 0;
 
    c->tail = (c->tail + 1) % c->maxLen;
 
    return 0;
}