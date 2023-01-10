// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "rotate_order.hpp"
#include "rotate_order_directives.hpp"

// Functions

#include "rotate_order_functions.hpp"

// Processes

void rotate_order::load_input()
{
    // Reset
    {
        HLS_PROTO("load-reset");

        this->reset_load_input();

        load_state_req_dbg.write(0);

        load_ready.ack.reset_ack();
        load_done.req.reset_req();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
    }

    // Load
    while(true)
    {
        HLS_PROTO("load-dma");

        wait();

        this->load_compute_ready_handshake();

        load_state_req_dbg.write(load_state_req);

        switch (load_state_req)
        {
            case POLL_PROD_VALID_REQ:
            {
                dma_info_t dma_info(VALID_FLAG_OFFSET / DMA_WORD_PER_BEAT, READY_FLAG_OFFSET / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                int32_t valid_task = 0;

                wait();

                // Wait for producer to send new data
                while (valid_task != 1)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    valid_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    last_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
            case POLL_CONS_READY_REQ:
            {
                int32_t end_sync_offset = SYNC_VAR_SIZE + 2 * num_samples + READY_FLAG_OFFSET;
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                int32_t ready_for_task = 0;

                wait();

                // Wait for consumer to accept new data
                while (ready_for_task != 1)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    ready_for_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
            case LOAD_DATA_REQ:
            // Load data
            {
                unsigned offset = num_samples * rotate_channels_ready;
                dma_info_t dma_info(SYNC_VAR_SIZE / DMA_WORD_PER_BEAT, num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < num_samples; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_ppfChannels);

                    dataBv = this->dma_read_chnl.get();
                    wait();
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        plm_ppfChannels[offset + i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }
            // Load Rotation factors
            {
                dma_info_t dma_info((SYNC_VAR_SIZE + num_samples) / DMA_WORD_PER_BEAT, 18 / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();
                this->dma_read_ctrl.put(dma_info);

                dataBv = this->dma_read_chnl.get();
                wait();
                m_fCosAlpha = int2fp<FPDATA, WORD_SIZE>(dataBv.range(DATA_WIDTH - 1, 0).to_int64());
                m_fSinAlpha = int2fp<FPDATA, WORD_SIZE>(dataBv.range(2 * DATA_WIDTH - 1, DATA_WIDTH).to_int64());
                dataBv = this->dma_read_chnl.get();
                wait();
                m_fCosBeta = int2fp<FPDATA, WORD_SIZE>(dataBv.range(DATA_WIDTH - 1, 0).to_int64());
                m_fSinBeta = int2fp<FPDATA, WORD_SIZE>(dataBv.range(2 * DATA_WIDTH - 1, DATA_WIDTH).to_int64());;
                dataBv = this->dma_read_chnl.get();
                wait();
                m_fCosGamma = int2fp<FPDATA, WORD_SIZE>(dataBv.range(DATA_WIDTH - 1, 0).to_int64());
                m_fSinGamma = int2fp<FPDATA, WORD_SIZE>(dataBv.range(2 * DATA_WIDTH - 1, DATA_WIDTH).to_int64());;
                dataBv = this->dma_read_chnl.get();
                wait();
                m_fCos2Alpha = int2fp<FPDATA, WORD_SIZE>(dataBv.range(DATA_WIDTH - 1, 0).to_int64());
                m_fSin2Alpha = int2fp<FPDATA, WORD_SIZE>(dataBv.range(2 * DATA_WIDTH - 1, DATA_WIDTH).to_int64());;
                dataBv = this->dma_read_chnl.get();
                wait();
                m_fCos2Beta = int2fp<FPDATA, WORD_SIZE>(dataBv.range(DATA_WIDTH - 1, 0).to_int64());
                m_fSin2Beta = int2fp<FPDATA, WORD_SIZE>(dataBv.range(2 * DATA_WIDTH - 1, DATA_WIDTH).to_int64());;
                dataBv = this->dma_read_chnl.get();
                wait();
                m_fCos2Gamma = int2fp<FPDATA, WORD_SIZE>(dataBv.range(DATA_WIDTH - 1, 0).to_int64());
                m_fSin2Gamma = int2fp<FPDATA, WORD_SIZE>(dataBv.range(2 * DATA_WIDTH - 1, DATA_WIDTH).to_int64());;
                dataBv = this->dma_read_chnl.get();
                wait();
                m_fCos3Alpha = int2fp<FPDATA, WORD_SIZE>(dataBv.range(DATA_WIDTH - 1, 0).to_int64());
                m_fSin3Alpha = int2fp<FPDATA, WORD_SIZE>(dataBv.range(2 * DATA_WIDTH - 1, DATA_WIDTH).to_int64());;
                dataBv = this->dma_read_chnl.get();
                wait();
                m_fCos3Beta = int2fp<FPDATA, WORD_SIZE>(dataBv.range(DATA_WIDTH - 1, 0).to_int64());
                m_fSin3Beta = int2fp<FPDATA, WORD_SIZE>(dataBv.range(2 * DATA_WIDTH - 1, DATA_WIDTH).to_int64());;
                dataBv = this->dma_read_chnl.get();
                wait();
                m_fCos3Gamma = int2fp<FPDATA, WORD_SIZE>(dataBv.range(DATA_WIDTH - 1, 0).to_int64());
                m_fSin3Gamma = int2fp<FPDATA, WORD_SIZE>(dataBv.range(2 * DATA_WIDTH - 1, DATA_WIDTH).to_int64());;
            }
            break;
            default:
            break;
        }

        wait();

        this->load_compute_done_handshake();
    }
} // Function : load_input

void rotate_order::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        store_state_req_dbg.write(0);

        store_ready.ack.reset_ack();
        store_done.req.reset_req();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
    }

    // Store
    while(true)
    {
        HLS_PROTO("store-dma");

        wait();

        this->store_compute_ready_handshake();

        store_state_req_dbg.write(store_state_req);

        switch (store_state_req)
        {
            case UPDATE_PROD_READY_REQ:
            {
                dma_info_t dma_info(READY_FLAG_OFFSET / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 1;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
            case UPDATE_PROD_VALID_REQ:
            {
                dma_info_t dma_info(VALID_FLAG_OFFSET / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 0;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
            case UPDATE_CONS_VALID_REQ:
            {
                int32_t end_sync_offset = SYNC_VAR_SIZE + 2 * num_samples + VALID_FLAG_OFFSET;
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 1;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
            case UPDATE_CONS_READY_REQ:
            {
                int32_t end_sync_offset = SYNC_VAR_SIZE + 2 * num_samples + READY_FLAG_OFFSET;
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 0;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
            case STORE_DATA_REQ:
            {
                unsigned offset = num_samples * rotate_channels_done;
                int32_t end_sync_offset = (2 * SYNC_VAR_SIZE) + (2 * num_samples);
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_write_ctrl.put(dma_info);

                for (int i = 0; i < num_samples; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_ppfChannels);

                    wait();

                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_ppfChannels[offset + i + k];
                    }

                    this->dma_write_chnl.put(dataBv);
                }

                // Wait till the last write is accepted at the cache
                wait();
                while (!(this->dma_write_chnl.ready)) wait();
            }
            break;
            case STORE_FENCE:
            {
                // Block till L2 to be ready to receive a fence, then send
                this->acc_fence.put(0x2);
                wait();
            }
            break;
            case ACC_DONE:
            {
                // Ensure the previous fence was accepted, then acc_done
                while (!(this->acc_fence.ready)) wait();
                wait();
                this->accelerator_done();
                wait();
            }
            break;
            default:
            break;
        }

        wait();

        this->store_compute_done_handshake();
    }
} // Function : store_output

void rotate_order::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        compute_state_req_dbg.write(0);

        load_ready.req.reset_req();
        load_done.ack.reset_ack();
        store_ready.req.reset_req();
        store_done.ack.reset_ack();

        load_state_req = 0;
        store_state_req = 0;

        rotate_channels_ready = 0;
        rotate_channels_done = 0;

        wait();
    }

    while(true)
    {
        // Poll producer's valid for new task
        {
            HLS_PROTO("poll-prod-valid");

            load_state_req = POLL_PROD_VALID_REQ;

            compute_state_req_dbg.write(1);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }
        // Reset producer's valid
        {
            HLS_PROTO("update-prod-valid");

            store_state_req = UPDATE_PROD_VALID_REQ;

            compute_state_req_dbg.write(2);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(3);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }
        // Load input data
        {
            HLS_PROTO("load-input-data");

            load_state_req = LOAD_DATA_REQ;

            compute_state_req_dbg.write(4);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }
        // update producer's ready to accept new data
        {
            HLS_PROTO("update-prod-ready");

            store_state_req = UPDATE_PROD_READY_REQ;

            compute_state_req_dbg.write(5);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(6);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            compute_state_req_dbg.write(7);
        }
        // Control which compute to start
        {
            HLS_PROTO("control-rotate");

            rotate_channels_ready++;

            if (rotate_channels_ready == 3) {
                this->compute_rotate_1_ready_handshake();
                wait();
                this->compute_rotate_1_done_handshake();
                wait();
            } else if (rotate_channels_ready == 8) {
                this->compute_rotate_2_ready_handshake();
                wait();
                this->compute_rotate_2_done_handshake();
                wait();
            } else if (rotate_channels_ready == 15) {
                this->compute_rotate_3_ready_handshake();
                wait();
                this->compute_rotate_3_done_handshake();
                wait();
            } else {
                continue;
            }
        }

        while (rotate_channels_ready != rotate_channels_done)
        {
            // Poll consumer's ready to know if we can send new data
            {
                HLS_PROTO("poll-for-cons-ready");

                load_state_req = POLL_CONS_READY_REQ;

                compute_state_req_dbg.write(8);

                this->compute_load_ready_handshake();
                wait();
                this->compute_load_done_handshake();
                wait();
            }
            // Reset consumer's ready
            {
                HLS_PROTO("update-cons-ready");

                store_state_req = UPDATE_CONS_READY_REQ;

                compute_state_req_dbg.write(9);

                this->compute_store_ready_handshake();
                wait();
                this->compute_store_done_handshake();
                wait();

                // Wait for all writes to be done and then issue fence
                store_state_req = STORE_FENCE;

                compute_state_req_dbg.write(10);

                this->compute_store_ready_handshake();
                wait();
                this->compute_store_done_handshake();
                wait();
            }
            // Store output data
            {
                HLS_PROTO("store-output-data");

                store_state_req = STORE_DATA_REQ;

                compute_state_req_dbg.write(11);

                this->compute_store_ready_handshake();
                wait();
                this->compute_store_done_handshake();
                wait();

                // Wait for all writes to be done and then issue fence
                store_state_req = STORE_FENCE;

                compute_state_req_dbg.write(12);

                this->compute_store_ready_handshake();
                wait();
                this->compute_store_done_handshake();
                wait();
            }
            // update consumer's ready for new data available
            {
                HLS_PROTO("update-cons-valid");

                store_state_req = UPDATE_CONS_VALID_REQ;

                compute_state_req_dbg.write(13);

                this->compute_store_ready_handshake();
                wait();
                this->compute_store_done_handshake();
                wait();

                // Wait for all writes to be done and then issue fence
                store_state_req = STORE_FENCE;

                compute_state_req_dbg.write(14);

                this->compute_store_ready_handshake();
                wait();
                this->compute_store_done_handshake();
                wait();
            }

            rotate_channels_done++;

            if (rotate_channels_done == 15) rotate_channels_done = rotate_channels_ready = 0;
        }
        // End operation
        {
            HLS_PROTO("end-acc");

            if (last_task == 1)
            {
                store_state_req = ACC_DONE;

                compute_state_req_dbg.write(15);

                this->compute_store_ready_handshake();
                wait();
                this->compute_store_done_handshake();
                wait();
                this->process_done();
            }
        }
    } // while (true)
} // Function : compute_kernel

void rotate_order::rotate_order_1()
{

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
    }

    // Main Loop
    while(true)
    {
        HLS_PROTO("rotate-order-1");

        wait();

        this->rotate_1_compute_ready_handshake();

        // Rotate Order 1
        {
            for (unsigned niSample = 0; niSample < num_samples; niSample++)
            {
                FPDATA m_pfTempSample[3];
                FPDATA m_ppfChannels[3];

                m_ppfChannels[kX] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kX + niSample]);
                m_ppfChannels[kY] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kY + niSample]);
                m_ppfChannels[kZ] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kZ + niSample]);

                m_pfTempSample[kY] = -m_ppfChannels[kX] * m_fSinAlpha
                                    + m_ppfChannels[kY] * m_fCosAlpha;
                m_pfTempSample[kZ] = m_ppfChannels[kZ];
                m_pfTempSample[kX] = m_ppfChannels[kX] * m_fCosAlpha
                                    + m_ppfChannels[kY] * m_fSinAlpha;

                // Beta rotation
                m_ppfChannels[kY] = m_pfTempSample[kY];
                m_ppfChannels[kZ] = m_pfTempSample[kZ] * m_fCosBeta
                                    + m_pfTempSample[kX] * m_fSinBeta;
                m_ppfChannels[kX] = m_pfTempSample[kX] * m_fCosBeta
                                    - m_pfTempSample[kZ] * m_fSinBeta;

                // Gamma rotation
                m_pfTempSample[kY] = -m_ppfChannels[kX] * m_fSinGamma
                                    + m_ppfChannels[kY] * m_fCosGamma;
                m_pfTempSample[kZ] = m_ppfChannels[kZ];
                m_pfTempSample[kX] = m_ppfChannels[kX] * m_fCosGamma
                                    + m_ppfChannels[kY] * m_fSinGamma;

                plm_ppfChannels[num_samples*kX + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kX]);
                plm_ppfChannels[num_samples*kY + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kY]);
                plm_ppfChannels[num_samples*kZ + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kZ]);
            }
        }

        this->rotate_1_compute_done_handshake();
    }
}

void rotate_order::rotate_order_2()
{

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
    }

    // Main Loop
    while(true)
    {
        HLS_PROTO("rotate-order-2");

        wait();

        this->rotate_2_compute_ready_handshake();

        // Rotate Order 1
        {
            FPDATA fSqrt3 = sqrt(3);

            for (unsigned niSample = 0; niSample < num_samples; niSample++)
            {
                FPDATA m_pfTempSample[5];
                FPDATA m_ppfChannels[5];

                m_ppfChannels[kR] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kR + niSample]);
                m_ppfChannels[kS] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kS + niSample]);
                m_ppfChannels[kT] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kT + niSample]);
                m_ppfChannels[kU] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kU + niSample]);
                m_ppfChannels[kV] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kV + niSample]);

                // Alpha rotation
                m_pfTempSample[kV] = - m_ppfChannels[kU] * m_fSin2Alpha
                                    + m_ppfChannels[kV] * m_fCos2Alpha;
                m_pfTempSample[kT] = - m_ppfChannels[kS] * m_fSinAlpha
                                    + m_ppfChannels[kT] * m_fCosAlpha;
                m_pfTempSample[kR] = m_ppfChannels[kR];
                m_pfTempSample[kS] = m_ppfChannels[kS] * m_fCosAlpha
                                    + m_ppfChannels[kT] * m_fSinAlpha;
                m_pfTempSample[kU] = m_ppfChannels[kU] * m_fCos2Alpha
                                    + m_ppfChannels[kV] * m_fSin2Alpha;

                // Beta rotation
                m_ppfChannels[kV] = -m_fSinBeta * m_pfTempSample[kT]
                                                + m_fCosBeta * m_pfTempSample[kV];
                m_ppfChannels[kT] = -m_fCosBeta * m_pfTempSample[kT]
                                                + m_fSinBeta * m_pfTempSample[kV];
                m_ppfChannels[kR] = (0.75f * m_fCos2Beta + 0.25f) * m_pfTempSample[kR]
                                    + (0.5 * fSqrt3 * pow(m_fSinBeta,2.0) ) * m_pfTempSample[kU]
                                    + (fSqrt3 * m_fSinBeta * m_fCosBeta) * m_pfTempSample[kS];
                m_ppfChannels[kS] = m_fCos2Beta * m_pfTempSample[kS]
                                    - fSqrt3 * m_fCosBeta * m_fSinBeta * m_pfTempSample[kR]
                                    + m_fCosBeta * m_fSinBeta * m_pfTempSample[kU];
                m_ppfChannels[kU] = (0.25f * m_fCos2Beta + 0.75f) * m_pfTempSample[kU]
                                    - m_fCosBeta * m_fSinBeta * m_pfTempSample[kS]
                                    +0.5 * fSqrt3 * pow(m_fSinBeta,2.0) * m_pfTempSample[kR];

                // Gamma rotation
                m_pfTempSample[kV] = - m_ppfChannels[kU] * m_fSin2Gamma
                                    + m_ppfChannels[kV] * m_fCos2Gamma;
                m_pfTempSample[kT] = - m_ppfChannels[kS] * m_fSinGamma
                                    + m_ppfChannels[kT] * m_fCosGamma;

                m_pfTempSample[kR] = m_ppfChannels[kR];
                m_pfTempSample[kS] = m_ppfChannels[kS] * m_fCosGamma
                                    + m_ppfChannels[kT] * m_fSinGamma;
                m_pfTempSample[kU] = m_ppfChannels[kU] * m_fCos2Gamma
                                    + m_ppfChannels[kV] * m_fSin2Gamma;

                plm_ppfChannels[num_samples*kR + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kR]);
                plm_ppfChannels[num_samples*kS + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kS]);
                plm_ppfChannels[num_samples*kT + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kT]);
                plm_ppfChannels[num_samples*kU + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kU]);
                plm_ppfChannels[num_samples*kV + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kV]);
            }
        }

        this->rotate_2_compute_done_handshake();
    }
}

void rotate_order::rotate_order_3()
{

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
    }

    // Main Loop
    while(true)
    {
        HLS_PROTO("rotate-order-3");

        wait();

        this->rotate_3_compute_ready_handshake();

        // Rotate Order 1
        {
            FPDATA fSqrt3_2 = sqrt(3.f/2.f);
            FPDATA fSqrt15 = sqrt(15.f);
            FPDATA fSqrt5_2 = sqrt(5.f/2.f);

            for (unsigned niSample = 0; niSample < num_samples; niSample++)
            {
                FPDATA m_pfTempSample[7];
                FPDATA m_ppfChannels[7];

                m_ppfChannels[kK] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kK + niSample]);
                m_ppfChannels[kL] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kL + niSample]);
                m_ppfChannels[kM] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kM + niSample]);
                m_ppfChannels[kN] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kN + niSample]);
                m_ppfChannels[kO] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kO + niSample]);
                m_ppfChannels[kP] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kP + niSample]);
                m_ppfChannels[kQ] = int2fp<FPDATA, WORD_SIZE>(plm_ppfChannels[num_samples*kQ + niSample]);

                // Alpha rotation
                m_pfTempSample[kQ] = - m_ppfChannels[kP] * m_fSin3Alpha
                                    + m_ppfChannels[kQ] * m_fCos3Alpha;
                m_pfTempSample[kO] = - m_ppfChannels[kN] * m_fSin2Alpha
                                    + m_ppfChannels[kO] * m_fCos2Alpha;
                m_pfTempSample[kM] = - m_ppfChannels[kL] * m_fSinAlpha
                                    + m_ppfChannels[kM] * m_fCosAlpha;
                m_pfTempSample[kK] = m_ppfChannels[kK];
                m_pfTempSample[kL] = m_ppfChannels[kL] * m_fCosAlpha
                                    + m_ppfChannels[kM] * m_fSinAlpha;
                m_pfTempSample[kN] = m_ppfChannels[kN] * m_fCos2Alpha
                                    + m_ppfChannels[kO] * m_fSin2Alpha;
                m_pfTempSample[kP] = m_ppfChannels[kP] * m_fCos3Alpha
                                    + m_ppfChannels[kQ] * m_fSin3Alpha;

                // Beta rotation
                m_ppfChannels[kQ] = 0.125f * m_pfTempSample[kQ] * (5.f + 3.f*m_fCos2Beta)
                            - fSqrt3_2 * m_pfTempSample[kO] *m_fCosBeta * m_fSinBeta
                            + 0.25f * fSqrt15 * m_pfTempSample[kM] * pow(m_fSinBeta,2.0f);
                m_ppfChannels[kO] = m_pfTempSample[kO] * m_fCos2Beta
                            - fSqrt5_2 * m_pfTempSample[kM] * m_fCosBeta * m_fSinBeta
                            + fSqrt3_2 * m_pfTempSample[kQ] * m_fCosBeta * m_fSinBeta;
                m_ppfChannels[kM] = 0.125f * m_pfTempSample[kM] * (3.f + 5.f*m_fCos2Beta)
                            - fSqrt5_2 * m_pfTempSample[kO] *m_fCosBeta * m_fSinBeta
                            + 0.25f * fSqrt15 * m_pfTempSample[kQ] * pow(m_fSinBeta,2.0f);
                m_ppfChannels[kK] = 0.25f * m_pfTempSample[kK] * m_fCosBeta * (-1.f + 15.f*m_fCos2Beta)
                            + 0.5f * fSqrt15 * m_pfTempSample[kN] * m_fCosBeta * pow(m_fSinBeta,2.f)
                            + 0.5f * fSqrt5_2 * m_pfTempSample[kP] * pow(m_fSinBeta,3.f)
                            + 0.125f * fSqrt3_2 * m_pfTempSample[kL] * (m_fSinBeta + 5.f * m_fSin3Beta);
                m_ppfChannels[kL] = 0.0625f * m_pfTempSample[kL] * (m_fCosBeta + 15.f * m_fCos3Beta)
                            + 0.25f * fSqrt5_2 * m_pfTempSample[kN] * (1.f + 3.f * m_fCos2Beta) * m_fSinBeta
                            + 0.25f * fSqrt15 * m_pfTempSample[kP] * m_fCosBeta * pow(m_fSinBeta,2.f)
                            - 0.125 * fSqrt3_2 * m_pfTempSample[kK] * (m_fSinBeta + 5.f * m_fSin3Beta);
                m_ppfChannels[kN] = 0.125f * m_pfTempSample[kN] * (5.f * m_fCosBeta + 3.f * m_fCos3Beta)
                            + 0.25f * fSqrt3_2 * m_pfTempSample[kP] * (3.f + m_fCos2Beta) * m_fSinBeta
                            + 0.5f * fSqrt15 * m_pfTempSample[kK] * m_fCosBeta * pow(m_fSinBeta,2.f)
                            + 0.125 * fSqrt5_2 * m_pfTempSample[kL] * (m_fSinBeta - 3.f * m_fSin3Beta);
                m_ppfChannels[kP] = 0.0625f * m_pfTempSample[kP] * (15.f * m_fCosBeta + m_fCos3Beta)
                            - 0.25f * fSqrt3_2 * m_pfTempSample[kN] * (3.f + m_fCos2Beta) * m_fSinBeta
                            + 0.25f * fSqrt15 * m_pfTempSample[kL] * m_fCosBeta * pow(m_fSinBeta,2.f)
                            - 0.5 * fSqrt5_2 * m_pfTempSample[kK] * pow(m_fSinBeta,3.f);

                // Gamma rotation
                m_pfTempSample[kQ] = - m_ppfChannels[kP] * m_fSin3Gamma
                                    + m_ppfChannels[kQ] * m_fCos3Gamma;
                m_pfTempSample[kO] = - m_ppfChannels[kN] * m_fSin2Gamma
                                    + m_ppfChannels[kO] * m_fCos2Gamma;
                m_pfTempSample[kM] = - m_ppfChannels[kL] * m_fSinGamma
                                    + m_ppfChannels[kM] * m_fCosGamma;
                m_pfTempSample[kK] = m_ppfChannels[kK];
                m_pfTempSample[kL] = m_ppfChannels[kL] * m_fCosGamma
                                    + m_ppfChannels[kM] * m_fSinGamma;
                m_pfTempSample[kN] = m_ppfChannels[kN] * m_fCos2Gamma
                                    + m_ppfChannels[kO] * m_fSin2Gamma;
                m_pfTempSample[kP] = m_ppfChannels[kP] * m_fCos3Gamma
                                    + m_ppfChannels[kQ] * m_fSin3Gamma;

                plm_ppfChannels[num_samples*kK + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kK]);
                plm_ppfChannels[num_samples*kL + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kL]);
                plm_ppfChannels[num_samples*kM + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kM]);
                plm_ppfChannels[num_samples*kN + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kN]);
                plm_ppfChannels[num_samples*kO + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kO]);
                plm_ppfChannels[num_samples*kP + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kP]);
                plm_ppfChannels[num_samples*kQ + niSample] = fp2int<FPDATA, WORD_SIZE>(m_pfTempSample[kQ]);
            }
        }

        this->rotate_3_compute_done_handshake();
    }
}
