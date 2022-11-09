#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "fft2_stratus.h"
#include "../../../../fir_stratus/sw/linux/include/fir_stratus.h"

#if (FFT2_FX_WIDTH == 64)
typedef unsigned long long token_t;
typedef double native_t;
#define fx2float fixed64_to_double
#define float2fx double_to_fixed64
#define FX_IL 42
#elif (FFT2_FX_WIDTH == 32)
typedef int token_t;
typedef float native_t;
#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 4
#endif /* FFT2_FX_WIDTH */

const float ERR_TH = 0.05;

/* <<--params-def-->> */
#define LOGN_SAMPLES 10
//#define NUM_FFTS     46
#define NUM_FFTS     1
//#define LOGN_SAMPLES 12
//#define NUM_FFTS     13
/*#define NUM_SAMPLES (NUM_FFTS * (1 << LOGN_SAMPLES))*/
#define DO_INVERSE   0
#define DO_SHIFT     0
#define SCALE_FACTOR 0

/* <<--params-->> */
const int32_t logn_samples = LOGN_SAMPLES;
const int32_t num_samples = (1 << logn_samples);
const int32_t num_ffts = 1;
const int32_t do_inverse = 0;
const int32_t do_shift = 0;
const int32_t scale_factor = 1;
unsigned len;

unsigned in_words_adj;
unsigned out_words_adj;
unsigned in_len;
unsigned out_len;
unsigned in_size;
unsigned out_size;
unsigned out_offset;
unsigned mem_size;
unsigned acc_offset;
unsigned acc_size;
unsigned sync_size;

#define NACC 1

#define SYNC_VAR_SIZE 4

#define NUM_DEVICES 3

///////////////////////////////////////////////////////////////
// Bare metal math APIs for SW implementation
///////////////////////////////////////////////////////////////
native_t _pow(native_t a, native_t b) {
    native_t c = 1;
    for (int i=0; i<b; i++)
        c *= a;
    return c;
}

native_t _fact(native_t x) {
    native_t ret = 1;
    for (int i=1; i<=x; i++)
        ret *= i;
    return ret;
}

native_t _sin(native_t x) {
    native_t y = x;
    native_t s = -1;
    for (int i=3; i<=100; i+=2) {
        y+=s*(_pow(x,i)/_fact(i));
        s *= -1;
    }
    return y;
}

native_t _cos(native_t x) {
    native_t y = 1;
    native_t s = -1;
    for (int i=2; i<=100; i+=2) {
        y+=s*(_pow(x,i)/_fact(i));
        s *= -1;
    }
    return y;
}

native_t _tan(native_t x) {
     return (_sin(x)/_cos(x));
}

typedef struct {
    float r;
    float i;
} cpx_num;

#   define S_MUL(a,b) ( (a)*(b) )
#define C_MUL(m,a,b) \
    do{ (m).r = (a).r*(b).r - (a).i*(b).i;\
        (m).i = (a).r*(b).i + (a).i*(b).r; }while(0)
#   define C_FIXDIV(c,div) /* NOOP */
#   define C_MULBYSCALAR( c, s ) \
    do{ (c).r *= (s);\
        (c).i *= (s); }while(0)

#define  C_ADD( res, a,b)\
    do { \
        (res).r=(a).r+(b).r;  (res).i=(a).i+(b).i; \
    }while(0)
#define  C_SUB( res, a,b)\
    do { \
        (res).r=(a).r-(b).r;  (res).i=(a).i-(b).i; \
    }while(0)

#  define HALF_OF(x) ((x)*.5)

struct fft2_stratus_access fft2_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.logn_samples = LOGN_SAMPLES,
		.num_ffts = NUM_FFTS,
		.do_inverse = 0,
		.do_shift = DO_SHIFT,
		.scale_factor = SCALE_FACTOR,
		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_FULL,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	}
};

struct fft2_stratus_access fft2_cfg_001[] = {
	{
		/* <<--descriptor-->> */
		.logn_samples = LOGN_SAMPLES,
		.num_ffts = NUM_FFTS,
		.do_inverse = 1,
		.do_shift = DO_SHIFT,
		.scale_factor = SCALE_FACTOR,
		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_FULL,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	}
};

struct fir_stratus_access fir_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.logn_samples = LOGN_SAMPLES,
		.num_firs = NUM_FFTS,
		.do_inverse = 0,
		.do_shift = DO_SHIFT,
		.scale_factor = SCALE_FACTOR,
		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_FULL,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	}
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "fft2_stratus.0",
		.ioctl_req = FFT2_STRATUS_IOC_ACCESS,
		.esp_desc = &(fft2_cfg_000[0].esp),
	}
};

esp_thread_info_t cfg_001[] = {
	{
		.run = true,
		.devname = "fft2_stratus.1",
		.ioctl_req = FFT2_STRATUS_IOC_ACCESS,
		.esp_desc = &(fft2_cfg_001[0].esp),
	}
};

esp_thread_info_t cfg_002[] = {
	{
		.run = true,
		.devname = "fir_stratus.0",
		.ioctl_req = FIR_STRATUS_IOC_ACCESS,
		.esp_desc = &(fir_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
