#include "P5.h"

uint32_t weight( uint32_t x ) {
    x = ( x & 0x55555555 ) + ( ( x >>  1 ) & 0x55555555 );
    x = ( x & 0x33333333 ) + ( ( x >>  2 ) & 0x33333333 );
    x = ( x & 0x0F0F0F0F ) + ( ( x >>  4 ) & 0x0F0F0F0F );
    x = ( x & 0x00FF00FF ) + ( ( x >>  8 ) & 0x00FF00FF );
    x = ( x & 0x0000FFFF ) + ( ( x >> 16 ) & 0x0000FFFF );

    return x;
}

extern void main_P3();

void main_P5() {

    err("Starting P3\n");
    pid_t pid = fork();
    if (0 == pid) {
        exec(&main_P3, NULL);
    }
    err("Spawned P3\n");
    int r = waitp(pid);
    char b[1024];
    err("Returned with");err(ss(b,r));err("\n");

    while( 1 ) {
        write( STDOUT_FILENO, "P5", 2 );

        uint32_t lo = 1 <<  8;
        uint32_t hi = 1 << 24;

        for( uint32_t x = lo; x < hi; x++ ) {
            uint32_t r = weight( x );
        }
    }

    exit( EXIT_SUCCESS );
}
