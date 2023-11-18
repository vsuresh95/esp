#include "stdint.h"
#include "stdio.h"
typedef float native_t;

// extern uint64_t get_counter();
//static
 void sw_run(int32_t do_relu, int32_t transpose, int32_t ninputs,
		   int32_t d3, int32_t d2, int32_t d1,
		   native_t *in1, native_t *in2, native_t *out);
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