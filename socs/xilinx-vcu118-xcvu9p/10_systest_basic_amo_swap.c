// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <00_systest_helper.h>

#define N_CPU 2

#define STR(x) #x
#define XSTR(x) STR(x)
#pragma message "Inside test = " XSTR(TEST_ID)

void basic_amo_swap ()
{
    uint64_t hartid;
	uint64_t old_val;
	volatile uint64_t* shared_count = (volatile uint64_t*) 0x90000000;
	volatile uint64_t* first = (volatile uint64_t*) 0x90001000;
	volatile uint64_t* handshake = (volatile uint64_t*) 0x90002000;
	volatile uint64_t* hart_count = (volatile uint64_t*) 0x90003000;

	// read hart ID
	hartid = read_hartid ();

	write_dword_fcs(hart_count+hartid, 0, false, false);

	while (1) {
		// acquire the lock
		while (1) {
			// check if lock is set
			if (read_dword_fcs(handshake, false, false) != 1) {
				// try to set lock
				old_val = amo_swap (handshake, 1);

				// check if lock was set
				if (old_val != 1) {
					break;
				}

				// printf ("%0d: wait\n", hartid);

				// if (hartid == 0) {
				// 	printf ("wait\n");
				// } else {
				// 	printf ("stuck\n");
				// }
			}
		}

        if (read_dword_fcs(first, false, false) != 1) {
			write_dword_fcs(first, 1, false, false);
			write_dword_fcs(shared_count, 0, false, false);
		}

		uint64_t shared_var = read_dword_fcs(shared_count, false, false);

		uint64_t hart_var[N_CPU];
		for (uint64_t i = 0; i < N_CPU; i++) {
			hart_var[i] = read_dword_fcs(hart_count+i, false, false);
		}

		shared_var++;
		hart_var[hartid]++;

		write_dword_fcs(shared_count, shared_var, false, false);
		write_dword_fcs(hart_count+hartid, hart_var[hartid], false, false);
		
		if (shared_var % 50 == 0) {
			printf ("%0d: %0d,", hartid, shared_var);
		
			for (uint64_t i = 0; i < N_CPU; i++) {
				printf (" %0d", hart_var[i]);
			}
		
			printf ("\n");
		}

    	amo_swap (handshake, 0);
	}

	return;
} 