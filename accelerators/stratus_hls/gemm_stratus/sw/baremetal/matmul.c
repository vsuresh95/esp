#include "matmul.h"

#ifndef __linux__
#include "../linux/app/gemm_directives.h"
#else
#include "gemm_directives.h"
#endif
// static void sw_comp(native_t* gold){
// 		// #include "fcn_gold.h"
// 	#include "fcn_input.h"
// 	int i = 0;
// 	const int offset = round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)));
// 	for (i = 0; i < ninputs * (d1*d3); i++) gold[i] = 0.0;
//  	for (i = 0; i < ninputs; i++) {
// 		native_t* input_a = &input[i * offset];
// 		native_t* input_b = &input[ninputs * offset + i*d2*d3];
// 		const int output_offset = i*d1*d3;
// 		//weight stationary
//     // sw_comp_start = get_counter();
// 		for(int z = 0; z < d2; z++){
// 			int input_b_offset = z*d3;
// 			for(int x = 0; x < d1; x++){
//                 const int output_offset2 = output_offset+x*d3;
//                 native_t temp_wt = input_a[x*d2 + z];
// 				for (int y = 0; y < d3; y++){
// 					gold[output_offset2 + y] += ((temp_wt * (input_b[input_b_offset + y]))); //>>FX_IL
//                 }
// 			}
// 		}
// 	}
// }

extern int64_t sw_comp_start, sw_comp_end;
// static 
// void sw_run(int32_t do_relu, int32_t transpose, int32_t ninputs,
// 		   int32_t d3, int32_t d2, int32_t d1,
// 		   native_t *in1, native_t *in2, native_t *out)
// {
// 	// #include "fcn_input.h"
//     int i, j, k, l;
//     // struct timespec th_start, th_end;
//     native_t *in1_l, *in2_l, *out_l;
//     // sw_comp_start = get_counter();

// 	const int offset = d1*d2; //round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)));
// 	for (i = 0; i < ninputs * (d1*d3); i++) out[i] = 0.0;
//  	for (i = 0; i < ninputs; i++) {
// 		in1_l = &in1[i * offset];
// 		in2_l = &in2[i*d2*d3];
// 		const int output_offset = i*d1*d3;
// 		//weight stationary
//     // sw_comp_start = get_counter();
// 		for(int z = 0; z < d2; z++){
// 			int input_b_offset = z*d3;
// 			for(int x = 0; x < d1; x++){
// 				// round_up(ninputs * (d1*d2 + d2*d3), DMA_WORD_PER_BEAT(sizeof(token_t)));
// 				// native_t temp_wt = input[i * d1*d2 + x*d2 + z]; 
// 				// native_t temp_wt = input[i *offset  + x*d2 + z] ;
// 				//BM const int output_offset2 = output_offset+x*d3;
// 				out_l = &out[output_offset + x*d1];// + x*d3
// 				native_t temp_wt = in1_l[x*d2 + z];
// 				// int64_t temp_wt = in[ninputs * d1*d2 + i*d2*d3 + d2*z + y]; 
// 				if(!transpose){
// 					for (int y = 0; y < d3; y++){
// 						out_l[y] += ((temp_wt * (in2_l[input_b_offset + y]))); //>>FX_IL
// 						// printf("NT out[%d] (%d) += A[%d] (%d) * B[%d] (%d)\n", y, out[y], (x*d2 + z), temp_wt, (input_b_offset + y), in2_l[input_b_offset + y]);

// 						// printf("gold[%d] (%d) += in[%d] (%d) * in[%d] (%d) [%d]\n", (i*d1*d3 + x*d3 + y), (int)gold[i*d1*d3 + x*d3 + y], 
// 						// 															(i * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + x*d2 + z),(int)input[i * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))  + x*d2 + z], 
// 						// 															(ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))  + i*d2*d3 + z*d3 + y), (int)input[ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + i*d2*d3 + z*d3 + y], 
// 						// 															((int)(temp_wt * (input[ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + i*d2*d3 + z*d3 + y]))));
// 					}
// 				}
// 				else{
// 					for (int y = 0; y < d3; y++){
// 						// printf("accumul orig: %d, A: %d, B: %d\n", (int)out_l[z*d1+y], (int)temp_wt, (int)in2_l[y*d2+z]);
// 						out_l[y] += ((temp_wt * (in2_l[y*d2+z]))); //>>FX_IL //x*d1+
// 						// printf("T out[%d] (%d) += A[%d] (%d) * B[%d] (%d)\n", z*d1+y, (int)out_l[z*d1+y], (x*d2 + z), (int)temp_wt, (y*d2+z), (int)in2_l[y*d2+z]);
						
// 					}
// 				}
// 			}
// 		}
// 	}
//     // sw_comp_end = get_counter();
// 	//if relu
// 	if(do_relu){
// 		// printf("do_relu: %d\n", do_relu);
// 		for (i = 0; i < ninputs * (d1*d3); i++) 
// 			if(out[i] < 0)
// 				out[i] = 0.0;
// 	}
// }


void sw_run_tile(int32_t do_relu, int32_t transpose, int32_t ninputs,
		   int32_t d3, int32_t d2, int32_t d1,
		   native_t *in1, native_t *in2, native_t *out)
{
	// #include "fcn_input.h"
    int i, j, k, l;
    // struct timespec th_start, th_end;
    native_t *in1_l, *in2_l, *out_l;
    // sw_comp_start = get_counter();

	const int offset = d1*d2; //round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)));
	for (i = 0; i < ninputs * (d1*d3); i++) out[i] = 0.0;
 	for (i = 0; i < ninputs; i++) {
		in1_l = &in1[i * offset];
		in2_l = &in2[i*d2*d3];
		const int output_offset = i*d1*d3;
		//weight stationary
    // sw_comp_start = get_counter();
		for(int z = 0; z < d2; z++){
			int input_b_offset = z*d3;
			for(int x = 0; x < d1; x++){
					// round_up(ninputs * (d1*d2 + d2*d3), DMA_WORD_PER_BEAT(sizeof(token_t)));
					// native_t temp_wt = input[i * d1*d2 + x*d2 + z]; 
					// native_t temp_wt = input[i *offset  + x*d2 + z] ;
					//BM const int output_offset2 = output_offset+x*d3;
					out_l = &out[output_offset + x*d1];// + x*d3
					native_t temp_wt = in1_l[x*d2 + z];
					// int64_t temp_wt = in[ninputs * d1*d2 + i*d2*d3 + d2*z + y]; 
                    if(!transpose){
                        for (int y = 0; y < d3; y++){
                            out_l[y] += ((temp_wt * (in2_l[input_b_offset + y]))); //>>FX_IL
                            // printf("NT out[%d] (%d) += A[%d] (%d) * B[%d] (%d)\n", y, out[y], (x*d2 + z), temp_wt, (input_b_offset + y), in2_l[input_b_offset + y]);

                            // printf("gold[%d] (%d) += in[%d] (%d) * in[%d] (%d) [%d]\n", (i*d1*d3 + x*d3 + y), (int)gold[i*d1*d3 + x*d3 + y], 
                            // 															(i * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + x*d2 + z),(int)input[i * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))  + x*d2 + z], 
                            // 															(ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))  + i*d2*d3 + z*d3 + y), (int)input[ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + i*d2*d3 + z*d3 + y], 
                            // 															((int)(temp_wt * (input[ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + i*d2*d3 + z*d3 + y]))));
                        }
                    }
                    else{
                        for (int y = 0; y < d3; y++){
							// printf("accumul orig: %d, A: %d, B: %d\n", (int)out_l[z*d1+y], (int)temp_wt, (int)in2_l[y*d2+z]);
                            out_l[y] += ((temp_wt * (in2_l[y*d2+z]))); //>>FX_IL //x*d1+
                            // printf("T out[%d] (%d) += A[%d] (%d) * B[%d] (%d)\n", z*d1+y, (int)out_l[z*d1+y], (x*d2 + z), (int)temp_wt, (y*d2+z), (int)in2_l[y*d2+z]);
							
                        }
                    }
			}
		}
	}
	//if relu
	if(do_relu){
		// printf("do_relu: %d\n", do_relu);
		for (i = 0; i < ninputs * (d1*d3); i++) 
			if(out[i] < 0)
				out[i] = 0.0;
	}

    // sw_comp_end = get_counter();
}

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
				   uint16_t* index_d1_incr)
{
				// 	,
				//    uint16_t& m2_loop_iters,
				//    uint16_t& m2_plm_incr){
	*size_matrix1 = matrix_d1 * matrix_d2;
    *size_matrix2 = matrix_d2 * matrix_d3;
    *size_matrix_out = matrix_d1 * matrix_d3;// * ninputs;

	printf("sizem1:%d sizem2:%d sizeout:%d \n", *size_matrix1, *size_matrix2, *size_matrix_out);

    // m2_loop_iters = 1;
    // m2_plm_incr = 1;

    uint8_t d3_odd = matrix_d3 % 2;
    uint8_t is_less_than_matrix2 = (*size_matrix2 > DMA_CHUNK || !transpose);

    if ((matrix_d2 > DMA_CHUNK) || (is_less_than_matrix2 && d3_odd)) {
		*load_cfg = LESS_THAN_ROW;
		*loadable_rows = 1;
		*loadable_chunk = DMA_CHUNK;
		uint32_t matrix_mul;
		// calculate_chunks(matrix_chk_in, matrix_rem_in1, matrix_d2, 0);
		*matrix_chk_in = matrix_d2 >> DMA_CHUNK_LOG;
		// calculating the number of cols (covered the by the chunks)
		matrix_mul = *matrix_chk_in << DMA_CHUNK_LOG;
		*matrix_rem_in1 = matrix_d2 - matrix_mul;

		// adding the last chunk if it is necessary
		if (*matrix_rem_in1 != 0) { ++(*matrix_chk_in); }

		*matrix_rem_in2 = *matrix_rem_in1;
		*index_d1_incr = matrix_d2;
    } else if (is_less_than_matrix2) {
		*load_cfg = LESS_THAN_MATRIX2;
		if (*size_matrix2 > DMA_CHUNK) {
			*loadable_rows = DMA_CHUNK / matrix_d2;
			if (*loadable_rows != 1)
			*loadable_rows = ((*loadable_rows) >> 1) << 1;
		} else {
			*loadable_rows = matrix_d3;
		}
		*loadable_chunk = *loadable_rows * matrix_d2;
		*matrix_chk_in = 1;
		*matrix_rem_in1 = *size_matrix1 % *loadable_chunk;
		*matrix_rem_in2 = *size_matrix2 % *loadable_chunk;
		*index_d1_incr = *loadable_chunk;
	// 	if (!transpose) {
	// 		// m2_loop_iters = matrix_d2;
	// 		// m2_plm_incr = matrix_d2;
	// 	}
    } else 
		{
		*load_cfg = MORE_THAN_MATRIX2;
		*loadable_rows = matrix_d3;
		*loadable_chunk = *size_matrix2;
		*matrix_chk_in = 1;
		*matrix_rem_in1 = *size_matrix1 % *loadable_chunk;
		*matrix_rem_in2 = *size_matrix2;
		*index_d1_incr = *loadable_chunk;
		}
	// calculate_chunks(matrix_chk_out, matrix_rem_out, size_matrix_out, 1);
// calculating the number of chunks (ceil)
 		if (*load_cfg == LESS_THAN_MATRIX2 && *loadable_rows != 1) 
		{
			*matrix_chk_out = (*size_matrix_out) / *loadable_rows;
			uint32_t matrix_mul = (*matrix_chk_out) * (*loadable_rows); 
			*matrix_rem_out = *(size_matrix_out) - matrix_mul;
			// adding the last chunk if it is necessary
			if (*matrix_rem_out > 0) { ++(*matrix_chk_out); 
			}
		}
		else
		{
			*matrix_chk_out = (*size_matrix_out) >> OUT_DMA_CHUNK_LOG;
			// calculating the number of cols (covered the by the chunks)
			uint32_t matrix_mul = (*matrix_chk_out) << OUT_DMA_CHUNK_LOG; 
			*matrix_rem_out = *(size_matrix_out) - matrix_mul;
			// adding the last chunk if it is necessary
			if (*matrix_rem_out > 0) { ++(*matrix_chk_out); 
			}
		}
	printf("cfg: %d loadable rows: %d\nloadable chunk:%d\nmatrix rem in2:%d\nmatrix rem in1:%d\nmatrix rem out:%d\nmatrix chnk in:%d\nmatrix chnk out:%d\n", (int)*load_cfg, *loadable_rows, *loadable_chunk
	, *matrix_rem_in1, *matrix_rem_in2, *matrix_rem_out, *matrix_chk_in, *matrix_chk_out);
}
