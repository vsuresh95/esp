#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "fir_stratus.h"

#if (FIR_FX_WIDTH == 64)
typedef unsigned long long token_t;
typedef double native_t;
#define fx2float fixed64_to_double
#define float2fx double_to_fixed64
#define FX_IL 42
#elif (FIR_FX_WIDTH == 32)
typedef int token_t;
typedef float native_t;
#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 14
#endif /* FIR_FX_WIDTH */

/* <<--params-def-->> */
#define LOGN_SAMPLES 6
//#define NUM_FIRS     46
#define NUM_FIRS     1
//#define LOGN_SAMPLES 12
//#define NUM_FIRS     13
/*#define NUM_SAMPLES (NUM_FIRS * (1 << LOGN_SAMPLES))*/
#define DO_INVERSE   0
#define DO_SHIFT     1
#define SCALE_FACTOR 0

/* <<--params-->> */
const int32_t logn_samples = LOGN_SAMPLES;
/*const int32_t num_samples = NUM_SAMPLES;*/
const int32_t num_firs = NUM_FIRS;
const int32_t do_inverse = DO_INVERSE;
const int32_t do_shift = DO_SHIFT;
const int32_t scale_factor = SCALE_FACTOR;

#define NACC 1

struct fir_stratus_access fir_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.logn_samples = LOGN_SAMPLES,
		.num_firs = NUM_FIRS,
		.do_inverse = DO_INVERSE,
		.do_shift = DO_SHIFT,
		.scale_factor = SCALE_FACTOR,
		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_NONE,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	}
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "fir_stratus.0",
		.ioctl_req = FIR_STRATUS_IOC_ACCESS,
		.esp_desc = &(fir_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
