// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

// complex number multiplication
inline void compMul(const CompNum &x, const CompNum &y, CompNum &res)
{
    res.re = x.re * y.re - x.im * y.im;
    res.im = x.re * y.im + x.im * y.re;
}

// complex number addition
inline void compAdd(const CompNum &x, const CompNum &y, CompNum &res)
{
    res.re = x.re + y.re;
    res.im = x.im + y.im;
}

// complex number substraction
inline void compSub(const CompNum &x, const CompNum &y, CompNum &res)
{
    res.re = x.re - y.re;
    res.im = x.im - y.im;
}

// bit reverse
inline unsigned int fft2_rev(unsigned int v)
{
    unsigned int r = v;
    int s = 31;
    int i;

    for (i = 0; i < 31; i++) {
        HLS_UNROLL_N(8, "fft2-rev-unroll");
        v >>= 1;
        if (v != 0) {
            r <<= 1;
            r |= v & 1;
            s--;
        }
    }

    r <<= s;

    return r;
}

inline void audio_ffi::fft2_bit_reverse(unsigned int offset, unsigned int n, unsigned int bits)
{
	unsigned int i, s, shift;
        s = 31;
        shift = s - bits + 1;
        //printf(" BIT_REV offset %u  n %u bits %u\n", offset, n, bits);
        for (i = 0; i < n; i++) {
            unsigned int r;
            FPDATA_WORD t1_real, t1_imag;
            FPDATA_WORD t2_real, t2_imag;

            r = fft2_rev(i);
            r >>= shift;

            unsigned int iidx = 2*(offset + i);
            unsigned int ridx = 2*(offset + r);
            //printf("   BR : offset %u i %u r %u : iidx %u ridx %u\n", offset, i, r, iidx, ridx);

            wait();
            t1_real = A0[iidx];
            t1_imag = A0[iidx + 1];
            wait();
            t2_real = A0[ridx];
            t2_imag = A0[ridx + 1];

            if (i < r) {
                HLS_PROTO("bit-rev-memwrite");
                HLS_BREAK_DEP(A0);
                wait();
                A0[iidx] = t2_real;
                A0[iidx + 1] = t2_imag;
                wait();
                A0[ridx] = t1_real;
                A0[ridx + 1] = t1_imag;
            }
        }
        //printf(" ... BIT_REV returns...\n");
}



inline void audio_ffi::fft2_do_shift(unsigned int offset, unsigned int num_samples, unsigned int logn_samples)
{
    int md = (num_samples/2);
    for(unsigned oi = 0; oi < md; oi++) {
        unsigned int iidx = 2*(offset + oi);
        unsigned int midx = 2*(offset + md + oi);
        //printf("ACCEL: DO_SHIFT: offset %u : iidx %u %u midx %u %u\n", offset, iidx, (iidx+1), midx, (midx+1));

        FPDATA_WORD ti_real, ti_imag;
        FPDATA_WORD tim_real, tim_imag;
        wait();
        ti_real = A0[iidx];
        ti_imag = A0[iidx + 1];
        wait();
        tim_real = A0[midx];
        tim_imag = A0[midx + 1];
        {
            HLS_PROTO("do-shift-memwrite");
            HLS_BREAK_DEP(A0);
            wait();
            A0[iidx]     = tim_real;
            A0[iidx + 1] = tim_imag;
            wait();
            A0[midx]     = ti_real;
            A0[midx + 1] = ti_imag;
        }
    }
}


// These values are the same whether audio_ffi or iaudio_ffi (i.e. indep of do_inverse)
inline FPDATA myCos(int m)
{
	switch(m) {
	case 1: return 2;
	case 2: return 0.999999940395355;
	case 3: return 0.292893260717392;
	case 4: return 0.0761204659938812;
	case 5: return 0.0192147195339203;
	case 6: return 0.00481527345255017;
	case 7: return 0.00120454386342317;
	case 8: return 0.000301181309623644;
	case 9: return 7.52981650293805e-05;
	case 10: return 1.88247176993173e-05;
	case 11: return 4.70619079351309e-06;
	case 12: return 1.1765483804993e-06;
	case 13: return 2.94137151968243e-07;
	case 14: return 7.35342879920609e-08;
	case 15: return 1.83835719980152e-08;
	case 16: return 4.5958929995038e-09;
	default: return 0.0;
	}
}

// These are the SIGN=-1 values, use negative of these for SIGN = 1 (inverse)
inline FPDATA mySin(int m)
{
	switch(m) {
	case 1: return 8.74227765734759e-08;
	case 2: return -1;
	case 3: return -0.70710676908493;
	case 4: return -0.382683455944061;
	case 5: return -0.1950903236866;
	case 6: return -0.0980171412229538;
	case 7: return -0.0490676760673523;
	case 8: return -0.0245412290096283;
	case 9: return -0.0122715383768082;
	case 10: return -0.00613588467240334;
	case 11: return -0.00306795677170157;
	case 12: return -0.00153398024849594;
	case 13: return -0.000766990357078612;
	case 14: return -0.000383495207643136;
	case 15: return -0.000191747603821568;
	case 16: return -9.58738019107841e-05;
        default:  return 0.0;
	}
}


inline FPDATA myInvCos(int m)
{
	switch(m) {
	case 1: return 2;
	case 2: return 0.999999940395355;
	case 3: return 0.292893260717392;
	case 4: return 0.0761204659938812;
	case 5: return 0.0192147195339203;
	case 6: return 0.00481527345255017;
	case 7: return 0.00120454386342317;
	case 8: return 0.000301181309623644;
	case 9: return 7.52981650293805e-05;
	case 10: return 1.88247176993173e-05;
	case 11: return 4.70619079351309e-06;
	case 12: return 1.1765483804993e-06;
	case 13: return 2.94137151968243e-07;
	case 14: return 7.35342879920609e-08;
	case 15: return 1.83835719980152e-08;
	case 16: return 4.5958929995038e-09;
	default: return 0.0;
	}
}

inline FPDATA myInvSin(int m)
{
	switch(m) {
	case 1: return 8.74227765734759e-08;
	case 2: return -1;
	case 3: return -0.70710676908493;
	case 4: return -0.382683455944061;
	case 5: return -0.1950903236866;
	case 6: return -0.0980171412229538;
	case 7: return -0.0490676760673523;
	case 8: return -0.0245412290096283;
	case 9: return -0.0122715383768082;
	case 10: return -0.00613588467240334;
	case 11: return -0.00306795677170157;
	case 12: return -0.00153398024849594;
	case 13: return -0.000766990357078612;
	case 14: return -0.000383495207643136;
	case 15: return -0.000191747603821568;
	case 16: return -9.58738019107841e-05;
        default:  return 0.0;
	}
}

//////////////////////////////////////////////////////
// INPUT -> LOAD/STORE START HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_ffi::input_load_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("input-load-start-handshake");

        input_load_start.req.req();
    }
}

inline void audio_ffi::load_input_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-input-start-handshake");

        input_load_start.ack.ack();
    }
}

inline void audio_ffi::input_store_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("input-store-start-handshake");

        input_store_start.req.req();
    }
}

inline void audio_ffi::store_input_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-input-start-handshake");

        input_store_start.ack.ack();
    }
}

//////////////////////////////////////////////////////
// OUTPUT -> LOAD/STORE START HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_ffi::output_load_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-load-start-handshake");

        output_load_start.req.req();
    }
}

inline void audio_ffi::load_output_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-output-start-handshake");

        output_load_start.ack.ack();
    }
}

inline void audio_ffi::output_store_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-store-start-handshake");

        output_store_start.req.req();
    }
}

inline void audio_ffi::store_output_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-output-start-handshake");

        output_store_start.ack.ack();
    }
}

//////////////////////////////////////////////////////
// LOAD/STORE -> INPUT DONE HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_ffi::input_load_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("input-load-done-handshake");

        load_input_done.ack.ack();
    }
}

inline void audio_ffi::load_input_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-input-done-handshake");

        load_input_done.req.req();
    }
}

inline void audio_ffi::input_store_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("input-store-done-handshake");

        store_input_done.ack.ack();
    }
}

inline void audio_ffi::store_input_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-input-done-handshake");

        store_input_done.req.req();
    }
}

//////////////////////////////////////////////////////
// LOAD/STORE -> OUTPUT DONE HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_ffi::output_load_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-load-done-handshake");

        load_output_done.ack.ack();
    }
}

inline void audio_ffi::load_output_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-output-done-handshake");

        load_output_done.req.req();
    }
}

inline void audio_ffi::output_store_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-store-done-handshake");

        store_output_done.ack.ack();
    }
}

inline void audio_ffi::store_output_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-output-done-handshake");

        store_output_done.req.req();
    }
}

//////////////////////////////////////////////////////
// ASI <-> COMPUTE HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_ffi::input_fft_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("input-fft-handshake");

        input_to_fft.req.req();
    }
}

inline void audio_ffi::fft_input_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("fft-input-handshake");

        input_to_fft.ack.ack();
    }
}

inline void audio_ffi::fft_fir_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("fft-fir-handshake");

        fft_to_fir.req.req();
    }
}

inline void audio_ffi::fir_fft_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("fir-fft-handshake");

        fft_to_fir.ack.ack();
    }
}

inline void audio_ffi::fir_ifft_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("fir-ifft-handshake");

        fir_to_ifft.req.req();
    }
}

inline void audio_ffi::ifft_fir_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("ifft-fir-handshake");

        fir_to_ifft.ack.ack();
    }
}

inline void audio_ffi::ifft_output_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("ifft-output-handshake");

        ifft_to_store.req.req();
    }
}

inline void audio_ffi::output_ifft_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-ifft-handshake");

        ifft_to_store.ack.ack();
    }
}
