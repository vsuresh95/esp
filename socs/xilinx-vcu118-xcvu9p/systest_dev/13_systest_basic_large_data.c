// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <00_systest_helper.h>

#define N_ITER 15000

#define STR(x) #x
#define XSTR(x) STR(x)
#pragma message "Inside test = " XSTR(TEST_ID)

void basic_large_data ()
{
    uint64_t hartid;
	uint64_t* shared_ptr = (uint64_t*) 0x94561000;
	volatile uint64_t* handshake = (volatile uint64_t*) 0x94000000;
	int err_cnt = 0;

	// read hart ID
	hartid = read_hartid ();

	if (hartid == 0) {
		for (int i = 0; i < N_ITER; i++) {
			*(shared_ptr + i) = i;
		}

		// printf ("%0d done\n", hartid);

		*handshake = 0xcafecafe;
		asm volatile ("fence w, w");
		while (*handshake != 0xbeefbeef);

		// printf ("%0d start\n", hartid);

		for (int i = 0; i < N_ITER; i++) {
			if (*(shared_ptr + i) != 2*i) err_cnt++;
		}
	} else {
		while (*handshake != 0xcafecafe);
		for (int i = 0; i < N_ITER; i++) {
			*(shared_ptr + i) += i;
		}

		// printf ("%0d done\n", hartid);

		*handshake = 0xbeefbeef;
		asm volatile ("fence w, w");

		while (1);
	}

	printf ("Errors = %d\n", err_cnt);

	return;
} 