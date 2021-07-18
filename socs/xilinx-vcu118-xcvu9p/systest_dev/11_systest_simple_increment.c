// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <00_systest_helper.h>

#define N_CPU 2

#define STR(x) #x
#define XSTR(x) STR(x)
#pragma message "Inside test = " XSTR(TEST_ID)

void simple_increment ()
{
    uint64_t hartid;
    uint64_t shared_val;
	volatile uint64_t* shared_ptr = (volatile uint64_t*) 0x90001000; 
	volatile uint64_t* init_done = (volatile uint64_t*) 0x90002000; 
	volatile uint64_t* handshake = (volatile uint64_t*) 0x90003000; 

	// read hart ID
	hartid = read_hartid ();

	if (hartid == 0) {
		*shared_ptr = 0;
		*init_done = 0xcafecafe;
	} else {
		while (*init_done != 0xcafecafe);
	}

	for (int i = 0; i < 100000; i++) {
		spin_for_lock (handshake);
		
		shared_val = *shared_ptr;
		shared_val++;

		if (shared_val % 5000 == 0) {
			printf ("%0d: %0d\n", hartid, shared_val);
		}

		*shared_ptr = shared_val;

		release_lock (handshake);

		// delay (shared_val % 500);
	}

	return;
} 