// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"
#include "monitors.h"

double intvl_write;
double intvl_read; 
double intvl_acc_write;
double intvl_acc_read;

#define ITERATIONS 20

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

	//statically declare monitor arg structure
	esp_monitor_args_t mon_args;

	//statically declare monitor vals structures
	esp_monitor_vals_t vals_start, vals_end, vals_diff;

	//set read_mode to ALL
	mon_args.read_mode = ESP_MON_READ_ALL;

	FILE *fp_write = fopen("cpu_write.txt", "a");
	FILE *fp_acc_write = fopen("acc_write.txt", "a");
	FILE *fp_read = fopen("cpu_read.txt", "a");
	FILE *fp_acc_read = fopen("acc_read.txt", "a");

	intvl_write = 0;
	intvl_acc_write = 0;
	intvl_read = 0;
	intvl_acc_read = 0;
   
	for (i = 0; i < ITERATIONS; i++)
	{ 
  	    void* dst = (void*)(buf);
	    int64_t value_64 = 123;

	    ////////////////////////////////////////////
	    // CPU WRITE
	    ////////////////////////////////////////////
	    esp_monitor(mon_args, &vals_start);

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
        if (i > 1) intvl_write += t_diff;

	    esp_monitor(mon_args, &vals_end);

	    vals_diff = esp_monitor_diff(vals_start, vals_end);
	    if (i > 15) esp_monitor_print(mon_args, vals_diff, fp_write);

	    ////////////////////////////////////////////
	    // ACC READ
	    ////////////////////////////////////////////
        sensor_dma_cfg_000[0].rd_sp_offset = 2*mem_words;
        sensor_dma_cfg_000[0].rd_wr_enable = 0;
        sensor_dma_cfg_000[0].rd_size = mem_words;
        sensor_dma_cfg_000[0].sensor_src_offset = 0;
        sensor_dma_cfg_000[0].spandex_conf = spandex_config.spandex_reg;

        t_start = clock();

	    esp_run(cfg_000, NACC);

        t_end = clock();
        t_diff = (double) (t_end - t_start);
        if (i > 1) intvl_acc_read += t_diff;

	    esp_monitor(mon_args, &vals_end);

	    vals_diff = esp_monitor_diff(vals_start, vals_end);
	    if (i > 15) esp_monitor_print(mon_args, vals_diff, fp_acc_read);

	    ////////////////////////////////////////////
	    // ACC WRITE
	    ////////////////////////////////////////////
        sensor_dma_cfg_000[0].wr_sp_offset = 2*mem_words;
        sensor_dma_cfg_000[0].rd_wr_enable = 1;
        sensor_dma_cfg_000[0].wr_size = mem_words;
        sensor_dma_cfg_000[0].sensor_dst_offset = mem_words;
        sensor_dma_cfg_000[0].spandex_conf = spandex_config.spandex_reg;

        t_start = clock();

	    esp_run(cfg_000, NACC);

        t_end = clock();
        t_diff = (double) (t_end - t_start);
        if (i > 1) intvl_acc_write += t_diff;

	    esp_monitor(mon_args, &vals_end);

	    vals_diff = esp_monitor_diff(vals_start, vals_end);
	    if (i > 15) esp_monitor_print(mon_args, vals_diff, fp_acc_write);

	    ////////////////////////////////////////////
	    // CPU READ
	    ////////////////////////////////////////////
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
        if (i > 1) intvl_read += t_diff;

	    esp_monitor(mon_args, &vals_end);

	    vals_diff = esp_monitor_diff(vals_start, vals_end);
	    if (i > 15) esp_monitor_print(mon_args, vals_diff, fp_read);
    }

	fclose(fp_write);
	fclose(fp_acc_write);
	fclose(fp_read);
	fclose(fp_acc_read);

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
