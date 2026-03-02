#include "stdint.h"
#include "stdio.h"
typedef float native_t;

// extern uint64_t get_counter();
//static
//  void sw_run(int32_t do_relu, int32_t transpose, int32_t ninputs,
// 		   int32_t d3, int32_t d2, int32_t d1,
// 		   native_t *in1, native_t *in2, native_t *out);
void calculate_tiles(uint32_t ninputs,
				   uint32_t matrix_d1,
				   uint32_t matrix_d2,
				   uint32_t matrix_d3,
				   uint32_t transpose,
				   uint32_t* size_matrix1,
				   uint32_t* size_matrix2,
				   uint32_t* size_matrix_out,
				   uint32_t* matrix_chk_in,
				   uint16_t* matrix_rem_in1,
				   uint16_t* matrix_rem_in2,
				   uint32_t* matrix_chk_out,
				   uint16_t* matrix_rem_out,
				   uint16_t* load_cfg,
				   uint16_t* loadable_rows,
				   uint16_t* loadable_chunk,
				   uint16_t* index_d1_incr);

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