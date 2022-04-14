// Copyright (c) 2011-2022 Columbia University, System Level Design Group
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
        config.batch_size = batch_size;

        wait(); conf_info.write(config);
        conf_done.write(true);
    }

    ESP_REPORT_INFO("config done");

    // Compute
    {
        // Print information about begin time
        sc_time begin_time = sc_time_stamp();
        ESP_REPORT_TIME(begin_time, "BEGIN - nerf_mlp");

        // Wait the termination of the accelerator
        do { wait(); } while (!acc_done.read());
        debug_info_t debug_code = debug.read();

        // Print information about end time
        sc_time end_time = sc_time_stamp();
        ESP_REPORT_TIME(end_time, "END - nerf_mlp");

        esc_log_latency(sc_object::basename(), clock_cycle(end_time - begin_time));
        wait(); conf_done.write(false);
    }

    // Validate
    {
        dump_memory(); // store the output in more suitable data structure if needed
        // check the results with the golden model
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
    // Optional usage check
#ifdef CADENCE
    if (esc_argc() != 1)
    {
        ESP_REPORT_INFO("usage: %s\n", esc_argv()[0]);
        sc_stop();
    }
#endif

    // Input data and golden output (aligned to DMA_WIDTH makes your life easier)
    unsigned weights_offset = 
    /* layer 0 */ LAYER_0_INPUTS*LAYER_0_OUTPUTS + LAYER_0_OUTPUTS +
    /* layer 1 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS +
    /* layer 2 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
    /* layer 3 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;
    /* layer 4 */ LAYER_4_INPUTS*LAYER_4_OUTPUTS + LAYER_4_OUTPUTS;
    /* layer 5 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
    /* layer 6 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
    /* layer 7 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
    /* layer 8 */ LAYER_8_INPUTS*LAYER_8_OUTPUTS + LAYER_8_OUTPUTS + 
    /* layer 9 */ LAYER_9_INPUTS*LAYER_9_OUTPUTS + LAYER_9_OUTPUTS + 
    /* layer 10 */ LAYER_10_INPUTS*LAYER_10_OUTPUTS + LAYER_10_OUTPUTS;

    in_words_adj = weights_offset +
    /* pos inputs */ LAYER_0_INPUTS +
    /* dir inputs */ (LAYER_8_INPUTS-LAYER_N_DIMS);

    out_words_adj = LAYER_10_OUTPUTS;

    in_size = in_words_adj * (1);
    out_size = out_words_adj * (1);

    in = new int64_t[in_size];
    for (int i = 0; i < 1; i++)
        for (int j = 0; j < in_size; j++)
            in[i * in_words_adj + j] = (int64_t) (j%10);

    // Compute golden output
    gold = new int64_t[out_size];

    ping = new int64_t[LAYER_4_INPUTS];
    pong = new int64_t[LAYER_4_INPUTS];

    unsigned in_offset = 0;

    // Layer 0
    for (uint16_t col_wgt = 0; col_wgt < LAYER_0_OUTPUTS; col_wgt++)
    {
        ping[col_wgt] = in[in_offset+LAYER_0_INPUTS*LAYER_0_OUTPUTS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_0_INPUTS; row_wgt++)
        {
            ping[col_wgt] += in[in_offset+col_wgt*LAYER_0_INPUTS+row_wgt] * in[weights_offset+row_wgt];
        }

        if (ping[col_wgt] < 0) ping[col_wgt] = 0;
    }

    in_offset += LAYER_0_INPUTS*LAYER_0_OUTPUTS + LAYER_0_OUTPUTS;

    // Layer 1
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        pong[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            pong[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * ping[row_wgt];
        }

        if (pong[col_wgt] < 0) pong[col_wgt] = 0;
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 2
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        ping[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            ping[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * pong[row_wgt];
        }

        if (ping[col_wgt] < 0) ping[col_wgt] = 0;
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 3
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        pong[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            pong[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * ping[row_wgt];
        }

        if (pong[col_wgt] < 0) pong[col_wgt] = 0;
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 4
    for (uint16_t row_wgt = LAYER_N_DIMS; row_wgt < LAYER_4_INPUTS; row_wgt++)
    {
        pong[row_wgt] = in[weights_offset+row_wgt-LAYER_N_DIMS];
    }

    for (uint16_t col_wgt = 0; col_wgt < LAYER_4_OUTPUTS; col_wgt++)
    {
        ping[col_wgt] = in[in_offset+LAYER_4_INPUTS*LAYER_4_OUTPUTS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_4_INPUTS; row_wgt++)
        {
            ping[col_wgt] += in[in_offset+col_wgt*LAYER_4_INPUTS+row_wgt] * pong[row_wgt];
        }

        if (ping[col_wgt] < 0) ping[col_wgt] = 0;
    }

    in_offset += LAYER_4_INPUTS*LAYER_4_OUTPUTS + LAYER_4_OUTPUTS;

    // Layer 5
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        pong[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            pong[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * ping[row_wgt];
        }

        if (pong[col_wgt] < 0) pong[col_wgt] = 0;
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 6
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        ping[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            ping[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * pong[row_wgt];
        }

        if (ping[col_wgt] < 0) ping[col_wgt] = 0;
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 7
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        pong[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            pong[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * ping[row_wgt];
        }
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 8
    for (uint16_t row_wgt = LAYER_N_DIMS; row_wgt < LAYER_8_INPUTS; row_wgt++)
    {
        pong[row_wgt] = in[weights_offset+LAYER_0_INPUTS+row_wgt-LAYER_N_DIMS];
    }

    for (uint16_t col_wgt = 0; col_wgt < LAYER_8_OUTPUTS; col_wgt++)
    {
        ping[col_wgt] = in[in_offset+LAYER_4_INPUTS*LAYER_8_OUTPUTS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_8_INPUTS; row_wgt++)
        {
            ping[col_wgt] += in[in_offset+col_wgt*LAYER_8_INPUTS+row_wgt] * pong[row_wgt];
        }

        if (ping[col_wgt] < 0) ping[col_wgt] = 0;
    }

    in_offset += LAYER_8_INPUTS*LAYER_8_OUTPUTS + LAYER_8_OUTPUTS;

    // Layer 9
    for (uint16_t col_wgt = 0; col_wgt < LAYER_9_OUTPUTS; col_wgt++)
    {
        pong[col_wgt] = in[in_offset+LAYER_9_INPUTS*LAYER_9_OUTPUTS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_9_INPUTS; row_wgt++)
        {
            pong[col_wgt] += in[in_offset+col_wgt*LAYER_9_INPUTS+row_wgt] * ping[row_wgt];
        }

        if (pong[col_wgt] < 0) pong[col_wgt] = 0;
    }

    in_offset += LAYER_9_INPUTS*LAYER_9_OUTPUTS + LAYER_9_OUTPUTS;

    // Layer 10
    for (uint16_t col_wgt = 0; col_wgt < LAYER_10_OUTPUTS; col_wgt++)
    {
        gold[col_wgt] = in[in_offset+LAYER_10_INPUTS*LAYER_10_OUTPUTS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_10_INPUTS; row_wgt++)
        {
            gold[col_wgt] += in[in_offset+col_wgt*LAYER_10_INPUTS+row_wgt] * pong[row_wgt];
        }

        if (gold[col_wgt] < 0) gold[col_wgt] = 0;
    }

    in_offset += LAYER_10_INPUTS*LAYER_10_OUTPUTS + LAYER_10_OUTPUTS;

    // Memory initialization:
    for (int i = 0; i < in_size / DMA_WORD_PER_BEAT; i++)  {
        sc_dt::sc_bv<DMA_WIDTH> data_bv(in[i]);
        for (int j = 0; j < DMA_WORD_PER_BEAT; j++)
            data_bv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH) = in[i * DMA_WORD_PER_BEAT + j];
        mem[i] = data_bv;
    }

    ESP_REPORT_INFO("load memory completed");
}

void system_t::dump_memory()
{
    // Get results from memory
    out = new int64_t[out_size];
    uint32_t offset = in_size;

    offset = offset / DMA_WORD_PER_BEAT;
    for (int i = 0; i < out_size / DMA_WORD_PER_BEAT; i++)
        for (int j = 0; j < DMA_WORD_PER_BEAT; j++)
            out[i * DMA_WORD_PER_BEAT + j] = mem[offset + i].range((j + 1) * DATA_WIDTH - 1, j * DATA_WIDTH).to_int64();

    ESP_REPORT_INFO("dump memory completed");
}

int system_t::validate()
{
    // Check for mismatches
    uint32_t errors = 0;

    for (int i = 0; i < out_size; i++)
        if (gold[i] != out[i])
        {
            errors++;
            ESP_REPORT_INFO("gold[%d] = %lu out[%d] = %lu\n", i, gold[i], i, out[i]);
        }

    delete [] in;
    delete [] out;
    delete [] gold;

    return errors;
}
