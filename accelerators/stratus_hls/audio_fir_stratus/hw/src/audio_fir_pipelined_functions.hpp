// Copyright (c) 2011-2019 Columbia University, System Level Design Group
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

// These values are the same whether FFT or iFFT (i.e. indep of do_inverse)
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

inline void audio_fir::input_load_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("input-load-start-handshake");

        input_load_start.req.req();
    }
}

inline void audio_fir::load_input_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-input-start-handshake");

        input_load_start.ack.ack();
    }
}

inline void audio_fir::input_store_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("input-store-start-handshake");

        input_store_start.req.req();
    }
}

inline void audio_fir::store_input_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-input-start-handshake");

        input_store_start.ack.ack();
    }
}

//////////////////////////////////////////////////////
// FILTERS -> LOAD/STORE START HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_fir::filters_load_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("filters-load-start-handshake");

        filters_load_start.req.req();
    }
}

inline void audio_fir::load_filters_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-filters-start-handshake");

        filters_load_start.ack.ack();
    }
}

inline void audio_fir::filters_store_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("filters-store-start-handshake");

        filters_store_start.req.req();
    }
}

inline void audio_fir::store_filters_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-filters-start-handshake");

        filters_store_start.ack.ack();
    }
}

//////////////////////////////////////////////////////
// OUTPUT -> LOAD/STORE START HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_fir::output_load_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-load-start-handshake");

        output_load_start.req.req();
    }
}

inline void audio_fir::load_output_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-output-start-handshake");

        output_load_start.ack.ack();
    }
}

inline void audio_fir::output_store_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-store-start-handshake");

        output_store_start.req.req();
    }
}

inline void audio_fir::store_output_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-output-start-handshake");

        output_store_start.ack.ack();
    }
}

//////////////////////////////////////////////////////
// LOAD/STORE -> INPUT DONE HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_fir::input_load_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("input-load-done-handshake");

        load_input_done.ack.ack();
    }
}

inline void audio_fir::load_input_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-input-done-handshake");

        load_input_done.req.req();
    }
}

inline void audio_fir::input_store_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("input-store-done-handshake");

        store_input_done.ack.ack();
    }
}

inline void audio_fir::store_input_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-input-done-handshake");

        store_input_done.req.req();
    }
}

//////////////////////////////////////////////////////
// LOAD/STORE -> FILTERS DONE HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_fir::filters_load_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("filters-load-done-handshake");

        load_filters_done.ack.ack();
    }
}

inline void audio_fir::load_filters_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-filters-done-handshake");

        load_filters_done.req.req();
    }
}

inline void audio_fir::filters_store_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("filters-store-done-handshake");

        store_filters_done.ack.ack();
    }
}

inline void audio_fir::store_filters_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-filters-done-handshake");

        store_filters_done.req.req();
    }
}

//////////////////////////////////////////////////////
// LOAD/STORE -> OUTPUT DONE HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_fir::output_load_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-load-done-handshake");

        load_output_done.ack.ack();
    }
}

inline void audio_fir::load_output_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-output-done-handshake");

        load_output_done.req.req();
    }
}

inline void audio_fir::output_store_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-store-done-handshake");

        store_output_done.ack.ack();
    }
}

inline void audio_fir::store_output_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-output-done-handshake");

        store_output_done.req.req();
    }
}

//////////////////////////////////////////////////////
// ASI <-> COMPUTE HANDSHAKES
//////////////////////////////////////////////////////

inline void audio_fir::input_compute_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("input-compute-handshake");

        input_to_compute.req.req();
    }
}

inline void audio_fir::compute_input_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-input-handshake");

        input_to_compute.ack.ack();
    }
}

inline void audio_fir::filters_compute_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("filters-compute-handshake");

        filters_to_compute.req.req();
    }
}

inline void audio_fir::compute_filters_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-filters-handshake");

        filters_to_compute.ack.ack();
    }
}

inline void audio_fir::compute_output_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-output-handshake");

        compute_to_output.req.req();
    }
}

inline void audio_fir::output_compute_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-compute-handshake");

        compute_to_output.ack.ack();
    }
}