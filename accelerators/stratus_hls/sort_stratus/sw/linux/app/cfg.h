#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
// #include <test/test.h>
// #include <test/time.h>
#include "sort_stratus.h"

#define DEVNAME "sort_stratus.0"
#define NAME "sort_stratus"

#define SYNC_VAR_SIZE 6
#define UPDATE_VAR_SIZE 2
#define VALID_FLAG_OFFSET 0
#define END_FLAG_OFFSET 2
#define READY_FLAG_OFFSET 4

#define LEN 32

#define NACC 1
#define NUM_DEVICES 1

struct sort_test {
	struct test_info info;
	struct sort_stratus_access desc;
	float *hbuf;
	float *sbuf;
	unsigned int n_elems;
	unsigned int n_batches;
	bool verbose;
};

struct sort_stratus_access sort_cfg_000[] = {
    {
        .size = LEN,
        .batch = 1,
        .input_offset = 0,
        .output_offset = 0,
		.src_offset = 0,
		.dst_offset = 0,
        .esp.coherence = ACC_COH_FULL,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
    }
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = DEVNAME,
		.ioctl_req = SORT_STRATUS_IOC_ACCESS,
		.esp_desc = &(sort_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
