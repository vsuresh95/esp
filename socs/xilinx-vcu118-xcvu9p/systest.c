// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

void thread_entry (int cid, int nc)
{
	return;
}

uint64_t amo_swap (volatile uint64_t* handshake, uint64_t value) {
	uint64_t old_val;

	asm volatile (
		"mv t0, %2;"
		"mv t2, %1;"
		"amoswap.d.aqrl t1, t0, (t2);"
		"mv %0, t1"
		: "=r" (old_val)
		: "r" (handshake), "r" (value)
		: "t0", "t1", "t2", "memory"
	);
	
	return old_val;
}

void amo_add (volatile uint64_t* handshake, uint64_t value) {
	asm volatile (
		"mv t0, %1;"
		"mv t2, %0;"
		"amoadd.d.aqrl t1, t0, (t2);"
		:
		: "r" (handshake), "r" (value)
		: "t0", "t1", "t2", "memory"
	);
}

void self_inval () {
	volatile uint64_t* handshake = (volatile uint64_t*) 0x90000020;

	// dummy lr to generate self-invalidate
	asm volatile (
		"li t0, 0;"
		"mv t1, %0;"
		"lr.d.aq t0, (t1)"
		:
		: "r" (handshake)
	);
}

void wb_flush () {
	volatile uint64_t* handshake = (volatile uint64_t*) 0x90000030;

	// dummy sc to generate WB flush
	asm volatile (
		"li t0, 0;"
		"li t1, 1;"
		"mv t2, %0;"
		"sc.d.rl t0, t1, (t2)"
		:
		: "r" (handshake)
	);
}

#define BUF_LENGTH 100
#define N_CPU 2

int main (int argc, char **argv)
{
    uint64_t hartid;
	
	// read hart ID
	asm volatile (
		"csrr %0, mhartid"
		: "=r" (hartid)
	);

	// set up two buffer locations
	volatile uint64_t* buffer1 = (volatile uint64_t*) 0x90000000;
	volatile uint64_t* buffer2 = (volatile uint64_t*) 0x90010000;

	// test finish barrier
	volatile uint64_t* finish = (volatile uint64_t*) 0x90020000;

	// lock location
	volatile uint64_t* lock = (volatile uint64_t*) 0x90020080;

	// swapped value of lock
	uint64_t old_val;

	// acquire the lock
	while (1) {
		if (*lock != 1) {
			old_val = amo_swap (lock, 1); 

			if (old_val == 0) break;
		} 
	}

	// printf ("H%0d\n", hartid); 

	// first processor will set up the buffer
	if (*finish > N_CPU || *finish == 0) {
		*finish = 0;

		for (int i = 0; i < BUF_LENGTH; i++) {
			buffer1[i] = i;
			buffer2[i] = 2*i;
		}
	}

	// perform critical section, add two buffers
	for (int i = 0; i < BUF_LENGTH; i++) {
		buffer1[i] += buffer2[i];
	} 

	// each processor will add finish by 1
	amo_add (finish, 1);

	// release the lock
	old_val = amo_swap (lock, 0);

	while (*finish != N_CPU);

	if (hartid == 0) {

		int err_cnt = 0;

		// validate the result
		for (int i = 0; i < BUF_LENGTH; i++) {
			if (buffer1[i] != (2*N_CPU + 1)*i) {
				err_cnt++;
			}
		}

		if (err_cnt) {
			printf ("FAIL %0d\n", err_cnt);
		} else {
			printf ("PASS\n");
		}
	} else {
		while (1);
	}

	return 0;
} 