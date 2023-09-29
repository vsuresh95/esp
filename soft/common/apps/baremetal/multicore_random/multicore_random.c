/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <helper.h>

int main(int argc, char * argv[])
{
	const unsigned n_threads = NUM_THREADS;
	const unsigned t_id = TEST_ID;
 	unsigned hartid;

 	asm volatile (
 		"csrr %0, mhartid"
 		: "=r" (hartid)
 	);
	
	volatile unsigned* lock = (volatile unsigned*) 0x90010020;
	volatile unsigned* checkpoint = (volatile unsigned*) 0x90010030;
	volatile unsigned* first = (volatile unsigned*) 0x90010040;

	unsigned errors = 0;

	multicore_print("[HART %d TEST %d] Hello from ESP!\n", hartid, t_id);

	if (amo_swap(first, 1) == 1) {
		amo_add(checkpoint, 1);
	} else {
		*checkpoint = 1;
	}

	while(*checkpoint != n_threads);
	*checkpoint = 0;

	switch (t_id) {
		// Each core writes to alternating elements of an array, and each
		// core reads back different alternating elements. This is repeated
		// with AMO add for write.
		case 0:
		{
			volatile unsigned* buffer1 = (volatile unsigned*) 0x90020000;
			const unsigned buffer1_size = 64 * n_threads;

			// CP 1 : Each core write to alternating elements of an array.
			errors = 0;

			for (unsigned i = hartid; i < buffer1_size; i+=n_threads) {
				buffer1[i] = (hartid + 1) * i;
			}

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;

			// CP 2 : All cores will test the values of the array
			for (unsigned i = ((hartid + 1) % n_threads); i < buffer1_size; i+=n_threads) {
				if (buffer1[i] != ((i % n_threads + 1) * i)) {
					printf("[HART %d] %d E = %d A = %d\n", hartid, i, ((i % n_threads + 1) * i), buffer1[i]);
					errors++;
				}
			}
			
			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;

			// CP 3 : Each core amo_add a value to alternating elements of an array.
			for (unsigned i = hartid; i < buffer1_size; i+=n_threads) {
				amo_add(buffer1+i, (hartid + 1) * i);
			}

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;

			// CP 4 : All cores will test the values of the array
			errors = 0;

			for (unsigned i = ((hartid + 1) % n_threads); i < buffer1_size; i+=n_threads) {
				if (buffer1[i] != ((i % n_threads + 1) * 2 * i)) {
					printf("[HART %d] %d E = %d A = %d\n", hartid, i, ((i % n_threads + 1) * 2 * i), buffer1[i]);
					errors++;
				}
			}

			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;
		}
		break;
		// Each core writes to a different line in the LLC set - eventually causing eviction.
		// We conservatively assume the largest LLC set size of 16.
		case 1:
		{
			const unsigned llc_way_bits = 10;
			const unsigned llc_way_offset = (1 << llc_way_bits);
			const unsigned llc_way_word_offset = llc_way_offset/sizeof(unsigned);

			volatile unsigned* buffer1 = (volatile unsigned*) 0x90020000;
			const unsigned buffer1_size = 64 * n_threads;

			// CP 1 : Each core write to alternating elements of an array.
			errors = 0;

			// Core 0 writes to 0th way, core 1st way, and so on.
			for (unsigned i = hartid; i < buffer1_size; i+=n_threads) {
				buffer1[i * llc_way_word_offset] = (hartid + 1) * i;
			}

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;

			// CP 2 : All cores will test the values of the array
			for (unsigned i = ((hartid + 1) % n_threads); i < buffer1_size; i+=n_threads) {
				if (buffer1[i * llc_way_word_offset] != ((i % n_threads + 1) * i)) {
					printf("[HART %d] %d E = %d A = %d\n", hartid, i, ((i % n_threads + 1) * i), buffer1[i * llc_way_word_offset]);
					errors++;
				}
			}

			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;

			// CP 3 : Each core amo_add a value to alternating elements of an array.
			for (unsigned i = hartid; i < buffer1_size; i+=n_threads) {
				amo_add(buffer1 + (i * llc_way_word_offset), (hartid + 1) * i);
			}

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;

			// CP 4 : All cores will test the values of the array
			for (unsigned i = ((hartid + 1) % n_threads); i < buffer1_size; i+=n_threads) {
				if (buffer1[i * llc_way_word_offset] != ((i % n_threads + 1) * 2 * i)) {
					printf("[HART %d] %d E = %d A = %d\n", hartid, i, ((i % n_threads + 1) * 2 * i), buffer1[i * llc_way_word_offset]);
					errors++;
				}
			}

			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;			
		}
		break;
		// Each core executes a critical section guarded by LR-SC synchronizations.
		case 2:
		{
			const unsigned llc_way_bits = 10;
			const unsigned llc_way_offset = (1 << llc_way_bits);
			const unsigned llc_way_word_offset = llc_way_offset/sizeof(unsigned);

			volatile unsigned* buffer1 = (volatile unsigned*) 0x90020000;
			const unsigned buffer1_size = 8 * n_threads;
			const unsigned num_iters = 64;

			// CP 1 : Each core write to alternating elements of an array.
			errors = 0;

			// Core 0 writes to 0th way, core 1st way, and so on.
			for (unsigned i = 0; i < num_iters; i++) {
				acquire_lock_lr_sc(lock);
				for (unsigned j = hartid; j < buffer1_size; j+=n_threads) {
					buffer1[j * llc_way_word_offset + i] = (hartid + 1) * j;
				}
				release_lock(lock);
			}

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;

			// CP 2 : All cores will test the values of the array
			for (unsigned i = 0; i < num_iters; i++) {
				acquire_lock_lr_sc(lock);
				for (unsigned j = ((hartid + 1) % n_threads); j < buffer1_size; j+=n_threads) {
					if (buffer1[j * llc_way_word_offset + i] != ((j % n_threads + 1) * j)) {
						printf("[HART %d] %d %d E = %d A = %d\n", hartid, i, j, ((j % n_threads + 1) * j), buffer1[j * llc_way_word_offset + i]);
						errors++;
					}
				}
				release_lock(lock);
			}

			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;			
		}
		break;
		case 3:
		{
            // Reference buffer
            volatile unsigned* r_buffer = (volatile unsigned*) 0x91000000;

            // Test buffer
            volatile unsigned* t_buffer = (volatile unsigned*) 0x92000000;

	        volatile unsigned* test_fail = (volatile unsigned*) 0x90010120;

            const unsigned n_elem = RAND_MAX;

            const unsigned LOAD = 0;
            const unsigned STORE = 1;
            const unsigned AMO = 2;

            unsigned op_count = 0;

			// Zero initialize the buffers.
			for (unsigned i = 0; i < n_elem/n_threads; i++) {
				r_buffer[(hartid*n_elem/n_threads) + i] = 0;
				t_buffer[(hartid*n_elem/n_threads) + i] = 0;
			}

            *test_fail = 0;
            *lock = 0;

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;

            while (1) {
                // Exit if test has failed.
                if (*test_fail == 1) break;

                // Randomly perform load/store/AMO
                unsigned op = rand(hartid) % 3;

                if (op == LOAD) {
                    unsigned ld_offset = rand(hartid);

                    // Read test and reference value
                    unsigned t_value = t_buffer[ld_offset];
                    unsigned r_value = r_buffer[ld_offset];

                    // Test if they are equal
                    if (t_value != r_value) {
                        *test_fail = 1;
                        printf("[HART %d OP %d] T = 0x%x R = 0x%x\n", hartid, op_count, t_value, r_value);
                    }

                    op_count++;
                } else if (op == STORE) {
                    unsigned st_offset = rand(hartid);
                    unsigned st_value = rand(hartid);

                    // Update test and reference value
                    acquire_lock(lock);
                    t_buffer[st_offset] = st_value;
                    r_buffer[st_offset] = st_value;
                    release_lock(lock);

                    op_count++;
                } else if (op == AMO) {
                    unsigned amo_offset = rand(hartid);
                    unsigned amo_value = rand(hartid);

                    // Atomic update test and reference value
                    unsigned t_value = amo_swap(&t_buffer[amo_offset], amo_value);
                    unsigned r_value = amo_swap(&r_buffer[amo_offset], amo_value);

                    // Test if the old values are equal
                    if (t_value != r_value) {
                        *test_fail = 1;
                        printf("[HART %d OP %d] T = 0x%x R = 0x%x\n", hartid, op_count, t_value, r_value);
                    }

                    op_count++;					
                }

                if (op_count % 100 == 0) {
                    if (hartid == 0) {
                        printf("[HART 0] %d OP DONE!\n", op_count);
                    }

                    amo_add(checkpoint, 1);
                    while(*checkpoint != n_threads);
                    *checkpoint = 0;
                }
            }

            while(1);
        }
        break; 
		default:
		break;
	}

	return 0;
}