void golden_data_init(float *gold, float *gold_ref, float *gold_filter, float *gold_twiddle)
{
	int j;
	const float LO = -1.0;
	const float HI = 1.0;

	// srand((unsigned int) time(NULL));

	for (j = 0; j < 2 * len; j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
        // printf("j: %d\n", j);
        // printf("scaling_factor: %f\n", scaling_factor);
        // printf("RAND_MAX: %d\n", RAND_MAX);
		gold[j] = LO + scaling_factor * (HI - LO);
		gold_ref[j] = LO + scaling_factor * (HI - LO);
		// uint32_t ig = ((uint32_t*)gold)[j];
		// printf("  IN[%u] = 0x%08x\n", j, ig);
	}

	for (j = 0; j < 2 * (len+1); j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		gold_filter[j] = LO + scaling_factor * (HI - LO);
		// uint32_t ig = ((uint32_t*)gold_filter)[j];
		// printf("  FLT[%u] = 0x%08x\n", j, ig);
	}

	for (j = 0; j < 2 * len; j+=2) {
        native_t phase = -3.14159265358979323846264338327 * ((native_t) ((j+1) / len) + .5);
        // // YJ: comment out the following 2 lines
        // gold_twiddle[j] = -6.42796e-08; // _cos(phase);
        // gold_twiddle[j + 1] = -1; // _sin(phase);

        // YJ: add the following 2 lines
        gold_twiddle[j] = cos(phase);
        gold_twiddle[j + 1] = sin(phase);

		// uint32_t ig = ((uint32_t*)gold_twiddle)[j];
		// printf("  TWD[%u] = 0x%08x\n", j, ig);
		// ig = ((uint32_t*)gold_twiddle)[j+1];
		// printf("  TWD[%u] = 0x%08x\n", j+1, ig);
	} 
}

void fft_sw_impl(float *gold)
{
	fft2_comp(gold, num_ffts, num_samples, logn_samples, 0 /* do_inverse */, do_shift);
}

void fir_sw_impl(float *gold, float *gold_filter, float *gold_twiddle, float *gold_freqdata)
{
	int j;

    cpx_num fpnk, fpk, f1k, f2k, tw, tdc;
    cpx_num fk, fnkc, fek, fok, tmp;
    cpx_num cptemp;
    cpx_num inv_twd;
    cpx_num *tmpbuf = (cpx_num *) gold;
    cpx_num *freqdata = (cpx_num *) gold_freqdata;
    cpx_num *super_twiddles = (cpx_num *) gold_twiddle;
    cpx_num *filter = (cpx_num *) gold_filter;

	// for (j = 0; j < 2 * len; j++) {
	// 	printf("  1 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold)[j]);
    // }

    // Post-processing
    tdc.r = tmpbuf[0].r;
    tdc.i = tmpbuf[0].i;
    C_FIXDIV(tdc,2);
    freqdata[0].r = tdc.r + tdc.i;
    freqdata[len].r = tdc.r - tdc.i;
    freqdata[len].i = freqdata[0].i = 0;

    for ( j=1;j <= len/2 ; ++j ) {
        fpk    = tmpbuf[j]; 
        fpnk.r =   tmpbuf[len-j].r;
        fpnk.i = - tmpbuf[len-j].i;
        C_FIXDIV(fpk,2);
        C_FIXDIV(fpnk,2);

        C_ADD( f1k, fpk , fpnk );
        C_SUB( f2k, fpk , fpnk );
        C_MUL( tw , f2k , super_twiddles[j-1]);

        freqdata[j].r = HALF_OF(f1k.r + tw.r);
        freqdata[j].i = HALF_OF(f1k.i + tw.i);
        freqdata[len-j].r = HALF_OF(f1k.r - tw.r);
        freqdata[len-j].i = HALF_OF(tw.i - f1k.i);
    }

	// for (j = 0; j < 2 * (len+1); j++) {
	// 	printf("  2 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold_freqdata)[j]);
    // }

    // FIR
	for (j = 0; j < len + 1; j++) {
        C_MUL( cptemp, freqdata[j], filter[j]);
        freqdata[j] = cptemp;
    }

	// for (j = 0; j < 2 * (len+1); j++) {
	// 	printf("  3 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold_freqdata)[j]);
    // }

    // Pre-processing
    tmpbuf[0].r = freqdata[0].r + freqdata[len].r;
    tmpbuf[0].i = freqdata[0].r - freqdata[len].r;
    C_FIXDIV(tmpbuf[0],2);

    for (j = 1; j <= len/2; ++j) {
        fk = freqdata[j];
        fnkc.r = freqdata[len-j].r;
        fnkc.i = -freqdata[len-j].i;
        C_FIXDIV( fk , 2 );
        C_FIXDIV( fnkc , 2 );
        inv_twd = super_twiddles[j-1];
        C_MULBYSCALAR(inv_twd,-1);

        C_ADD (fek, fk, fnkc);
        C_SUB (tmp, fk, fnkc);
        C_MUL (fok, tmp, inv_twd);
        C_ADD (tmpbuf[j],     fek, fok);
        C_SUB (tmpbuf[len-j], fek, fok);
        tmpbuf[len-j].i *= -1;
    }

	// for (j = 0; j < 2 * len; j++) {
	// 	printf("  4 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold)[j]);
    // }
}

void ifft_sw_impl(float *gold)
{
	fft2_comp(gold, num_ffts, num_samples, logn_samples, 1 /* do_inverse */, do_shift);
}
