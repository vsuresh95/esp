// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __NERF_MLP_DIRECTIVES_HPP__
#define __NERF_MLP_DIRECTIVES_HPP__

#if (DMA_WIDTH == 32)
#define DMA_BEAT_PER_WORD 2
#define DMA_WORD_PER_BEAT 0
#elif (DMA_WIDTH == 64)
#define DMA_BEAT_PER_WORD 1
#define DMA_WORD_PER_BEAT 2
#endif

#define PLM_WGT_0_NAME "nerf_plm_wgt_0"
#define PLM_WGT_1_NAME "nerf_plm_wgt_1"
#define PLM_WGT_2_NAME "nerf_plm_wgt_2"
#define PLM_WGT_3_NAME "nerf_plm_wgt_3"
#define PLM_WGT_4_NAME "nerf_plm_wgt_4"
#define PLM_WGT_5_NAME "nerf_plm_wgt_5"
#define PLM_WGT_6_NAME "nerf_plm_wgt_6"
#define PLM_WGT_7_NAME "nerf_plm_wgt_7"
#define PLM_WGT_8_NAME "nerf_plm_wgt_8"
#define PLM_WGT_9_NAME "nerf_plm_wgt_9"
#define PLM_WGT_10_NAME "nerf_plm_wgt_10"
#define PLM_WGT_11_NAME "nerf_plm_wgt_11"

#define PLM_BIAS_0_NAME "nerf_plm_bias_0"
#define PLM_BIAS_1_NAME "nerf_plm_bias_1"
#define PLM_BIAS_2_NAME "nerf_plm_bias_2"
#define PLM_BIAS_3_NAME "nerf_plm_bias_3"
#define PLM_BIAS_4_NAME "nerf_plm_bias_4"
#define PLM_BIAS_5_NAME "nerf_plm_bias_5"
#define PLM_BIAS_6_NAME "nerf_plm_bias_6"
#define PLM_BIAS_7_NAME "nerf_plm_bias_7"
#define PLM_BIAS_8_NAME "nerf_plm_bias_8"
#define PLM_BIAS_9_NAME "nerf_plm_bias_9"
#define PLM_BIAS_10_NAME "nerf_plm_bias_10"
#define PLM_BIAS_11_NAME "nerf_plm_bias_11"

#define PLM_ACT_1_NAME "nerf_plm_act_1"
#define PLM_ACT_2_NAME "nerf_plm_act_2"
#define PLM_ACT_3_NAME "nerf_plm_act_3"
#define PLM_ACT_4_NAME "nerf_plm_act_4"
#define PLM_ACT_5_NAME "nerf_plm_act_5"
#define PLM_ACT_6_NAME "nerf_plm_act_6"
#define PLM_ACT_7_NAME "nerf_plm_act_7"
#define PLM_ACT_8_NAME "nerf_plm_act_8"
#define PLM_ACT_9_NAME "nerf_plm_act_9"
#define PLM_ACT_10_NAME "nerf_plm_act_10"

#define PLM_POS_NAME "nerf_plm_pos"
#define PLM_DIR_NAME "nerf_plm_dir"

#define PLM_OUT_NAME "nerf_plm_out"

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

#define PRESERVE_SIGNALS				\
    HLS_PRESERVE_SIGNAL(cur_load_data_dbg, true);   \
    HLS_PRESERVE_SIGNAL(cur_output_valid_dbg, true);   \
    HLS_PRESERVE_SIGNAL(cur_output_dbg, true);   \
    HLS_PRESERVE_SIGNAL(cur_input_valid_dbg, true);   \
    HLS_PRESERVE_SIGNAL(cur_wgt_dbg, true);   \
    HLS_PRESERVE_SIGNAL(cur_input_dbg, true);

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
#define PRESERVE_SIGNALS

#endif /* STRATUS_HLS */

#endif /* __NERF_MLP_DIRECTIVES_HPP_ */
