// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "nerf_mlp.hpp"
#include "nerf_mlp_directives.hpp"

// Functions

#include "nerf_mlp_functions.hpp"

// Processes

// Function to manage load operations for all layers
void nerf_mlp::load_input_dma(uint32_t len, uint32_t offset, sc_dt::sc_int<DATA_WIDTH> *plm_input)
{
    dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);

    this->dma_read_ctrl.put(dma_info);

    for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
    {
        HLS_BREAK_DEP(plm_input);

        sc_dt::sc_bv<DMA_WIDTH> dataBv;

        dataBv = this->dma_read_chnl.get();
        wait();

        // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
        for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
        {
            HLS_UNROLL_SIMPLE;
            plm_input[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
        }
    }

    wait();
}

void nerf_mlp::load_input()
{

    // Reset
    {
        HLS_PROTO("load-reset");

        this->reset_load_input();

        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t batch_size;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        batch_size = config.batch_size;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        wait();

        uint32_t offset = 0;

        // Load layer 0 weights
        uint32_t len = round_up(15360, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_0);
        offset += len; 
 
        // Load layer 0 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_0);
        offset += len; 
 
        // Load layer 1 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_1);
        offset += len;
 
        // Load layer 1 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_1);
        offset += len; 
 
        // Load layer 2 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_2);
        offset += len;

        // Load layer 2 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_2);
        offset += len; 
 
        // Load layer 3 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_3);
        offset += len;

        // Load layer 3 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_3);
        offset += len; 
 
        // Load layer 4 weights
        len = round_up(80896, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_4);
        offset += len;

        // Load layer 4 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_4);
        offset += len; 
 
        // Load layer 5 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_5);
        offset += len;

        // Load layer 5 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_5);
        offset += len; 
 
        // Load layer 6 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_6);
        offset += len;

        // Load layer 6 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_6);
        offset += len; 
 
        // Load layer 7 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_7);
        offset += len;

        // Load layer 7 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_7);
        offset += len; 
 
        // Load layer 8 weights
        len = round_up(71680, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_8);
        offset += len;

        // Load layer 8 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_8);
        offset += len; 
 
        // Load layer 9 weights
        len = round_up(32768, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_9);
        offset += len;

        // Load layer 9 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_9);
        offset += len; 
 
        // Load layer 10 weights
        len = round_up(372, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_10);
        offset += len;

        // Load layer 10 biases
        len = round_up(256, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_10);
        offset += len; 
 
        // Load input position vectors
        len = round_up(60, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_pos);
        offset += len; 

        // Load input direction vectors
        len = round_up(24, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_pos);
        offset += len;

        store_offset = offset;

        this->load_compute_handshake();
    }

    // Conclude
    {
        this->process_done();
    }
}

void nerf_mlp::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t batch_size;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        batch_size = config.batch_size;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        uint32_t offset = round_up(store_offset, DMA_WORD_PER_BEAT) * 1;

        wait();

        uint32_t len = round_up(LAYER_10_OUTPUTS, DMA_WORD_PER_BEAT);

        this->store_compute_handshake();

        // Configure DMA transaction
        dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
        offset += len;

        this->dma_write_ctrl.put(dma_info);

        for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
        {
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            // Read from PLM
            wait();
            for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
            {
                HLS_UNROLL_SIMPLE;
                dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = regs_pong[i];
            }
            this->dma_write_chnl.put(dataBv);
        }
    }

    // Conclude
    {
        this->accelerator_done();
        this->process_done();
    }
}

// Function to manage tiled MAC's
void nerf_mlp::compute_kernel_tile(uint16_t input_dim, uint16_t output_dim, sc_dt::sc_int<DATA_WIDTH> *plm_wgt, sc_dt::sc_int<DATA_WIDTH> *plm_bias, int64_t *regs_input, int64_t *regs_output, int64_t *regs_mul, int64_t *regs_wgt)
{
    // Read from biases from scratchpad
    for (uint16_t bias_index = 0; bias_index < LAYER_0_OUTPUTS; bias_index++)
    {
        HLS_UNROLL_LOOP(ON, "read_bias");
        HLS_BREAK_ARRAY_DEPENDENCY(plm_bias);
        regs_output[bias_index] = plm_bias[bias_index];
    }

    for (uint16_t col_wgt = 0; col_wgt < output_dim; col_wgt++)
    {
        // Loop across the weight row
        for (uint16_t row_wgt = 0; row_wgt < input_dim; row_wgt += MAC_TILE_SIZE)
        {
            // Read tile sized weights from scratchpad into registers
            for (uint16_t tile_index = 0; tile_index < MAC_TILE_SIZE; tile_index++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_0);
                regs_wgt[tile_index] = plm_wgt[col_wgt*input_dim + row_wgt + tile_index];
            }

            // Multiply and store products in registers
            for (uint16_t tile_index = 0; tile_index < MAC_TILE_SIZE; tile_index++)
            {
                HLS_UNROLL_LOOP(ON, "multiply");
                regs_mul[tile_index] = regs_wgt[tile_index] * regs_input[row_wgt + tile_index];
            }

            // Sum the products
            for (uint16_t tile_index = 0; tile_index < MAC_TILE_SIZE; tile_index++)
            {
                HLS_UNROLL_LOOP(ON, "sum");
                regs_output[col_wgt] += regs_mul[tile_index];
            }
        }
    }
}

void nerf_mlp::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t batch_size;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        batch_size = config.batch_size;

        // after the load has brought in all the data
        this->compute_load_handshake(); 
    }

    // Compute 
    HLS_FLATTEN_ARRAY(regs_ping);
    HLS_FLATTEN_ARRAY(regs_wgt);
    HLS_FLATTEN_ARRAY(regs_bias);
    HLS_FLATTEN_ARRAY(regs_mul);
    HLS_FLATTEN_ARRAY(regs_pong);

    // First, the design is not pipelined
    // Weights will be transposed, to avoid column order read
    // Needs to be tiled - cannot fit
    // fix weight plm offset
    // Take volume density output
    // Need sigmoid for Layer 10

    /////////////////////////////////
    // Layer 0 1x60 x 60x256
    ///////////////////////////////// 
    {
        // Read from Layer 0 inputs from scratchpad
        for (uint16_t elem_in = 0; elem_in < LAYER_0_INPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_inputs_0");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_pos);
            regs_ping[elem_in] = plm_pos[elem_in];
        }

        compute_kernel_tile(LAYER_0_INPUTS, LAYER_0_OUTPUTS, plm_wgt_0, plm_bias_0,
                            regs_ping, regs_pong, regs_mul, regs_wgt);

        // ReLU on the output values
        for (uint16_t row_wgt = 0; row_wgt < LAYER_0_OUTPUTS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_0");
            if (regs_pong[row_wgt] < 0) regs_pong[row_wgt] = 0;
        }
    }

    ////////////////////////////////
    // Layer 1 1x256 x 256x256
    ///////////////////////////////// 
    {
        compute_kernel_tile(LAYER_N_DIMS, LAYER_N_DIMS, plm_wgt_1, plm_bias_1,
                            regs_pong, regs_ping, regs_mul, regs_wgt);
 
        // ReLU on the output values
        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_1");
            if (regs_ping[row_wgt] < 0) regs_ping[row_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 2 1x256 x 256x256
    ///////////////////////////////// 
    {
        compute_kernel_tile(LAYER_N_DIMS, LAYER_N_DIMS, plm_wgt_2, plm_bias_2,
                            regs_ping, regs_pong, regs_mul, regs_wgt);

        // ReLU on the output values
        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_2");
            if (regs_pong[row_wgt] < 0) regs_pong[row_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 3 1x256 x 256x256
    ///////////////////////////////// 
    {
        compute_kernel_tile(LAYER_N_DIMS, LAYER_N_DIMS, plm_wgt_3, plm_bias_3,
                            regs_pong, regs_ping, regs_mul, regs_wgt);

        // ReLU on the output values
        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_3");
            if (regs_ping[row_wgt] < 0) regs_ping[row_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 4 1x316 x 316x256
    ///////////////////////////////// 
    {
        // Read from Layer 0 inputs from scratchpad
        for (uint16_t elem_in = LAYER_N_DIMS; elem_in < LAYER_4_INPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_inputs_4");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_pos);
            regs_ping[elem_in] = plm_pos[elem_in-LAYER_N_DIMS];
        }

        compute_kernel_tile(LAYER_4_INPUTS, LAYER_4_OUTPUTS, plm_wgt_4, plm_bias_4,
                            regs_ping, regs_pong, regs_mul, regs_wgt);

        // ReLU on the output values
        for (uint16_t row_wgt = 0; row_wgt < LAYER_4_OUTPUTS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_4");
            if (regs_pong[row_wgt] < 0) regs_pong[row_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 5 1x256 x 256x256
    ///////////////////////////////// 
    {
        compute_kernel_tile(LAYER_N_DIMS, LAYER_N_DIMS, plm_wgt_5, plm_bias_5,
                            regs_pong, regs_ping, regs_mul, regs_wgt);

        // ReLU on the output values
        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_5");
            if (regs_ping[row_wgt] < 0) regs_ping[row_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 6 1x256 x 256x256
    ///////////////////////////////// 
    {
        compute_kernel_tile(LAYER_N_DIMS, LAYER_N_DIMS, plm_wgt_6, plm_bias_6,
                            regs_ping, regs_pong, regs_mul, regs_wgt);

        // ReLU on the output values
        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_6");
            if (regs_pong[row_wgt] < 0) regs_pong[row_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 7 1x256 x 256x256
    ///////////////////////////////// 
    {
        compute_kernel_tile(LAYER_N_DIMS, LAYER_N_DIMS, plm_wgt_7, plm_bias_7,
                            regs_pong, regs_ping, regs_mul, regs_wgt);
    }

    /////////////////////////////////
    // Layer 8 1x280 x 256x256
    ///////////////////////////////// 
    {
        // Read from Layer 0 inputs from scratchpad
        for (uint16_t elem_in = LAYER_N_DIMS; elem_in < LAYER_8_INPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_inputs_8");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_dir);
            regs_ping[elem_in] = plm_dir[elem_in-LAYER_N_DIMS];
        }
        
        compute_kernel_tile(LAYER_8_INPUTS, LAYER_8_OUTPUTS, plm_wgt_8, plm_bias_8,
                            regs_ping, regs_pong, regs_mul, regs_wgt);

        // ReLU on the output values
        for (uint16_t row_wgt = 0; row_wgt < LAYER_8_OUTPUTS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_8");
            if (regs_pong[row_wgt] < 0) regs_pong[row_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 9 1x256 x 256x128
    ///////////////////////////////// 
    {
        compute_kernel_tile(LAYER_9_INPUTS, LAYER_9_OUTPUTS, plm_wgt_9, plm_bias_9,
                            regs_pong, regs_ping, regs_mul, regs_wgt);

        // ReLU on the output values
        for (uint16_t row_wgt = 0; row_wgt < LAYER_9_OUTPUTS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_9");
            if (regs_ping[row_wgt] < 0) regs_ping[row_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 10 1x128 x 128x3
    ///////////////////////////////// 
    {
        compute_kernel_tile(LAYER_10_INPUTS, LAYER_10_OUTPUTS, plm_wgt_10, plm_bias_10,
                            regs_ping, regs_pong, regs_mul, regs_wgt);
    }

    // Conclude
    {
        // the store can now write back the output data
        this->compute_store_handshake();

        this->process_done();
    }
}
