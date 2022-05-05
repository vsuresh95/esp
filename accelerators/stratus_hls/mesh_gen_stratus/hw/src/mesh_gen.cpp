// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#define HASH_ENTRY_LEN 2 // 3 short + 1 int + 1 int ~= 4 int = 2 long

#include "mesh_gen.hpp"
#include "mesh_gen_directives.hpp"

// Functions

#include "mesh_gen_functions.hpp"

// Processes

void mesh_gen::load_input()
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
    int32_t num_hash_table;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        num_hash_table = config.num_hash_table;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        wait();

        // number of allocated hash table entries
        uint32_t length = round_up(num_hash_table, DMA_WORD_PER_BEAT);

        // Fetch hash entries one by one
        for (uint16_t offset = 0; offset < length; offset += HASH_ENTRY_LEN)
        {
            dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, HASH_ENTRY_LEN / DMA_WORD_PER_BEAT, DMA_SIZE);

            this->dma_read_ctrl.put(dma_info);

            for (uint16_t j = 0; j < HASH_ENTRY_LEN; j += DMA_WORD_PER_BEAT)
            {
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                dataBv = this->dma_read_chnl.get();
                wait();

                // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    plm_hashtable[j + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                }
            }

            this->load_compute_handshake();
        }
    }

    // Conclude
    {
        this->process_done();
    }
}



void mesh_gen::store_output()
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
    int32_t num_hash_table;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        num_hash_table = config.num_hash_table;
    }

    // Store
    {
        HLS_PROTO("store-dma");

        // number of allocated hash table entries
        uint32_t length = round_up(num_hash_table, DMA_WORD_PER_BEAT);
        uint32_t store_offset = num_hash_table * 2; // 2 d-words per hash table entry

        // Store back computed triangles for hash entries read
        for (uint16_t offset = 0; offset < length; offset += HASH_ENTRY_LEN)
        {
            wait();

            uint32_t len = round_up(512*5*3*3/2, DMA_WORD_PER_BEAT); // 512 voxels, 5 triangle, 3 points, 3 dim, float
            store_offset += len;

            // Chunking
            this->store_compute_handshake();

            // Configure DMA transaction
            dma_info_t dma_info(store_offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);

            this->dma_write_ctrl.put(dma_info);

            for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
            {
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                // Read from PLM
                wait();
                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_triangles[i + k];
                }
                this->dma_write_chnl.put(dataBv);
            }        
        }
    }

    // Conclude
    {
        this->accelerator_done();
        this->process_done();
    }
}


void mesh_gen::compute_kernel()
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
    int32_t num_hash_table;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        num_hash_table = config.num_hash_table;
    }


    // Compute
    {
        // number of allocated hash table entries
        uint32_t length = round_up(num_hash_table, DMA_WORD_PER_BEAT);

        // Perform compute for all hash table entries
        for (uint16_t offset = 0; offset < length; offset += HASH_ENTRY_LEN)
        {
            this->compute_load_handshake();

            // Read voxel block
            uint32_t len = round_up(, DMA_WORD_PER_BEAT); // size of 512 voxels

            dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, length / DMA_WORD_PER_BEAT, DMA_SIZE);

            this->dma_read_ctrl.put(dma_info);

            for (uint16_t j = 0; j < len; j += DMA_WORD_PER_BEAT)
            {
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                dataBv = this->dma_read_chnl.get();
                wait();

                // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    plm_voxelblock[j + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                }
            }

            // Calculate cube index

            // Triangle calculation

            this->compute_store_handshake();
        }

        // Conclude
        {
            this->process_done();
        }
    }
}
