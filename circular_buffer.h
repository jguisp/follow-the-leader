#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <avr/io.h>

typedef struct {
    char* buffer;
    int head;
    int tail;
    int maxLen;
} circular_buffer_t;

void circular_buffer_init(circular_buffer_t **c, volatile uint8_t size);
int circular_buffer_push(circular_buffer_t *c, volatile char data);
int circular_buffer_pop(circular_buffer_t *c, volatile char *data);

#endif /* CIRCULAR_BUFFER_H */