
// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"

#define MEM_WORDS 2048
#define ITERATIONS 10

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned size;

/* User-defined code */
static void init_parameters()
{
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = MEM_WORDS;
		out_words_adj = ITERATIONS*MEM_WORDS;
	} else {
		in_words_adj = round_up(MEM_WORDS, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(ITERATIONS*MEM_WORDS, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj * (1);
	out_len =  out_words_adj * (1);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset = in_len;
	size = (out_offset * sizeof(token_t)) + out_size;
}


int main(int argc, char **argv)
{
	token_t *gold;
	token_t *buf;

	init_parameters();

	buf = (token_t *) esp_alloc(size+10);
	cfg_000[0].hw_buf = buf;
    
    volatile token_t* sm_sync = (volatile token_t*) buf;
    unsigned mem_words = MEM_WORDS;
    unsigned err_cnt = 0;
    int i, j;
	struct timespec t_start;
	struct timespec t_end;
    
	for (j = 0; j < 10; j++)
		sm_sync[j] = 0;

	gold = malloc(out_size);

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	/* <<--print-params-->> */
	printf("  .rd_sp_offset = %d\n", rd_sp_offset);
	printf("  .rd_wr_enable = %d\n", rd_wr_enable);
	printf("  .wr_size = %d\n", wr_size);
	printf("  .wr_sp_offset = %d\n", wr_sp_offset);
	printf("  .rd_size = %d\n", rd_size);
	printf("  .dst_offset = %d\n", dst_offset);
	printf("  .src_offset = %d\n", src_offset);
	printf("\n  ** START **\n");

    gettime(&t_start);

	esp_run(cfg_000, NACC);
    
    for (i = 0; i < ITERATIONS; i++)
    {
        for (j = 0; j < mem_words; j++)
            buf[j+10] = (j+i)*2;
    
        // Op 1 - load mem_words data from 0 in mem to mem_words in SP
        sm_sync[1] = 0; // load/store
        sm_sync[2] = 0; // abort
        sm_sync[3] = mem_words; // rd_size
        sm_sync[4] = mem_words; // rd_sp_offset
        sm_sync[5] = 0; // src_offset
        
	    // printf("Before Load\n");

        sm_sync[0] = 1;
        while(sm_sync[0] != 0);

        // Op 2 - store mem_words data from mem_words in SP to mem_words in mem, and abort
        sm_sync[1] = 1; // load/store
        sm_sync[6] = (i+1)/ITERATIONS; // abort
        sm_sync[7] = mem_words; // wr_size
        sm_sync[8] = mem_words; // wr_sp_offset
        sm_sync[9] = (i+1)*mem_words; // dst_offset
        
	    // printf("Before Store\n");

        sm_sync[0] = 1;
        while(sm_sync[0] != 0);
        
        for (j = 0; j < mem_words; j++)
        {
            if (buf[((i+1)*mem_words)+j+10] != (j+i)*2)
            {
                err_cnt++;
            }
        }
    }

    gettime(&t_end);

	printf("\n  ** DONE **\n");

	printf("Errors = %d\n", err_cnt);

    unsigned long long t_diff = ts_subtract(&t_start, &t_end);

	printf("Time = %llu\n", t_diff);

	free(gold);
	esp_free(buf);

    printf("+ Test PASSED\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return 0;
}
