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
#define LOAD_STATE_STORE_SYNC 3
#define LOAD_STATE_STORE_HANDSHAKE 4
#define LOAD_STATE_PROC_DONE 5

#define STORE_STATE_WAIT_FOR_HANDSHAKE 0
#define STORE_STATE_LOAD_SYNC 1
#define STORE_STATE_DMA_SEND 2
#define STORE_STATE_SYNC 3
#define STORE_STATE_LOAD_HANDSHAKE 4
#define STORE_STATE_PROC_ACC_DONE 5

#define STORE_STATE_LOAD_FENCE 6
// #define STORE_STATE_LOAD_FENCE_RDY 7
#define STORE_STATE_STORE_FENCE 8
// #define STORE_STATE_STORE_FENCE_RDY 9

void tiled_app::load_input()
{

    // Reset
    {
        HLS_PROTO("load-reset");
        load_iter_dbg.write(0);
        load_state_dbg.write(0);
        load_unit_sp_write_dbg.write(0);
        input_ready.req.reset_req();
        load_next_tile.ack.reset_ack();
        this->reset_dma_read();
        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t input_tile_start_offset;
    int32_t output_spin_sync_offset  ;
    int32_t input_spin_sync_offset   ;
    int32_t num_tiles;
    int32_t tile_size;
    int32_t tile_no;
    conf_info_t config;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        output_spin_sync_offset = config.output_spin_sync_offset  ;
        input_spin_sync_offset  = config.input_spin_sync_offset   ;
        num_tiles = config.num_tiles;
        tile_size = config.tile_size;
        tile_no = config.rd_wr_enable;
        input_tile_start_offset = config.input_tile_start_offset;
    }
   
    // Load
    {
        HLS_PROTO("load-dma");
        wait();
	    int64_t load_state = LOAD_STATE_WAIT_FOR_INPUT_SYNC;
        bool ping = true;
        int32_t curr_tile = 0;
        uint32_t offset = input_tile_start_offset; //64 + tile_no*tile_size; //SYNC_BITS; //0;
        uint32_t sp_offset = 0;
        // uint32_t sync_offset = tile_no; //2*tile_size;
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
                    dma_info_t dma_info2(input_spin_sync_offset, 1, DMA_SIZE);
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
                    load_state = LOAD_STATE_STORE_SYNC;
                }
                break;
                case LOAD_STATE_STORE_SYNC: {
                    int64_t data = 0;
                    sc_dt::sc_bv<DMA_WIDTH> dataBvin;
                    dma_info_t dma_info2(output_spin_sync_offset, 1, DMA_SIZE); //sync_offset+1
                    this->dma_read_ctrl.put(dma_info2);
                    wait();
                    dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
                    wait();
                    data = dataBvin.range(DMA_WIDTH - 1, 0).to_int64();
                    wait();
                    if(data == 1) load_state = LOAD_STATE_STORE_SYNC; //LOAD_STATE_INIT_DMA;
                    else load_state = LOAD_STATE_STORE_HANDSHAKE;
                }
                break;
                case LOAD_STATE_STORE_HANDSHAKE: {
                    this->load_compute_handshake();
                    wait();
                    // this->store_compute_handshake();

                    this->load_next_tile_ack();
                    wait();
                    curr_tile++;
                    if(curr_tile == num_tiles) load_state = LOAD_STATE_PROC_DONE;
                    else load_state = LOAD_STATE_WAIT_FOR_INPUT_SYNC;
                    ping = !ping;
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
        // store_unit_sp_read_dbg.write(0);
        // input_ready.ack.reset_ack();

        output_ready.ack.reset_ack();

        load_sync_done.req.reset_req();
        store_sync_done.req.reset_req();
        wait();
        // explicit PLM ports reset if any
        this->reset_accelerator_done();
        this->reset_dma_write();
        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t output_tile_start_offset;
    int32_t output_update_sync_offset;
    int32_t input_update_sync_offset ;
    int32_t num_tiles;
    int32_t tile_size;
    int32_t tile_no;
    conf_info_t config;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        output_tile_start_offset = config.output_tile_start_offset;
        output_update_sync_offset = config.output_update_sync_offset;
        input_update_sync_offset  = config.input_update_sync_offset ;
        num_tiles = config.num_tiles;
        tile_size = config.tile_size;
        tile_no = config.rd_wr_enable;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        // wait();

        bool ping = true;
        // uint32_t read_offset =  output_tile_start_offset; //64 + tile_no*tile_size; //SYNC_BITS; //0;
        // uint32_t store_offset = round_up(read_offset + tile_size, DMA_WORD_PER_BEAT);//+ SYNC_BITS*DMA_WORD_PER_BEAT
        // uint32_t store_offset = round_up(output_tile_start_offset);//+ SYNC_BITS*DMA_WORD_PER_BEAT;;
        uint32_t offset = output_tile_start_offset;
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
                    this->store_sync_done_req();
                    wait();

                    // this->compute_load_handshake();
                    this->store_compute_handshake();
                    wait();
                    store_state = STORE_STATE_LOAD_SYNC;
                    // store_state = STORE_STATE_DMA_SEND;
                }
                break;
                case STORE_STATE_LOAD_SYNC: {
                    sc_dt::sc_bv<DMA_WIDTH> dataBvSync;
                    dataBvSync.range(DATA_WIDTH - 1, 0) = 0;
                    dma_info_t dma_info_sync(input_update_sync_offset, 1, DMA_SIZE);//tile_no sync location
                    this->dma_write_ctrl.put(dma_info_sync);
                    wait();
                    this->dma_write_chnl.put(dataBvSync);
                    wait();
                    // store_unit_sp_read_dbg.write(0);
                    // wait();
                    while (!(this->dma_write_chnl.ready)) wait();
                    // // Block till L2 to be ready to receive a fence, then send
                    // this->acc_fence.put(0x2);
                    // wait();
                    // while (!(this->acc_fence.ready)) wait();
                    // wait();
                    store_state = STORE_STATE_LOAD_FENCE;
                }
                break;


                case STORE_STATE_LOAD_FENCE:{
                    this->acc_fence.put(0x2);
                    wait();
                    while (!(this->acc_fence.ready)) wait();
                    wait();
                    // this->compute_store_handshake();
		            // wait();
                    this->load_sync_done_req();
                    store_state = STORE_STATE_DMA_SEND;
                }
                break;

                // case STORE_STATE_LOAD_FENCE_RDY:{
                //     if(!(this->acc_fence.ready)){ 
                //         wait();
                //         store_state = STORE_STATE_LOAD_FENCE_RDY; 
                //     }
                //     else store_state = STORE_STATE_DMA_SEND; 
                // }
                // break;

                case STORE_STATE_DMA_SEND: {
                    uint32_t len = length;// > PLM_OUT_WORD ? PLM_OUT_WORD : length;
                    // uint32_t len_w_sync = len+SYNC_BITS; //
                    // uint8_t odd = ((len_w_sync)&1 == 1); 
                    uint32_t len_final = len; //odd ? len_w_sync + 1 : len_w_sync;
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
                            wait();
                            // store_unit_sp_read_dbg.write(plm_in_ping[sp_offset + i]);
                        // }
                        // wait();
                        this->dma_write_chnl.put(dataBv);
                        wait();
                    }
                            
                    // store_state = STORE_STATE_LOAD_HANDSHAKE;
                    store_state = STORE_STATE_SYNC;
                } 
                break;

                case STORE_STATE_SYNC: {
                
                    sc_dt::sc_bv<DMA_WIDTH> dataBvSync;
                    dataBvSync.range(DATA_WIDTH - 1, 0) = 1;
                    dma_info_t dma_info_sync(output_update_sync_offset, 1, DMA_SIZE);//tile_no+1 next sync location
                    wait();
                    this->dma_write_ctrl.put(dma_info_sync);
                    wait();
                    this->dma_write_chnl.put(dataBvSync);
                    wait();
                    store_unit_sp_read_dbg.write(1);
                    wait();
                    while(!(this->dma_write_chnl.ready)) wait(); 
                    store_state = STORE_STATE_STORE_FENCE;
                }
                break;

                case STORE_STATE_STORE_FENCE:{
                    // if(!(this->dma_write_chnl.ready)){ wait();

                    //     store_state = STORE_STATE_STORE_FENCE_DMA_CHL_RDY; }
                    // else {
                        // Block till L2 to be ready to receive a fence, then send
                        this->acc_fence.put(0x2);
                        wait();
                        while(!(this->acc_fence.ready)) wait(); 
                        // store_state = STORE_STATE_LOAD_HANDSHAKE;
                    // }

                    curr_tile++;
                    ping = !ping;
                    if(curr_tile == num_tiles){
                        store_state = STORE_STATE_PROC_ACC_DONE;
                    }
                    else store_state = STORE_STATE_WAIT_FOR_HANDSHAKE;
                }
                break;

                // case STORE_STATE_STORE_FENCE_RDY:{
                //     if(!(this->acc_fence.ready)){ 
                //         wait();
                //         store_state = STORE_STATE_STORE_FENCE_RDY; }
                //     else store_state = STORE_STATE_LOAD_HANDSHAKE; 
                // }
                // break;

                // case STORE_STATE_LOAD_HANDSHAKE:{
                //     curr_tile++;
                //     ping = !ping;
                //     if(curr_tile == num_tiles){
                //         store_state = STORE_STATE_PROC_ACC_DONE;
                //     }
                //     else store_state = STORE_STATE_WAIT_FOR_HANDSHAKE;
                // }
                // break;
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
        input_ready.ack.reset_ack();
        output_ready.req.reset_req();
        load_next_tile.req.reset_req();

        load_sync_done.ack.reset_ack();
        store_sync_done.ack.reset_ack();
    }

    int32_t num_tiles;
    // int32_t tile_size;
    // int32_t tile_no;
    conf_info_t config;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        num_tiles = config.num_tiles;
    }

    {
        HLS_PROTO("compute-block");
        for(int32_t b = 0; b < num_tiles; b++){
            this->compute_load_handshake(); // Ack new input tile
            wait();
            this->store_sync_done_ack(); //Block till Store has finished writing previous data over DMA
            wait();
            this->compute_store_handshake(); //Non blocking signal to Store to resume
            wait();
            this->load_sync_done_ack();  //Block till Store has updated sync bit for Read tile
            wait();
            this->load_next_tile_req();  //Enable next iteration of input data tile
            wait();
        }

    }
    // Conclude
    {
        this->process_done();
    }
}
