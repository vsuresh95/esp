// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"

double intvl_write;
double intvl_read; 
double intvl_acc_write;
double intvl_acc_read;

#define ITERATIONS 1000

int main(int argc, char **argv)
{
    clock_t t_start;
    clock_t t_end;
    double t_diff;

	int errors = 0;
    int i, j;

	token_t *gold;
	token_t *buf;

    unsigned mem_words = 1024; 
    size_t mem_size = mem_words*sizeof(token_t); 

	buf = (token_t *) esp_alloc(2*mem_size);
	cfg_000[0].hw_buf = buf;

	gold = buf + mem_words;

	intvl_write = 0;
	intvl_acc_write = 0;
	intvl_read = 0;
	intvl_acc_read = 0;
   
	for (i = 0; i < ITERATIONS; i++)
	{ 
  	    void* dst = (void*)(buf);
	    int64_t value_64 = 123;

        t_start = clock();

        for (j = 0; j < mem_words; j+=2)
        {
	        asm volatile (
	    		"mv t0, %0;"
	    		"mv t1, %1;"
	    		".word " QU(WRITE_CODE)
	    		: 
	    		: "r" (dst), "r" (value_64)
	    		: "t0", "t1", "memory"
	    	);

	    	dst += 16;
        }

        t_end = clock();
        t_diff = (double) (t_end - t_start);
        intvl_write += t_diff;

        sensor_dma_cfg_000[0].rd_sp_offset = 2*mem_words;
        sensor_dma_cfg_000[0].rd_wr_enable = 0;
        sensor_dma_cfg_000[0].rd_size = mem_words;
        sensor_dma_cfg_000[0].sensor_src_offset = 0;
        sensor_dma_cfg_000[0].spandex_conf = spandex_config.spandex_reg;

        t_start = clock();

	    esp_run(cfg_000, NACC);

        t_end = clock();
        t_diff = (double) (t_end - t_start);
        intvl_acc_read += t_diff;

        sensor_dma_cfg_000[0].wr_sp_offset = 2*mem_words;
        sensor_dma_cfg_000[0].rd_wr_enable = 1;
        sensor_dma_cfg_000[0].wr_size = mem_words;
        sensor_dma_cfg_000[0].sensor_dst_offset = mem_words;
        sensor_dma_cfg_000[0].spandex_conf = spandex_config.spandex_reg;

        t_start = clock();

	    esp_run(cfg_000, NACC);

        t_end = clock();
        t_diff = (double) (t_end - t_start);
        intvl_acc_write += t_diff;

  	    dst = (void*)(gold);

        t_start = clock();

 	    for (j = 0; j < mem_words; j+=2)
 	    {
	    	asm volatile (
	    		"mv t0, %1;"
	    		".word " QU(READ_CODE) ";"
	    		"mv %0, t1"
	    		: "=r" (value_64)
	    		: "r" (dst)
	    		: "t0", "t1", "memory"
	    	);

	    	dst += 16;
 	    }

        t_end = clock();
        t_diff = (double) (t_end - t_start);
        intvl_read += t_diff;
    }

	esp_free(buf);

	if (!errors)
		printf("+ Test PASSED\n");
	else
		printf("+ Test FAILED\n");

	printf("CPU write = %f\n", intvl_write/ITERATIONS);
	printf("ACC read = %f\n", intvl_acc_read/ITERATIONS);
	printf("ACC write = %f\n", intvl_acc_write/ITERATIONS);
	printf("CPU read = %f\n", intvl_read/ITERATIONS);

	return errors;
}
