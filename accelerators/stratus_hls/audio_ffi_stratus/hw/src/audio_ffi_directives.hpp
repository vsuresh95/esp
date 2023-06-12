// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FFI_DIRECTIVES_HPP__
#define __AUDIO_FFI_DIRECTIVES_HPP__

#define DMA_BEAT_PER_WORD 1
#define DMA_WORD_PER_BEAT 2
#define PLM_IN_NAME "audio_ffi_plm_block_in_dma64"
#define PLM_FLT_NAME "audio_ffi_plm_block_flt_dma64"
#define PLM_TW_NAME "audio_ffi_plm_block_twd_dma64"

#if defined(STRATUS_HLS)

#define HLS_MAP_plm(_mem, _plm_block_name)      \
    HLS_MAP_TO_MEMORY(_mem, _plm_block_name)

#define HLS_PROTO(_s)                           \
    HLS_DEFINE_PROTOCOL(_s)

#define HLS_FLAT(_a)                            \
    HLS_FLATTEN_ARRAY(_a);

#define HLS_BREAK_DEP(_a)                       \
    HLS_BREAK_ARRAY_DEPENDENCY(_a)

#define HLS_UNROLL_SIMPLE                       \
    HLS_UNROLL_LOOP(ON)

#define HLS_UNROLL_N(_n, _name)                 \
    HLS_UNROLL_LOOP(AGGRESSIVE, _n, _name)

#if defined(HLS_DIRECTIVES_BASIC)

#else

#error Unsupported or undefined HLS configuration

#endif /* HLS_DIRECTIVES_* */

#else /* !STRATUS_HLS */

#define HLS_MAP_plm(_mem, _plm_block_name)
#define HLS_PROTO(_s)
#define HLS_FLAT(_a)
#define HLS_BREAK_DEP(_a)
#define HLS_UNROLL_SIMPLE
#define HLS_UNROLL_N(_n, _name)

#endif /* STRATUS_HLS */

#endif /* __AUDIO_FFI_DIRECTIVES_HPP_ */
