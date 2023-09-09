/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <helper.h>

int main(int argc, char * argv[])
{
 	unsigned hartid;

 	asm volatile (
 		"csrr %0, mhartid"
 		: "=r" (hartid)
 	);
	
	volatile unsigned* lock = (volatile unsigned*) 0x90010020;
	volatile unsigned* checkpoint = (volatile unsigned*) 0x90010030;
	volatile unsigned* first = (volatile unsigned*) 0x90010040;

	volatile unsigned* buffer1 = (volatile unsigned*) 0x90020000;
	const unsigned buffer1_size = 1024 * NUM_THREADS;

	unsigned errors = 0;

	multicore_print("[HART %d] Hello from ESP!\n", hartid);

	if (amo_swap(first, 1) == 1) {
		amo_add(checkpoint, 1);
	} else {
		*checkpoint = 1;
	}

	while(*checkpoint != NUM_THREADS);
	*checkpoint = 0;

	// CP 1 : Each core write to alternating elements of an array.
	errors = 0;

	for (unsigned i = hartid; i < buffer1_size; i+=NUM_THREADS) {
		buffer1[i] = (hartid + 1) * i;
	}

	amo_add(checkpoint, 1);
	while(*checkpoint != NUM_THREADS);
	*checkpoint = 0;

	// CP 2 : All cores will test the values of the array
	for (unsigned i = ((hartid + 1) % NUM_THREADS); i < buffer1_size; i+=NUM_THREADS) {
		if (buffer1[i] != ((i % NUM_THREADS + 1) * i)) {
			printf("[HART %d] %d E = %d A = %d\n", hartid, i, ((i % NUM_THREADS + 1) * i), buffer1[i]);
			errors++;
		}
	}
	
	multicore_print("[HART %d] Errors = %d\n", hartid, errors);

	amo_add(checkpoint, 1);
	while(*checkpoint != NUM_THREADS);
	*checkpoint = 0;

	// CP 3 : Each core amo_add a value to alternating elements of an array.
	for (unsigned i = hartid; i < buffer1_size; i+=NUM_THREADS) {
		amo_add(buffer1+i, (hartid + 1) * i);
	}

	amo_add(checkpoint, 1);
	while(*checkpoint != NUM_THREADS);
	*checkpoint = 0;

	// CP 4 : All cores will test the values of the array
	errors = 0;

	for (unsigned i = ((hartid + 1) % NUM_THREADS); i < buffer1_size; i+=NUM_THREADS) {
		if (buffer1[i] != ((i % NUM_THREADS + 1) * 2 * i)) {
			printf("[HART %d] %d E = %d A = %d\n", hartid, i, ((i % NUM_THREADS + 1) * i), buffer1[i]);
 			errors++;
		}
	}

	multicore_print("[HART %d] Errors = %d\n", hartid, errors);

	amo_add(checkpoint, 1);
	while(*checkpoint != NUM_THREADS);
	*checkpoint = 0;

	return 0;
}