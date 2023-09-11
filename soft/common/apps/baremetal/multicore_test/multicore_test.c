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
		// Each core executes multiple critical sections guarded by different LR-SC synchronizations and AMO.
		case 3:
		{
			const unsigned llc_way_bits = 10;
			const unsigned llc_way_offset = (1 << llc_way_bits);
			const unsigned llc_way_word_offset = llc_way_offset/sizeof(unsigned);

			const unsigned buffer1_size = 8 * n_threads;
			const unsigned num_iters = 8;
			const unsigned num_tests = 8;

			// Create an array of locks
			volatile unsigned** lock_array = (volatile unsigned**) 0x90010050;

			for (unsigned i = 0; i < num_tests; i++) {
				void* ptr = (void*) (0x90011000 + (i * 0x10));
				lock_array[i] = (volatile unsigned*) (ptr);
			}

			// Create an array of buffers
			volatile unsigned** buffer_array = (volatile unsigned**) 0x90020000;

			for (unsigned i = 0; i < num_tests; i++) {
				void* ptr = (void*) (0x91020000 + (i * 0x1000000));
				buffer_array[i] = (volatile unsigned*) (ptr);
			}

			// CP 1 : Each core write to alternating elements of an array - with AMO.
			errors = 0;

			// Core 0 writes to 0th way, core 1st way, and so on.
			for (unsigned k = 0; k < num_tests; k++) {
				for (unsigned i = 0; i < num_iters; i++) {
					acquire_lock(lock_array[k]);
					for (unsigned j = hartid; j < buffer1_size; j+=n_threads) {
						buffer_array[k][j * llc_way_word_offset + i] = (hartid + 1) * j;
					}
					release_lock(lock_array[k]);
				}
			}

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;

			// CP 2 : All cores will test the values of the array - with AMO.
			for (unsigned k = 0; k < num_tests; k++) {
				for (unsigned i = 0; i < num_iters; i++) {
					acquire_lock(lock_array[k]);
					for (unsigned j = ((hartid + 1) % n_threads); j < buffer1_size; j+=n_threads) {
						if (buffer_array[k][j * llc_way_word_offset + i] != ((j % n_threads + 1) * j)) {
							printf("[HART %d] %d %d %d E = %d A = %d\n", hartid, k, i, j, ((j % n_threads + 1) * j), buffer_array[k][j * llc_way_word_offset + i]);
							errors++;
						}
					}
					release_lock(lock_array[k]);
				}
			}

			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			// CP 3 : Each core write to alternating elements of an array - with LR/SC.
			errors = 0;

			// Core 0 writes to 0th way, core 1st way, and so on.
			for (unsigned k = 0; k < num_tests; k++) {
				for (unsigned i = 0; i < num_iters; i++) {
					acquire_lock_lr_sc(lock_array[k]);
					for (unsigned j = hartid; j < buffer1_size; j+=n_threads) {
						buffer_array[k][j * llc_way_word_offset + i] = (hartid + 1) * j;
					}
					release_lock(lock_array[k]);
				}
			}

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;

			// CP 4 : All cores will test the values of the array - with LR/SC.
			for (unsigned k = 0; k < num_tests; k++) {
				for (unsigned i = 0; i < num_iters; i++) {
					acquire_lock_lr_sc(lock_array[k]);
					for (unsigned j = ((hartid + 1) % n_threads); j < buffer1_size; j+=n_threads) {
						if (buffer_array[k][j * llc_way_word_offset + i] != ((j % n_threads + 1) * j)) {
							printf("[HART %d] %d $d %d E = %d A = %d\n", hartid, k, i, j, ((j % n_threads + 1) * j), buffer_array[k][j * llc_way_word_offset + i]);
							errors++;
						}
					}
					release_lock(lock_array[k]);
				}
			}

			multicore_print("[HART %d] Errors = %d\n", hartid, errors);

			amo_add(checkpoint, 1);
			while(*checkpoint != n_threads);
			*checkpoint = 0;			
		}
		break;
		default:
		break;
	}

	return 0;
}