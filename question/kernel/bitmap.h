#ifndef __BITMAP_H
#define __BITMAP_H

// http://stackoverflow.com/questions/1225998/what-is-a-bitmap-in-c

#include <stdint.h>   /* for uint32_t */

typedef uint32_t word_t;
enum { BITS_PER_WORD = sizeof(word_t) * 32 };
#define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define BIT_OFFSET(b)  ((b) % BITS_PER_WORD)

void set_bit(word_t *words, int n);
void clear_bit(word_t *words, int n);
int get_bit(word_t *words, int n);

#endif
