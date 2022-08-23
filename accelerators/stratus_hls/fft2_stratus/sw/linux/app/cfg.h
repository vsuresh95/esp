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
#define FX_IL 14
#endif /* FFT2_FX_WIDTH */

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
/*const int32_t num_samples = NUM_SAMPLES;*/
const int32_t num_ffts = NUM_FFTS;
const int32_t do_inverse = DO_INVERSE;
const int32_t do_shift = DO_SHIFT;
const int32_t scale_factor = SCALE_FACTOR;

#define NACC 1

#define SYNC_VAR_SIZE 2

#define NUM_DEVICES 3

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
