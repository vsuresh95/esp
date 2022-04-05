// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "nerf_mlp.hpp"
#include "nerf_mlp_directives.hpp"

// Functions

#include "nerf_mlp_functions.hpp"

// Processes

// Function to manage load operations for all layers
void nerf_mlp::load_input_dma(uint32_t len, uint32_t offset, uint32_t wgt_or_bias, uint32_t plm_num)
{
    dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);

    this->dma_read_ctrl.put(dma_info);

    for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
    {
        if (wgt_or_bias)
        {
            switch (plm_num)
            {
                case 0: HLS_BREAK_DEP(plm_wgt_0); break;
                case 1: HLS_BREAK_DEP(plm_wgt_1); break;
                case 2: HLS_BREAK_DEP(plm_wgt_2); break;
                case 3: HLS_BREAK_DEP(plm_wgt_3); break;
                case 4: HLS_BREAK_DEP(plm_wgt_4); break;
                case 5: HLS_BREAK_DEP(plm_wgt_5); break;
                case 6: HLS_BREAK_DEP(plm_wgt_6); break;
                case 7: HLS_BREAK_DEP(plm_wgt_7); break;
                case 8: HLS_BREAK_DEP(plm_wgt_8); break;
                case 9: HLS_BREAK_DEP(plm_wgt_9); break;
                case 10: HLS_BREAK_DEP(plm_wgt_10); break;
            }
        }
        else
        {
            switch (plm_num)
            {
                case 0: HLS_BREAK_DEP(plm_bias_0); break;
                case 1: HLS_BREAK_DEP(plm_bias_1); break;
                case 2: HLS_BREAK_DEP(plm_bias_2); break;
                case 3: HLS_BREAK_DEP(plm_bias_3); break;
                case 4: HLS_BREAK_DEP(plm_bias_4); break;
                case 5: HLS_BREAK_DEP(plm_bias_5); break;
                case 6: HLS_BREAK_DEP(plm_bias_6); break;
                case 7: HLS_BREAK_DEP(plm_bias_7); break;
                case 8: HLS_BREAK_DEP(plm_bias_8); break;
                case 9: HLS_BREAK_DEP(plm_bias_9); break;
                case 10: HLS_BREAK_DEP(plm_bias_10); break;
            }
        }

        sc_dt::sc_bv<DMA_WIDTH> dataBv;

        dataBv = this->dma_read_chnl.get();
        wait();

        // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
        for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
        {
            HLS_UNROLL_SIMPLE;
            if (wgt_or_bias)
            {
                switch (plm_num)
                {
                    case 0: plm_wgt_0[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 1: plm_wgt_1[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 2: plm_wgt_2[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 3: plm_wgt_3[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 4: plm_wgt_4[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 5: plm_wgt_5[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 6: plm_wgt_6[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 7: plm_wgt_7[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 8: plm_wgt_8[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 9: plm_wgt_9[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 10: plm_wgt_10[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                }
            }
            else
            {
                switch (plm_num)
                {
                    case 0: plm_bias_0[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 1: plm_bias_1[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 2: plm_bias_2[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 3: plm_bias_3[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 4: plm_bias_4[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 5: plm_bias_5[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 6: plm_bias_6[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 7: plm_bias_7[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 8: plm_bias_8[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 9: plm_bias_9[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                    case 10: plm_bias_10[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64(); break;
                }
            } 
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
        load_input_dma(len, offset, WEIGHT_DMA, 0);
        offset += len; 
 
        // Load layer 1 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, WEIGHT_DMA, 1);
        offset += len;
 
        // Load layer 2 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, WEIGHT_DMA, 2);
        offset += len;

        // Load layer 3 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, WEIGHT_DMA, 3);
        offset += len;

        // Load layer 4 weights
        len = round_up(80896, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, WEIGHT_DMA, 4);
        offset += len;

        // Load layer 5 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, WEIGHT_DMA, 5);
        offset += len;

        // Load layer 6 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, WEIGHT_DMA, 6);
        offset += len;

        // Load layer 7 weights
        len = round_up(65536, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, WEIGHT_DMA, 7);
        offset += len;

        // Load layer 8 weights
        len = round_up(71680, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, WEIGHT_DMA, 8);
        offset += len;

        // Load layer 9 weights
        len = round_up(32768, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, WEIGHT_DMA, 9);
        offset += len;

        // Load layer 10 weights
        len = round_up(372, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, WEIGHT_DMA, 10);
        offset += len;

        // Load all layer biases
        for (int i = 0; i < 11; i++)
        {
            len = round_up(256, DMA_WORD_PER_BEAT);
            load_input_dma(len, offset, BIAS_DMA, i);
            offset += len; 
        }

        // Load input position vectors
        {
            len = round_up(60, DMA_WORD_PER_BEAT);
            offset += len;

            dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);

            this->dma_read_ctrl.put(dma_info);

            for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
            {
                HLS_BREAK_DEP(plm_pos);

                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                dataBv = this->dma_read_chnl.get();
                wait();

                // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    plm_pos[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                }
            }
        }

        wait();

        // Load input direction vectors
        {
            len = round_up(24, DMA_WORD_PER_BEAT);
            offset += len;

            dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);

            this->dma_read_ctrl.put(dma_info);

            for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
            {
                HLS_BREAK_DEP(plm_dir);

                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                dataBv = this->dma_read_chnl.get();
                wait();

                // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    plm_dir[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                }
            }
        }

        wait();

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

        uint32_t offset = round_up(3, DMA_WORD_PER_BEAT) * 1;

        wait();

        uint32_t len = round_up(3, DMA_WORD_PER_BEAT);

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
                dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = regs_out[i];
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

    /////////////////////////////////
    // Layer 0 1x60 x 60x256
    ///////////////////////////////// 
    {
        // Read from Layer 0 inputs from scratchpad
        for (int elem_in = 0; elem_in < LAYER_0_INPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_inputs_0");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_pos);
            regs_ping[elem_in] = plm_pos[elem_in];
        }

        // Read from Layer 0 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_0_OUTPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_0");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_0);
            regs_pong[elem_in] = plm_bias_0[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_0_OUTPUTS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_0_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_0");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_0);
                regs_wgt[elem_wgt] = plm_wgt_0[row_wgt*LAYER_0_INPUTS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_0_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_0");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_ping[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_0_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_0");
                regs_pong[row_wgt] += regs_mul[elem_wgt];
            }
        }

        // ReLU on the output values
        for (int row_wgt = 0; row_wgt < LAYER_0_OUTPUTS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_0");
            if (regs_pong[row_wgt] < 0) regs_pong[row_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 1 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Read from Layer 1 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_N_DIMS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_1");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_1);
            regs_ping[elem_in] = plm_bias_1[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_1");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_1);
                regs_wgt[elem_wgt] = plm_wgt_1[row_wgt*LAYER_N_DIMS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_1");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_pong[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_1");
                regs_ping[row_wgt] += regs_mul[elem_wgt];
            }
        } 

        // ReLU on the output values
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_1");
            if (regs_ping[row_wgt] < 0) regs_ping[row_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 2 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Read from Layer 2 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_N_DIMS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_2");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_2);
            regs_pong[elem_in] = plm_bias_2[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_2");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_2);
                regs_wgt[elem_wgt] = plm_wgt_2[row_wgt*LAYER_N_DIMS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_2");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_ping[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_2");
                regs_pong[row_wgt] += regs_mul[elem_wgt];
            }
        } 

        // ReLU on the output values
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_2");
            if (regs_pong[row_wgt] < 0) regs_pong[row_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 3 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Read from Layer 0 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_N_DIMS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_3");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_3);
            regs_ping[elem_in] = plm_bias_3[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_3");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_3);
                regs_wgt[elem_wgt] = plm_wgt_3[row_wgt*LAYER_N_DIMS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_3");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_pong[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_3");
                regs_ping[row_wgt] += regs_mul[elem_wgt];
            }
        } 

        // ReLU on the output values
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
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
        for (int elem_in = LAYER_N_DIMS; elem_in < LAYER_4_INPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_inputs_4");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_pos);
            regs_ping[elem_in] = plm_pos[elem_in-LAYER_N_DIMS];
        }

        // Read from Layer 4 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_4_OUTPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_4");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_4);
            regs_pong[elem_in] = plm_bias_4[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_4_OUTPUTS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_4_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_4");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_4);
                regs_wgt[elem_wgt] = plm_wgt_4[row_wgt*LAYER_N_DIMS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_4_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_4");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_ping[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_4_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_4");
                regs_pong[row_wgt] += regs_mul[elem_wgt];
            }
        }

        // ReLU on the output values
        for (int row_wgt = 0; row_wgt < LAYER_4_OUTPUTS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_4");
            if (regs_pong[row_wgt] < 0) regs_pong[row_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 5 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Read from Layer 5 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_N_DIMS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_5");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_5);
            regs_ping[elem_in] = plm_bias_5[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_5");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_5);
                regs_wgt[elem_wgt] = plm_wgt_5[row_wgt*LAYER_N_DIMS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_5");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_pong[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_5");
                regs_ping[row_wgt] += regs_mul[elem_wgt];
            }
        }

        // ReLU on the output values
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_5");
            if (regs_ping[row_wgt] < 0) regs_ping[row_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 6 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Read from Layer 6 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_N_DIMS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_6");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_6);
            regs_pong[elem_in] = plm_bias_6[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_6");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_6);
                regs_wgt[elem_wgt] = plm_wgt_6[row_wgt*LAYER_N_DIMS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_6");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_ping[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_6");
                regs_pong[row_wgt] += regs_mul[elem_wgt];
            }
        }

        // ReLU on the output values
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_6");
            if (regs_pong[row_wgt] < 0) regs_pong[row_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 7 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Read from Layer 7 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_N_DIMS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_7");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_7);
            regs_ping[elem_in] = plm_bias_7[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_7");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_7);
                regs_wgt[elem_wgt] = plm_wgt_7[row_wgt*LAYER_N_DIMS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_7");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_pong[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_N_DIMS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_7");
                regs_ping[row_wgt] += regs_mul[elem_wgt];
            }
        }
    }

    /////////////////////////////////
    // Layer 8 1x280 x 256x256
    ///////////////////////////////// 
    {
        // Read from Layer 0 inputs from scratchpad
        for (int elem_in = LAYER_N_DIMS; elem_in < LAYER_8_INPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_inputs_8");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_dir);
            regs_ping[elem_in] = plm_dir[elem_in-LAYER_N_DIMS];
        }
        
        // Read from Layer 8 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_8_OUTPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_8");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_8);
            regs_pong[elem_in] = plm_bias_8[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_8_OUTPUTS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_8_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_8");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_8);
                regs_wgt[elem_wgt] = plm_wgt_8[row_wgt*LAYER_8_INPUTS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_8_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_8");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_ping[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_8_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_8");
                regs_pong[row_wgt] += regs_mul[elem_wgt];
            }
        } 

        // ReLU on the output values
        for (int row_wgt = 0; row_wgt < LAYER_8_OUTPUTS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_8");
            if (regs_pong[row_wgt] < 0) regs_pong[row_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 9 1x256 x 256x128
    ///////////////////////////////// 
    {
        // Read from Layer 9 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_9_OUTPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_9");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_9);
            regs_ping[elem_in] = plm_bias_9[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_9_OUTPUTS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_9_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_9");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_9);
                regs_wgt[elem_wgt] = plm_wgt_9[row_wgt*LAYER_9_INPUTS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_9_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_9");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_pong[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_9_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_9");
                regs_ping[row_wgt] += regs_mul[elem_wgt];
            }
        } 

        // ReLU on the output values
        for (int row_wgt = 0; row_wgt < LAYER_9_OUTPUTS; row_wgt++)
        {
            HLS_UNROLL_LOOP(ON, "relu_9");
            if (regs_ping[row_wgt] < 0) regs_ping[row_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 10 1x128 x 128x3
    ///////////////////////////////// 
    {
        // Read from Layer 10 bias from scratchpad
        for (int elem_in = 0; elem_in < LAYER_10_OUTPUTS; elem_in++)
        {
            HLS_UNROLL_LOOP(ON, "read_bias_10");
            HLS_BREAK_ARRAY_DEPENDENCY(plm_bias_10);
            regs_out[elem_in] = plm_bias_10[elem_in];
        }

        // Loop across the weight rows
        for (int row_wgt = 0; row_wgt < LAYER_10_OUTPUTS; row_wgt++)
        {
            // Read from Layer 0 weights from scratchpad
            for (int elem_wgt = 0; elem_wgt < LAYER_10_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "read_weights_10");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_10);
                regs_wgt[elem_wgt] = plm_wgt_10[row_wgt*LAYER_10_INPUTS + elem_wgt];
            }

            // Multiply and store products in registers
            for (int elem_wgt = 0; elem_wgt < LAYER_10_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "multiply_10");
                regs_mul[elem_wgt] = regs_wgt[elem_wgt] * regs_ping[elem_wgt];
            }

            // Sum the products
            for (int elem_wgt = 0; elem_wgt < LAYER_10_INPUTS; elem_wgt++)
            {
                HLS_UNROLL_LOOP(ON, "sum_10");
                regs_out[row_wgt] += regs_mul[elem_wgt];
            }
        }
    }

    // Conclude
    {
        // the store can now write back the output data
        this->compute_store_handshake();

        this->process_done();
    }
}
