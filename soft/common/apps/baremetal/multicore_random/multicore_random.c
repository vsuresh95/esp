/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <helper.h>

static unsigned first = 0;

static void checkpoint_update(volatile unsigned* checkpoint, unsigned n_threads) {
	unsigned checkpoint_val = amo_add(checkpoint, 1);

	while(*checkpoint != n_threads);
	if (checkpoint_val == n_threads - 1) {
		*checkpoint = 0;
	}
}

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

	unsigned errors = 0;

	multicore_print("[HART %d TEST %d] Hello from ESP!\n", hartid, t_id);

	if (amo_swap(&first, 1) == 1) {
		checkpoint_update(checkpoint, n_threads);
	} else {
		*checkpoint = 1;
		while(*checkpoint != n_threads);
		*checkpoint = 0;
	}

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

			checkpoint_update(checkpoint, n_threads);

			// CP 2 : All cores will test the values of the array
			for (unsigned i = ((hartid + 1) % n_threads); i < buffer1_size; i+=n_threads) {
				if (buffer1[i] != ((i % n_threads + 1) * i)) {
					printf("[HART %d] %d E = %d A = %d\n", hartid, i, ((i % n_threads + 1) * i), buffer1[i]);
					errors++;
				}
			}
			
			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			checkpoint_update(checkpoint, n_threads);

			// CP 3 : Each core amo_add a value to alternating elements of an array.
			for (unsigned i = hartid; i < buffer1_size; i+=n_threads) {
				amo_add(buffer1+i, (hartid + 1) * i);
			}

			checkpoint_update(checkpoint, n_threads);

			// CP 4 : All cores will test the values of the array
			errors = 0;

			for (unsigned i = ((hartid + 1) % n_threads); i < buffer1_size; i+=n_threads) {
				if (buffer1[i] != ((i % n_threads + 1) * 2 * i)) {
					printf("[HART %d] %d E = %d A = %d\n", hartid, i, ((i % n_threads + 1) * 2 * i), buffer1[i]);
					errors++;
				}
			}

			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			checkpoint_update(checkpoint, n_threads);
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

			checkpoint_update(checkpoint, n_threads);

			// CP 2 : All cores will test the values of the array
			for (unsigned i = ((hartid + 1) % n_threads); i < buffer1_size; i+=n_threads) {
				if (buffer1[i * llc_way_word_offset] != ((i % n_threads + 1) * i)) {
					printf("[HART %d] %d E = %d A = %d\n", hartid, i, ((i % n_threads + 1) * i), buffer1[i * llc_way_word_offset]);
					errors++;
				}
			}

			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			checkpoint_update(checkpoint, n_threads);

			// CP 3 : Each core amo_add a value to alternating elements of an array.
			for (unsigned i = hartid; i < buffer1_size; i+=n_threads) {
				amo_add(buffer1 + (i * llc_way_word_offset), (hartid + 1) * i);
			}

			checkpoint_update(checkpoint, n_threads);

			// CP 4 : All cores will test the values of the array
			for (unsigned i = ((hartid + 1) % n_threads); i < buffer1_size; i+=n_threads) {
				if (buffer1[i * llc_way_word_offset] != ((i % n_threads + 1) * 2 * i)) {
					printf("[HART %d] %d E = %d A = %d\n", hartid, i, ((i % n_threads + 1) * 2 * i), buffer1[i * llc_way_word_offset]);
					errors++;
				}
			}

			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			checkpoint_update(checkpoint, n_threads);
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

			checkpoint_update(checkpoint, n_threads);

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

			checkpoint_update(checkpoint, n_threads);
		}
		break;
		case 3:
		{
            // Reference buffer
            volatile unsigned* r_buffer = (volatile unsigned*) 0x91020000;

            // Test buffer
            volatile unsigned* t_buffer = (volatile unsigned*) 0x91040000;

			// Buffer lock
			volatile unsigned* buf_lock = (volatile unsigned*) 0x91060000;

	        volatile unsigned* test_fail = (volatile unsigned*) 0x90010120;

            const unsigned n_elem = RAND_MAX;

            const unsigned LOAD = 0;
            const unsigned STORE = 1;
            const unsigned AMO = 2;
            const unsigned LRSC = 3;

            unsigned op_count = 0;

            const unsigned elem_per_lock = 4;

			const unsigned llc_set_offset = 8;

			// Zero initialize the buffers.
			for (unsigned i = 0; i < n_elem/n_threads; i++) {
				unsigned init_offset = (hartid*n_elem/n_threads) + i;
				r_buffer[init_offset << llc_set_offset] = 0;
				t_buffer[init_offset << llc_set_offset] = 0;
			}

			// Zero initialize the locks
			for (unsigned i = 0; i < (n_elem/n_threads)/elem_per_lock; i++) {
				unsigned init_offset = ((hartid*n_elem/n_threads)/elem_per_lock) + i;
				buf_lock[init_offset] = 0;
			}

            *test_fail = 0;

			checkpoint_update(checkpoint, n_threads);

            while (1) {
                // Exit if test has failed.
                if (*test_fail == 1) break;

                // Randomly perform load/store/AMO/LR-SC
                unsigned op = rand(hartid) % 4;

                if (op == LOAD) {
                    unsigned ld_offset = rand(hartid);
                    unsigned ld_lock_offset = ld_offset/elem_per_lock;

                    // Read test and reference value
                    acquire_lock(&buf_lock[ld_lock_offset]);
                    unsigned t_value = t_buffer[ld_offset << llc_set_offset];
                    unsigned r_value = r_buffer[ld_offset << llc_set_offset];
                    release_lock(&buf_lock[ld_lock_offset]);

                    // Test if they are equal
                    if (t_value != r_value) {
                        *test_fail = 1;
                        printf("[HART %d OP %d] T = 0x%x R = 0x%x\n", hartid, op_count, t_value, r_value);
                    }

                    op_count++;
                } else if (op == STORE) {
                    unsigned st_offset = rand(hartid);
                    unsigned st_value = rand(hartid);
                    unsigned st_lock_offset = st_offset/elem_per_lock;

                    // Update test and reference value
                    acquire_lock(&buf_lock[st_lock_offset]);
                    t_buffer[st_offset << llc_set_offset] = st_value;
                    r_buffer[st_offset << llc_set_offset] = st_value;
                    release_lock(&buf_lock[st_lock_offset]);

                    op_count++;
                } else if (op == AMO) {
                    unsigned amo_offset = rand(hartid);
                    unsigned amo_value = rand(hartid);
                    unsigned amo_lock_offset = amo_offset/elem_per_lock;

                    // Atomic update test and reference value
                    acquire_lock(&buf_lock[amo_lock_offset]);
                    unsigned t_value = amo_swap(&t_buffer[amo_offset << llc_set_offset], amo_value);
                    unsigned r_value = amo_swap(&r_buffer[amo_offset << llc_set_offset], amo_value);
                    release_lock(&buf_lock[amo_lock_offset]);

                    // Test if the old values are equal
                    if (t_value != r_value) {
                        *test_fail = 1;
                        printf("[HART %d OP %d] T = 0x%x R = 0x%x\n", hartid, op_count, t_value, r_value);
                    }

                    op_count++;					
                } else if (op == LRSC) {
                    unsigned lrsc_offset = rand(hartid);
                    unsigned lrsc_value = rand(hartid);
                    unsigned lrsc_lock_offset = lrsc_offset/elem_per_lock;

                    // Update test and reference value acquiring lock with LR-SC
                    acquire_lock_lr_sc(&buf_lock[lrsc_lock_offset]);
                    t_buffer[lrsc_offset << llc_set_offset] = lrsc_value;
                    r_buffer[lrsc_offset << llc_set_offset] = lrsc_value;
                    release_lock(&buf_lock[lrsc_lock_offset]);

                    op_count++;							
                }

                if (op_count % 100 == 0) {
                    if (hartid == 0) {
                        printf("[HART 0] %d OP DONE!\n", op_count);
                    }

					checkpoint_update(checkpoint, n_threads);
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