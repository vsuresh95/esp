/* User-defined code */
static void init_parameters(int test, int32_t do_relu, int32_t transpose, int32_t ninputs,
			    int32_t d3, int32_t d2, int32_t d1,
			    unsigned *in_len, unsigned *in1_len, unsigned *out_len,
			    unsigned *in_size, unsigned *out_size, unsigned *size, native_t* sw_buf, int inp_offset, unsigned* st_offset)
{
    int32_t ld_offset1, ld_offset2;//, st_offset;
    unsigned in2_len;
    int i;
    #ifdef ENABLE_SM
    *in1_len = round_up( d1 * d2, DMA_WORD_PER_BEAT(sizeof(token_t)));//ninputs *
    in2_len = round_up( d2 * d3, DMA_WORD_PER_BEAT(sizeof(token_t))); //ninputs *
    *in_len = *in1_len + in2_len;
    *out_len = round_up( d1 * d3, DMA_WORD_PER_BEAT(sizeof(token_t)));//ninputs *
    #else
    *in1_len = round_up(ninputs * d1 * d2, DMA_WORD_PER_BEAT(sizeof(token_t)));
    in2_len = round_up(ninputs * d2 * d3, DMA_WORD_PER_BEAT(sizeof(token_t))); 
    *in_len = *in1_len + in2_len;
    *out_len = round_up(ninputs * d1 * d3, DMA_WORD_PER_BEAT(sizeof(token_t)));
    
    #endif
    *in_size = *in_len * sizeof(token_t);
    *out_size = *out_len * sizeof(token_t);
    *size = *in_size + *out_size;

    #ifdef ENABLE_SM
    ld_offset1 = inp_offset; //0;
    ld_offset2 = ld_offset1 + *in1_len;
    *st_offset = 2*inp_offset + (*in_len);
    #else

    ld_offset1 = 0;
    ld_offset2 = *in1_len;
    *st_offset = *in_len;
    
    #endif
    gemm_cfg_000[0].do_relu = do_relu;
    gemm_cfg_000[0].transpose = transpose;
    gemm_cfg_000[0].ninputs = ninputs;
    gemm_cfg_000[0].d1 = d1;
    gemm_cfg_000[0].d2 = d2;
    gemm_cfg_000[0].d3 = d3;
    gemm_cfg_000[0].ld_offset1 = ld_offset1;
    gemm_cfg_000[0].ld_offset2 = ld_offset2;
    gemm_cfg_000[0].st_offset = *st_offset;

    for (i = 0; i < *in_len; ++i) {
        sw_buf[i] = i % 17 - 8;
	//printf("Sw_buf[%d] = %x\n", i, (int) sw_buf[i]);
    }

    // print test info
    printf("  Prepare test %d parameters\n", test);
    printf("    .do_relu = %d\n", do_relu);
    printf("    .transpose = %d\n", transpose);
    printf("    .ninputs = %d\n", ninputs);
    printf("    .d3 = %d\n", d3);
    printf("    .d2 = %d\n", d2);
    printf("    .d1 = %d\n", d1);
    printf("    .st_offset = %d\n", *st_offset);
    printf("    .ld_offset1 = %d\n", ld_offset1);
    printf("    .ld_offset2 = %d\n", ld_offset2);
}

static void sw_run(int32_t do_relu, int32_t transpose, int32_t ninputs,
		   int32_t d3, int32_t d2, int32_t d1,
		   native_t *in1, native_t *in2, native_t *out)
{
    int i, j, k, l;
    // struct timespec th_start, th_end;
    native_t *in1_l, *in2_l, *out_l;

    // gettime(&th_start);

    for (l = 0; l < ninputs; ++l)
    {
        in1_l = &in1[l * d1 * d2];
        in2_l = &in2[l * d2 * d3];
        out_l = &out[l * d1 * d3];

        for (i = 0; i < d1; ++i)
        {
            for (j = 0; j < d3; ++j)
            {
                native_t accumulator = 0.0;

                for (k = 0; k < d2; ++k)
                {
                    int mtx_in1_i = i * d2 + k;
                    int mtx_in2_i = transpose ? (j * d2 + k) : (k * d3 + j);

                    accumulator += in1_l[mtx_in1_i] * in2_l[mtx_in2_i];
                }

                out_l[i * d3 + j] = accumulator;
            }
        }
    }

    // gettime(&th_end);

    // unsigned long long hw_ns = ts_subtract(&th_start, &th_end);
    // printf("    Software execution time: %llu ns\n", hw_ns);
}
