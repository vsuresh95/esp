#include "stdint.h"
typedef float native_t;

// extern uint64_t get_counter();
//static
 void sw_run(int32_t do_relu, int32_t transpose, int32_t ninputs,
		   int32_t d3, int32_t d2, int32_t d1,
		   native_t *in1, native_t *in2, native_t *out);
