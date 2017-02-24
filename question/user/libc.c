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

// Copy the int x to the end of the string s
char* ss(char* b, int x) {
    b[0] = '\0';
    if (x == 0) { // Little hack beacuse itoa dosen't like 0
        b = "0";
    } else {
        itoa(b, x);
    }
    return b;
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

int open(const void* path, int flags) {
    int fd;

    asm volatile( "mov r0, %2 \n" // assign r0 = path
    "mov r1, %3 \n" // assign r1 =  flags
    "svc %1     \n" // make system call SYS_OPEN
    "mov %0, r0 \n" // assign r  = r0
    : "=r" (fd)
    : "I" (SYS_OPEN), "r" (path), "r" (flags)
    : "r0", "r1");

    return fd;
}

int close(int fd) {
    int r;

    asm volatile( "mov r0, %2 \n" // assign r0 = fd
    "svc %1     \n" // make system call SYS_CLOSE
    "mov %0, r0 \n" // assign r  = r0
    : "=r" (r)
    : "I" (SYS_CLOSE), "r" (fd)
    : "r0");

    return r;
}

int mount() {
    int r;

    asm volatile( "svc %1     \n" // make system call SYS_MOUNT
    "mov %0, r0 \n" // assign r  = r0
    : "=r" (r)
    : "I" (SYS_MOUNT)
    : "r0");

    return r;
}

int unmount() {
    int r;

    asm volatile( "svc %1     \n" // make system call SYS_UNMOUNT
    "mov %0, r0 \n" // assign r  = r0
    : "=r" (r)
    : "I" (SYS_UNMOUNT)
    : "r0");

    return r;
}

void err(char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

void puts(char* msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
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

int fork_wait() {
    int r;

    asm volatile("svc %1     \n" // make system call SYS_FORKWAIT
    "mov %0, r0 \n" // assign r  = r0
    : "=r" (r)
    : "I" (SYS_FORKWAIT)
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

// Wait non blocking to put in a wait request Note -1 is still waiting, -2 is error, other is exit status
int waitpnb(int pid) {
    return _waitp(pid);
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

// TODO silly function for sleep
int _slow( uint32_t x ) {
    if ( !( x & 1 ) || ( x < 2 ) ) {
        return ( x == 2 );
    }

    for( uint32_t d = 3; ( d * d ) <= x ; d += 2 ) {
        if( !( x % d ) ) {
            return 0;
        }
    }

    return 1;
}

void sleep(int t) {
    // TODO use sleeping flag on process
    int x = 0;
    for (int i=0; i<4*t; i++) {
        uint32_t lo = 1 <<  8;
        uint32_t hi = 1 << 16;

        for( uint32_t x = lo; x < hi; x++ ) {
            int r = _slow(x);
        }
    }
    return;
}
