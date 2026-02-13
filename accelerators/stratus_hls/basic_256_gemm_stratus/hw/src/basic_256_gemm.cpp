// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "basic_256_gemm.hpp"
#include "basic_256_gemm_directives.hpp"

// Functions

#include "basic_256_gemm_functions.hpp"

#define TILE_SIZE 16  // 16x16 tiles


// Processes

void basic_256_gemm::load_input()
{

    // Reset
    {
        HLS_PROTO("load-reset");
        this->reset_load_input();
        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t C_mat_offset;
    int32_t B_mat_offset;
    int32_t A_mat_offset;
    int32_t K;
    int32_t M;
    int32_t N;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        C_mat_offset = config.C_mat_offset;
        B_mat_offset = config.B_mat_offset;
        A_mat_offset = config.A_mat_offset;
        K = config.K;
        M = config.M;
        N = config.N;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        
        wait();

        bool ping = true;

        for (int i_tile = 0; i_tile < M; i_tile += TILE_SIZE){
            for (int j_tile = 0; j_tile < N; j_tile += TILE_SIZE){
                for (int k_tile = 0; k_tile < K; k_tile += TILE_SIZE){
                    wait();

                    int i_size = TILE_SIZE;
                    int j_size = TILE_SIZE;
                    int k_size = TILE_SIZE;

                    if(i_tile + TILE_SIZE > M){
                        i_size = M - i_tile;  
                    }
                    if(j_tile + TILE_SIZE > N){  
                        j_size = N - j_tile;
                    }
                    if(k_tile + TILE_SIZE > K){ 
                        k_size = K - k_tile;
                    }
                    
                    // === Load Matrix A tile (i_size * k_size)
                    uint32_t A_tile_size = i_size * k_size;
                    
                    #if (DMA_WORD_PER_BEAT == 0)
                        uint32_t A_length = A_tile_size;
                    #else
                        uint32_t A_length = round_up(A_tile_size, DMA_WORD_PER_BEAT);
                    #endif
                    
                    uint32_t plm_A_base = 0;

                    // Load A tile row by row
                    for (int i = 0; i < i_size; i++){
                        // Calculate offset in memory for A tile.
                        uint32_t A_row_offset = A_mat_offset + (i_tile + i) * K + k_tile;
                        #if (DMA_WORD_PER_BEAT == 0)
                            uint32_t row_length = k_size;
                        #else
                            uint32_t row_length = round_up(k_size, DMA_WORD_PER_BEAT);
                        #endif
                        
                        #if (DMA_WORD_PER_BEAT == 0)
                            dma_info_t dma_info(A_row_offset * DMA_BEAT_PER_WORD, 
                                              row_length * DMA_BEAT_PER_WORD, DMA_SIZE);
                        #else
                            dma_info_t dma_info(A_row_offset / DMA_WORD_PER_BEAT, 
                                              row_length / DMA_WORD_PER_BEAT, DMA_SIZE);
                        #endif
                        
                        wait();
                        this->dma_read_ctrl.put(dma_info);
                        
                        #if (DMA_WORD_PER_BEAT == 0)
                            for (uint32_t j = 0; j < row_length; j++){
                                sc_dt::sc_bv<DATA_WIDTH> dataBv;
                                for (uint16_t k = 0; k < DMA_BEAT_PER_WORD; k++)
                                {
                                    dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH) = 
                                        this->dma_read_chnl.get();
                                    wait();
                                }
                                if (ping)
                                    plm_in_ping[plm_A_base + i * k_size + j] = dataBv.to_int64();
                                else
                                    plm_in_pong[plm_A_base + i * k_size + j] = dataBv.to_int64();
                            }
                        #else
                            for (uint32_t j = 0; j < row_length; j += DMA_WORD_PER_BEAT){
                                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                                dataBv = this->dma_read_chnl.get();
                                wait();
                                for (uint16_t kk = 0; kk < DMA_WORD_PER_BEAT; kk++){
                                    HLS_UNROLL_SIMPLE;
                                    if (j + kk < k_size) {
                                        if (ping)
                                            plm_in_ping[plm_A_base + i * k_size + j + kk] = 
                                                dataBv.range((kk+1) * DATA_WIDTH - 1, kk * DATA_WIDTH).to_int64();
                                        else
                                            plm_in_pong[plm_A_base + i * k_size + j + kk] = 
                                                dataBv.range((kk+1) * DATA_WIDTH - 1, kk * DATA_WIDTH).to_int64();
                                    }
                                }
                            }
                        #endif
                    }
                    wait();
                    
                    // === Load B matrix tile (k_size × j_size) ===
                    uint32_t plm_B_base = i_size * k_size;
                    
                    // Load B tile row by row
                    for (int k = 0; k < k_size; k++){
                        // Calculate offset in memory for B tile
                        uint32_t B_row_offset = B_mat_offset + (k_tile + k) * N + j_tile;
                        
                        #if (DMA_WORD_PER_BEAT == 0)
                            uint32_t row_length = j_size;
                        #else
                            uint32_t row_length = round_up(j_size, DMA_WORD_PER_BEAT);
                        #endif
                        
                        #if (DMA_WORD_PER_BEAT == 0)
                            dma_info_t dma_info(B_row_offset * DMA_BEAT_PER_WORD, 
                                              row_length * DMA_BEAT_PER_WORD, DMA_SIZE);
                        #else
                            dma_info_t dma_info(B_row_offset / DMA_WORD_PER_BEAT, 
                                              row_length / DMA_WORD_PER_BEAT, DMA_SIZE);
                        #endif
                        wait();
                        this->dma_read_ctrl.put(dma_info);
                        

                        #if (DMA_WORD_PER_BEAT == 0)
                            for (uint32_t j = 0; j < row_length; j++){
                                sc_dt::sc_bv<DATA_WIDTH> dataBv;
                                for (uint16_t kk = 0; kk < DMA_BEAT_PER_WORD; kk++){
                                    dataBv.range((kk+1) * DMA_WIDTH - 1, kk * DMA_WIDTH) = 
                                        this->dma_read_chnl.get();
                                    wait();
                                }
                                if (ping)
                                    plm_in_ping[plm_B_base + k * j_size + j] = dataBv.to_int64();
                                else
                                    plm_in_pong[plm_B_base + k * j_size + j] = dataBv.to_int64();
                            }
                        #else
                            for (uint32_t j = 0; j < row_length; j += DMA_WORD_PER_BEAT){
                                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                                dataBv = this->dma_read_chnl.get();
                                wait();
                                
                                for (uint16_t kk = 0; kk < DMA_WORD_PER_BEAT; kk++){
                                    HLS_UNROLL_SIMPLE;
                                    if (j + kk < j_size) {
                                        if (ping)
                                            plm_in_ping[plm_B_base + k * j_size + j + kk] = 
                                                dataBv.range((kk+1) * DATA_WIDTH - 1, kk * DATA_WIDTH).to_int64();
                                        else
                                            plm_in_pong[plm_B_base + k * j_size + j + kk] = 
                                                dataBv.range((kk+1) * DATA_WIDTH - 1, kk * DATA_WIDTH).to_int64();
                                    }
                                }
                            }
                        #endif
                    }
                    //tell signal that load is done.
                    this->load_compute_handshake();
                    ping = !ping;
                    wait();
                }
                wait();
            }
            wait();
        }
    }
    // Conclude
    {
        this->process_done();
    }
}



void basic_256_gemm::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t C_mat_offset;
    int32_t B_mat_offset;
    int32_t A_mat_offset;
    int32_t K;
    int32_t M;
    int32_t N;
    {
        HLS_PROTO("store-config");
        cfg.wait_for_config(); 
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        C_mat_offset = config.C_mat_offset;
        B_mat_offset = config.B_mat_offset;
        A_mat_offset = config.A_mat_offset;
        K = config.K;
        M = config.M;
        N = config.N;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();
        
        bool ping = true;

        // Tile over M and N dimensions
        for (int i_tile = 0; i_tile < M; i_tile += TILE_SIZE){
            for (int j_tile = 0; j_tile < N; j_tile += TILE_SIZE){
                for (int k_tile = 0; k_tile < K; k_tile += TILE_SIZE){
                    // Wait for compute to complete
                    this->store_compute_handshake();

                    //tile dims
                    int i_size = TILE_SIZE;
                    int j_size = TILE_SIZE;

                    if(i_tile + TILE_SIZE > M){
                        i_size = M - i_tile; 
                    }
                    if(j_tile + TILE_SIZE > N){
                        j_size = N - j_tile; 
                    }

                    // Only store after full accumulation (LAST K tile)
                    if (k_tile + TILE_SIZE >= K){
                        // Store C tile row by row
                        for (int i = 0; i < i_size; i++){
                            uint32_t C_row_offset = C_mat_offset + (i_tile + i) * N + j_tile;
                            
                            #if (DMA_WORD_PER_BEAT == 0)
                                uint32_t row_length = j_size;
                            #else
                                uint32_t row_length = round_up(j_size, DMA_WORD_PER_BEAT);
                            #endif
                            
                            #if (DMA_WORD_PER_BEAT == 0)
                                dma_info_t dma_info(C_row_offset * DMA_BEAT_PER_WORD, 
                                                  row_length * DMA_BEAT_PER_WORD, DMA_SIZE);
                            #else
                                dma_info_t dma_info(C_row_offset / DMA_WORD_PER_BEAT, 
                                                  row_length / DMA_WORD_PER_BEAT, DMA_SIZE);
                            #endif
                            
                            wait();
                            this->dma_write_ctrl.put(dma_info);
                            

                            #if (DMA_WORD_PER_BEAT == 0)
                                for (uint32_t j = 0; j < row_length; j++){
                                    sc_dt::sc_int<DATA_WIDTH> data;
                                    wait();
                                    if (ping)
                                        data = plm_out_ping[i * j_size + j];
                                    else
                                        data = plm_out_pong[i * j_size + j];
                                    sc_dt::sc_bv<DATA_WIDTH> dataBv(data);

                                    uint16_t k = 0;
                                    for (k = 0; k < DMA_BEAT_PER_WORD - 1; k++){
                                        this->dma_write_chnl.put(
                                            dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH));
                                        wait();}
                                    this->dma_write_chnl.put(
                                        dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH));
                                }
                            #else
                                for (uint32_t j = 0; j < row_length; j += DMA_WORD_PER_BEAT){
                                    sc_dt::sc_bv<DMA_WIDTH> dataBv;
                                    
                                    wait();
                                    for (uint16_t kk = 0; kk < DMA_WORD_PER_BEAT; kk++){
                                        HLS_UNROLL_SIMPLE;
                                        if (j + kk < j_size) {
                                            if (ping)
                                                dataBv.range((kk+1) * DATA_WIDTH - 1, kk * DATA_WIDTH) = 
                                                    plm_out_ping[i * j_size + j + kk];
                                            else
                                                dataBv.range((kk+1) * DATA_WIDTH - 1, kk * DATA_WIDTH) = 
                                                    plm_out_pong[i * j_size + j + kk];
                                        }
                                    }
                                    this->dma_write_chnl.put(dataBv);
                                }
                            #endif
                        }
                    }
                    wait();
                }
                ping = !ping;
                wait();
            }
            wait();
        }
    }
    // Conclude
    {
        this->accelerator_done();
        this->process_done();
    }
}


void basic_256_gemm::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");
        this->reset_compute_kernel();
        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t C_mat_offset;
    int32_t B_mat_offset;
    int32_t A_mat_offset;
    int32_t K;
    int32_t M;
    int32_t N;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        C_mat_offset = config.C_mat_offset;
        B_mat_offset = config.B_mat_offset;
        A_mat_offset = config.A_mat_offset;
        K = config.K;
        M = config.M;
        N = config.N;
    }


    // Compute
    bool ping_in = true;
    bool ping_out = true;
    {
        // Tile over M and N dimensions
        for (int i_tile = 0; i_tile < M; i_tile += TILE_SIZE){
            for (int j_tile = 0; j_tile < N; j_tile += TILE_SIZE){
                // tile dims
                int i_size = TILE_SIZE;
                int j_size = TILE_SIZE;
                
                if(i_tile + TILE_SIZE > M){
                    i_size = M - i_tile; 
                }
                if(j_tile + TILE_SIZE > N){
                    j_size = N - j_tile; 
                }                

                // Clear output tile before accumulation
                for (int i = 0; i < i_size; i++) {
                    for (int j = 0; j < j_size; j++) {
                        if (ping_out)
                            plm_out_ping[i * j_size + j] = 0;
                        else
                            plm_out_pong[i * j_size + j] = 0;
                    }
                }

                // Accumulate over K dimension
                for (int k_tile = 0; k_tile < K; k_tile += TILE_SIZE){
                    // Wait for load to complete
                    this->compute_load_handshake();
                    
                    int k_size = (k_tile + TILE_SIZE > K) ? (K - k_tile) : TILE_SIZE;
                    
                    uint32_t A_base = 0;
                    uint32_t B_base = i_size * k_size;

                    // Compute tile multiplication: C_tile += A_tile × B_tile
                    for (int i = 0; i < i_size; i++){
                        for (int j = 0; j < j_size; j++){
                            sc_dt::sc_int<DATA_WIDTH> sum;
                            if (ping_out)
                                sum = plm_out_ping[i * j_size + j];
                            else
                                sum = plm_out_pong[i * j_size + j];
                            
                            for (int k = 0; k < k_size; k++){
                                sc_dt::sc_int<DATA_WIDTH> a_val, b_val;
                                if (ping_in) {
                                    a_val = plm_in_ping[A_base + i * k_size + k];
                                    // B is stored row-major as B[k][j], not B[j][k]
                                    b_val = plm_in_ping[B_base + k * j_size + j]; 
                                } 
                                else {
                                    a_val = plm_in_pong[A_base + i * k_size + k];
                                    b_val = plm_in_pong[B_base + k * j_size + j];
                                }
                                sum += a_val * b_val;
                            }
                            
                            if (ping_out)
                                plm_out_ping[i * j_size + j] = sum;
                            else
                                plm_out_pong[i * j_size + j] = sum;
                        }
                    }

                    // Signal store
                    this->compute_store_handshake();
                    ping_in = !ping_in;
                }

                ping_out = !ping_out;
            }
        }
    }

    // Conclude
    {
        this->process_done();
    }
}