#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>

#include "PL011.h"

#include "lib/libc.h"
#include "lib/env.h"

typedef struct {
    void* addr;
    char name[100];
} prog_t;

#endif
