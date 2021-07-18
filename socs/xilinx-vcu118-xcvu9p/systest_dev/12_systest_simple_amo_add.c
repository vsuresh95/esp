// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <00_systest_helper.h>

#define N_ITER 100000

#define STR(x) #x
#define XSTR(x) STR(x)
#pragma message "Inside test = " XSTR(TEST_ID)

void simple_amo_add ()
{
    uint64_t hartid;
	volatile uint64_t* shared_ptr = (volatile uint64_t*) 0x90001000; 
	volatile uint64_t* init_done = (volatile uint64_t*) 0x90002000;
	volatile uint64_t* add_done = (volatile uint64_t*) 0x90003000;

	// read hart ID
	hartid = read_hartid ();

	if (hartid == 0) {
		*shared_ptr = 0;
		*init_done = 0xcafecafe;
		*add_done = 0;
	} else {
		while (*init_done != 0xcafecafe);
	}

	for (int i = 0; i < N_ITER; i++) {
		amo_add (shared_ptr, 1);
	}

	amo_add (add_done, 1);

	if (hartid == 0) {
		while (*add_done != 2);

		if (*shared_ptr == 2*N_ITER) {
			printf("PASS\n");
		} else {
			printf("FAIL %0d\n", *shared_ptr);
		}
	} else {
		while (1);
	}

	return;
} 