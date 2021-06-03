// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <00_systest_helper.h>

extern void simple_reqv ();
extern void simple_reqwt ();
extern void simple_reqwt_compare ();

int main (int argc, char **argv)
{
	uint64_t hartid;

	// read hart ID
	asm volatile (
		"csrr %0, mhartid"
		: "=r" (hartid)
	);

	printf ("%0d: Start T%0d\n", hartid, TEST_ID);

	switch (TEST_ID) {
		case 3:
			simple_reqwt_compare();
			break;
		case 2:
			simple_reqwt();
			break;
		default:
			simple_reqv();
			break;
	}

	return 0;
}