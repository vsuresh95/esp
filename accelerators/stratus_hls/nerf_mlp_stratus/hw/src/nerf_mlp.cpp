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

    for (uint32_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
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
        cur_load_data_dbg.write(0);

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
        uint32_t len = round_up(LAYER_0_INPUTS*LAYER_0_OUTPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_0);
        offset += len; 
 
        // Load layer 0 biases
        len = round_up(LAYER_0_OUTPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_0);
        offset += len; 

        // Load layer 1 weights
        len = round_up(LAYER_N_DIMS*LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_1);
        offset += len;
 
        // Load layer 1 biases
        len = round_up(LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_1);
        offset += len; 
 
        // Load layer 2 weights
        len = round_up(LAYER_N_DIMS*LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_2);
        offset += len;

        // Load layer 2 biases
        len = round_up(LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_2);
        offset += len; 
 
        // Load layer 3 weights
        len = round_up(LAYER_N_DIMS*LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_3);
        offset += len;

        // Load layer 3 biases
        len = round_up(LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_3);
        offset += len; 
 
        // Load layer 4 weights
        len = round_up(LAYER_4_INPUTS*LAYER_4_OUTPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_4);
        offset += len;

        // Load layer 4 biases
        len = round_up(LAYER_4_OUTPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_4);
        offset += len; 
 
        // Load layer 5 weights
        len = round_up(LAYER_N_DIMS*LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_5);
        offset += len;

        // Load layer 5 biases
        len = round_up(LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_5);
        offset += len; 
 
        // Load layer 6 weights
        len = round_up(LAYER_N_DIMS*LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_6);
        offset += len;

        // Load layer 6 biases
        len = round_up(LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_6);
        offset += len; 
 
        // Load layer 7 weights
        len = round_up(LAYER_N_DIMS*LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_7);
        offset += len;

        // Load layer 7 biases
        len = round_up(LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_7);
        offset += len; 
 
        // Load layer 8 weights
        len = round_up(LAYER_8_INPUTS*LAYER_8_OUTPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_8);
        offset += len;

        // Load layer 8 biases
        len = round_up(LAYER_8_OUTPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_8);
        offset += len; 
 
        // Load layer 9 weights
        len = round_up(LAYER_9_INPUTS*LAYER_9_OUTPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_9);
        offset += len;

        // Load layer 9 biases
        len = round_up(LAYER_9_OUTPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_9);
        offset += len;
 
        // Load layer 10 weights
        len = round_up(LAYER_10_INPUTS*LAYER_10_OUTPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_wgt_10);
        offset += len;

        // Load layer 10 biases
        len = round_up(LAYER_10_OUTPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_bias_10);
        offset += len; 

        // Load input position vectors
        len = round_up(LAYER_0_INPUTS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_pos);
        offset += len; 

        // Load input direction vectors
        len = round_up(LAYER_8_INPUTS-LAYER_N_DIMS, DMA_WORD_PER_BEAT);
        load_input_dma(len, offset, plm_dir);
        offset += len;

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

        unsigned output_offset = 
        /* layer 0 */    LAYER_0_INPUTS*LAYER_0_OUTPUTS + LAYER_0_OUTPUTS +
        /* layer 1 */    LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
        /* layer 2 */    LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
        /* layer 3 */    LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
        /* layer 4 */    LAYER_4_INPUTS*LAYER_4_OUTPUTS + LAYER_4_OUTPUTS + 
        /* layer 5 */    LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
        /* layer 6 */    LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
        /* layer 7 */    LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
        /* layer 8 */    LAYER_8_INPUTS*LAYER_8_OUTPUTS + LAYER_8_OUTPUTS + 
        /* layer 9 */    LAYER_9_INPUTS*LAYER_9_OUTPUTS + LAYER_9_OUTPUTS + 
        /* layer 10 */   LAYER_10_INPUTS*LAYER_10_OUTPUTS + LAYER_10_OUTPUTS +
        /* pos inputs */ LAYER_0_INPUTS_ROUND +
        /* dir inputs */ (LAYER_8_INPUTS-LAYER_N_DIMS);

        uint32_t offset = round_up(output_offset, DMA_WORD_PER_BEAT) * 1;

        wait();

        uint32_t len = round_up(LAYER_10_OUTPUTS, DMA_WORD_PER_BEAT);

        this->store_compute_handshake();

        // Configure DMA transaction
        dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
        offset += len;

        this->dma_write_ctrl.put(dma_info);

        for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
        {
            HLS_BREAK_DEP(plm_out);

            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            // Read from PLM
            wait();
            for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
            {
                HLS_UNROLL_SIMPLE;
                dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out[i+k];
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
        cur_wgt_dbg.write(0);
        cur_input_dbg.write(0);
        cur_output_dbg.write(0);
        cur_input_valid_dbg.write(0);
        cur_output_valid_dbg.write(0);

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
        // Loop across the weight column
        col_loop_0: for (uint16_t col_wgt = 0; col_wgt < LAYER_0_OUTPUTS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_0_INPUTS;

            int8_t partial_sum = plm_bias_0[col_wgt];

            // Loop across the weight row
            row_loop_0: for (uint16_t row_wgt = 0; row_wgt < LAYER_0_INPUTS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_0[col_wgt_offset + row_wgt] * plm_pos[row_wgt]);
            }

            if (partial_sum > 0)
                plm_act_1[col_wgt] = partial_sum;
            else
                plm_act_1[col_wgt] = 0;
        }
    }

    ////////////////////////////////
    // Layer 1 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Loop across the weight column
        col_loop_1: for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_N_DIMS;

            int8_t partial_sum = plm_bias_1[col_wgt];

            // Loop across the weight row
            row_loop_1: for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_1[col_wgt_offset + row_wgt] * plm_act_1[row_wgt]);
            }

            if (partial_sum > 0)
                plm_act_2[col_wgt] = partial_sum;
            else
                plm_act_2[col_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 2 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Loop across the weight column
        col_loop_2: for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_N_DIMS;

            int8_t partial_sum = plm_bias_2[col_wgt];

            // Loop across the weight row
            row_loop_2: for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_2[col_wgt_offset + row_wgt] * plm_act_2[row_wgt]);
            }

            if (partial_sum > 0)
                plm_act_3[col_wgt] = partial_sum;
            else
                plm_act_3[col_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 3 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Loop across the weight column
        col_loop_3: for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_N_DIMS;

            int8_t partial_sum = plm_bias_3[col_wgt];

            // Loop across the weight row
            row_loop_3: for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_3[col_wgt_offset + row_wgt] * plm_act_3[row_wgt]);
            }

            if (partial_sum > 0)
                plm_act_4[col_wgt] = partial_sum;
            else
                plm_act_4[col_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 4 1x316 x 316x256
    ///////////////////////////////// 
    {
        // Read from Layer 0 inputs from scratchpad
        input_loop_4: for (uint16_t elem_in = LAYER_N_DIMS; elem_in < LAYER_4_INPUTS; elem_in++)
        {
            plm_act_4[elem_in] = plm_pos[elem_in-LAYER_N_DIMS];
        }

        // Loop across the weight column
        col_loop_4: for (uint16_t col_wgt = 0; col_wgt < LAYER_4_OUTPUTS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_4_INPUTS;

            int8_t partial_sum = plm_bias_4[col_wgt];

            // Loop across the weight row
            row_loop_4: for (uint16_t row_wgt = 0; row_wgt < LAYER_4_INPUTS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_4[col_wgt_offset + row_wgt] * plm_act_4[row_wgt]);
            }

            if (partial_sum > 0)
                plm_act_5[col_wgt] = partial_sum;
            else
                plm_act_5[col_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 5 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Loop across the weight column
        col_loop_5: for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_N_DIMS;

            int8_t partial_sum = plm_bias_5[col_wgt];

            // Loop across the weight row
            row_loop_5: for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_5[col_wgt_offset + row_wgt] * plm_act_5[row_wgt]);
            }

            if (partial_sum > 0)
                plm_act_6[col_wgt] = partial_sum;
            else
                plm_act_6[col_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 6 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Loop across the weight column
        col_loop_6: for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_N_DIMS;

            int8_t partial_sum = plm_bias_6[col_wgt];

            // Loop across the weight row
            row_loop_6: for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_6[col_wgt_offset + row_wgt] * plm_act_6[row_wgt]);
            }

            if (partial_sum > 0)
                plm_act_7[col_wgt] = partial_sum;
            else
                plm_act_7[col_wgt] = 0;
        }
    }
    
    /////////////////////////////////
    // Layer 7 1x256 x 256x256
    ///////////////////////////////// 
    {
        // Loop across the weight column
        col_loop_7: for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_N_DIMS;

            int8_t partial_sum = plm_bias_7[col_wgt];

            // Loop across the weight row
            row_loop_7: for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_7[col_wgt_offset + row_wgt] * plm_act_7[row_wgt]);
            }

            plm_act_8[col_wgt] = partial_sum;
        }

        // NOTE: NO RELU
    }

    /////////////////////////////////
    // Layer 8 1x280 x 256x256
    ///////////////////////////////// 
    {
        input_loop_8: for (uint16_t elem_in = LAYER_N_DIMS; elem_in < LAYER_8_INPUTS; elem_in++)
        {
            plm_act_8[elem_in] = plm_dir[elem_in-LAYER_N_DIMS];
        }

        // Loop across the weight column
        col_loop_8: for (uint16_t col_wgt = 0; col_wgt < LAYER_8_OUTPUTS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_8_INPUTS;

            int8_t partial_sum = plm_bias_8[col_wgt];

            // Loop across the weight row
            row_loop_8: for (uint16_t row_wgt = 0; row_wgt < LAYER_8_INPUTS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_8[col_wgt_offset + row_wgt] * plm_act_8[row_wgt]);
            }

            if (partial_sum > 0)
                plm_act_9[col_wgt] = partial_sum;
            else
                plm_act_9[col_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 9 1x256 x 256x128
    ///////////////////////////////// 
    {
        // Loop across the weight column
        col_loop_9: for (uint16_t col_wgt = 0; col_wgt < LAYER_9_OUTPUTS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_9_INPUTS;

            int8_t partial_sum = plm_bias_9[col_wgt];

            // Loop across the weight row
            row_loop_9: for (uint16_t row_wgt = 0; row_wgt < LAYER_9_INPUTS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_9[col_wgt_offset + row_wgt] * plm_act_9[row_wgt]);
            }

            if (partial_sum > 0)
                plm_act_10[col_wgt] = partial_sum;
            else
                plm_act_10[col_wgt] = 0;
        }
    }

    /////////////////////////////////
    // Layer 10 1x128 x 128x3
    ///////////////////////////////// 
    {
        // Loop across the weight column
        col_loop_10: for (uint16_t col_wgt = 0; col_wgt < LAYER_10_OUTPUTS; col_wgt++)
        {
            uint32_t col_wgt_offset = col_wgt*LAYER_10_INPUTS;

            int8_t partial_sum = plm_bias_10[col_wgt];

            // Loop across the weight row
            row_loop_10: for (uint16_t row_wgt = 0; row_wgt < LAYER_10_INPUTS; row_wgt++)
            {
                partial_sum = partial_sum + (plm_wgt_10[col_wgt_offset + row_wgt] * plm_act_10[row_wgt]);
            }

            plm_out[col_wgt] = partial_sum;
        }

        // NO RELU

    }

    // Conclude
    {
        // the store can now write back the output data
        this->compute_store_handshake();

        this->process_done();
    }
}
