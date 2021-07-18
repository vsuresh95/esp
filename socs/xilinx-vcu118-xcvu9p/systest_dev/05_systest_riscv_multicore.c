// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <00_systest_helper.h>

#define N_CPU 2

#define STR(x) #x
#define XSTR(x) STR(x)
#pragma message "Inside test = " XSTR(TEST_ID)

void riscv_multicore ()
{
    uint64_t hartid;
    static uint64_t first;
    static uint64_t shared_count;
    static uint64_t hart_count[N_CPU];
	volatile uint64_t* lock = (volatile uint64_t*) 0x90020080;

	// read hart ID
	hartid = read_hartid ();

    hart_count[hartid] = 0;

    // infinitely running test
    while (1) {
	    // acquire the lock
	    spin_for_lock (lock);

        if (first != 1) {
            shared_count = 0;
            first = 1;
        }

        shared_count++;
        hart_count[hartid]++;

        printf ("%0d: %0d, %0d %0d\n", hartid, shared_count, hart_count[0], hart_count[1]);

    	// release the lock
    	release_lock (lock);
    }

	return;
} 