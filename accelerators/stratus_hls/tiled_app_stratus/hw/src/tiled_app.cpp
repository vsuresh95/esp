// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "tiled_app.hpp"
#include "tiled_app_directives.hpp"

// Functions

#include "tiled_app_functions.hpp"

//#define PRINT_ALL 1

// Processes

// #define SYNC_BITS 1

#define POLL_PROD_VALID_REQ 1
#define LOAD_DATA_REQ 2
#define POLL_CONS_READY_REQ 3
#define LOAD_DONE 5

#define UPDATE_CONS_VALID_REQ 6
#define UPDATE_PROD_VALID_REQ 7
#define UPDATE_CONS_READY_REQ 8
#define UPDATE_PROD_READY_REQ 9
#define STORE_DATA_REQ 10
#define STORE_DONE 11
#define STORE_FENCE 12

#define POLL_DONE 13
#define UPDATE_DONE 14

#define COMPUTE 4

#define ALPHA 1

void tiled_app::asi_controller(){
    {
        HLS_PROTO("asi_controller-reset");
        load_next_tile.req.reset_req(); //invoke load
        load_done.ack.reset_ack();
        input_ready.req.reset_req(); //invoke compute
        compute_done.ack.reset_ack();
        output_ready.req.reset_req(); //invoke store
        store_done.ack.reset_ack();

        asi_state_dbg.write(0);

        this->reset_accelerator_done();
        wait();
    }
    conf_info_t config;
    // int32_t num_tiles;
    {
        HLS_PROTO("asi_controller-config");
        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        // num_tiles = config.num_tiles;
    }

    while(true){
        HLS_PROTO("asi_controller-body");
        HLS_UNROLL_LOOP(OFF);

        ////Read next tile
        ////Update load sync
        //asi_state_dbg.write(UPDATE_PROD_READY_REQ);
        //store_state = UPDATE_PROD_READY_REQ;
        //this->compute_store_handshake(); //Non blocking signal to Store to resume
        //wait();
        //this->store_done_ack(); 
        //wait();


        ////Write Fence
        //asi_state_dbg.write(STORE_FENCE);
        //store_state = STORE_FENCE;
        //this->compute_store_handshake(); //Non blocking signal to Store to resume
        //wait();
        //this->store_done_ack();  
        //wait();

        //Read next tile
        asi_state_dbg.write(POLL_PROD_VALID_REQ);
        load_state = POLL_PROD_VALID_REQ;
        this->load_next_tile_req();  //Enable next iteration of input data tile
        wait();
        this->load_done_ack();
        wait();


        asi_state_dbg.write(UPDATE_PROD_VALID_REQ);
        store_state = UPDATE_PROD_VALID_REQ;
        this->compute_store_handshake(); //Non blocking signal to Store to resume
        wait();
        this->store_done_ack();  
        wait();

        //Write Fence
        asi_state_dbg.write(STORE_FENCE);
        store_state = STORE_FENCE;
        this->compute_store_handshake(); //Non blocking signal to Store to resume
        wait();
        this->store_done_ack();  
        wait();

        asi_state_dbg.write(LOAD_DATA_REQ);
        load_state = LOAD_DATA_REQ;
        this->load_next_tile_req();  
        wait();
        this->load_done_ack();
        wait();

        if(! last_task){
            //Update load sync
            asi_state_dbg.write(UPDATE_PROD_READY_REQ);
            store_state = UPDATE_PROD_READY_REQ;
            this->compute_store_handshake(); //Non blocking signal to Store to resume
            wait();
            this->store_done_ack(); //Block till Store has finished writing previous data over DMA
            wait();

            //Write Fence
            asi_state_dbg.write(STORE_FENCE);
            store_state = STORE_FENCE;
            this->compute_store_handshake(); //Non blocking signal to Store to resume
            wait();
            this->store_done_ack();  
            wait();
        }

        //Compute Intensity
        asi_state_dbg.write(COMPUTE);
        this->load_compute_handshake();
        wait();
        this->compute_done_ack();
        wait();

        //Check if can store next tile
        asi_state_dbg.write(POLL_CONS_READY_REQ);
        load_state = POLL_CONS_READY_REQ;
        this->load_next_tile_req();  
        wait();
        this->load_done_ack();
        wait();

        //Store next tile
        asi_state_dbg.write(UPDATE_CONS_READY_REQ);
        store_state = UPDATE_CONS_READY_REQ;
        this->compute_store_handshake(); //Non blocking signal to Store to resume
        wait();
        this->store_done_ack();  
        wait();

        //Write Fence
        asi_state_dbg.write(STORE_FENCE);
        store_state = STORE_FENCE;
        this->compute_store_handshake(); //Non blocking signal to Store to resume
        wait();
        this->store_done_ack();  
        wait();

        //Store next tile
        asi_state_dbg.write(STORE_DATA_REQ);
        store_state = STORE_DATA_REQ;
        this->compute_store_handshake(); //Non blocking signal to Store to resume
        wait();
        this->store_done_ack();  
        wait();
        
        //Write Fence
        asi_state_dbg.write(STORE_FENCE);
        store_state = STORE_FENCE;
        this->compute_store_handshake(); //Non blocking signal to Store to resume
        wait();
        this->store_done_ack();  
        wait();

        //Store next tile
        asi_state_dbg.write(UPDATE_CONS_VALID_REQ);
        store_state = UPDATE_CONS_VALID_REQ;
        this->compute_store_handshake(); //Non blocking signal to Store to resume
        wait();
        this->store_done_ack();  
        wait();

        //Write Fence
        asi_state_dbg.write(STORE_FENCE);
        store_state = STORE_FENCE;
        this->compute_store_handshake(); //Non blocking signal to Store to resume
        wait();
        this->store_done_ack();  
        wait();

        if(last_task){ 
            this->accelerator_done();
            this->process_done();
        }
    }
}

void tiled_app::load_input()
{

    // Reset
    {
        HLS_PROTO("load-reset");
        load_iter_dbg.write(0);
        load_state_dbg.write(0);
        load_unit_sp_write_dbg.write(0);
        load_next_tile.ack.reset_ack();

        load_done.req.reset_req();

        this->reset_dma_read();
        wait();
    }

    // Config
    /* <<--params-->> */
    // int32_t ping_pong_en;
    int32_t input_tile_start_offset;
    int32_t output_spin_sync_offset;
    int32_t input_spin_sync_offset ;
    // int32_t num_tiles;
    int32_t tile_size;
    // int32_t tile_no;
    bool ping;
    int32_t curr_tile;
    uint32_t offset;
    uint32_t sp_offset;
    uint32_t length; 
    int64_t data, data2;
    conf_info_t config;
    {
        HLS_PROTO("load-config");
        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        // ping_pong_en = config.ping_pong_en;
        output_spin_sync_offset = config.output_spin_sync_offset  ;
        input_spin_sync_offset  = config.input_spin_sync_offset   ;
        // num_tiles = config.num_tiles;
        tile_size = config.tile_size;
        // tile_no = config.rd_wr_enable;
        input_tile_start_offset = config.input_tile_start_offset;

        ping = true;
        curr_tile = 0;
        offset = input_tile_start_offset;
        sp_offset = 0;
        length = tile_size; 
    }
   
    while(true){
        HLS_UNROLL_LOOP(OFF);

        {
            HLS_PROTO("load-next-tile");
            this->load_next_tile_ack();
            wait();

            load_state_dbg.write(load_state);
            load_iter_dbg.write(curr_tile);
            wait();
        }
        // load_state_dbg.write(load_state);
        switch(load_state){
            // case LOAD_DONE:{
            //     this->process_done();
            // }
            // break;
            case POLL_PROD_VALID_REQ: 
            case POLL_CONS_READY_REQ: { 
                {
                    HLS_PROTO("load-dma-poll");
                    int32_t sync_offset = input_spin_sync_offset;
                    int32_t sync_len = 2;
                    if(load_state == POLL_CONS_READY_REQ) {
                        sync_offset = output_spin_sync_offset;
                        sync_len = 1;
                    }
                    // Wait on SYNC FLAG
                    while(true){ 
                        HLS_UNROLL_LOOP(OFF);
                        sc_dt::sc_bv<DMA_WIDTH> dataBvin;
                        dma_info_t dma_info2(sync_offset, sync_len, DMA_SIZE);
                        this->dma_read_ctrl.put(dma_info2);
                        wait();
                        load_unit_sp_write_dbg.write(1); 
                        dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
                        wait();
                        load_unit_sp_write_dbg.write(2); 
                        data = dataBvin.range(DMA_WIDTH - 1, 0).to_int64();
                        wait();
                        load_unit_sp_write_dbg.write(3); 
                        if(load_state == POLL_PROD_VALID_REQ) {
                            dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
                            data2 = dataBvin.range(DATA_WIDTH - 1, 0).to_int64();
                            wait();
                            if(data2) last_task = 1;
                            load_unit_sp_write_dbg.write(4); 
                            wait();
                        }
                        curr_tile++;
                        if(data == 1){
                            load_state_dbg.write(POLL_DONE);
                            wait();
                            load_unit_sp_write_dbg.write(5); 
                            break;
                        }
                    }
                    wait();
                }
                // wait(); 
            }
            break;
            case LOAD_DATA_REQ: {
                //READ NEW DATA
                {   HLS_PROTO("load-dma-data");
                    uint32_t len = length > PLM_IN_WORD ? PLM_IN_WORD : length;
                    dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
                    this->dma_read_ctrl.put(dma_info);
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
                    wait();
                    ping = !ping;
                    load_state_dbg.write(LOAD_DONE);
                    wait();
                }
            }
            break;
        }

        {
            HLS_PROTO("load-dma-handshake");
            this->load_done_req();
            wait();
        }
    }
	// }
}



void tiled_app::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");
	    store_iter_dbg.write(0);
        store_state_dbg.write(0);

        output_ready.ack.reset_ack();
        store_done.req.reset_req();
        wait();
        last_task = 0;
        // explicit PLM ports reset if any
        this->reset_dma_write();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t output_tile_start_offset;
    int32_t output_update_sync_offset;
    int32_t input_update_sync_offset ;
    int32_t output_spin_sync_offset;
    int32_t input_spin_sync_offset ;
    // int32_t num_tiles;
    int32_t tile_size;
    // int32_t tile_no;
    uint32_t offset;
    bool ping;
    uint32_t sp_offset;
    int32_t curr_tile;
    uint32_t length;
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
        output_spin_sync_offset = config.output_spin_sync_offset;
        input_spin_sync_offset  = config.input_spin_sync_offset ;
        // num_tiles = config.num_tiles;
        tile_size = config.tile_size;
        // tile_no = config.rd_wr_enable;
        offset = output_tile_start_offset;
        ping = true;
        sp_offset = 0;
        curr_tile = 0;
        length = tile_size;
    }

    // Store

    while(true){
        HLS_UNROLL_LOOP(OFF);
        store_state_dbg.write(store_state);
        store_iter_dbg.write(curr_tile);
        // Wait for handshake
        this->store_compute_handshake();
        int32_t sync_flag = (store_state == UPDATE_PROD_VALID_REQ || store_state == UPDATE_CONS_READY_REQ )? 0 : 1;

        switch(store_state){
            // case STORE_DONE:
            // {
            //     this->process_done();
            // }
            case STORE_FENCE:{
                this->acc_fence.put(0x2);
                wait();
                while (!(this->acc_fence.ready)) wait();
                wait();
            }
            break;
            //State for last task
            case UPDATE_CONS_VALID_REQ:
            case UPDATE_PROD_VALID_REQ:
            case UPDATE_CONS_READY_REQ:
            case UPDATE_PROD_READY_REQ:{
                {
                    HLS_PROTO("store-dma-poll");
                    int32_t sync_offset = input_update_sync_offset; //READY_FLAG_OFFSET;
                    int32_t sync_len = 1;
                    if(store_state == UPDATE_PROD_VALID_REQ) sync_offset = input_spin_sync_offset;//VALID_FLAG_OFFSET;
                    else if(store_state == UPDATE_CONS_VALID_REQ) {
                        sync_offset = output_update_sync_offset;
                        sync_len = 2;
                    }
                    else if(store_state == UPDATE_CONS_READY_REQ) sync_offset = output_spin_sync_offset;

                    dma_info_t dma_info(sync_offset / DMA_WORD_PER_BEAT, sync_len / DMA_WORD_PER_BEAT, DMA_SIZE);
                    sc_dt::sc_bv<DMA_WIDTH> dataBv;
                    dataBv.range(DMA_WIDTH - 1, 0) = sync_flag;
                    this->dma_write_ctrl.put(dma_info);
                    wait();
                    this->dma_write_chnl.put(dataBv);
                    wait();
                    if(store_state == UPDATE_CONS_VALID_REQ){
                        dataBv.range(DMA_WIDTH - 1, 0) = last_task;
                        this->dma_write_chnl.put(dataBv);
                        wait();
                    }

                    // Wait till the write is accepted at the cache (and previous fences)
                    while (!(this->dma_write_chnl.ready)) wait();
                    wait();
                }
            }
            break;

            case STORE_DATA_REQ: {
                {   HLS_PROTO("store-dma-data");
                    uint32_t len = length;
                    uint32_t len_final = len; 
                    dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len_final, DMA_SIZE);
                    this->dma_write_ctrl.put(dma_info);
                    for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                    {
                        HLS_UNROLL_LOOP(OFF);
                        HLS_BREAK_DEP(plm_in_ping);
                        HLS_BREAK_DEP(plm_in_pong);
                        sc_dt::sc_bv<DMA_WIDTH> dataBv;
                        // Read from PLM
                        wait();
                        if (ping)
                            dataBv.range(DATA_WIDTH - 1, 0) = plm_in_ping[sp_offset + i];
                        else
                            dataBv.range(DATA_WIDTH - 1, 0) = plm_in_pong[sp_offset + i];
                        wait();
                        this->dma_write_chnl.put(dataBv);
                        wait();
                    }
                    curr_tile++;
                }
            } 
            break;
        }
        {
            HLS_PROTO("store-dma-handshake");
            this->store_done_req();
            wait();
        }
    }
    // }
}





//
//
////compute is invoked after read tile is over.
////compute should read the sync variable and set to 1, and keep spinning for 0
////read should have started with reading the sync variable (for next tile)
////once 0, exit to read_input() again
//void tiled_app::compute_kernel()
//{
//    // Reset
//    {
//        HLS_PROTO("compute-reset");
//        // this->reset_compute_kernel();
//        wait();
//        input_ready.ack.reset_ack();
//        compute_done.req.reset_req();
//        compute_state_dbg.write(0);
//
//    }
//
//    int32_t num_tiles;
//    // int32_t ping_pong_en;
//    int32_t tile_size;
//    int32_t compute_over_data;
//    int32_t op_count;
//    bool ping;
//    int32_t compute_tiles;
//    int32_t stride;
//    conf_info_t config;
//    {
//        HLS_PROTO("compute-config");
//
//        cfg.wait_for_config(); // config process
//        config = this->conf_info.read();
//
//        // User-defined config code
//        /* <<--local-params-->> */
//        num_tiles = config.num_tiles;
//        // ping_pong_en = config.ping_pong_en;
//        tile_size = config.tile_size;
//        compute_over_data = config.compute_over_data;
//        // if(compute_over_data> tile_size) compute_over_data = tile_size;
//        op_count = config.compute_iters;
//        // if(compute_iters <0) compute_iters = 0;
//        compute_tiles = 0;
//        ping = true;
//        stride = compute_over_data;
//    }
//
//    while(true){
//        {
//            HLS_PROTO("compute-block-input-handshake");
//            this->compute_load_handshake(); // Ack new input tile
//            wait();
//            compute_state_dbg.write(1);
//            wait();
//        }
//        //IF COMPUTE INTENSITY > 0
//        {
//            // if(compute_iters > 0){
//            //     int stride = 1;
//            //     for(int i = 0; i<compute_over_data; i+=stride){
//            //         HLS_UNROLL_LOOP(OFF);
//            //         HLS_BREAK_DEP(plm_in_ping);
//            //         HLS_BREAK_DEP(plm_in_pong);
//            //         int64_t beta = 2;
//            //         int64_t A_I = (ping)? plm_in_ping[i] : plm_in_pong[i];
//                    
//            //         for(int iter = 0; iter<compute_iters; iter++){
//            //             HLS_UNROLL_LOOP(OFF);
//            //             beta = beta * A_I + ALPHA;
//            //         }
//                    
//            //         if (ping)
//            //             plm_in_ping[i] = beta;
//            //         else
//            //             plm_in_pong[i] = beta;
//            //     }
//            // }
//            if(op_count > 0 && compute_tiles < num_tiles){
//                
//                for(int i = 0; i<tile_size; i+=stride){
//                    HLS_UNROLL_LOOP(OFF);
//                    HLS_BREAK_DEP(plm_in_ping);
//                    HLS_BREAK_DEP(plm_in_pong);
//                    int64_t beta = 2;
//                    int64_t A_I = (ping)? plm_in_ping[i] : plm_in_pong[i];
//                    
//                    for(int iter = 0; iter<op_count; iter++){
//                        HLS_UNROLL_LOOP(OFF);
//                        beta = beta * A_I + ALPHA;
//                    }
//                    
//                    if (ping)
//                        plm_in_ping[i] = beta;
//                    else
//                        plm_in_pong[i] = beta;
//                }
//                compute_tiles = compute_tiles + 1;
//            }
//        // }
//        }
//        //add return handshake
//        {
//            HLS_PROTO("compute-block-output-handshake");
//            ping = !ping;
//            compute_state_dbg.write(2);
//            this->compute_done_req();
//            wait();
//            // if(last_task){
//            //     compute_state_dbg.write(3);
//            //     this->process_done();
//            // }
//        }
//    }
//
//}

void tiled_app::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");
        // this->reset_compute_kernel();
        wait();
        input_ready.ack.reset_ack();
        compute_done.req.reset_req();
        compute_state_dbg.write(0);

    }

    int32_t num_tiles;
    // int32_t ping_pong_en;
    int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    int32_t compute_tiles;
    int32_t stride;
    conf_info_t config;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        num_tiles = config.num_tiles;
        tile_size = config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-block-input-handshake");
            this->compute_load_handshake(); // Ack new input tile
            wait();
            compute_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        {
            if(op_count > 0){
                // int stride = 1;
                for(int i = 0; i<compute_over_data; i+=stride){
                    HLS_UNROLL_LOOP(OFF);
                    HLS_BREAK_DEP(plm_in_ping);
                    HLS_BREAK_DEP(plm_in_pong);
                    // HLS_BREAK_DEP(compute_0_ping);
                    // HLS_BREAK_DEP(compute_0_pong);
                    int64_t beta = 2;
                    int64_t A_I = (ping)? plm_in_ping[i] : plm_in_pong[i];
                    
                    for(int iter = 0; iter<op_count; iter++){
                        HLS_UNROLL_LOOP(OFF);
                        beta = beta * A_I + ALPHA;
                    }
                    
                    if (ping)
                        plm_in_ping[i] = beta;
                    else
                        plm_in_pong[i] = beta;
                }
            }
        }
        //add return handshake
        {
            HLS_PROTO("compute-block-output-handshake");
            compute_state_dbg.write(2);
            wait();
            ping = !ping;
            this->compute_done_req();
            wait();
            compute_state_dbg.write(3);
            wait();
        }
    }
}




