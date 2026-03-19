// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "vector.hpp"
#include "vector_directives.hpp"

// Functions

#include "vector_functions.hpp"

// Processes

void vector::load_input()
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
    int32_t vector_op;
    int32_t input_len;
    int32_t input1_offset;
    int32_t input2_offset;
    int32_t n_channel;
    bool unary_op;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        vector_op = config.vector_op;
        input_len = config.input_len;
        input1_offset = config.input1_offset;
        input2_offset = config.input2_offset;
        n_channel = config.n_channel;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        wait();

        bool ping = true;
        uint32_t offset1 = round_up(input1_offset, DMA_WORD_PER_BEAT) * 1;
        uint32_t offset2 = round_up(input2_offset, DMA_WORD_PER_BEAT) * 1;

        // Calculate the length to write back based on the vector operation
        uint32_t length;
        uint32_t chunk_words;
        switch (vector_op) {
            case VECTOR_OP_ADD: {
                unary_op = false;
                length = round_up(input_len, DMA_WORD_PER_BEAT);
                chunk_words = PLM_IN_WORD;
            }
            break;
            case VECTOR_OP_AVG_POOL: {
                unary_op = true;
                length = round_up(n_channel * input_len * input_len, DMA_WORD_PER_BEAT);
                uint32_t in_feature_words = input_len * input_len;
                chunk_words = cheap_divider(PLM_IN_WORD, in_feature_words) * in_feature_words;
            }
            break;
            default:
            break;
        }

        // Chunking
        for (int rem = length; rem > 0; rem -= chunk_words)
        {
            wait();
            uint32_t len = rem > chunk_words ? chunk_words : rem;
            // Configure DMA transaction for Input 1
            {
                dma_info_t dma_info(offset1 / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
                offset1 += len;

                this->dma_read_ctrl.put(dma_info);

                for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_in1_ping);
                    HLS_BREAK_DEP(plm_in1_pong);

                    sc_dt::sc_bv<DMA_WIDTH> dataBv;

                    dataBv = this->dma_read_chnl.get();
                    wait();

                    // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (ping)
                            plm_in1_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            plm_in1_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }
            wait();
            // Configure DMA transaction for Input 2
            if (!unary_op)
            {
                dma_info_t dma_info(offset2 / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
                offset2 += len;

                this->dma_read_ctrl.put(dma_info);

                for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_in2_ping);
                    HLS_BREAK_DEP(plm_in2_pong);

                    sc_dt::sc_bv<DMA_WIDTH> dataBv;

                    dataBv = this->dma_read_chnl.get();
                    wait();

                    // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (ping)
                            plm_in2_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            plm_in2_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }
            this->load_compute_handshake();
            ping = !ping;
        }
    }

    // Conclude
    {
        this->process_done();
    }
}



void vector::store_output()
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
    int32_t vector_op;
    int32_t input_len;
    int32_t n_channel;
    int32_t stride;
    int32_t output_offset;
    uint32_t stride_log2;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        vector_op = config.vector_op;
        input_len = config.input_len;
        n_channel = config.n_channel;
        stride = config.stride;
        output_offset = config.output_offset;
    }

    {
        stride_log2 = ilog2(stride);
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        bool ping = true;
        uint32_t offset = round_up(output_offset, DMA_WORD_PER_BEAT) * 1;

        wait();

        // Calculate the length to write back based on the vector operation
        uint32_t length;
        uint32_t chunk_words;
        switch (vector_op) {
            case VECTOR_OP_ADD: {
                length = round_up(input_len, DMA_WORD_PER_BEAT);
                chunk_words = PLM_OUT_WORD;
            }
            break;
            case VECTOR_OP_AVG_POOL: {
                length = round_up(n_channel * (input_len >> stride_log2) * (input_len >> stride_log2), DMA_WORD_PER_BEAT);
                uint32_t output_len = input_len >> stride_log2;
                uint32_t out_feature_words = output_len * output_len;
                chunk_words = cheap_divider(PLM_OUT_WORD, out_feature_words) * out_feature_words;
            }
            break;
            default:
            break;
        }

        // Chunking
        for (int rem = length; rem > 0; rem -= chunk_words)
        {
            this->store_compute_handshake();

            // Configure DMA transaction
            uint32_t len = rem > chunk_words ? chunk_words : rem;
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
                    if (ping)
                        dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_ping[i + k];
                    else
                        dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_pong[i + k];
                }
                this->dma_write_chnl.put(dataBv);
            }
            ping = !ping;
        }
    }

    // Conclude
    {
        this->accelerator_done();
        this->process_done();
    }
}


void vector::compute_kernel()
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
    int32_t vector_op;
    int32_t input_len;
    int32_t stride;
    int32_t do_relu;
    int32_t n_channel;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        vector_op = config.vector_op;
        input_len = config.input_len;
        stride = config.stride;
        do_relu = config.do_relu;
        n_channel = config.n_channel;
    }

    // Compute
    {
        switch (vector_op) {
            case VECTOR_OP_ADD: {
                bool ping = true;
                uint32_t length = round_up(input_len, DMA_WORD_PER_BEAT);

                for (int rem = length; rem > 0; rem -= PLM_IN_WORD)
                {
                    uint32_t len = rem > PLM_IN_WORD ? PLM_IN_WORD : rem;

                    this->compute_load_handshake();

                    // Computing phase implementation
                    for (int i = 0; i < len; i++) {
                        FPDATA out;
                        if (ping) {
                            out = int2fp<FPDATA, WORD_SIZE>(plm_in1_ping[i]) + int2fp<FPDATA, WORD_SIZE>(plm_in2_ping[i]);
                            out = (do_relu == 1) ? (out > FPDATA(0) ? out : FPDATA(0)) : out;
                            plm_out_ping[i] = fp2int<FPDATA, WORD_SIZE>(out);
                        } else {
                            out = int2fp<FPDATA, WORD_SIZE>(plm_in1_pong[i]) + int2fp<FPDATA, WORD_SIZE>(plm_in2_pong[i]);
                            out = (do_relu == 1) ? (out > FPDATA(0) ? out : FPDATA(0)) : out;
                            plm_out_pong[i] = fp2int<FPDATA, WORD_SIZE>(out);
                        }
                    }

                    this->compute_store_handshake();
                    ping = !ping;
                }
            }
            break;
            case VECTOR_OP_AVG_POOL: {
                bool in_ping = true;
                bool out_ping = true;

                uint32_t stride_log2 = ilog2(stride);
                uint32_t output_done = 0;
                uint32_t in_feature_words = input_len * input_len;
                uint32_t in_chunk_words = cheap_divider(PLM_IN_WORD, in_feature_words) * in_feature_words;
                uint32_t input_length = n_channel * in_feature_words;
                uint32_t output_len = input_len >> stride_log2;
                uint32_t out_feature_words = output_len * output_len;
                uint32_t out_chunk_words = cheap_divider(PLM_OUT_WORD, out_feature_words) * out_feature_words;

                for (int rem = input_length; rem > 0; rem -= in_chunk_words) {
                    uint32_t len = rem > in_chunk_words ? in_chunk_words : rem;

                    this->compute_load_handshake();

                    // For each channel, we will average pool the features
                    for (uint32_t in_off = 0; in_off < len; in_off += in_feature_words) {
                        uint32_t input_channel_offset = in_off;
                        uint32_t output_channel_offset = output_done;
                        // For each output element in the pooled output
                        for (int r = 0; r < output_len; r++) {
                            for (int c = 0; c < output_len; c++) {
                                FPDATA_WIDE sum = 0;
                                // Calculate the start index of the pooling window in input
                                uint32_t in_row = r << stride_log2;
                                uint32_t in_col = c << stride_log2;
                                // Sum over the pooling window
                                for (int pr = 0; pr < stride; pr++) {
                                    for (int pc = 0; pc < stride; pc++) {
                                        uint32_t idx = input_channel_offset + (in_row + pr) * input_len + (in_col + pc);
                                        if (idx < input_length) {
                                            if (in_ping)
                                                sum += FPDATA_WIDE(int2fp<FPDATA, WORD_SIZE>(plm_in1_ping[idx]));
                                            else
                                                sum += FPDATA_WIDE(int2fp<FPDATA, WORD_SIZE>(plm_in1_pong[idx]));
                                        }
                                    }
                                }
                                sum = sum >> (2 * stride_log2); // divide by stride*stride using bit shift
                                FPDATA avg = FPDATA(sum);
                                if (out_ping)
                                    plm_out_ping[output_channel_offset + r * output_len + c] = fp2int<FPDATA, WORD_SIZE>(avg);
                                else
                                    plm_out_pong[output_channel_offset + r * output_len + c] = fp2int<FPDATA, WORD_SIZE>(avg);
                            }
                        }
                        output_done += out_feature_words;
                    }

                    if (output_done >= out_chunk_words) {
                        this->compute_store_handshake();
                        out_ping = !out_ping;
                        output_done = 0;
                    }
                    in_ping = !in_ping;
                }

                if (output_done > 0) {
                    this->compute_store_handshake();
                }
            }
            break;
            default:
            break;
        }

        // Conclude
        {
            this->process_done();
        }
    }
}
