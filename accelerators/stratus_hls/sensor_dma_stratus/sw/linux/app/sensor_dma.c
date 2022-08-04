// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"

double intvl_write;
double intvl_read; 
double intvl_acc_write;
double intvl_acc_read;

int main(int argc, char **argv)
{
    clock_t t_start;
    clock_t t_end;
    double t_diff;

	int errors = 0;
    int j;

	token_t *gold;
	token_t *buf;

    unsigned mem_words = 1024; 
    size_t mem_size = mem_words*sizeof(token_t); 

	buf = (token_t *) esp_alloc(2*mem_size);
	cfg_000[0].hw_buf = buf;

	gold = buf + mem_words;
   
    t_start = clock();

	for (j = 0; j < mem_words; j++)
		buf[j] = (token_t) j;

    t_end = clock();
    t_diff = (double) (t_end - t_start);
    intvl_acc_read = t_diff;

    sensor_dma_cfg_000[0].rd_sp_offset = 2*mem_words;
    sensor_dma_cfg_000[0].rd_wr_enable = 0;
    sensor_dma_cfg_000[0].rd_size = mem_words;
    sensor_dma_cfg_000[0].sensor_src_offset = 0;

    t_start = clock();

	esp_run(cfg_000, NACC);

    t_end = clock();
    t_diff = (double) (t_end - t_start);
    intvl_write = t_diff;

    sensor_dma_cfg_000[0].wr_sp_offset = 2*mem_words;
    sensor_dma_cfg_000[0].rd_wr_enable = 1;
    sensor_dma_cfg_000[0].wr_size = mem_words;
    sensor_dma_cfg_000[0].sensor_dst_offset = mem_words;

    t_start = clock();

	esp_run(cfg_000, NACC);

    t_end = clock();
    t_diff = (double) (t_end - t_start);
    intvl_acc_write = t_diff;

    t_start = clock();

	for (j = 0; j < mem_words; j++)
		if (gold[j] != j) errors++;

    t_end = clock();
    t_diff = (double) (t_end - t_start);
    intvl_read = t_diff;

	esp_free(buf);

	if (!errors)
		printf("+ Test PASSED\n");
	else
		printf("+ Test FAILED\n");

	printf("CPU write = %f\n", intvl_write);
	printf("ACC read = %f\n", intvl_acc_read);
	printf("ACC write = %f\n", intvl_acc_write);
	printf("CPU read = %f\n", intvl_read);

	return errors;
}
