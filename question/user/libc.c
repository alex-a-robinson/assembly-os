#include "libc.h"

int  atoi( char* x        ) {
    char* p = x; bool s = false; int r = 0;

    if     ( *p == '-' ) {
        s =  true; p++;
    }
    else if( *p == '+' ) {
        s = false; p++;
    }

    for( int i = 0; *p != '\x00'; i++, p++ ) {
        r = s ? ( r * 10 ) - ( *p - '0' ) :
        ( r * 10 ) + ( *p - '0' ) ;
    }

    return r;
}

void itoa( char* r, int x ) {
    char* p = r; int t, n;

    if( x < 0 ) {
        p++; t = -x; n = 1;
    }
    else {
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

void yield() {
    asm volatile( "svc %0     \n" // make system call SYS_YIELD
    :
    : "I" (SYS_YIELD)
    : );

    return;
}

int write( int fd, const void* x, size_t n ) {
    int r;

    asm volatile( "mov r0, %2 \n" // assign r0 = fd
    "mov r1, %3 \n" // assign r1 =  x
    "mov r2, %4 \n" // assign r2 =  n
    "svc %1     \n" // make system call SYS_WRITE
    "mov %0, r0 \n" // assign r  = r0
    : "=r" (r)
    : "I" (SYS_WRITE), "r" (fd), "r" (x), "r" (n)
    : "r0", "r1", "r2" );

    return r;
}

void err(char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

int  read( int fd,       void* x, size_t n ) {
    int r;

    asm volatile( "mov r0, %2 \n" // assign r0 = fd
    "mov r1, %3 \n" // assign r1 =  x
    "mov r2, %4 \n" // assign r2 =  n
    "svc %1     \n" // make system call SYS_READ
    "mov %0, r0 \n" // assign r  = r0
    : "=r" (r)
    : "I" (SYS_READ),  "r" (fd), "r" (x), "r" (n)
    : "r0", "r1", "r2" );

    return r;
}

int fork() {
    int r;

    asm volatile("svc %1     \n" // make system call SYS_FORK
    "mov %0, r0 \n" // assign r  = r0
    : "=r" (r)
    : "I" (SYS_FORK)
    : "r0" );

    return r;
}

void ps(int pid) {
    asm volatile( "mov r0, %1 \n" // assign r0 =  x
    "svc %0     \n" // make system call SYS_EXIT
    :
    : "I" (SYS_PS), "r" (pid)
    : "r0" );

    return;
}

void exit( int x ) {
    asm volatile( "mov r0, %1 \n" // assign r0 =  x
    "svc %0     \n" // make system call SYS_EXIT
    :
    : "I" (SYS_EXIT), "r" (x)
    : "r0" );

    return;
}

void exec( const void* x ) {
    asm volatile( "mov r0, %1 \n" // assign r0 = x
    "svc %0     \n" // make system call SYS_EXEC
    :
    : "I" (SYS_EXEC), "r" (x)
    : "r0" );

    return;
}

int kill( int pid, int x ) {
    int r;

    asm volatile( "mov r0, %2 \n" // assign r0 =  pid
    "mov r1, %3 \n" // assign r1 =    x
    "svc %1     \n" // make system call SYS_KILL
    "mov %0, r0 \n" // assign r0 =    r
    : "=r" (r)
    : "I" (SYS_KILL), "r" (pid), "r" (x)
    : "r0", "r1" );

    return r;
}

int sharem( void* ptr ) {
    int r;

    asm volatile( "mov r0, %2 \n" // assign r0 =  ptr
    "svc %1     \n" // make system call SYS_SHARE
    "mov %0, r0 \n" // assign r0 =    r
    : "=r" (r)
    : "I" (SYS_SHARE), "r" (ptr)
    : "r0");

    return r;
}

int unsharem( void* ptr ) {
    int r;

    asm volatile( "mov r0, %2 \n" // assign r0 =  ptr
    "svc %1     \n" // make system call SYS_UNSHARE
    "mov %0, r0 \n" // assign r0 =    r
    : "=r" (r)
    : "I" (SYS_UNSHARE), "r" (ptr)
    : "r0");

    return r;
}

int _lockm( void* ptr ) {
    int r;

    asm volatile( "mov r0, %2 \n" // assign r0 =  ptr
    "svc %1     \n" // make system call SYS_LOCK
    "mov %0, r0 \n" // assign r0 =    r
    : "=r" (r)
    : "I" (SYS_LOCK), "r" (ptr)
    : "r0");

    return r;
}

int lockm(void* ptr) {
    int locked = _lockm(ptr);
    while (!locked) {
        yield(); // No point waiting when it cannot be unlocked while we're on the cpu
        locked = _lockm(ptr);
    }
    return locked;
}

int unlockm( void* ptr ) {
    int r;

    asm volatile( "mov r0, %2 \n" // assign r0 =  ptr
    "svc %1     \n" // make system call SYS_UNLOCK
    "mov %0, r0 \n" // assign r0 =    r
    : "=r" (r)
    : "I" (SYS_UNLOCK), "r" (ptr)
    : "r0");

    return r;
}

int _waitp(int pid) {
    int r;

    asm volatile( "mov r0, %2 \n" // assign r0 =  pid
    "svc %1     \n" // make system call SYS_WAIT
    "mov %0, r0 \n" // assign r0 =    r
    : "=r" (r)
    : "I" (SYS_WAIT), "r" (pid)
    : "r0");

    return r;
}

int waitp(int pid) {
    int result = _waitp(pid);
    while (result == -1) { // -1 shows still waiting
        yield();
        result = _waitp(pid);
    }

    if (result == -2) {
        return -1; // Return an error
    }

    return result;
}

void sleep(int t) {
    // TODO use sleeping flag on process
    int x = 0;
    for (int i=0; i<1000*t; i++) {
        x++;
    }
    return;
}
