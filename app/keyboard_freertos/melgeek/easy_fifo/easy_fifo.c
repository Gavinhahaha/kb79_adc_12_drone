#include "easy_fifo.h"
#include <string.h>

static void _fifo_copy_in(fifo_t *fifo, const uint8_t *src, fifo_size_t len, fifo_size_t off)
{
    fifo_size_t size    = fifo->mask + 1;
    fifo_size_t esize   = fifo->esize;
    fifo_size_t tmp_len = 0;

    off &= fifo->mask;

    if (esize != 1)
    {
        off  *= esize;
        size *= esize;
        len  *= esize;
    }

    tmp_len = (len) < (size - off) ? (len) : (size - off);

    memcpy(fifo->data + off, src, tmp_len);
    memcpy(fifo->data, src + tmp_len, len - tmp_len);
}

static void _fifo_copy_out(fifo_t *fifo, uint8_t *dst, fifo_size_t len, fifo_size_t off)
{
    fifo_size_t size    = fifo->mask + 1;
    fifo_size_t esize   = fifo->esize;
    fifo_size_t tmp_len = 0;

    off &= fifo->mask;
    if (esize != 1)
    {
        off  *= esize;
        size *= esize;
        len  *= esize;
    }

    tmp_len = (len) < (size - off) ? (len) : (size - off);

    memcpy(dst, fifo->data + off, tmp_len);
    memcpy(dst + tmp_len, fifo->data, len - tmp_len);
}

void fifo_init(fifo_t *fifo, uint8_t *data_buf, fifo_size_t size, fifo_size_t esize)
{
    size = size / esize;

    fifo->in    = 0;
    fifo->out   = 0;
    fifo->mask  = size - 1;
    fifo->data  = data_buf;
    fifo->esize = esize;
}

fifo_size_t fifo_in(fifo_t *fifo, const uint8_t *buffer, fifo_size_t len)
{
    fifo_size_t tmp_len = 0;

    tmp_len = (fifo->mask + 1) - (fifo->in - fifo->out);
    len = len > tmp_len ? tmp_len : len;

    _fifo_copy_in(fifo, buffer, len, fifo->in);
    fifo->in += len;
	
    return len;
}

fifo_size_t fifo_out(fifo_t *fifo, uint8_t *buffer, fifo_size_t len)
{
    fifo_size_t tmp_len = 0;

    tmp_len = fifo->in - fifo->out;
    len = len > tmp_len ? tmp_len : len;
    _fifo_copy_out(fifo, buffer, len, fifo->out);

    fifo->out += len;

    return len;
}

fifo_size_t fifo_peek(fifo_t *fifo, uint8_t *buffer, fifo_size_t len)
{
    fifo_size_t tmp_len = 0;

    tmp_len = fifo->in - fifo->out;
    len = len > tmp_len ? tmp_len : len;
    _fifo_copy_out(fifo, buffer, len, fifo->out);

    return len;
}

void fifo_reset(fifo_t *fifo)
{
    fifo->in  = 0;
    fifo->out = 0;
}

fifo_size_t fifo_size(fifo_t *fifo)
{
    return (fifo->mask + 1);
}

fifo_size_t fifo_esize(fifo_t *fifo)
{
    return (fifo->esize);
}

fifo_size_t fifo_len(fifo_t *fifo)
{
    return (fifo->in - fifo->out);
}

fifo_size_t fifo_avail(fifo_t *fifo)
{
    return (fifo_size(fifo) - fifo_len(fifo));
}

bool fifo_is_empty(fifo_t *fifo)
{
    return (fifo->in == fifo->out);
}

bool fifo_is_full(fifo_t *fifo)
{
    return (fifo_len(fifo) > fifo->mask);
}

