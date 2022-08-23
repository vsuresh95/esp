/*
 * Copyright (c) 2011-2019 Columbia University, System Level Design Group
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_FIR_H
#define TEST_FIR_H

#include <stdbool.h>
#include <limits.h>
#include <math.h>

void fir_do_shift(float *A0, unsigned int offset, unsigned int num_samples, unsigned int bits);
unsigned int fir_rev(unsigned int v);
void fir_bit_reverse(float *w, unsigned int offset, unsigned int n, unsigned int bits);
int  fir_comp(float *data, unsigned nffts, unsigned int n, unsigned int logn, int do_inverse, int do_shift);


#endif /* TEST_FIR_H */
