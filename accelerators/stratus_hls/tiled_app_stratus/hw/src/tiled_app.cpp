// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "tiled_app.hpp"
#include "tiled_app_directives.hpp"

// Functions

#include "tiled_app_functions.hpp"

//#define PRINT_ALL 1

// Processes

// #define SYNC_BITS 1

// #define INPUT_ASI 1
// #define OUTPUT_ASI 2
// 
// #define POLL_PROD_VALID_REQ 1
// #define LOAD_DATA_REQ 2
// #define POLL_CONS_READY_REQ 3
// #define LOAD_DONE 5
// 
// #define UPDATE_CONS_VALID_REQ 6
// #define UPDATE_PROD_VALID_REQ 7
// #define UPDATE_CONS_READY_REQ 8
// #define UPDATE_PROD_READY_REQ 9
// #define STORE_DATA_REQ 10
// #define STORE_DONE 11
// #define STORE_FENCE 12
// 
// #define POLL_DONE 13
// #define UPDATE_DONE 14
// 
// #define COMPUTE 13

#define ALPHA 1


void tiled_app::load_input()
{

    // Reset
    {
        HLS_PROTO("load-reset");
        load_iter_dbg.write(0);
        load_state_dbg.write(0);
        load_unit_sp_write_dbg.write(0);
#ifdef SYNTH_APP_CFA
        load_next_tile.ack.reset_ack();
#endif

        load_done.req.reset_req();

        this->reset_dma_read();
        wait();
    }

    // Config
    /* <<--params-->> */
    // int32_t ping_pong_en;
    int32_t input_tile_start_offset;
    //int32_t cons_rdy_offset;
    //int32_t prod_valid_offset ;
    // int32_t num_tiles;
    int32_t tile_size;
    // int32_t tile_no;
#ifndef SYNTH_APP_CFA
    bool ping;
#endif
   // int32_t curr_tile;
    uint32_t offset;
    uint32_t sp_offset;
    uint32_t length; 
    //int64_t data;

    //monolithic add
    //int32_t task_arbiter;
    conf_info_t config;
    {
        HLS_PROTO("load-config");
        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        // ping_pong_en = config.ping_pong_en;
        //cons_rdy_offset = config.cons_ready_offset  ;
        //prod_valid_offset  = config.prod_valid_offset   ;
        // num_tiles = config.num_tiles;
        tile_size = config.tile_size;
        // tile_no = config.rd_wr_enable;
        input_tile_start_offset = config.input_offset;

#ifndef SYNTH_APP_CFA
        ping = true;
#endif
        //curr_tile = 0;
        offset = input_tile_start_offset;
        sp_offset = 0;
        length = tile_size; 
        //monolithic add
        //task_arbiter = 0;
    }
    
	

    
    while(true){
        HLS_UNROLL_LOOP(OFF);

                        //READ NEW DATA
                {   HLS_PROTO("load-dma-data");

        #ifdef  ENABLE_SM
            // Addition for ASI
            this->start_load_asi_handshake();
                                wait();
        #endif
                    #ifdef  ENABLE_SM
                                load_req_dma_read.write(1);
                                wait();
				//uncommented jul14
                                while(dma_read_arbiter.read() != LOAD_UNIT ) wait();
                    #endif

                    uint32_t len = length > PLM_IN_WORD ? PLM_IN_WORD : length;
                    dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
                    this->dma_read_ctrl.put(dma_info);
                    for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                    {
                        HLS_UNROLL_LOOP(OFF);
                        HLS_BREAK_DEP(plm_in_ping);
#ifndef SYNTH_APP_CFA
                        HLS_BREAK_DEP(plm_in_pong);
#endif
                        
                        sc_dt::sc_bv<DMA_WIDTH> dataBv;
                        dataBv = this->dma_read_chnl.get();
                        wait();
#ifndef SYNTH_APP_CFA
                        if (ping)
#endif
                          plm_in_ping[sp_offset + i] = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
#ifndef SYNTH_APP_CFA
                         else
                             plm_in_pong[sp_offset + i] = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
#endif
                        load_unit_sp_write_dbg.write(dataBv.range(DATA_WIDTH - 1, 0).to_int64()); 
                    }
                    wait();
#ifndef SYNTH_APP_CFA
 		    ping = !ping;
#endif
                    load_state_dbg.write(LOAD_DONE);
                    wait();
                    #ifdef  ENABLE_SM
                        load_req_dma_read.write(0);// load relinquishes dma read
                        wait();
                    #endif
                }
        //     }
        //     break;
        // }

        {
            HLS_PROTO("load-dma-handshake");

            this->load_compute_handshake();
            // monolithic change 
            // if (load_state_req_module == INPUT_ASI) {
                load_unit_sp_write_dbg.write(0xc0dec0de);
            //     this->load_done_req();
            // } else if (load_state_req_module == OUTPUT_ASI) {
            //     load_unit_sp_write_dbg.write(0xdeadbeef);
            //     this->load_output_done_handshake();
            // }

            // load_state_req_module = 0;
            // this->load_done_req();
//#if 0
            #ifdef  ENABLE_SM
            // Addition for ASI
            this->end_load_asi_handshake();
#ifdef SYNTH_APP_CFA
	    wait();
	    this->load_next_tile.ack.ack();
#endif
            #endif
//#endif
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

        output_ready.ack.reset_ack();
        store_done.req.reset_req();
#ifdef SYNTH_APP_CFA
        load_next_tile.req.reset_req();
#endif
        wait();
        // explicit PLM ports reset if any
        this->reset_dma_write();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t output_tile_start_offset;
    //int32_t cons_valid_offset;
    //int32_t prod_rdy_offset ;
    //int32_t cons_rdy_offset;
    //int32_t prod_valid_offset ;
    // int32_t num_tiles;
    int32_t tile_size;
    // int32_t tile_no;
    uint32_t offset;
#ifndef SYNTH_APP_CFA
    bool ping;
#endif
    uint32_t sp_offset;
    //int32_t curr_tile;
    uint32_t length;
    conf_info_t config;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        output_tile_start_offset = config.output_offset;
        //cons_valid_offset = config.cons_valid_offset;
        //prod_rdy_offset  = config.prod_ready_offset ;
        //cons_rdy_offset = config.cons_ready_offset;
        //prod_valid_offset  = config.prod_valid_offset ;
        // num_tiles = config.num_tiles;
        tile_size = config.tile_size;
        // tile_no = config.rd_wr_enable;
        offset = output_tile_start_offset;
#ifndef SYNTH_APP_CFA
        ping = true;
#endif
        sp_offset = 0;
        //curr_tile = 0;
        length = tile_size;
    }

    // Store

    while(true){
        HLS_UNROLL_LOOP(OFF);
        #ifdef  ENABLE_SM
            //Addition for ASI
            this->start_store_asi_handshake();
        #endif

                {   HLS_PROTO("store-dma-data");

                    this->store_compute_handshake();
#ifdef  ENABLE_SM
                    store_req_dma_write.write(1);
                    wait();
//BM uncommented jul 13
                    while(dma_write_arbiter.read() != STORE_UNIT ) wait();
#endif

                    uint32_t len = length;
                    uint32_t len_final = len; 
                    dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len_final, DMA_SIZE);
                    this->dma_write_ctrl.put(dma_info);
                    for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                    {
                        HLS_UNROLL_LOOP(OFF);
                        HLS_BREAK_DEP(output_ping);
#ifndef SYNTH_APP_CFA
                        HLS_BREAK_DEP(output_pong);
#endif			
                        sc_dt::sc_bv<DMA_WIDTH> dataBv;
                        // Read from PLM
                        wait();
#ifndef SYNTH_APP_CFA
                        if (ping)
#endif			
                            dataBv.range(DATA_WIDTH - 1, 0) = output_ping[sp_offset + i];
#ifndef SYNTH_APP_CFA
                        else
                            dataBv.range(DATA_WIDTH - 1, 0) = output_pong[sp_offset + i];
#endif			
                        wait();
                        this->dma_write_chnl.put(dataBv);
                        wait();
                    }
                    //curr_tile++;
                    wait();
                    
                    // Wait till the write is accepted at the cache (and previous fences)
              //      while (!(this->dma_write_chnl.ready)) wait();
              //      wait();

              //      //FENCE
              //      this->acc_fence.put(0x2);
              //      wait();
              //      while (!(this->acc_fence.ready)) wait();
              //      wait();

#ifndef SYNTH_APP_CFA
		    ping  =! ping;
#endif			


#ifdef  ENABLE_SM
                    store_req_dma_write.write(0);
                    wait();
//            this->end_load_asi_handshake();
                    wait();
		    this->end_store_asi_handshake();
                    wait();
// jul14
#ifdef SYNTH_APP_CFA
		    load_next_tile.req.req();
                    wait();
#endif
#endif
                }
	
    }
}





#ifdef SYNTH_APP_CFA
void tiled_app::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");
        // this->reset_compute_kernel();
        wait();
        input_ready.ack.reset_ack();
       // compute_done.req.reset_req();
        output_ready.req.reset_req();
        compute0_state_dbg.write(0);
        //load_next_tile.req.reset_req();

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    //bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size = config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        // compute_tiles = 0;
        num_comp_units = config.num_comp_units;
       // ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-block-input-handshake");
                HLS_UNROLL_LOOP(OFF);
            this->compute_load_handshake(); // Ack new input tile
            wait();
            compute0_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        // {
        if(op_count == 0 || num_comp_units == 0){
            {
                HLS_PROTO("compute-block-output-handshake");
                //this->compute_done_req();
                this->compute_store_handshake();
                wait();
            }
        }
        else{
            // int stride = 1;
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(plm_in_ping);
                // HLS_BREAK_DEP(plm_in_pong);

                HLS_BREAK_DEP(output_ping);
                // HLS_BREAK_DEP(output_pong);

                int64_t beta = 2;
                //int64_t A_I = (ping)? plm_in_ping[i] : plm_in_pong[i];
                int64_t A_I = plm_in_ping[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }


                //if (ping)
                    output_ping[i] = beta;
                //else
                //    output_pong[i] = beta;

            }
            // }
        // }
            //add return handshake
            {
                HLS_PROTO("compute-block-output-handshake");
                compute0_state_dbg.write(2);
                wait();
                //ping = !ping;
  

                compute0_state_dbg.write(3);
                wait();
		//load_next_tile.req.req();
                //wait();
                //this->compute_done_req();
                this->compute_store_handshake();
                wait();
                compute0_state_dbg.write(4);
                wait();
            }
        }
    }
}




#else

//#include "monolithic_synth_funcs.hpp"

//===================Code gen for tiled_app_functions.hpp===================


inline void tiled_app::compute_0_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_0_done-req-handshake");
        compute_0_done.req.req();
    }
}
inline void tiled_app::compute_0_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_0_done-ack-handshake");
        compute_0_done.ack.ack();
    }
}
inline void tiled_app::compute_1_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_1_done-req-handshake");
        compute_1_done.req.req();
    }
}
inline void tiled_app::compute_1_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_1_done-ack-handshake");
        compute_1_done.ack.ack();
    }
}
inline void tiled_app::compute_2_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_2_done-req-handshake");
        compute_2_done.req.req();
    }
}
inline void tiled_app::compute_2_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_2_done-ack-handshake");
        compute_2_done.ack.ack();
    }
}
inline void tiled_app::compute_3_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_3_done-req-handshake");
        compute_3_done.req.req();
    }
}
inline void tiled_app::compute_3_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_3_done-ack-handshake");
        compute_3_done.ack.ack();
    }
}
inline void tiled_app::compute_4_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_4_done-req-handshake");
        compute_4_done.req.req();
    }
}
inline void tiled_app::compute_4_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_4_done-ack-handshake");
        compute_4_done.ack.ack();
    }
}
inline void tiled_app::compute_5_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_5_done-req-handshake");
        compute_5_done.req.req();
    }
}
inline void tiled_app::compute_5_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_5_done-ack-handshake");
        compute_5_done.ack.ack();
    }
}
inline void tiled_app::compute_6_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_6_done-req-handshake");
        compute_6_done.req.req();
    }
}
inline void tiled_app::compute_6_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_6_done-ack-handshake");
        compute_6_done.ack.ack();
    }
}
inline void tiled_app::compute_7_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_7_done-req-handshake");
        compute_7_done.req.req();
    }
}
inline void tiled_app::compute_7_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_7_done-ack-handshake");
        compute_7_done.ack.ack();
    }
}
inline void tiled_app::compute_8_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_8_done-req-handshake");
        compute_8_done.req.req();
    }
}
inline void tiled_app::compute_8_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_8_done-ack-handshake");
        compute_8_done.ack.ack();
    }
}
inline void tiled_app::compute_9_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_9_done-req-handshake");
        compute_9_done.req.req();
    }
}
inline void tiled_app::compute_9_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_9_done-ack-handshake");
        compute_9_done.ack.ack();
    }
}
inline void tiled_app::compute_10_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_10_done-req-handshake");
        compute_10_done.req.req();
    }
}
inline void tiled_app::compute_10_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_10_done-ack-handshake");
        compute_10_done.ack.ack();
    }
}
inline void tiled_app::compute_11_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_11_done-req-handshake");
        compute_11_done.req.req();
    }
}
inline void tiled_app::compute_11_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_11_done-ack-handshake");
        compute_11_done.ack.ack();
    }
}
inline void tiled_app::compute_12_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_12_done-req-handshake");
        compute_12_done.req.req();
    }
}
inline void tiled_app::compute_12_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_12_done-ack-handshake");
        compute_12_done.ack.ack();
    }
}
inline void tiled_app::compute_13_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_13_done-req-handshake");
        compute_13_done.req.req();
    }
}
inline void tiled_app::compute_13_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_13_done-ack-handshake");
        compute_13_done.ack.ack();
    }
}



//===================Code gen for tiled_app.cpp===================


void tiled_app::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-0-reset");
        wait();
        input_ready.ack.reset_ack();
        compute_0_done.req.reset_req();
        compute0_state_dbg.write(0);
        //load_next_tile.req.reset_req();

    }

    ////int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-0-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        ////num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-0-block-input-handshake");
            //this->input_ready_ack(); // Ack new input tile
            this->compute_load_handshake(); // Ack new input tile
            wait();
            compute0_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 1){
            {
                HLS_PROTO("compute-0-output-handshake");
                this->compute_0_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(plm_in_ping);
                HLS_BREAK_DEP(plm_in_pong);
                HLS_BREAK_DEP(compute_0_ping);
                HLS_BREAK_DEP(compute_0_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? plm_in_ping[i] : plm_in_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_0_ping[i] = beta;
                else
                    compute_0_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-0-block-output-handshake");
                ping = !ping;
                compute0_state_dbg.write(2);
                wait();
                this->compute_0_done_req();
                wait();
                compute0_state_dbg.write(3);
                wait();
		//#ifndef SYNTH_APP_CFA
		//load_next_tile.req.req();
                //wait();
                //compute0_state_dbg.write(3);
                //wait();
		//#endif
            }
        }
    }
}

void tiled_app::compute_kernel_1()
{
    // Reset
    {
        HLS_PROTO("compute-1-reset");
        wait();
        compute_0_done.ack.reset_ack();
        compute_1_done.req.reset_req();
        compute1_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-1-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-1-block-input-handshake");
            this->compute_0_done_ack(); // Ack new input tile
            wait();
            compute1_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 2){
            {
                HLS_PROTO("compute-1-output-handshake");
                this->compute_1_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_0_ping);
                HLS_BREAK_DEP(compute_0_pong);
                HLS_BREAK_DEP(compute_1_ping);
                HLS_BREAK_DEP(compute_1_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_0_ping[i] : compute_0_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_1_ping[i] = beta;
                else
                    compute_1_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-1-block-output-handshake");
                ping = !ping;
                compute1_state_dbg.write(2);
                wait();
                this->compute_1_done_req();
                wait();
                compute1_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_2()
{
    // Reset
    {
        HLS_PROTO("compute-2-reset");
        wait();
        compute_1_done.ack.reset_ack();
        compute_2_done.req.reset_req();
        compute2_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-2-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-2-block-input-handshake");
            this->compute_1_done_ack(); // Ack new input tile
            wait();
            compute2_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 3){
            {
                HLS_PROTO("compute-2-output-handshake");
                this->compute_2_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_1_ping);
                HLS_BREAK_DEP(compute_1_pong);
                HLS_BREAK_DEP(compute_2_ping);
                HLS_BREAK_DEP(compute_2_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_1_ping[i] : compute_1_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_2_ping[i] = beta;
                else
                    compute_2_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-2-block-output-handshake");
                ping = !ping;
                compute2_state_dbg.write(2);
                wait();
                this->compute_2_done_req();
                wait();
                compute2_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_3()
{
    // Reset
    {
        HLS_PROTO("compute-3-reset");
        wait();
        compute_2_done.ack.reset_ack();
        compute_3_done.req.reset_req();
        compute3_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-3-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-3-block-input-handshake");
            this->compute_2_done_ack(); // Ack new input tile
            wait();
            compute3_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 4){
            {
                HLS_PROTO("compute-3-output-handshake");
                this->compute_3_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_2_ping);
                HLS_BREAK_DEP(compute_2_pong);
                HLS_BREAK_DEP(compute_3_ping);
                HLS_BREAK_DEP(compute_3_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_2_ping[i] : compute_2_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_3_ping[i] = beta;
                else
                    compute_3_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-3-block-output-handshake");
                ping = !ping;
                compute3_state_dbg.write(2);
                wait();
                this->compute_3_done_req();
                wait();
                compute3_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_4()
{
    // Reset
    {
        HLS_PROTO("compute-4-reset");
        wait();
        compute_3_done.ack.reset_ack();
        compute_4_done.req.reset_req();
        compute4_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-4-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-4-block-input-handshake");
            this->compute_3_done_ack(); // Ack new input tile
            wait();
            compute4_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 5){
            {
                HLS_PROTO("compute-4-output-handshake");
                this->compute_4_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_3_ping);
                HLS_BREAK_DEP(compute_3_pong);
                HLS_BREAK_DEP(compute_4_ping);
                HLS_BREAK_DEP(compute_4_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_3_ping[i] : compute_3_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_4_ping[i] = beta;
                else
                    compute_4_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-4-block-output-handshake");
                ping = !ping;
                compute4_state_dbg.write(2);
                wait();
                this->compute_4_done_req();
                wait();
                compute4_state_dbg.write(3);
                wait();
            }
        }
    }
}
void tiled_app::compute_kernel_5()
{
    // Reset
    {
        HLS_PROTO("compute-5-reset");
        wait();
        compute_4_done.ack.reset_ack();
        compute_5_done.req.reset_req();
        compute5_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-5-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-5-block-input-handshake");
            this->compute_4_done_ack(); // Ack new input tile
            wait();
            compute5_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 6){
            {
                HLS_PROTO("compute-5-output-handshake");
                this->compute_5_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_4_ping);
                HLS_BREAK_DEP(compute_4_pong);
                HLS_BREAK_DEP(compute_5_ping);
                HLS_BREAK_DEP(compute_5_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_4_ping[i] : compute_4_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_5_ping[i] = beta;
                else
                    compute_5_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-5-block-output-handshake");
                ping = !ping;
                compute5_state_dbg.write(2);
                wait();
                this->compute_5_done_req();
                wait();
                compute5_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_6()
{
    // Reset
    {
        HLS_PROTO("compute-6-reset");
        wait();
        compute_5_done.ack.reset_ack();
        compute_6_done.req.reset_req();
        compute6_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-6-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-6-block-input-handshake");
            this->compute_5_done_ack(); // Ack new input tile
            wait();
            compute6_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 7){
            {
                HLS_PROTO("compute-6-output-handshake");
                this->compute_6_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_5_ping);
                HLS_BREAK_DEP(compute_5_pong);
                HLS_BREAK_DEP(compute_6_ping);
                HLS_BREAK_DEP(compute_6_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_5_ping[i] : compute_5_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_6_ping[i] = beta;
                else
                    compute_6_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-6-block-output-handshake");
                ping = !ping;
                compute6_state_dbg.write(2);
                wait();
                this->compute_6_done_req();
                wait();
                compute6_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_7()
{
    // Reset
    {
        HLS_PROTO("compute-7-reset");
        wait();
        compute_6_done.ack.reset_ack();
        compute_7_done.req.reset_req();
        compute7_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-7-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-7-block-input-handshake");
            this->compute_6_done_ack(); // Ack new input tile
            wait();
            compute7_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 8){
            {
                HLS_PROTO("compute-7-output-handshake");
                this->compute_7_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_6_ping);
                HLS_BREAK_DEP(compute_6_pong);
                HLS_BREAK_DEP(compute_7_ping);
                HLS_BREAK_DEP(compute_7_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_6_ping[i] : compute_6_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_7_ping[i] = beta;
                else
                    compute_7_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-7-block-output-handshake");
                ping = !ping;
                compute7_state_dbg.write(2);
                wait();
                this->compute_7_done_req();
                wait();
                compute7_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_8()
{
    // Reset
    {
        HLS_PROTO("compute-8-reset");
        wait();
        compute_7_done.ack.reset_ack();
        compute_8_done.req.reset_req();
        compute8_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-8-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-8-block-input-handshake");
            this->compute_7_done_ack(); // Ack new input tile
            wait();
            compute8_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 9){
            {
                HLS_PROTO("compute-8-output-handshake");
                this->compute_8_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_7_ping);
                HLS_BREAK_DEP(compute_7_pong);
                HLS_BREAK_DEP(compute_8_ping);
                HLS_BREAK_DEP(compute_8_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_7_ping[i] : compute_7_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_8_ping[i] = beta;
                else
                    compute_8_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-8-block-output-handshake");
                ping = !ping;
                compute8_state_dbg.write(2);
                wait();
                this->compute_8_done_req();
                wait();
                compute8_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_9()
{
    // Reset
    {
        HLS_PROTO("compute-9-reset");
        wait();
        compute_8_done.ack.reset_ack();
        compute_9_done.req.reset_req();
        compute9_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-9-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-9-block-input-handshake");
            this->compute_8_done_ack(); // Ack new input tile
            wait();
            compute9_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 10){
            {
                HLS_PROTO("compute-9-output-handshake");
                this->compute_9_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_8_ping);
                HLS_BREAK_DEP(compute_8_pong);
                HLS_BREAK_DEP(compute_9_ping);
                HLS_BREAK_DEP(compute_9_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_8_ping[i] : compute_8_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_9_ping[i] = beta;
                else
                    compute_9_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-9-block-output-handshake");
                ping = !ping;
                compute9_state_dbg.write(2);
                wait();
                this->compute_9_done_req();
                wait();
                compute9_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_10()
{
    // Reset
    {
        HLS_PROTO("compute-10-reset");
        wait();
        compute_9_done.ack.reset_ack();
        compute_10_done.req.reset_req();
        compute10_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-10-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-10-block-input-handshake");
            this->compute_9_done_ack(); // Ack new input tile
            wait();
            compute10_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 11){
            {
                HLS_PROTO("compute-10-output-handshake");
                this->compute_10_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_9_ping);
                HLS_BREAK_DEP(compute_9_pong);
                HLS_BREAK_DEP(compute_10_ping);
                HLS_BREAK_DEP(compute_10_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_9_ping[i] : compute_9_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_10_ping[i] = beta;
                else
                    compute_10_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-10-block-output-handshake");
                ping = !ping;
                compute10_state_dbg.write(2);
                wait();
                this->compute_10_done_req();
                wait();
                compute10_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_11()
{
    // Reset
    {
        HLS_PROTO("compute-11-reset");
        wait();
        compute_10_done.ack.reset_ack();
        compute_11_done.req.reset_req();
        compute11_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-11-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-11-block-input-handshake");
            this->compute_10_done_ack(); // Ack new input tile
            wait();
            compute11_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 12){
            {
                HLS_PROTO("compute-11-output-handshake");
                this->compute_11_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_10_ping);
                HLS_BREAK_DEP(compute_10_pong);
                HLS_BREAK_DEP(compute_11_ping);
                HLS_BREAK_DEP(compute_11_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_10_ping[i] : compute_10_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_11_ping[i] = beta;
                else
                    compute_11_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-11-block-output-handshake");
                ping = !ping;
                compute11_state_dbg.write(2);
                wait();
                this->compute_11_done_req();
                wait();
                compute11_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_12()
{
    // Reset
    {
        HLS_PROTO("compute-12-reset");
        wait();
        compute_11_done.ack.reset_ack();
        compute_12_done.req.reset_req();
        compute12_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-12-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-12-block-input-handshake");
            this->compute_11_done_ack(); // Ack new input tile
            wait();
            compute12_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 13){
            {
                HLS_PROTO("compute-12-output-handshake");
                this->compute_12_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_11_ping);
                HLS_BREAK_DEP(compute_11_pong);
                HLS_BREAK_DEP(compute_12_ping);
                HLS_BREAK_DEP(compute_12_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_11_ping[i] : compute_11_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_12_ping[i] = beta;
                else
                    compute_12_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-12-block-output-handshake");
                ping = !ping;
                compute12_state_dbg.write(2);
                wait();
                this->compute_12_done_req();
                wait();
                compute12_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_13()
{
    // Reset
    {
        HLS_PROTO("compute-13-reset");
        wait();
        compute_12_done.ack.reset_ack();
        compute_13_done.req.reset_req();
        compute13_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-13-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-13-block-input-handshake");
            this->compute_12_done_ack(); // Ack new input tile
            wait();
            compute13_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 14){
            {
                HLS_PROTO("compute-13-output-handshake");
                this->compute_13_done_req();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_12_ping);
                HLS_BREAK_DEP(compute_12_pong);
                HLS_BREAK_DEP(compute_13_ping);
                HLS_BREAK_DEP(compute_13_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_12_ping[i] : compute_12_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    compute_13_ping[i] = beta;
                else
                    compute_13_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-13-block-output-handshake");
                ping = !ping;
                compute13_state_dbg.write(2);
                wait();
                this->compute_13_done_req();
                wait();
                compute13_state_dbg.write(3);
                wait();
            }
        }
    }
}

void tiled_app::compute_kernel_14()
{
    // Reset
    {
        HLS_PROTO("compute-14-reset");
        wait();
        compute_13_done.ack.reset_ack();
        output_ready.req.reset_req();
        compute14_state_dbg.write(0);

    }

    //int32_t num_tiles;
    // int32_t ping_pong_en;
    //int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-14-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        //num_tiles = config.num_tiles;
        //tile_size= config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-14-block-input-handshake");
            this->compute_13_done_ack(); // Ack new input tile
            wait();
            compute14_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 15){
            {
                HLS_PROTO("compute-14-output-handshake");
            	this->compute_store_handshake(); // Ack new input tile
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_13_ping);
                HLS_BREAK_DEP(compute_13_pong);
                HLS_BREAK_DEP(output_ping);
                HLS_BREAK_DEP(output_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_13_ping[i] : compute_13_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    output_ping[i] = beta;
                else
                    output_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-14-block-output-handshake");
                ping = !ping;
                compute14_state_dbg.write(2);
                wait();
            	this->compute_store_handshake(); // Ack new input tile
                wait();
                compute14_state_dbg.write(3);
                wait();
            }
        }
    }
}

/*
void tiled_app::compute_kernel_5()
{
    // Reset
    {
        HLS_PROTO("compute-5-reset");
        wait();
        compute_4_done.ack.reset_ack();
        output_ready.req.reset_req();
        compute5_state_dbg.write(0);

    }

    int32_t num_tiles;
    // int32_t ping_pong_en;
    int32_t tile_size;
    int32_t compute_over_data;
    int32_t op_count;
    bool ping;
    // int32_t compute_tiles;
    int32_t stride;
    int32_t num_comp_units;
    conf_info_t config;
    {
        HLS_PROTO("compute-5-config");

        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
       // <<--local-params-->> 
        num_tiles = config.num_tiles;
        tile_size = config.tile_size;
        compute_over_data = config.compute_over_data;
        op_count = config.compute_iters;
        num_comp_units = config.num_comp_units;
        // compute_tiles = 0;
        ping = true;
        stride = 1;
    }

    while(true){
        {
            HLS_PROTO("compute-5-block-input-handshake");
            this->compute_4_done_ack(); // Ack new input tile
            wait();
            compute5_state_dbg.write(1);
            wait();
        }
        //IF COMPUTE INTENSITY > 0
        if(op_count == 0 || num_comp_units < 6){
            {
                HLS_PROTO("compute-5-output-handshake");
                //this->output_ready_req();
                    this->compute_store_handshake();
                wait();
            }
        }
        else{
            for(int i = 0; i<compute_over_data; i+=stride){
                HLS_UNROLL_LOOP(OFF);
                HLS_BREAK_DEP(compute_4_ping);
                HLS_BREAK_DEP(compute_4_pong);
                HLS_BREAK_DEP(output_ping);
                HLS_BREAK_DEP(output_pong);
                int64_t beta = 2;
                int64_t A_I = (ping)? compute_4_ping[i] : compute_4_pong[i];
                
                for(int iter = 0; iter<op_count; iter++){
                    HLS_UNROLL_LOOP(OFF);
                    beta = beta * A_I + ALPHA;
                }
                
                if (ping)
                    output_ping[i] = beta;
                else
                    output_pong[i] = beta;
            }
            //add return handshake
            {
                HLS_PROTO("compute-5-block-output-handshake");
                ping = !ping;
                compute5_state_dbg.write(2);
                wait();
        	//output_ready.req.req();
                //this->compute_done_req();
                this->compute_store_handshake();
                wait();
                compute5_state_dbg.write(3);
                wait();
            }
        }
    }
}
*/


#endif
