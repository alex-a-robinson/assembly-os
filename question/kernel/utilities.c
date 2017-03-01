#include "utilities.h"

void error(char* msg) {
    char* x = msg;
    PL011_t* device = UART1;
    for (int i = 0; i < strlen(x); i++) {
        PL011_putc(device, *x++, true); // Write to STDERR
    }
}

void sitoa( char* r, int x ) {
    char* p = r; int t, n;

    if( x < 0 ) {
        p++; t = -x; n = 1;
    } else {
        t = +x; n = 1;
    }

    while( t >= n ) {
        p++; n *= 10;
    }

    *p-- = '\x00';

    do {
        *p-- = '0' + ( t % 10 ); t /= 10;
    } while( t );

    if( x < 0 ) {
        *p-- = '-';
    }

    return;
}

// Copy the int x to the end of the string s
char* s(char* b, int x) {
    b[0] = '\0';
    if (x == 0) { // Little hack beacuse itoa dosen't like 0
        b = "0";
    } else {
        sitoa(b, x);
    }
    return b;
}

// // TODO make full path out of small path + root_dir
// int get_full_path(char* path, char* full_path) {
//     // TODO
//
// }
