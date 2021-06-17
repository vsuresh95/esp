// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <00_systest_helper.h>

#define N_CPU 2
#define BUF_LENGTH 500

#define STR(x) #x
#define XSTR(x) STR(x)
#pragma message "Inside test = " XSTR(TEST_ID)

void riscv_multicore_reqv ()
{
    uint64_t hartid;
    uint64_t buffer1_val;
    uint64_t buffer2_val;
    static uint64_t print_lock;
    static uint64_t first;
    static uint64_t finish;
    static uint64_t buffer1[BUF_LENGTH];
    static uint64_t buffer2[BUF_LENGTH];
    static uint64_t lock;

	// read hart ID
	hartid = read_hartid ();

    for (int i = 0; i < 10; i++) {
        // acquire the lock
        spin_for_lock (&lock);

    	printf ("%0d: Entered\n", hartid); 

        if (first != 1) {
    		for (uint64_t i = 0; i < BUF_LENGTH; i++) {
    			// write the initial value of 'i' to buffer1[i] and '2i' to buffer2[i]
    			write_dword_fcs(buffer1+i, i, false, false);
    			write_dword_fcs(buffer2+i, 2*i, false, false);
    		}

            first = 1;
            finish = 0;
        }

    	// perform critical section, add two buffers
    	for (uint64_t i = 0; i < BUF_LENGTH; i++) {
    		// read from buffer1 & buffer2, and write the write the sum to buffer1
    		buffer1_val = read_dword_fcs(buffer1+i, true, false);
    		buffer2_val = read_dword_fcs(buffer2+i, true, false);
    		write_dword_fcs(buffer1+i, buffer1_val + buffer2_val, false, false);
        }

    	printf ("%0d: Finished\n", hartid); 

        finish++;

        // release the lock
        release_lock (&lock);

    	// barrier for all to finish
    	while (finish != N_CPU);

    	if (hartid == 0) {
    		int err_cnt = 0;

    	    printf ("%0d: Check\n", hartid);

    		// validate the result
    		for (uint64_t i = 0; i < BUF_LENGTH; i++) {
    			buffer1_val = read_dword_fcs(buffer1+i, false, false);
    			if (buffer1_val != (2*N_CPU + 1)*i) {
    				err_cnt++;
    			}
    		}

    		if (err_cnt)
    			printf ("FAIL %0d\n", err_cnt);
    		else
    			printf ("PASS\n");

            first = 0;

            finish++;
    	}

    	// barrier for validation to finish
    	while (finish != N_CPU+1);

    	if (hartid == 0) {
            printf ("Test complete!\n");
        }
    }

	return;
} 