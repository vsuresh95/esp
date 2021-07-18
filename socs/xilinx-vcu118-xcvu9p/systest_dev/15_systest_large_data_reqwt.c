// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <00_systest_helper.h>

#define N_ITER 2000

#define STR(x) #x
#define XSTR(x) STR(x)
#pragma message "Inside test = " XSTR(TEST_ID)

void large_data_reqwt ()
{
    uint64_t hartid;
	uint64_t* shared_ptr = (uint64_t*) 0xA0101000;
	volatile uint64_t* handshake = (volatile uint64_t*) 0x90000000;
	int err_cnt = 0;

	// read hart ID
	hartid = read_hartid ();

	if (hartid == 0) {
		for (int i = 0; i < N_ITER; i++) {
			*(shared_ptr + i) = i;
		}

		printf ("1111\n");

		*handshake = 0x1111;
		asm volatile ("fence w, w");
		while (*handshake != 0x2222);

		for (int i = 0; i < N_ITER; i++) {
			if (read_dword_fcs (shared_ptr + i, true, false) != 2*i) err_cnt++;
		}

		printf ("Errors 3 = %d\n", err_cnt);

		for (int i = 0; i < N_ITER; i++) {
			*(shared_ptr + i) = 3*i;
		}

		printf ("3333\n");

		*handshake = 0x3333;
		asm volatile ("fence w, w");

		while (*handshake != 0x4444);

		err_cnt = 0;

		for (int i = 0; i < N_ITER; i++) {
			if (read_dword_fcs (shared_ptr + i, true, false) != 4*i) err_cnt++;
		}

		printf ("Errors 6 = %d\n", err_cnt);
	} else {
		while (*handshake != 0x1111);

		for (int i = 0; i < N_ITER; i++) {
			if (read_dword_fcs (shared_ptr + i, true, false) != i) err_cnt++;
		}

		printf ("Errors 1 = %d\n", err_cnt);

		for (int i = 0; i < N_ITER; i++) {
			write_dword_fcs (shared_ptr + i, 2*i, true, false);
		}

		err_cnt = 0;

		for (int i = 0; i < N_ITER; i++) {
			if (read_dword_fcs (shared_ptr + i, true, false) != 2*i) err_cnt++;
		}

		printf ("Errors 2 = %d\n", err_cnt);

		*handshake = 0x2222;
		asm volatile ("fence rw, rw");

		while (*handshake != 0x3333);

		err_cnt = 0;

		for (int i = 0; i < N_ITER; i++) {
			if (read_dword_fcs (shared_ptr + i, true, false) != 3*i) err_cnt++;
		}

		printf ("Errors 4 = %d\n", err_cnt);

		for (int i = 0; i < N_ITER; i++) {
			write_dword_fcs (shared_ptr + i, 4*i, true, false);
		}

		err_cnt = 0;

		for (int i = 0; i < N_ITER; i++) {
			if (read_dword_fcs (shared_ptr + i, true, false) != 4*i) err_cnt++;
		}

		printf ("Errors 5 = %d\n", err_cnt);

		*handshake = 0x4444;
		asm volatile ("fence w, w");

		while (1);
	}

	return;
} 