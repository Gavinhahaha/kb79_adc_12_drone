#ifndef EASY_FIFO_H_
#define EASY_FIFO_H_

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t fifo_size_t ;

typedef struct _fifo_s {
    fifo_size_t in;
    fifo_size_t out;
    fifo_size_t mask;//total fifos buffer size
    fifo_size_t esize;//each fifo buffer size
    uint8_t     *data;
} fifo_t;






void fifo_init(fifo_t *fifo, uint8_t *data_buf, fifo_size_t size, fifo_size_t esize);

fifo_size_t fifo_in(fifo_t *fifo, const uint8_t *buffer, fifo_size_t len);
fifo_size_t fifo_out(fifo_t *fifo, uint8_t *buffer, fifo_size_t len);
fifo_size_t fifo_peek(fifo_t *fifo, uint8_t *buffer, fifo_size_t len);

void fifo_reset(fifo_t *fifo);

fifo_size_t fifo_size(fifo_t *fifo);
fifo_size_t fifo_esize(fifo_t *fifo);
fifo_size_t fifo_len(fifo_t *fifo);
fifo_size_t fifo_avail(fifo_t *fifo);

bool fifo_is_empty(fifo_t *fifo);
bool fifo_is_full(fifo_t *fifo);

#endif /* EASY_FIFO_H_ */
