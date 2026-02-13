// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <sstream>
#include "system.hpp"

// Process
void system_t::config_proc()
{

    // Reset
    {
        conf_done.write(false);
        conf_info.write(conf_info_t());
        wait();
    }

    ESP_REPORT_INFO("reset done");

    // Config
    load_memory();
    {
        conf_info_t config;
        // Custom configuration
        /* <<--params-->> */
        config.C_mat_offset = C_mat_offset;
        config.B_mat_offset = B_mat_offset;
        config.A_mat_offset = A_mat_offset;
        config.K = K;
        config.M = M;
        config.N = N;

        ESP_REPORT_INFO("Configuring accelerator with: M=%d, N=%d, K=%d", M, N, K);
        ESP_REPORT_INFO("Memory offsets: A=%d, B=%d, C=%d", A_mat_offset, B_mat_offset, C_mat_offset);

        wait(); conf_info.write(config);
        conf_done.write(true);
    }

    ESP_REPORT_INFO("config done");

    // Compute
    {
        sc_time begin_time = sc_time_stamp();
        ESP_REPORT_TIME(begin_time, "BEGIN - basic_256_gemm");

        do { wait(); } while (!acc_done.read());
        debug_info_t debug_code = debug.read();

        sc_time end_time = sc_time_stamp();
        ESP_REPORT_TIME(end_time, "END - basic_256_gemm");

        esc_log_latency(sc_object::basename(), clock_cycle(end_time - begin_time));
        wait(); 
        conf_done.write(false);
    }

     // Validate
    {
        dump_memory();
        if (validate())
        {
            ESP_REPORT_ERROR("validation failed!");
        } else
        {
            ESP_REPORT_INFO("validation passed!");
        }
    }

    // Conclude
    {
        sc_stop();
    }
}

// Functions
void system_t::load_memory()
{
#ifdef CADENCE
    if (esc_argc() != 1)
    {
        ESP_REPORT_INFO("usage: %s\n", esc_argv()[0]);
        sc_stop();
    }
#endif
    // Calculate adjusted sizes for A, B, and C (aligned to DMA_WIDTH)
    ESP_REPORT_INFO("Matrix dimensions: M=%d, N=%d, K=%d", M, N, K);
    uint32_t A_words_adj, B_words_adj;
    
    #if (DMA_WORD_PER_BEAT == 0)
        A_words_adj = M * K;
        B_words_adj = K * N;
        out_words_adj = M * N;
    #else
        A_words_adj = round_up(M * K, DMA_WORD_PER_BEAT);
        B_words_adj = round_up(K * N, DMA_WORD_PER_BEAT);
        out_words_adj = round_up(M * N, DMA_WORD_PER_BEAT);
    #endif

    uint32_t A_size = A_words_adj;
    uint32_t B_size = B_words_adj;
    out_size = out_words_adj;

    ESP_REPORT_INFO("Adjusted sizes: A=%d words, B=%d words, C=%d words", A_size, B_size, out_size);

    // Allocate temporary arrays for A and B
    int32_t* A = new int32_t[A_size];
    int32_t* B = new int32_t[B_size];

    //Initialize A (M × K)
    ESP_REPORT_INFO("Initializing Matrix A (%dx%d)", M, K);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < K; j++) {
            A[i * K + j] = (int32_t)(i * K + j + 1);  // 1, 2, 3, 4, ...
        }
    }
    // Pad A if necessary
    for (int i = M * K; i < A_size; i++) {
        A[i] = 0;
    }

    // Debug: Print first few elements of A
    if (M <= 8 && K <= 8) {
        ESP_REPORT_INFO("Matrix A (first few rows):");
        for (int i = 0; i < (M < 4 ? M : 4); i++) {
            for (int j = 0; j < (K < 4 ? K : 4); j++) {
                printf("%4d ", A[i * K + j]);
            }
            printf("\n");
        }
    }

    // Initialize Matrix B (K × N)
    ESP_REPORT_INFO("Initializing Matrix B (%dx%d)", K, N);
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < N; j++) {
            B[i * N + j] = (int32_t)(i * N + j + 1);
        }
    }
    // Pad B if necessary
    for (int i = K * N; i < B_size; i++) {
        B[i] = 0;
    }

    // Debug: Print first few elements of B
    if (K <= 8 && N <= 8) {
        ESP_REPORT_INFO("Matrix B (first few rows):");
        for (int i = 0; i < (K < 4 ? K : 4); i++) {
            for (int j = 0; j < (N < 4 ? N : 4); j++) {
                printf("%4d ", B[i * N + j]);
            }
            printf("\n");
        }
    }

     // Compute golden output: C = A × B
    ESP_REPORT_INFO("Computing golden output (C = A × B)");
    gold = new int32_t[out_size];
    
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            int32_t sum = 0;
            for (int k = 0; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            gold[i * N + j] = sum;
        }
    }
    for (int i = M * N; i < out_size; i++) {
        gold[i] = 0;
    }

    // Debug: Print expected output
    if (M <= 8 && N <= 8) {
        ESP_REPORT_INFO("Expected Matrix C (first few rows):");
        for (int i = 0; i < (M < 4 ? M : 4); i++) {
            for (int j = 0; j < (N < 4 ? N : 4); j++) {
                printf("%6d ", gold[i * N + j]);
            }
            printf("\n");
        }
    }

    ESP_REPORT_INFO("Memory offsets: A=%d, B=%d, C=%d", A_mat_offset, B_mat_offset, C_mat_offset);

    // Write Matrix A to memory
    ESP_REPORT_INFO("Writing Matrix A to memory at offset %d", A_mat_offset);
    #if (DMA_WORD_PER_BEAT == 0)
        for (int i = 0; i < A_size; i++) {
            sc_dt::sc_bv<DATA_WIDTH> data_bv(A[i]);
            for (int j = 0; j < DMA_BEAT_PER_WORD; j++)
                mem[A_mat_offset * DMA_BEAT_PER_WORD + DMA_BEAT_PER_WORD * i + j] = 
                    data_bv.range((j + 1) * DMA_WIDTH - 1, j * DMA_WIDTH);
        }
    #else
        for (int i = 0; i < A_size / DMA_WORD_PER_BEAT; i++) {
            sc_dt::sc_bv<DMA_WIDTH> data_bv;
            for (int j = 0; j < DMA_WORD_PER_BEAT; j++)
                data_bv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH) = 
                    A[i * DMA_WORD_PER_BEAT + j];
            mem[A_mat_offset / DMA_WORD_PER_BEAT + i] = data_bv;
        }
    #endif

    // Write Matrix B to memory
    ESP_REPORT_INFO("Writing Matrix B to memory at offset %d", B_mat_offset);
    #if (DMA_WORD_PER_BEAT == 0)
        for (int i = 0; i < B_size; i++) {
            sc_dt::sc_bv<DATA_WIDTH> data_bv(B[i]);
            for (int j = 0; j < DMA_BEAT_PER_WORD; j++)
                mem[B_mat_offset * DMA_BEAT_PER_WORD + DMA_BEAT_PER_WORD * i + j] = 
                    data_bv.range((j + 1) * DMA_WIDTH - 1, j * DMA_WIDTH);
        }
    #else
        for (int i = 0; i < B_size / DMA_WORD_PER_BEAT; i++) {
            sc_dt::sc_bv<DMA_WIDTH> data_bv;
            for (int j = 0; j < DMA_WORD_PER_BEAT; j++)
                data_bv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH) = 
                    B[i * DMA_WORD_PER_BEAT + j];
            mem[B_mat_offset / DMA_WORD_PER_BEAT + i] = data_bv;
        }
    #endif

    // Initialize C region to a recognizable pattern (helps debug)
    ESP_REPORT_INFO("Initializing C memory region to 0xDEADBEEF pattern");
    #if (DMA_WORD_PER_BEAT == 0)
        for (int i = 0; i < out_size; i++) {
            sc_dt::sc_bv<DATA_WIDTH> data_bv(0xDEADBEEF);
            for (int j = 0; j < DMA_BEAT_PER_WORD; j++)
                mem[C_mat_offset * DMA_BEAT_PER_WORD + DMA_BEAT_PER_WORD * i + j] = 
                    data_bv.range((j + 1) * DMA_WIDTH - 1, j * DMA_WIDTH);
        }
    #else
        for (int i = 0; i < out_size / DMA_WORD_PER_BEAT; i++) {
            sc_dt::sc_bv<DMA_WIDTH> data_bv;
            for (int j = 0; j < DMA_WORD_PER_BEAT; j++)
                data_bv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH) = 0xDEADBEEF;
            mem[C_mat_offset / DMA_WORD_PER_BEAT + i] = data_bv;
        }
    #endif

    // Clean up temporary arrays
    delete [] A;
    delete [] B;

    ESP_REPORT_INFO("load memory completed");
}

void system_t::dump_memory()
{
    // Get results from memory at C_mat_offset
    out = new int32_t[out_size];

    ESP_REPORT_INFO("Reading output from C_mat_offset = %d", C_mat_offset);

    #if (DMA_WORD_PER_BEAT == 0)
        uint32_t offset = C_mat_offset * DMA_BEAT_PER_WORD;
        for (int i = 0; i < out_size; i++) {
            sc_dt::sc_bv<DATA_WIDTH> data_bv;
            for (int j = 0; j < DMA_BEAT_PER_WORD; j++)
                data_bv.range((j + 1) * DMA_WIDTH - 1, j * DMA_WIDTH) = 
                    mem[offset + DMA_BEAT_PER_WORD * i + j];

            out[i] = data_bv.to_int64();
        }
    #else
        uint32_t offset = C_mat_offset / DMA_WORD_PER_BEAT;
        for (int i = 0; i < out_size / DMA_WORD_PER_BEAT; i++)
            for (int j = 0; j < DMA_WORD_PER_BEAT; j++)
                out[i * DMA_WORD_PER_BEAT + j] = 
                    mem[offset + i].range((j + 1) * DATA_WIDTH - 1, j * DATA_WIDTH).to_int64();
    #endif

    // Debug: Print actual output
    if (M <= 8 && N <= 8) {
        ESP_REPORT_INFO("Actual Matrix C from accelerator (first few rows):");
        for (int i = 0; i < (M < 4 ? M : 4); i++) {
            for (int j = 0; j < (N < 4 ? N : 4); j++) {
                printf("%6d ", out[i * N + j]);
            }
            printf("\n");
        }
    }

    ESP_REPORT_INFO("dump memory completed");
}

int system_t::validate()
{
    // Check for mismatches
    uint32_t errors = 0;
    uint32_t max_errors_to_print = 10;

    ESP_REPORT_INFO("Validating output matrix C (%d x %d = %d elements):", M, N, M * N);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            int idx = i * N + j;
            if (gold[idx] != out[idx]) {
                if (errors < max_errors_to_print) {
                    ESP_REPORT_INFO("  MISMATCH at C[%d][%d]: expected %d, got %d", 
                                    i, j, gold[idx], out[idx]);
                }
                errors++;
            }
        }
    }

    if (errors == 0) {
        ESP_REPORT_INFO("*** VALIDATION PASSED *** All %d output elements matched!", M * N);
    } else {
        ESP_REPORT_INFO("*** VALIDATION FAILED *** Total errors: %d out of %d elements", 
                        errors, M * N);
        if (errors > max_errors_to_print) {
            ESP_REPORT_INFO("(only first %d errors printed)", max_errors_to_print);
        }
    }

    delete [] out;
    delete [] gold;

    return errors;
}