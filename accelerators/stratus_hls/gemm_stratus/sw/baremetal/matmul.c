#include "matmul.h"

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
void sw_run(int32_t do_relu, int32_t transpose, int32_t ninputs,
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
    // sw_comp_end = get_counter();
	//if relu
	if(do_relu){
		// printf("do_relu: %d\n", do_relu);
		for (i = 0; i < ninputs * (d1*d3); i++) 
			if(out[i] < 0)
				out[i] = 0.0;
	}
}


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
