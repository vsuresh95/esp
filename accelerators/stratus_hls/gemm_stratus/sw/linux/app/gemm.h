#ifndef __GEMM__
#define __GEMM__


#include "coh_func.h"

#ifdef __linux__
#include "libesp.h"
#include "gemm_stratus.h"
#endif
#include "gemm_directives.h"


typedef float native_t;
uint32_t size_mat1, size_mat2, size_mat_out, mat_chk_in,  mat_chk_out;
uint16_t load_cfg, mat_rem_in1, mat_rem_in2,mat_rem_out, loadable_rows, loadable_chunk, index_d1_incr;

#ifdef __linux__
token_t *acc_buf;
native_t *sw_buf;
native_t *out_arr;
#endif

int32_t input_buffer_offset[MAX_DEVICES];
int32_t output_buffer_offset[MAX_DEVICES];

int32_t accel_prod_last_element[MAX_DEVICES];
int32_t accel_cons_last_element[MAX_DEVICES];

int32_t accel_prod_valid_offset[MAX_DEVICES];
int32_t accel_cons_ready_offset[MAX_DEVICES];
int32_t accel_prod_ready_offset[MAX_DEVICES];
int32_t accel_cons_valid_offset[MAX_DEVICES];

int32_t * cpu_cons_ready_offset = &accel_prod_ready_offset[0];
int32_t * cpu_cons_valid_offset = &accel_prod_valid_offset[0];
int32_t * cpu_prod_ready_offset;// = &accel_cons_ready_offset[NUM_DEVICES-1];
int32_t * cpu_prod_valid_offset;// = &accel_cons_valid_offset[NUM_DEVICES-1];


static inline uint64_t get_counter() {
    uint64_t t_end = 0;
#ifndef __linux__
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);
#else
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);
#endif
	return t_end;
}


/* Write to the memory*/
static inline void write_mem (void* dst, int64_t value_64)
{
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE)
		:
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
}

/* Read from the memory*/
static inline int64_t read_mem (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

static inline void set_offsets(int num_devices, int tile_size){

	int32_t rel_accel_prod_ready_offset = RDY_OFFSET;
	int32_t rel_accel_prod_valid_offset = VLD_OFFSET;
	int32_t rel_accel_prod_last_offset = LAST_OFFSET;
	int32_t rel_input_buffer_offset = SYNC_VAR_SIZE;
	int32_t rel_accel_cons_ready_offset = tile_size + RDY_OFFSET;
	int32_t rel_accel_cons_valid_offset = tile_size + VLD_OFFSET;
	int32_t rel_accel_con_last_offset = tile_size + LAST_OFFSET;
	int32_t rel_output_buffer_offset = tile_size + SYNC_VAR_SIZE;

    for(int dev_id = 0; dev_id < num_devices; dev_id++){
		accel_prod_valid_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_prod_valid_offset;
		accel_cons_ready_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_cons_ready_offset;
		accel_prod_ready_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_prod_ready_offset;
		accel_cons_valid_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_cons_valid_offset;
		accel_cons_last_element[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_con_last_offset;
		accel_prod_last_element[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_prod_last_offset;
		input_buffer_offset[dev_id]  	= dev_id*(tile_size + SYNC_VAR_SIZE) + rel_input_buffer_offset;
		output_buffer_offset[dev_id] 	= dev_id*(tile_size + SYNC_VAR_SIZE) + rel_output_buffer_offset;
    }
	cpu_prod_ready_offset = &accel_cons_ready_offset[num_devices-1];
	cpu_prod_valid_offset = &accel_cons_valid_offset[num_devices-1];
}


static inline void reset_sync(token_t *buf, int num_devices){
	int n;
    printf("Inside reset sync\n");
	for(n = 0; n< num_devices; n++){
		write_mem(((void*)(buf + accel_prod_ready_offset[n])), 1);//BM
		// printf("reset accel_prod_ready acc_buf[%d]: %d\n", accel_prod_ready_offset, *(acc_buf + accel_prod_ready_offset));
		write_mem(((void*)(buf + accel_cons_valid_offset[n])), 0);
		// DBG_PRINT("reset accel_cons_valid acc_buf[%d]: %d\n", accel_cons_valid_offset, *(acc_buf + accel_cons_valid_offset));
		write_mem(((void*)(buf + accel_cons_last_element[n])), 0);
		// DBG_PRINT("reset accel_con_last acc_buf[%d]: %d\n", accel_con_last_offset, *(acc_buf + accel_con_last_offset));
		write_mem(((void*)(buf + accel_prod_valid_offset[n])), 0);
		// DBG_PRINT("reset accel_prod_valid acc_buf[%d]: %d\n", accel_prod_valid_offset, *(acc_buf + accel_prod_valid_offset));
		write_mem(((void*)(buf + accel_cons_ready_offset[n])), 0); 
		// DBG_PRINT("reset accel_cons_ready acc_buf[%d]: %d\n", accel_cons_ready_offset, *(acc_buf + accel_cons_ready_offset));
	}
	asm volatile ("fence w, w");	
}

static inline uint32_t poll_cons_rdy(token_t *buf){
	int64_t value_64 = 0;
	void* dst = (void*)(buf + (*cpu_cons_ready_offset));
	value_64 = read_mem(dst);
	// printf("Polling cons rdy acc_buf[%d]: %d\n", *cpu_cons_ready_offset, value_64);
	return ((value_64&0x1) == 1);
}

static inline uint32_t poll_prod_valid(token_t *buf){
	void* dst = (void*)(buf+(*cpu_prod_valid_offset));
	int64_t value_64 = 0;
	value_64 = read_mem(dst);
	return ((value_64&0x1) == 1);
}

static inline uint32_t accel_rdy(token_t*mem, int toggle){
	if(toggle){
		if(poll_cons_rdy(mem)) return 1;
	}
	else{
		if(poll_prod_valid(mem)) return 2;
	}
	return 0;
}

static inline void update_cons_valid(token_t *buf,int64_t last){
	#ifndef ESP
	asm volatile ("fence w, w");	//release semantics
	#endif
	// void* dst = (void*)(acc_buf+(*cpu_cons_valid_offset)+1);
	// write_mem(dst, last);

	// #ifdef ESP
	// asm volatile ("fence w, w");	//release semantics
	// #endif

	// dst = (void*)(acc_buf+(*cpu_cons_valid_offset));
	// int64_t value_64 = 1;
	// write_mem(dst, value_64);


	// int time_var = 0;
	// while(time_var<100) time_var++;

	buf[*cpu_cons_valid_offset+1] = last;
	buf[*cpu_cons_valid_offset] = 1;
	asm volatile ("fence w, w");	
}


static inline void update_cons_rdy(token_t *buf){
	asm volatile ("fence w, w");	//acquire semantics
	// void* dst = (void*)(acc_buf+(*cpu_cons_ready_offset));
	// int64_t value_64 = 0;
	// write_mem(dst, value_64);
	// int time_var = 0;
	// while(time_var<100) time_var++;
	buf[*cpu_cons_ready_offset] = 0;
	asm volatile ("fence w, w");	
}

static inline void update_prod_rdy(token_t *buf){
	asm volatile ("fence w, w");	//acquire semantics
	// void* dst = (void*)(acc_buf+(*cpu_prod_ready_offset));
	// int64_t value_64 = 1; 
	// write_mem(dst, value_64);
	// int time_var = 0;
	// while(time_var<100) time_var++;

	buf[*cpu_prod_ready_offset] = 1;
	asm volatile ("fence w, w");	
}

static inline void update_prod_valid(token_t *buf){
	asm volatile ("fence w, w");	//release semantics
	// void* dst = (void*)(acc_buf+(*cpu_prod_valid_offset));
	// int64_t value_64 = 0; 
	// write_mem(dst, value_64);
	// int time_var = 0;
	// while(time_var<100) time_var++;

	buf[*cpu_prod_valid_offset] = 0;
	asm volatile ("fence w, w");	//acquire semantics
}



static inline void update_con_last(token_t *buf){
	void* dst = (void*)(buf+(*cpu_cons_valid_offset)+1);
	int64_t value_64 = 1; 
	write_mem(dst, value_64);	
}
static inline void update_con_not_last(token_t *buf){
	void* dst = (void*)(buf+(*cpu_cons_valid_offset)+1);
	int64_t value_64 = 0; 
	write_mem(dst, value_64);
}

// static inline void write_to_cons(){
// 	void *dst = (void*)(&acc_buf[input_buffer_offset]);
// 	int64_t val_64 = 123; 
// 	int local_tile_size = tile_size;
// 	for (int j = 0; j < local_tile_size; j++){
// 		write_mem(dst, val_64);
// 		dst += 8;
// 	}
// }

// static inline void read_from_prod(){
// 	void *src = (void*)(acc_buf+output_buffer_offset);
// 	int64_t out_val;
// 	int local_tile_size = tile_size;
// 	for (int j = 0; j < local_tile_size; j++){
// 			out_val = read_mem(src);
// 			src += 8;
// 	}
// }



static inline void init_buf_input1 (int ninput, token_t *in, int matsize, native_t* sw_buf, int32_t offset)
{
    int i;
	
	static int tile_num = 1;
	// printf("init_buf_input1 Tile %d mat[%d - %d]\n", tile_num, offset, matsize);
		
#ifdef __FIXED
	// *offset_n = offset + mat1size; 
	for (i = 0; i < matsize; i++) {
		in[i] = float2fx(sw_buf[ninput + offset+ i], FX_IL); 
    	// printf("in2[%d] = %d, sw[%d] = %d\n", mat1size+i, in[mat1size+i], ninput*d2*d3 + offset2+ i, (int)sw_buf2[ninput*d2*d3 + offset2+ i]);
    }
	// *offset_n2 = offset2 + mat1size; 

#else 
#include "gold.h"
// #include "fcn_gold.h"
#endif
	tile_num++;
}

// static inline void init_buf_input2 (int ninput, token_t *in, int mat1size, int mat2size, native_t* sw_buf, native_t* sw_buf2, int32_t offset, int32_t offset2)
// {
//     int i;
	
// 	static int tile_num = 1;
// 	// printf("init_buf_input2 Tile %d B[%d - %d]\n", tile_num, offset2, mat2size);
		
// #ifdef __FIXED
// 	for (i = 0; i < mat2size; i++) {
// 		in[mat1size+i] = float2fx(sw_buf2[ninput*d2*d3 + offset2+ i], FX_IL); 
//     	// printf("in2[%d] = %d, sw[%d] = %d\n", mat1size+i, in[mat1size+i], ninput*d2*d3 + offset2+ i, (int)sw_buf2[ninput*d2*d3 + offset2+ i]);
//     }
// 	// *offset_n2 = offset2 + mat1size; 

// #else 
// #include "gold.h"
// // #include "fcn_gold.h"
// #endif
// 	tile_num++;
// }


static void init_buf_output (int ninput, int offset2, int len, token_t *res, native_t* out_arr, int size_mat_out)
{
    int i;
#ifdef __FIXED

	int offset = offset2+ninput*(round_up(size_mat_out, DMA_WORD_PER_BEAT(sizeof(token_t))));
	// printf("output tile O[%d - %d]\n", offset2, len);
	for (i = 0; i < len; i++) { //round_up(len, DMA_WORD_PER_BEAT(sizeof(token_t)))
		#ifdef __FIXED
		out_arr[offset+i] = fx2float(res[i], FX_IL);
		// printf("res[%d] = %u (%d)\n", i, res[i], (int)out_arr[offset+i]);
		#else
		out_arr[offset+i] = res[i], FX_IL;
		#endif
    }
#else 
#include "gold.h"
// #include "fcn_gold.h"
#endif
}

uint64_t sw_comp_start = 0;
uint64_t sw_comp_end = 0;
uint64_t hw_comp_start = 0;
uint64_t hw_comp_end = 0;
uint64_t hw_write_time = 0;
uint64_t hw_read_time = 0;

static inline void in_main(int ninputs, int d1, int d2, int d3, int transpose, int do_relu, int ld_offset2, token_t* mem,native_t *sw_buf, native_t * out_arr){
	// token_t* mem ;
	// mem = acc_buf;
	update_prod_rdy(mem);
	int tinput = 0;
	// while(tinput<ninputs){
	// printf("  Poll Cons Rdy...\n");
	while(!poll_cons_rdy(mem)); // wait for cons ready
	// printf("  Found Cons Rdy...\n");

	asm volatile ("fence w, w");	//release semantics
	mem[input_buffer_offset[0]+NINPUTS_OFFSET] = ninputs;
	mem[input_buffer_offset[0]+MAT_D1_OFFSET] = d1;
	mem[input_buffer_offset[0]+MAT_D2_OFFSET] = d2;
	mem[input_buffer_offset[0]+MAT_D3_OFFSET] = d3;
	mem[input_buffer_offset[0]+LD_OFFSET] = input_buffer_offset[0];
	mem[input_buffer_offset[0]+ST_OFFSET] = output_buffer_offset[0];
	mem[input_buffer_offset[0]+TRANSPOSE_OFFSET] = transpose;
	mem[input_buffer_offset[0]+DO_RELU_OFFSET] = do_relu;
	// printf("  Provided config. Update Cons Rdy and last...\n");

	asm volatile ("fence w, w");	//release semantics
	update_cons_rdy(mem);
	update_cons_valid(mem,0);

	hw_write_time = 0;
	hw_read_time = 0;
	int out_iters = ((d1*d3)/mat_chk_out);
	int in_iters = (d2*d3/(loadable_chunk));
	while(tinput<ninputs){
		uint32_t index_d1 = 0;
		int offset = 0;
		int offseti2 = 0;
		int offset2 = 0;
		int offset_n = 0;
		int offset_n2 = 0;
		int8_t load_a = 1;
		uint32_t in_chk = 0;
		uint32_t out_chk = 0;
		uint32_t out_row = 0;
		uint32_t out_tile = 0;
		int chk_per_tile = mat_chk_out; //(d1*d3/loadable_rows);
		uint32_t md1 = 0, md2 = 0;
		while(in_chk < mat_chk_in || out_chk < chk_per_tile){//
			if(out_chk < chk_per_tile && poll_prod_valid(mem))
			{
				int mat1size = OUT_DMA_CHUNK;
				
				// If true the next is the last (smaller) chunk
				if (load_cfg == LESS_THAN_MATRIX2 && loadable_rows != 1) {
					mat1size = loadable_rows;
				} else {
					if (out_chk == mat_chk_out - 1 && mat_rem_out != 0)
					mat1size = mat_rem_out;
				}

				uint64_t hw_read_time_start = get_counter();
				init_buf_output(tinput, out_tile+out_row*d3+offset2, mat1size, mem+output_buffer_offset[0],out_arr, size_mat_out);
				update_prod_valid(mem);
				update_prod_rdy(mem);
				// offset2 += mat_chk_out; //loadable_rows; //loadable_rows;
				uint64_t hw_read_time_end = get_counter();
				hw_read_time += (hw_read_time_end-hw_read_time_start);
				// out_row++;
				if(load_cfg == LESS_THAN_MATRIX2){
					out_row ++;
					if(out_row == loadable_rows){
						out_row = 0;
						offset2 += loadable_rows;
						if(offset2 >= d3){
							offset2 = 0;
							out_tile += loadable_rows*d3;
						}
					}
				} else {
					offset2 += OUT_DMA_CHUNK;
					if(offset2 >= size_mat2){
						offset2 = 0;
						// out_tile += loadable_rows*d3;
					}
				}
			
				// printf("  out chk:%d/%d\n", out_chk,mat_chk_out);//(d1*d3/loadable_rows));
				out_chk++;
			}
			else if((in_chk < mat_chk_in ) && poll_cons_rdy(mem)){//|| out_chk < mat_chk_out)
				int mat1size = loadable_chunk; //(round_up(d1*loadable_rows, DMA_WORD_PER_BEAT(sizeof(token_t))));
				int mat2size = loadable_chunk;
				if (load_cfg == LESS_THAN_ROW && in_chk == mat_chk_in - 1 && mat_rem_in2 != 0) {
					mat1size = mat_rem_in1;
					mat2size = mat_rem_in2;
				} else if (load_cfg != LESS_THAN_ROW) {
					if (md1 + loadable_rows > d1)
						mat1size = mat_rem_in1;
					if (md2 + loadable_rows > d3)
						mat2size = mat_rem_in2;
				}

				uint64_t hw_write_time_start = get_counter();
				// init_buf_input(0, mem+rel_input_buffer_offset, loadable_rows);
				if(load_a)//(in_chk==0)
				{
					init_buf_input1 (tinput*mat1size, mem+input_buffer_offset[0], mat1size, sw_buf, offset);
					load_a = !load_a;
				}
				init_buf_input1 (tinput*mat2size, mem+input_buffer_offset[0]+mat1size, mat2size,(sw_buf + ld_offset2),offseti2);
				uint64_t hw_write_time_end = get_counter();
				hw_write_time += (hw_write_time_end-hw_write_time_start);

				// offset = offset + mat1size;
				// offseti2 = offseti2+mat2size;
				// printf("  Update Cons Rdy and last...\n");
				update_cons_rdy(mem);
				update_cons_valid(mem,0);
				// printf("  in chk:%d/%d\n", in_chk,mat_chk_in);
				// in_chk ++;

				if(in_chk == mat_chk_in - 1){
					load_a = 1;
					if (md1 + loadable_rows >= d1 && md2 + loadable_rows >= d3){
						md1 += loadable_rows;
						offset+= loadable_chunk;
						md2 += loadable_rows;
						offseti2 += loadable_chunk;
						in_chk++;
					}else if(md2 + loadable_rows >= d3){
						md2 = 0;
						offset += loadable_chunk;
						md1 += loadable_rows;
						offseti2 = 0;
						in_chk = 0;
					} else
					{
						// md1 += loadable_rows;
						// offset+= loadable_chunk;
						// md2 = 0;

						in_chk = 0;
						md2 += loadable_rows;
						offseti2 += loadable_chunk;
					}
				} else {
					in_chk++;
					md2 += loadable_rows;
					offseti2 += loadable_chunk;
				}
			}
		}
		tinput++;
	}
}


#endif // __GEMM__