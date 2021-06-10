// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <00_systest_helper.h>

#define BUF_LENGTH 50
#define N_CPU 2

#define STR(x) #x
#define XSTR(x) STR(x)
#pragma message "Inside test = " XSTR(TEST_ID)

void simple_reqv ()
{
	uint64_t hartid;
	uint64_t old_val;
	uint64_t buffer1_val;
	uint64_t buffer2_val;
	volatile uint64_t* buffer1 = (volatile uint64_t*) 0x90010000;
	volatile uint64_t* buffer2 = (volatile uint64_t*) 0x90018000;
	volatile uint64_t* finish = (volatile uint64_t*) 0x90020000;
	volatile uint64_t* lock = (volatile uint64_t*) 0x90020080;

	// read hart ID
	hartid = read_hartid ();
	
	// acquire the lock
	spin_for_lock (lock);

	printf ("%0d: Entered\n", hartid); 

	// first processor will set up the buffer
	if (read_dword_fcs(finish, true, false) > N_CPU || read_dword_fcs(finish, true, false) == 0) {
		// initialize finish to 0 as first processor
		write_dword_fcs(finish, 0, false, false);

		for (uint64_t i = 0; i < BUF_LENGTH; i++) {
			// write the initial value of 'i' to buffer1[i] and '2i' to buffer2[i]
			write_dword_fcs(buffer1+i, i, false, false);
			write_dword_fcs(buffer2+i, 2*i, false, false);
		}
	}

	// perform critical section, add two buffers
	for (uint64_t i = 0; i < BUF_LENGTH; i++) {
		// read from buffer1 & buffer2, and write the write the sum to buffer1
		buffer1_val = read_dword_fcs(buffer1+i, true, false);
		buffer2_val = read_dword_fcs(buffer2+i, true, false);
		write_dword_fcs(buffer1+i, buffer1_val + buffer2_val, false, false);
	} 

	// each processor will add finish by 1
	amo_add (finish, 1);

	printf ("%0d: Finished\n", hartid); 

	// release the lock
	release_lock (lock);

	// barrier for all to finish
	while (read_dword_fcs(finish, false, false) != N_CPU);

	if (hartid == 0) {
		int err_cnt = 0;

		// validate the result
		for (uint64_t i = 0; i < BUF_LENGTH; i++) {
			buffer1_val = read_dword_fcs(buffer1+i, true, false);
			if (buffer1_val != (2*N_CPU + 1)*i) {
				err_cnt++;
			}
		}

		if (err_cnt)
			printf ("FAIL %0d\n", err_cnt);
		else
			printf ("PASS\n");
	} else {
		while (1);
	}

	return;
} 