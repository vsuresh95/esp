// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "tiled_app.hpp"
#include "tiled_app_directives.hpp"

// Functions

#include "tiled_app_functions.hpp"

//#define PRINT_ALL 1

// Processes

#define SYNC_BITS 1
#define LOAD_STATE_WAIT_FOR_INPUT_SYNC 0
#define LOAD_STATE_INIT_DMA 1
#define LOAD_STATE_READ_DMA 2
#define LOAD_STATE_STORE_HANDSHAKE 3
#define LOAD_STATE_PROC_DONE 4

#define STORE_STATE_WAIT_FOR_HANDSHAKE 0
#define STORE_STATE_DMA_INIT 1
#define STORE_STATE_DMA_SEND 2
#define STORE_STATE_SYNC 3
#define STORE_STATE_LOAD_HANDSHAKE 4
#define STORE_STATE_PROC_ACC_DONE 5

void tiled_app::load_input()
{

    // Reset
    {
        HLS_PROTO("load-reset");
        load_iter_dbg.write(0);
        load_state_dbg.write(0);
        load_unit_sp_write_dbg.write(0);
        input_ready.req.reset_req();
        output_ready.ack.reset_ack();
        this->reset_dma_read();
        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t num_tiles;
    int32_t tile_size;
    conf_info_t config;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        num_tiles = config.num_tiles;
        tile_size = config.tile_size;
    }
   
    // Load
    {
        HLS_PROTO("load-dma");
        wait();
	    int64_t load_state = LOAD_STATE_WAIT_FOR_INPUT_SYNC;
        bool ping = true;
        int32_t curr_tile = 0;
        uint32_t offset = 0; //SYNC_BITS; //0;
        uint32_t sp_offset = 0;
        uint32_t sync_offset = 2*tile_size;
        uint32_t length = tile_size; // round_up(tile_size, DMA_WORD_PER_BEAT);
        // uint32_t read = 0;
        while(true){
            HLS_UNROLL_LOOP(OFF);
            load_state_dbg.write(load_state);
            load_iter_dbg.write(curr_tile);
            switch(load_state){
                case LOAD_STATE_WAIT_FOR_INPUT_SYNC: {
                    int64_t data = 0;
                    sc_dt::sc_bv<DMA_WIDTH> dataBvin;
                    dma_info_t dma_info2(sync_offset, 1, DMA_SIZE);
                    this->dma_read_ctrl.put(dma_info2);
                    wait();
                    dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
                    wait();
                    data = dataBvin.range(DMA_WIDTH - 1, 0).to_int64();
                    wait();
                    if(data == 1) load_state = LOAD_STATE_READ_DMA; //LOAD_STATE_INIT_DMA;
                    else load_state = LOAD_STATE_WAIT_FOR_INPUT_SYNC;
                }
                break;
        
                // case LOAD_STATE_INIT_DMA : {
                //     uint32_t len = length > PLM_IN_WORD ? PLM_IN_WORD : length;
                //     dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
                //     this->dma_read_ctrl.put(dma_info);
                //     load_state = LOAD_STATE_READ_DMA;
                // }
                // break;
                case LOAD_STATE_READ_DMA: {
                    uint32_t len = length > PLM_IN_WORD ? PLM_IN_WORD : length;
                    dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
                    this->dma_read_ctrl.put(dma_info);
                    // uint32_t len = length > PLM_IN_WORD ? PLM_IN_WORD : length;
                    for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                    {
                        HLS_UNROLL_LOOP(OFF);
                        HLS_BREAK_DEP(plm_in_ping);
                        HLS_BREAK_DEP(plm_in_pong);
                        
                        sc_dt::sc_bv<DMA_WIDTH> dataBv;
                        dataBv = this->dma_read_chnl.get();
                        wait();
                        if (ping)
                            plm_in_ping[sp_offset + i] = dataBv.range( DATA_WIDTH - 1, 0).to_int64();
                        else
                            plm_in_pong[sp_offset + i] = dataBv.range( DATA_WIDTH - 1, 0).to_int64();
                        load_unit_sp_write_dbg.write(dataBv.range( DATA_WIDTH - 1, 0).to_int64()); 
                    }
                    load_state = LOAD_STATE_STORE_HANDSHAKE;
                }
                break;
                case LOAD_STATE_STORE_HANDSHAKE: {
                    this->load_compute_handshake();
                    wait();
                    this->store_compute_handshake();
                    wait();
                    curr_tile++;
                    if(curr_tile == num_tiles) load_state = LOAD_STATE_PROC_DONE;
                    else load_state = LOAD_STATE_WAIT_FOR_INPUT_SYNC;
                }
                break;
                case LOAD_STATE_PROC_DONE:{
                    this->process_done();
                }
                break;

                default: {
                    break;
                }

            }
            wait();
        }
	}
}



void tiled_app::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");
	    store_iter_dbg.write(0);
        store_state_dbg.write(0);
        store_unit_sp_read_dbg.write(0);
        input_ready.ack.reset_ack();
        output_ready.req.reset_req();
        wait();
        // explicit PLM ports reset if any
        this->reset_accelerator_done();
        this->reset_dma_write();
        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t num_tiles;
    int32_t tile_size;
    conf_info_t config;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        num_tiles = config.num_tiles;
        tile_size = config.tile_size;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        // wait();

        bool ping = true;
        uint32_t store_offset = round_up(tile_size, DMA_WORD_PER_BEAT);//+ SYNC_BITS*DMA_WORD_PER_BEAT;
        uint32_t offset = store_offset;
        // uint32_t sync_offset = round_up(2*tile_size, DMA_WORD_PER_BEAT);//+ SYNC_BITS*DMA_WORD_PER_BEAT;

        uint32_t sp_offset = 0;
        int32_t curr_tile = 0;
        uint32_t length = tile_size;
        uint32_t store_state = STORE_STATE_WAIT_FOR_HANDSHAKE;

        while(true){
            HLS_UNROLL_LOOP(OFF);
            store_state_dbg.write(store_state);
            store_iter_dbg.write(curr_tile);
            switch(store_state){
                case STORE_STATE_WAIT_FOR_HANDSHAKE:{
                    this->compute_load_handshake();
                    wait();
                    // store_state = STORE_STATE_DMA_INIT;
                    store_state = STORE_STATE_DMA_SEND;
                }
                break;
                // case STORE_STATE_DMA_INIT: {
                //     uint32_t len = length;//> PLM_OUT_WORD ? PLM_OUT_WORD : length;
                //     //len = len + 1; // appending sync bit
                //     dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
                //     this->dma_write_ctrl.put(dma_info);
                //     store_state = STORE_STATE_DMA_SEND;
                //     // wait();
                // }
                // break;

                case STORE_STATE_DMA_SEND: {
                    uint32_t len = length;// > PLM_OUT_WORD ? PLM_OUT_WORD : length;
                    uint32_t len_w_sync = len+SYNC_BITS; //
                    uint8_t odd = ((len_w_sync)&1 == 1); 
                    uint32_t len_final = odd ? len_w_sync + 1 : len_w_sync;
                    dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len_final, DMA_SIZE);
                    this->dma_write_ctrl.put(dma_info);
                    for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                    {
			            HLS_UNROLL_LOOP(OFF);
                        sc_dt::sc_bv<DMA_WIDTH> dataBv;
                        // Read from PLM
                        wait();
                        // if(i==len) dataBv.range(DATA_WIDTH - 1, 0) = 2;
                        // else {
                            if (ping)
                                dataBv.range(DATA_WIDTH - 1, 0) = plm_in_ping[sp_offset + i];
                            else
                                dataBv.range(DATA_WIDTH - 1, 0) = plm_in_pong[sp_offset + i];
                            store_unit_sp_read_dbg.write(plm_in_ping[sp_offset + i]);
                        // }
                        this->dma_write_chnl.put(dataBv);
                        wait();
                    }
                    sc_dt::sc_bv<DMA_WIDTH> dataBvSync;
                    dataBvSync.range(DATA_WIDTH - 1, 0) = 2;
                    this->dma_write_chnl.put(dataBvSync);
                    wait();
                    store_unit_sp_read_dbg.write(2);
                    wait();
                    if(odd){ // extra DMA write??
                        sc_dt::sc_bv<DMA_WIDTH> dataBvSync2;
                        dataBvSync2.range(DATA_WIDTH - 1, 0) = 666;
                        this->dma_write_chnl.put(dataBvSync2);
                        wait();
                    }
                            
                    store_state = STORE_STATE_LOAD_HANDSHAKE;
                    // store_state = STORE_STATE_SYNC;
                } 
                break;

                // case STORE_STATE_SYNC: {
                //     dma_info_t dma_info(sync_offset, 1, DMA_SIZE);
                //     this->dma_write_ctrl.put(dma_info);
                //     wait();
                //     sc_dt::sc_bv<DMA_WIDTH> dataBv;
                //     dataBv.range(DATA_WIDTH - 1, 0) = 2;
                //     this->dma_write_chnl.put(dataBv);
                //     store_state = STORE_STATE_LOAD_HANDSHAKE;

                // }
                // break;

                case STORE_STATE_LOAD_HANDSHAKE:{
                    this->compute_store_handshake();
		            wait();
                    curr_tile++;
                    if(curr_tile == num_tiles){
                        store_state = STORE_STATE_PROC_ACC_DONE;
                    }
                    else store_state = STORE_STATE_WAIT_FOR_HANDSHAKE;
                }
                break;
                case STORE_STATE_PROC_ACC_DONE: {
                    this->accelerator_done();
                    this->process_done();
                }break;
                default: {
                    break;
                }
                
            }
            wait();
        }
    }
}







// compute == synchronizer   
//compute is invoked after read tile is over.
//compute should read the sync variable and set to 1, and keep spinning for 0
//read should have started with reading the sync variable (for next tile)
//once 0, exit to read_input() again
void tiled_app::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");
        // this->reset_compute_kernel();
        wait();
    }


    // Conclude
    {
        this->process_done();
    }
}
