// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "vitdodec.hpp"
#include "vitdodec_directives.hpp"

// Functions

#include "vitdodec_functions.hpp"

// Processes

void vitdodec::load_input()
{

    // Reset
    {
        HLS_PROTO("load-reset");

        // this->reset_load_input();

        load_iter_dbg.write(0);
        load_state_dbg.write(0);
        load_unit_sp_write_dbg.write(0);
        load_next_tile.ack.reset_ack();

        load_done.req.reset_req();

        this->reset_dma_read();
        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t cbps;
    int32_t ntraceback;
    int32_t data_bits;
    int32_t in_length;
    int32_t input_start_offset;
    int32_t accel_cons_rdy_offset;
    int32_t accel_prod_vld_offset ;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        cbps = config.cbps;
        ntraceback = config.ntraceback;
        data_bits = config.data_bits;
        in_length = config.in_length;
        accel_cons_rdy_offset = config.accel_cons_rdy_offset  ;
        accel_prod_vld_offset  = config.accel_prod_vld_offset   ;
        input_start_offset = config.input_start_offset;
    }

    // Load
    // {
    //     HLS_PROTO("load-dma");
    //     bool ping = true;
    //     uint32_t offset = 0;

    //     wait();
    //     // Batching
    //     // Load
    while(true)
    {
        HLS_UNROLL_LOOP(OFF);
        HLS_PROTO("load-dma");

        wait();
        this->load_next_tile_ack();
        //BM this->load_compute_ready_handshake();
        wait();
        load_state_dbg.write(load_state);
        // load_iter_dbg.write(curr_tile);
        wait();
        // load_state_req_dbg.write(load_state_req);

#ifdef ENABLE_SM
        switch (load_state)
        {
            case POLL_PROD_VALID_REQ: 
            case POLL_CONS_READY_REQ: { 
                {
                    HLS_PROTO("load-dma-poll");
                    int32_t sync_offset = accel_prod_vld_offset;
                    int32_t sync_len = 2*DMA_WORD_PER_BEAT;//2;
                    if(load_state == POLL_CONS_READY_REQ) {
                        sync_offset = accel_cons_rdy_offset;
                        sync_len = 1*DMA_WORD_PER_BEAT;
                    }
                    // Wait on SYNC FLAG
                    while(true){ 
                        HLS_UNROLL_LOOP(OFF);
                        sc_dt::sc_bv<DMA_WIDTH> dataBvin;

#if (DMA_WORD_PER_BEAT == 0)
                    // data word is wider than NoC links
                    dma_info_t dma_info2(sync_offset * DMA_BEAT_PER_WORD, sync_len * DMA_BEAT_PER_WORD, DMA_SIZE);
#else
                    dma_info_t dma_info2(sync_offset / DMA_WORD_PER_BEAT, sync_len / DMA_WORD_PER_BEAT, DMA_SIZE);
#endif
                        // dma_info_t dma_info2(sync_offset, sync_len, DMA_SIZE);
                        this->dma_read_ctrl.put(dma_info2);
                        wait();
                        load_unit_sp_write_dbg.write(1); 

#if (DMA_WORD_PER_BEAT == 0)
                        for (uint16_t k = 0; k < DMA_BEAT_PER_WORD; k++)
                        {
                            dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH) = this->dma_read_chnl.get();
                            wait();
                        }
#else
                        dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
#endif
                        wait();
                        load_unit_sp_write_dbg.write(2); 
                        int64_t data = dataBvin.range(DMA_WIDTH, 0).to_int64();
                        wait();
                        load_unit_sp_write_dbg.write(3); 
                        if(load_state == POLL_PROD_VALID_REQ) {
                            dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
// #if (DMA_WORD_PER_BEAT == 0)
//                         for (uint16_t k = 0; k < DMA_BEAT_PER_WORD; k++)
//                         {
//                             dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH) = this->dma_read_chnl.get();
//                             wait();
//                         }
// #else
//                         dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
// #endif

                            int64_t data2 = dataBvin.range(DMA_WIDTH - 1, 0).to_int64();
                            // int8_t data2 = data>>8;
                            wait();
                            if(data2&0x1==1) last_task = 1;
                            load_unit_sp_write_dbg.write(4); 
                            wait();
                        }
                        // curr_tile++;
                        if(data&0x1 == 1){
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
            case LOAD_DATA_REQ:
#else
            for (uint16_t b = 0; b < 1; b++)
#endif
            {
                wait();
                uint32_t offset = input_start_offset; //BM
#if (DMA_WORD_PER_BEAT == 0)
                uint32_t length = in_length;
#else
                uint32_t length = round_up(in_length, DMA_WORD_PER_BEAT);
#endif
                // Chunking
                for (int rem = length; rem > 0; rem -= PLM_IN_WORD)
                {
                    wait();
                    // Configure DMA transaction
                    uint32_t len = rem > PLM_IN_WORD ? PLM_IN_WORD : rem;
#if (DMA_WORD_PER_BEAT == 0)
                    // data word is wider than NoC links
                    dma_info_t dma_info(offset * DMA_BEAT_PER_WORD, len * DMA_BEAT_PER_WORD, DMA_SIZE);
#else
                    dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
#endif
                    offset += len;

                    this->dma_read_ctrl.put(dma_info);

#if (DMA_WORD_PER_BEAT == 0)
                    // data word is wider than NoC links
                    for (uint16_t i = 0; i < len; i++)
                    {
                        sc_dt::sc_bv<DATA_WIDTH> dataBv;

                        for (uint16_t k = 0; k < DMA_BEAT_PER_WORD; k++)
                        {
                            dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH) = this->dma_read_chnl.get();
                            wait();
                        }

                        // Write to PLM
                        //if (ping)
                            plm_in_ping[i] = dataBv.to_int64();
                        //else
                        //    plm_in_pong[i] = dataBv.to_int64();
                    }
#else
                    for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                    {
                        HLS_BREAK_DEP(plm_in_ping);
                        //HLS_BREAK_DEP(plm_in_pong);

                        sc_dt::sc_bv<DMA_WIDTH> dataBv;

                        dataBv = this->dma_read_chnl.get();
                        wait();

                        // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                        for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                        {
                            HLS_UNROLL_SIMPLE;
                            //if (ping)
                                plm_in_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                            //else
                            //    plm_in_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        }
                    }

#endif
                    // BM: this->load_compute_handshake();
                    //ping = !ping;
                } // for (rem = 
            } // for (b = 0 ... // switch case
#ifdef ENABLE_SM
            break;
            default:
            break;
        } // switch case
#endif
        //BM ASI
        this->load_done_req();
    } // Load - while true
#ifndef ENABLE_SM
    // Conclude
    {
        this->process_done();
    }
#endif
}



void vitdodec::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        // this->reset_store_output();

	    store_iter_dbg.write(0);
        store_state_dbg.write(0);

        output_ready.ack.reset_ack();
        store_done.req.reset_req();
        wait();
        // explicit PLM ports reset if any
        this->reset_dma_write();

        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t cbps;
    int32_t ntraceback;
    int32_t data_bits;
    int32_t out_length;
    int32_t output_start_offset;
    int32_t accel_cons_vld_offset;
    int32_t accel_prod_rdy_offset ;
    int32_t accel_cons_rdy_offset;
    int32_t accel_prod_vld_offset ;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        cbps = config.cbps;
        ntraceback = config.ntraceback;
        data_bits = config.data_bits;
        out_length = config.out_length;
        output_start_offset = config.output_start_offset;
        accel_cons_vld_offset = config.accel_cons_vld_offset;
        accel_prod_rdy_offset  = config.accel_prod_rdy_offset ;
        accel_cons_rdy_offset = config.accel_cons_rdy_offset;
        accel_prod_vld_offset  = config.accel_prod_vld_offset ;
    }

 // Store
#ifdef ENABLE_SM
    while(true){
        HLS_UNROLL_LOOP(OFF);
        // store_iter_dbg.write(curr_tile);
        // Wait for handshake
         {
            HLS_PROTO("store-dma-handshake");
            this->store_compute_handshake();
            wait();
            store_state_dbg.write(store_state);
            wait();
         }
        int64_t sync_flag = (store_state == UPDATE_PROD_VALID_REQ || store_state == UPDATE_CONS_READY_REQ )? 0 : 1;
        // uint32_t offset = output_start_offset;

#if (DMA_WORD_PER_BEAT == 0)
        uint32_t offset = output_start_offset;// (24852) * 1;
#else
        // uint32_t store_offset = round_up(24852, DMA_WORD_PER_BEAT) * 1;
        uint32_t offset = round_up(output_start_offset, DMA_WORD_PER_BEAT) * 1;
#endif
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
             


#if (DMA_WORD_PER_BEAT == 0)       
                    int32_t sync_offset = accel_prod_rdy_offset; //READY_FLAG_OFFSET;
                    int32_t sync_len = 1;
                    if(store_state == UPDATE_PROD_VALID_REQ) sync_offset = accel_prod_vld_offset;//VALID_FLAG_OFFSET;
                    else if(store_state == UPDATE_CONS_VALID_REQ) {
                        sync_offset = accel_cons_vld_offset;
                        sync_len = 2;
                    }
                    else if(store_state == UPDATE_CONS_READY_REQ) sync_offset = accel_cons_rdy_offset;
#else
                    int32_t sync_offset = round_up(accel_prod_rdy_offset, DMA_WORD_PER_BEAT) * 1 ; //READY_FLAG_OFFSET;
                    int32_t sync_len = 1*DMA_WORD_PER_BEAT;
                    if(store_state == UPDATE_PROD_VALID_REQ) sync_offset = round_up(accel_prod_vld_offset, DMA_WORD_PER_BEAT) * 1 ;//VALID_FLAG_OFFSET;
                    else if(store_state == UPDATE_CONS_VALID_REQ) {
                        sync_offset = round_up(accel_cons_vld_offset, DMA_WORD_PER_BEAT) * 1 ;
                        sync_len = 2*DMA_WORD_PER_BEAT;
                    }
                    else if(store_state == UPDATE_CONS_READY_REQ) sync_offset = round_up(accel_cons_rdy_offset, DMA_WORD_PER_BEAT) * 1 ;
#endif

#if (DMA_WORD_PER_BEAT == 0)
                // data word is wider than NoC links
                dma_info_t dma_info(sync_offset * DMA_BEAT_PER_WORD, sync_len * DMA_BEAT_PER_WORD, DMA_SIZE);
#else
                dma_info_t dma_info(sync_offset / DMA_WORD_PER_BEAT, sync_len / DMA_WORD_PER_BEAT, DMA_SIZE);
#endif
                    // dma_info_t dma_info(sync_offset / DMA_WORD_PER_BEAT, sync_len / DMA_WORD_PER_BEAT, DMA_SIZE);
                    sc_dt::sc_bv<DMA_WIDTH> dataBv;
                    dataBv.range(DMA_WIDTH-1, 0) = sync_flag;
                    this->dma_write_ctrl.put(dma_info);
                    wait();
                    this->dma_write_chnl.put(dataBv);
                    wait();
                    if(store_state == UPDATE_CONS_VALID_REQ){
                        dataBv.range(DMA_WIDTH-1, 0) = (last_task);
                        this->dma_write_chnl.put(dataBv);
                        wait();
                    }

                    // this->dma_write_chnl.put(dataBv);
                    // wait();

                    // Wait till the write is accepted at the cache (and previous fences)
                    while (!(this->dma_write_chnl.ready)) wait();
                    wait();
                }
            }
            break;

            case STORE_DATA_REQ:
#else
    // Store
    {
        HLS_PROTO("store-dma");
        bool ping = true;
#if (DMA_WORD_PER_BEAT == 0)
        uint32_t store_offset = output_start_offset;// (24852) * 1;
#else
        // uint32_t store_offset = round_up(24852, DMA_WORD_PER_BEAT) * 1;
        uint32_t store_offset = round_up(output_start_offset, DMA_WORD_PER_BEAT) * 1;
#endif
        uint32_t offset = store_offset;

        wait();
        // Batching
        for (uint16_t b = 0; b < 1; b++)
#endif //if enable_sm
        {
            wait();
#if (DMA_WORD_PER_BEAT == 0)
            uint32_t length = out_length;
#else
            uint32_t length = round_up(out_length, DMA_WORD_PER_BEAT);
#endif
            // Chunking
            for (int rem = length; rem > 0; rem -= PLM_OUT_WORD)
            {
                // BM
                // this->store_compute_handshake();

                // Configure DMA transaction
                uint32_t len = rem > PLM_OUT_WORD ? PLM_OUT_WORD : rem;
#if (DMA_WORD_PER_BEAT == 0)
                // data word is wider than NoC links
                dma_info_t dma_info(offset * DMA_BEAT_PER_WORD, len * DMA_BEAT_PER_WORD, DMA_SIZE);
#else
                dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
#endif
                offset += len;

                this->dma_write_ctrl.put(dma_info);

#if (DMA_WORD_PER_BEAT == 0)
                // data word is wider than NoC links
                for (uint16_t i = 0; i < len; i++)
                {
                    // Read from PLM
                    sc_dt::sc_int<DATA_WIDTH> data;
                    wait();
                    //if (ping)
                        data = plm_out_ping[i];
                    //else
                    //    data = plm_out_pong[i];
                    sc_dt::sc_bv<DATA_WIDTH> dataBv(data);

                    uint16_t k = 0;
                    for (k = 0; k < DMA_BEAT_PER_WORD - 1; k++)
                    {
                        this->dma_write_chnl.put(dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH));
                        wait();
                    }
                    // Last beat on the bus does not require wait(), which is
                    // placed before accessing the PLM
                    this->dma_write_chnl.put(dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH));
                }
#else
                for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                {
                    sc_dt::sc_bv<DMA_WIDTH> dataBv;

                    // Read from PLM
                    wait();
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        //if (ping)
                            dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_ping[i + k];
                        //else
                        //    dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_pong[i + k];
                    }
                    this->dma_write_chnl.put(dataBv);
                }
#endif
                //ping = !ping;
            }
            wait();
            while (!(this->dma_write_chnl.ready)) wait();
        } // batch/case
#ifdef ENABLE_SM
        break;
        default:
        break;
    } //end switch
        wait();
        // this->store_compute_done_handshake();
        this->store_done_req();
    } //end while true
    
#else
    } // end load-dma

    // Conclude
    {
        this->accelerator_done();
        this->process_done();
    }
    #endif
}


void vitdodec::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        // this->reset_compute_kernel();

        // load_ready.req.reset_req();
        // load_done.ack.reset_ack();
        // store_ready.req.reset_req();
        // store_done.ack.reset_ack();

        input_ready.ack.reset_ack();
        compute_done.req.reset_req();
        compute_state_dbg.write(0);
        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t cbps;
    int32_t ntraceback;
    int32_t data_bits;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        cbps = config.cbps;
        ntraceback = config.ntraceback;
        data_bits = config.data_bits;
    }


    // Compute
    bool ping = true;
    {
        uint32_t in_length = 24852;
        uint32_t out_length = 18585;
        while(true)
        //for (uint16_t b = 0; b < 1; b++)
        {
            {
                HLS_PROTO("compute-block-input-handshake");
                this->compute_load_handshake(); // Ack new input tile
                wait();
            }
            // int out_rem = out_length;

            // for (int in_rem = in_length; in_rem > 0; in_rem -= PLM_IN_WORD)
            {

                // uint32_t in_len  = in_rem  > PLM_IN_WORD  ? PLM_IN_WORD  : in_rem;
                // uint32_t out_len = out_rem > PLM_OUT_WORD ? PLM_OUT_WORD : out_rem;

                //BM: this->compute_load_handshake();


                {
                    // INPUTS/OUTPUTS:          :  I/O   : Offset : Size
                    //    cbps                  : INPUT  :     X  : int = 4 bytes (REGISTER)
                    //    ntraceback            : INPUT  :     X  : int = 4 bytes (REGISTER)
                    //    data_bits             : INPUT  :     X  : int = 4 bytes (REGISTER)
                    //    d_branchtab27_generic : INPUT  :     0  : uint8_t[2][32] = 64 bytes
                    //    in_depuncture_pattern : INPUT  :    64  : uint8_t[8] (max is 6 bytes + 2 padding bytes)
                    //    depd_data             : INPUT  :    72  : uint8_t[MAX_ENCODED_BITS == 24780] (depunctured data)
                    //    <return_val>          : OUTPUT :     0  : uint8_t[MAX_ENCODED_BITS * 3 / 4 == 18585 ] : The decoded data stream

                    /* THESE ARE JUST USED LOCALLY IN THIS FUNCTION NOW  */
                    /*  BUT they must reset to zero on each invocation   */
                    /*  AND they might be used in other places in GnuRadio? */
                    //    l_metric0_generic     : INPUT  : uint8_t[64]
                    //    l_metric1_generic     : INPUT  : uint8_t[64]
                    //    l_path0_generic       : INPUT  : uint8_t[64]
                    //    l_path1_generic       : INPUT  : uint8_t[64]
                    //    l_store_pos           : INPUT  : int (position in circular traceback buffer?)
                    //    l_mmresult            : OUTPUT : uint8_t[64] 
                    //    l_ppresult            : OUTPUT : uint8_t[ntraceback_MAX][ 64 bytes ]

                    int in_count = 0;
                    int out_count = 0;
                    int n_decoded = 0;

                    int idx_brtab27_0  =  0;
                    int idx_brtab27_1  = 32;
                    int idx_depunc_ptn = 64;
                    int idx_depd_data  = 72;

                    uint8_t  l_metric0_generic[64];
                    uint8_t  l_metric1_generic[64];
                    uint8_t  l_path0_generic[64];
                    uint8_t  l_path1_generic[64];
                    uint8_t  l_mmresult[64];
                    uint8_t  l_ppresult[TRACEBACK_MAX][64];
                    int      l_store_pos = 0;

                    HLS_FLAT(l_metric0_generic);
                    HLS_FLAT(l_metric1_generic);
                    HLS_FLAT(l_path0_generic);
                    HLS_FLAT(l_path1_generic);
                    HLS_FLAT(l_mmresult);
                    HLS_FLAT(l_ppresult);




                    // This is the "reset" portion:
                    //  Do this before the real operation so local memories are "cleared to zero"

                    for (int i = 0; i < 64; i++) {
                        l_metric0_generic[i] = 0;
                        l_path0_generic[i] = 0;
                        l_metric1_generic[i] = 0;
                        l_path1_generic[i] = 0;
                        l_mmresult[i] = 0;
                        for (int j = 0; j < TRACEBACK_MAX; j++) {
                            l_ppresult[j][i] = 0;
                        }
                    }

                    int viterbi_butterfly_calls = 0;
                    while(n_decoded < data_bits) {
                        //printf("n_decoded = %u vs %u = data_bits\n", n_decoded, data_bits);
                        if ((in_count % 4) == 0) { //0 or 3
                            /* The basic Viterbi decoder operation, called a "butterfly"
                             * operation because of the way it looks on a trellis diagram. Each
                             * butterfly involves an Add-Compare-Select (ACS) operation on the two nodes
                             * where the 0 and 1 paths from the current node merge at the next step of
                             * the trellis.
                             *
                             * The code polynomials are assumed to have 1's on both ends. Given a
                             * function encode_state() that returns the two symbols for a given
                             * encoder state in the low two bits, such a code will have the following
                             * identities for even 'n' < 64:
                             *
                             * 	encode_state(n) = encode_state(n+65)
                             *	encode_state(n+1) = encode_state(n+64) = (3 ^ encode_state(n))
                             *
                             * Any convolutional code you would actually want to use will have
                             * these properties, so these assumptions aren't too limiting.
                             *
                             * Doing this as a macro lets the compiler evaluate at compile time the
                             * many expressions that depend on the loop index and encoder state and
                             * emit them as immediate arguments.
                             * This makes an enormous difference on register-starved machines such
                             * as the Intel x86 family where evaluating these expressions at runtime
                             * would spill over into memory.
                             */

                            // INPUTS/OUTPUTS:  All are 64-entry (bytes) arrays randomly accessed.
                            //    symbols : INPUT        : Array [  4 bytes ] 
                            //    mm0     : INPUT/OUTPUT : Array [ 64 bytes ]
                            //    mm1     : INPUT/OUTPUT : Array [ 64 bytes ]
                            //    pp0     : INPUT/OUTPUT : Array [ 64 bytes ] 
                            //    pp1     : INPUT/OUTPUT : Array [ 64 bytes ]
                            // d_branchtab27_generic[1].c[] : INPUT : Array [2].c[32] {GLOBAL}
                            //

                            /* void viterbi_butterfly2_generic(unsigned char *symbols, */
                            /* 				 unsigned char *mm0, unsigned char *mm1, */
                            /* 				 unsigned char *pp0, unsigned char *pp1) */
                            {
                                unsigned char *mm0       = l_metric0_generic;
                                unsigned char *mm1       = l_metric1_generic;
                                unsigned char *pp0       = l_path0_generic;
                                unsigned char *pp1       = l_path1_generic;
                                unsigned char symbols[4];// = (unsigned char)plm_in_ping[(idx_depd_data + in_count) & 0xfffffffc];
                                HLS_FLAT(symbols);

                                symbols[0] = (unsigned char)plm_in_ping[((idx_depd_data + in_count) & 0xfffffffc) + 0];
                                symbols[1] = (unsigned char)plm_in_ping[((idx_depd_data + in_count) & 0xfffffffc) + 1];
                                symbols[2] = (unsigned char)plm_in_ping[((idx_depd_data + in_count) & 0xfffffffc) + 2];
                                symbols[3] = (unsigned char)plm_in_ping[((idx_depd_data + in_count) & 0xfffffffc) + 3];

                                // These are used to "virtually" rename the uses below (for symmetry; reduces code size)
                                //  Really these are functionally "offset pointers" into the above arrays....
                                unsigned char *metric0, *metric1;
                                unsigned char *path0, *path1;

                                // Operate on 4 symbols (2 bits) at a time

                                unsigned char m0[16], m1[16], m2[16], m3[16], decision0[16], decision1[16], survivor0[16], survivor1[16];
                                unsigned char metsv[16], metsvm[16];
                                unsigned char shift0[16], shift1[16];
                                unsigned char tmp0[16], tmp1[16];
                                unsigned char sym0v[16], sym1v[16];
                                unsigned short simd_epi16;
                                unsigned int   first_symbol;
                                unsigned int   second_symbol;

                                HLS_FLAT(m0);
                                HLS_FLAT(m1);
                                HLS_FLAT(m2);
                                HLS_FLAT(m3);
                                HLS_FLAT(decision0);
                                HLS_FLAT(decision1);
                                HLS_FLAT(survivor0);
                                HLS_FLAT(survivor1);
                                HLS_FLAT(metsv);
                                HLS_FLAT(metsvm);
                                HLS_FLAT(shift0);
                                HLS_FLAT(shift1);
                                HLS_FLAT(tmp0);
                                HLS_FLAT(tmp1);
                                HLS_FLAT(sym0v);
                                HLS_FLAT(sym1v);


                                //printf("Setting up for next two symbols\n");
                                // Set up for the first two symbols (0 and 1)
                                metric0 = mm0;
                                path0 = pp0;
                                metric1 = mm1;
                                path1 = pp1;
                                first_symbol = 0;
                                second_symbol = first_symbol+1;
                                for (int j = 0; j < 16; j++) {
                                    sym0v[j] = symbols[first_symbol];
                                    sym1v[j] = symbols[second_symbol];
                                    //sym0v[j] = (unsigned char)plm_in_ping[((idx_depd_data + in_count) & 0xfffffffc) + first_symbol];
                                    //sym1v[j] = (unsigned char)plm_in_ping[((idx_depd_data + in_count) & 0xfffffffc) + second_symbol];
                                }

                                for (int s = 0; s < 2; s++) { // iterate across the 2 symbol groups
                                    // This is the basic viterbi butterfly for 2 symbols (we need therefore 2 passes for 4 total symbols)
                                    //printf("for s = %u :\n", s);
                                    for (int i = 0; i < 2; i++) {
                                        //printf("  for i = %u :\n", i);
                                        if (symbols[first_symbol] == 2) {
                                            for (int j = 0; j < 16; j++) {
                                                //metsvm[j] = d_branchtab27_generic[1].c[(i*16) + j] ^ sym1v[j];
                                                //metsvm[j] = d_brtab27[1][(i*16) + j] ^ sym1v[j];
                                                metsvm[j] = (unsigned char)plm_in_ping[idx_brtab27_1 + (i*16) + j] ^ sym1v[j];
                                                metsv[j] = 1 - metsvm[j];
                                            }
                                        }
                                        else if (symbols[second_symbol] == 2) {
                                            for (int j = 0; j < 16; j++) {
                                                //metsvm[j] = d_branchtab27_generic[0].c[(i*16) + j] ^ sym0v[j];
                                                //metsvm[j] = d_brtab27[0][(i*16) + j] ^ sym0v[j];
                                                //metsvm[j] = d_brtab27_0[(i*16) + j] ^ sym0v[j];
                                                metsvm[j] = (unsigned char)plm_in_ping[idx_brtab27_0 + (i*16) + j] ^ sym0v[j];
                                                metsv[j] = 1 - metsvm[j];
                                            }
                                        }
                                        else {
                                            for (int j = 0; j < 16; j++) {
                                                //metsvm[j] = (d_branchtab27_generic[0].c[(i*16) + j] ^ sym0v[j]) + (d_branchtab27_generic[1].c[(i*16) + j] ^ sym1v[j]);
                                                //metsvm[j] = (d_brtab27[0][(i*16) + j] ^ sym0v[j]) + (d_brtab27[1][(i*16) + j] ^ sym1v[j]);
                                                metsvm[j] = ((unsigned char)plm_in_ping[idx_brtab27_0 + (i*16) + j] ^ sym0v[j])
                                                    + ((unsigned char)plm_in_ping[idx_brtab27_1 + (i*16) + j] ^ sym1v[j]);
                                                metsv[j] = 2 - metsvm[j];
                                            }
                                        }

                                        for (int j = 0; j < 16; j++) {
                                            m0[j] = metric0[(i*16) + j] + metsv[j];
                                            m1[j] = metric0[((i+2)*16) + j] + metsvm[j];
                                            m2[j] = metric0[(i*16) + j] + metsvm[j];
                                            m3[j] = metric0[((i+2)*16) + j] + metsv[j];
                                        }

                                        for (int j = 0; j < 16; j++) {
                                            decision0[j] = ((m0[j] - m1[j]) > 0) ? 0xff : 0x0;
                                            decision1[j] = ((m2[j] - m3[j]) > 0) ? 0xff : 0x0;
                                            survivor0[j] = (decision0[j] & m0[j]) | ((~decision0[j]) & m1[j]);
                                            survivor1[j] = (decision1[j] & m2[j]) | ((~decision1[j]) & m3[j]);
                                        }

                                        for (int j = 0; j < 16; j += 2) {
                                            simd_epi16 = path0[(i*16) + j];
                                            simd_epi16 |= path0[(i*16) + (j+1)] << 8;
                                            simd_epi16 <<= 1;
                                            shift0[j] = simd_epi16;
                                            shift0[j+1] = simd_epi16 >> 8;

                                            simd_epi16 = path0[((i+2)*16) + j];
                                            simd_epi16 |= path0[((i+2)*16) + (j+1)] << 8;
                                            simd_epi16 <<= 1;
                                            shift1[j] = simd_epi16;
                                            shift1[j+1] = simd_epi16 >> 8;
                                        }
                                        for (int j = 0; j < 16; j++) {
                                            shift1[j] = shift1[j] + 1;
                                        }

                                        for (int j = 0, k = 0; j < 16; j += 2, k++) {
                                            metric1[(2*i*16) + j] = survivor0[k];
                                            metric1[(2*i*16) + (j+1)] = survivor1[k];
                                        }
                                        for (int j = 0; j < 16; j++) {
                                            tmp0[j] = (decision0[j] & shift0[j]) | ((~decision0[j]) & shift1[j]);
                                        }

                                        for (int j = 0, k = 8; j < 16; j += 2, k++) {
                                            metric1[((2*i+1)*16) + j] = survivor0[k];
                                            metric1[((2*i+1)*16) + (j+1)] = survivor1[k];
                                        }
                                        for (int j = 0; j < 16; j++) {
                                            tmp1[j] = (decision1[j] & shift0[j]) | ((~decision1[j]) & shift1[j]);
                                        }

                                        for (int j = 0, k = 0; j < 16; j += 2, k++) {
                                            path1[(2*i*16) + j] = tmp0[k];
                                            path1[(2*i*16) + (j+1)] = tmp1[k];
                                        }
                                        for (int j = 0, k = 8; j < 16; j += 2, k++) {
                                            path1[((2*i+1)*16) + j] = tmp0[k];
                                            path1[((2*i+1)*16) + (j+1)] = tmp1[k];
                                        }
                                    } // for (i = 0 to 2)

                                    // Set up for the second two symbols (2 and 3)
                                    metric0 = mm1;
                                    path0 = pp1;
                                    metric1 = mm0;
                                    path1 = pp0;
                                    first_symbol = 2;
                                    second_symbol = first_symbol+1;
                                    for (int j = 0; j < 16; j++) {
                                        sym0v[j] = symbols[first_symbol];
                                        sym1v[j] = symbols[second_symbol];
                                    }
                                } // for (s = 0 to 2)
                            } // END of call to viterbi_butterfly2_generic
                            viterbi_butterfly_calls++; // Do not increment until after the comparison code.

                            //printf("in_count = %u && (in_count % 16) == %u\n", in_count, (in_count%16));
                            if ((in_count > 0) && (in_count % 16) == 8) { // 8 or 11
                                unsigned char c;
                                //  Find current best path
                                // 
                                // INPUTS/OUTPUTS:  
                                //    RET_VAL     : (ignored)
                                //    mm0         : INPUT/OUTPUT  : Array [ 64 ]
                                //    mm1         : INPUT/OUTPUT  : Array [ 64 ]
                                //    pp0         : INPUT/OUTPUT  : Array [ 64 ] 
                                //    pp1         : INPUT/OUTPUT  : Array [ 64 ]
                                //    ntraceback  : INPUT         : int (I think effectively const for given run type; here 5 I think)
                                //    outbuf      : OUTPUT        : 1 byte
                                //    l_store_pos : GLOBAL IN/OUT : int (position in circular traceback buffer?)


                                //    l_mmresult  : GLOBAL OUTPUT : Array [ 64 bytes ] 
                                //    l_ppresult  : GLOBAL OUTPUT : Array [ntraceback][ 64 bytes ]

                                // CALL : viterbi_get_output_generic(l_metric0_generic, l_path0_generic, ntraceback, &c);
                                // unsigned char viterbi_get_output_generic(unsigned char *mm0, unsigned char *pp0, int ntraceback, unsigned char *outbuf) 
                                {
                                    unsigned char *mm0       = l_metric0_generic;
                                    unsigned char *pp0       = l_path0_generic;
                                    //int ntraceback = ntraceback;
                                    unsigned char *outbuf = &c;

                                    int i;
                                    int bestmetric, minmetric;
                                    int beststate = 0;
                                    int pos = 0;
                                    int j;
                                    //printf("Finding current best-path\n");
                                    //printf(" Setting l_store_pos = (%u + 1) MOD %u\n", l_store_pos, ntraceback);

                                    // circular buffer with the last ntraceback paths
                                    l_store_pos = (l_store_pos + 1) % ntraceback;

                                    //printf("Setting up l_mmresult and l_ppresult\n");
                                    for (i = 0; i < 4; i++) {
                                        for (j = 0; j < 16; j++) {
                                            l_mmresult[(i*16) + j] = mm0[(i*16) + j];
                                            l_ppresult[l_store_pos][(i*16) + j] = pp0[(i*16) + j];
                                        }
                                    }

                                    //printf("Setting up best and min metric\n");
                                    // Find out the best final state
                                    bestmetric = l_mmresult[beststate];
                                    minmetric = l_mmresult[beststate];
                                    //printf("  bestmetric = %u   minmetric = %u\n", bestmetric, minmetric);

                                    for (i = 1; i < 64; i++) {
                                        if (l_mmresult[i] > bestmetric) {
                                            bestmetric = l_mmresult[i];
                                            beststate = i;
                                        }
                                        if (l_mmresult[i] < minmetric) {
                                            minmetric = l_mmresult[i];
                                        }
                                    }

                                    //printf("  final bestmetric = %u   minmetric = %u\n", bestmetric, minmetric);
                                    // Trace back
                                    for (i = 0, pos = l_store_pos; i < (ntraceback - 1); i++) {
                                        // Obtain the state from the output bits
                                        // by clocking in the output bits in reverse order.
                                        // The state has only 6 bits
                                        beststate = l_ppresult[pos][beststate] >> 2;
                                        pos = (pos - 1 + ntraceback) % ntraceback;
                                    }

                                    //printf("  did traceback stuff... set outbuf at %p\n", outbuf);
                                    // Store output byte
                                    *outbuf = l_ppresult[pos][beststate];

                                    //printf("  set outbuf %p to %u\n", outbuf, *outbuf);

                                    for (i = 0; i < 4; i++) {
                                        for (j = 0; j < 16; j++) {
                                            pp0[(i*16) + j] = 0;
                                            mm0[(i*16) + j] = mm0[(i*16) + j] - minmetric;
                                        }
                                    }

                                    //return bestmetric;
                                }

                                //printf("    out_count = %u vs %u = ntraceback\n", out_count, ntraceback);
                                if (out_count >= ntraceback) {
                                    for (int i= 0; i < 8; i++) {
                                        //l_decoded[(out_count - ntraceback) * 8 + i] = (c >> (7 - i)) & 0x1;
                                        plm_out_ping[(out_count - ntraceback) * 8 + i] = (c >> (7 - i)) & 0x1;
                                        // if (n_decoded < 16) {
                                        //     printf("plm_out_ping[%2u] written as %u = %u\n", (out_count - ntraceback) * 8 + i, (c >> (7 - i)) & 0x1, (unsigned char)plm_out_ping[(out_count - ntraceback) * 8 + i]);
                                        // }
                                        n_decoded++;
                                    }
                                }
                                out_count++;
                            } // if (in_count >0) && (in_count % 16 == 8)
                        } // if (in_count % 4 == 0)
                        in_count++;
                    }

                }

#if(0)
                {
                    printf("\n memory out  = ");
                    int limi = 0;
                    //for (int li = 0; li < 32; li++) {
                    for (int li = 0; li < out_length; li++) {
                        std::cout << plm_out_ping[limi++];
                        if ((li % 8) == 7) { std::cout << " "; }
                        if ((li % 64) == 63) { std::cout << "\n               "; }
                    }
                    printf("\n\n");
                }
#endif


                //out_rem -= PLM_OUT_WORD;
                //BM: this->compute_store_handshake();
                //ping = !ping;
            }
            //BM
            {
                HLS_PROTO("compute-block-output-handshake");
                this->compute_done_req(); // Ack new input tile
                wait();
            }
            // this->compute_done_req();
        }

        // // Conclude
        // {
        //     this->process_done();
        // }
    }
}


void vitdodec::asi_controller(){
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
