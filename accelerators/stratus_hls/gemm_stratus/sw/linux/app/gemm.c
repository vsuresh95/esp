// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "esp.h"
#include "cfg.h"

void gemm(const native_t* mat_a, const native_t* mat_b, native_t* mat_c, unsigned dim_m, unsigned dim_n, unsigned dim_k) {
    const unsigned block_size = 16;
    native_t sum;

	for (unsigned m = 0; m < dim_m; m += block_size) {
	    for (unsigned n = 0; n < dim_n; n += block_size) {
	        for (unsigned k = 0; k < dim_k; k += block_size) {

                unsigned m_rem = (m + block_size < dim_m) ? m + block_size : dim_m;
                unsigned n_rem = (n + block_size < dim_n) ? n + block_size : dim_n;
                unsigned k_rem = (k + block_size < dim_k) ? k + block_size : dim_k;

                for (unsigned m_ = m; m_ < m_rem; m_++) {
                    for (unsigned n_ = n; n_ < n_rem; n_++) {
                        if (k == 0) sum = 0;
                        else sum = mat_c[m_ * dim_n + n_];
                        for (unsigned k_ = k; k_ < k_rem; k_++) {
							sum += mat_a[m_ * dim_k + k_] * mat_b[k_ * dim_n + n_];
                        }
                        mat_c[m_ * dim_n + n_] = sum;
                    }
                }
            }
        }
    }
}

/* User-defined code */
int validate_buffer(token_t *mem_c, native_t *gold_c)
{
    unsigned errors = 0;
    const unsigned len = dim_m * dim_n;
	const float ERR_TH = 0.05;

    for (unsigned j = 0; j < len; j++) {
		native_t val = fixed32_to_float(mem_c[j], FX_IL);
		if ((fabs(gold_c[j] - val) / fabs(gold_c[j])) > ERR_TH) {
            if (errors < 10) printf("\tGOLD[%u] = %.04f vs %.04f = out[%u]\n", j, gold_c[j], val, j);
            errors++;
        }
    }

    printf("\tError for %d values out of %d\n", errors, len);

    return errors;
}

/* User-defined code */
void init_buffer(token_t *mem_a, token_t *mem_b, native_t *gold_a, native_t *gold_b, native_t *gold_c)
{
    const float LO = -2.0;
    const float HI = 2.0;
    const unsigned len_a = dim_m * dim_k;
    const unsigned len_b = dim_n * dim_k;

    for (unsigned j = 0; j < len_a; j++) {
        float scaling_factor = (float) rand() / (float) RAND_MAX;
        gold_a[j] = LO + scaling_factor * (HI - LO);
        mem_a[j] = float_to_fixed32(gold_a[j], FX_IL);
    }

    for (unsigned j = 0; j < len_b; j++) {
        float scaling_factor = (float) rand() / (float) RAND_MAX;
        gold_b[j] = LO + scaling_factor * (HI - LO);
        mem_b[j] = float_to_fixed32(gold_b[j], FX_IL);
    }

    // Compute golden output
    uint64_t t_start = get_counter();
    gemm(gold_a, gold_b, gold_c, dim_m, dim_n, dim_k);
	t_sw += get_counter() - t_start;
}

int main(int argc, char **argv)
{
	int errors = 0;
	t_sw = 0; t_acc = 0;

    if (argc > 1) {
        ITERATIONS = atoi(argv[1]);
        dim_m = atoi(argv[2]);
        dim_n = atoi(argv[3]);
        dim_k = atoi(argv[4]);
	}

    printf("\tStarting test %d %d %d %d...\n", ITERATIONS, dim_m, dim_n, dim_k);
    unsigned flag_len = PAYLOAD_OFFSET/sizeof(token_t); // Number of unsigned elements reserved for flags
    unsigned mat_a_len = dim_m * dim_k;
    unsigned mat_b_len = dim_n * dim_k;
    unsigned mat_c_len = dim_m * dim_n;
    // Sync flag and data offsets
    unsigned mat_a_valid_offset = VALID_OFFSET;
    unsigned mat_a_offset = mat_a_valid_offset + flag_len;
    unsigned mat_b_offset = mat_a_offset + mat_a_len;
    unsigned mat_c_valid_offset = mat_b_offset + mat_b_len;
    unsigned mat_c_offset = mat_c_valid_offset + flag_len;

	// Accelerator struct parameters
	struct gemm_stratus_access *gemm_access_desc = (struct gemm_stratus_access *) malloc (sizeof(struct gemm_stratus_access));
	gemm_access_desc->dim_m = dim_m;
	gemm_access_desc->dim_n = dim_n;
	gemm_access_desc->dim_k = dim_k;
	gemm_access_desc->weight_base = mat_b_offset;
	gemm_access_desc->input_base = mat_a_valid_offset;
	gemm_access_desc->output_base = mat_c_valid_offset;
	gemm_access_desc->esp.src_offset = 0;
	gemm_access_desc->esp.dst_offset = 0;
    gemm_access_desc->esp.coherence = ACC_COH_RECALL;

    printf("\tAllocations\n");
    token_t *mem = (token_t *) esp_alloc((mat_c_offset + mat_c_len) * sizeof(token_t));
	native_t *gold = (native_t*) malloc((mat_c_offset + mat_c_len) * sizeof(native_t));

	// Opening accelerator file descriptor
	char devname[100] = "gemm_stratus.0";
    printf("\tOpening %s...\n", devname);

	char full_path[384];
	snprintf(full_path, 384, "/dev/%s", devname);
	int fd = open(full_path, O_RDWR, 0);
	if (fd < 0) {
		fprintf(stderr, "Error: cannot open %s", full_path);
		exit(EXIT_FAILURE);
	}

	// Setting up ESP memory buffer
    enum contig_alloc_policy policy;
	struct esp_access *esp_access_desc = (struct esp_access *) gemm_access_desc;
    contig_handle_t *handle = lookup_handle((void*) mem, &policy);
	esp_access_desc->contig = contig_to_khandle(*handle);
	esp_access_desc->ddr_node = contig_to_most_allocated(*handle);
	esp_access_desc->alloc_policy = policy;
	esp_access_desc->run = true;

    for (int i = 0; i < ITERATIONS; i++) {
        init_buffer(&mem[mat_a_offset], &mem[mat_b_offset],
                    &gold[mat_a_offset], &gold[mat_b_offset], &gold[mat_c_offset]);

        uint64_t t_start = get_counter();
        if (ioctl(fd, GEMM_STRATUS_IOC_ACCESS, esp_access_desc)) {
            perror("ioctl");
            exit(EXIT_FAILURE);
        }
        t_acc += get_counter() - t_start;
        
        errors += validate_buffer(&mem[mat_c_offset], &gold[mat_c_offset]);
    }

    // free
    esp_free(mem);
    free(gold);

	printf("\tSoftware time = %lu\n", t_sw/ITERATIONS);
	printf("\tAccel time = %lu\n", t_acc/ITERATIONS);
	printf("\tErrors = %d\n", errors);
    printf("\tClosing %s...\n", devname);
	if (close(fd) == -1) {
        perror("Error closing file");
        return 1;
    }

	return errors;
}
