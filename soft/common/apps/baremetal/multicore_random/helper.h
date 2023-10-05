#include <stdio.h>

#define RAND_MAX 64

void thread_entry (int cid, int nc) {
    return;
}

unsigned amo_add (volatile unsigned *handshake, unsigned value) {
	unsigned old_val;

	asm volatile (
		"mv t0, %2;"
		"mv t2, %1;"
 		"amoadd.w.aqrl t1, t0, (t2);"
		"mv %0, t1"
		: "=r" (old_val)
		: "r" (handshake), "r" (value)
		: "t0", "t1", "t2", "memory"
		);

	return old_val;
}

unsigned amo_swap (volatile unsigned *handshake, unsigned value) {
	unsigned old_val;

	asm volatile (
		"mv t0, %2;"
		"mv t2, %1;"
 		"amoswap.w.aqrl t1, t0, (t2);"
		"mv %0, t1"
		: "=r" (old_val)
		: "r" (handshake), "r" (value)
		: "t0", "t1", "t2", "memory"
		);

	return old_val;
}

void acquire_lock (volatile unsigned *handshake) {
    unsigned old_val;

    // acquire the lock
    while (1) {
        // check if lock is set
        if (*handshake != 1) {
            // try to set lock
            old_val = amo_swap (handshake, 1); 

            // check if lock was set
            if (old_val != 1){
                break;
            }
        }
    }
}

void release_lock (volatile unsigned *handshake) {
    *handshake = 0;
    asm volatile ("fence");
}

void acquire_lock_lr_sc (volatile unsigned *handshake) {
    // acquire the lock
    asm volatile (
        "try_lr_%=:     li t1, 1;"
        "               mv t2, %0;"
        "               lr.w.aq t0, (t2);"
        "               beq t0, t1, try_lr_%=;"
        "               sc.w.rl t0, t1, (t2);"
        "               bnez t0, try_lr_%="
        :
        : "r" (handshake)
        : "t0", "t1", "t2", "memory"
        );
}

void multicore_print(const char *fmt, ...) {
    // Lock to acquire before printing
	volatile unsigned* print_lock = (volatile unsigned*) 0x80020000;
	
    // acquire the lock
    acquire_lock (print_lock);

    printf(fmt);

    // release the lock
    release_lock (print_lock);
}

static unsigned long int next = 42;
unsigned rand(unsigned hartid)
{
	unsigned rand_tmp;
	next = next * 1103515245 + (hartid * 12343);
	rand_tmp = (unsigned) (next / 65536) % RAND_MAX;
	return rand_tmp;
}
