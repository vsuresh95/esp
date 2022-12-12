// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "fusion_unopt.hpp"
#include "fusion_unopt_directives.hpp"

// Functions

#include "fusion_unopt_functions.hpp"

// Processes

void fusion_unopt::load_input()
{

    // Reset
    {
        HLS_PROTO("load-reset");

        this->reset_load_input();

        // explicit PLM ports reset if any

        // User-defined reset code


        load_state_req_dbg.write(0);

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t veno;
    int32_t imgwidth;
    int32_t htdim;
    int32_t imgheight;
    int32_t sdf_block_size;
    int32_t sdf_block_size3;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        veno = config.veno;
        imgwidth = config.imgwidth;
        htdim = config.htdim;
        imgheight = config.imgheight;
        sdf_block_size = config.sdf_block_size;
        sdf_block_size3 = config.sdf_block_size3;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        wait();

        load_state_req_dbg.write(1);

        bool ping = true;
        uint32_t offset = 0;

        // Batching
        for (uint16_t b = 0; b < 1; b++)
        {
            wait();
#if (DMA_WORD_PER_BEAT == 0)
            uint32_t length = 25 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5 + veno + 1;
#else
            uint32_t length = round_up(25 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5 + veno, DMA_WORD_PER_BEAT);
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
                    if (ping)
                        plm_in_ping[i] = dataBv.to_int64();
                    else
                        plm_in_pong[i] = dataBv.to_int64();
                }
#else
                for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_in_ping);
                    HLS_BREAK_DEP(plm_in_pong);

                    sc_dt::sc_bv<DMA_WIDTH> dataBv;

                    dataBv = this->dma_read_chnl.get();
                    wait();

                    // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (ping)
                            plm_in_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            plm_in_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
#endif
                this->load_compute_handshake();
                load_state_req_dbg.write(0);
                ping = !ping;
            }
        }
    }

    // Conclude
    {
        this->process_done();
    }
}



void fusion_unopt::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        // explicit PLM ports reset if any

        // User-defined reset code


        store_state_req_dbg.write(0);

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t veno;
    int32_t imgwidth;
    int32_t htdim;
    int32_t imgheight;
    int32_t sdf_block_size;
    int32_t sdf_block_size3;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        veno = config.veno;
        imgwidth = config.imgwidth;
        htdim = config.htdim;
        imgheight = config.imgheight;
        sdf_block_size = config.sdf_block_size;
        sdf_block_size3 = config.sdf_block_size3;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        bool ping = true;
#if (DMA_WORD_PER_BEAT == 0)
        uint32_t store_offset = (25 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5 + veno + 1) * 1;
#else
        uint32_t store_offset = round_up(25 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5 + veno + 1, DMA_WORD_PER_BEAT) * 1;
        // uint32_t store_offset = round_up(PLM_IN_WORD, DMA_WORD_PER_BEAT) * 1;
#endif
        uint32_t offset = store_offset;

        wait();
        // Batching
        for (uint16_t b = 0; b < 1; b++)
        {
            wait();
#if (DMA_WORD_PER_BEAT == 0)
            uint32_t length = veno * sdf_block_size3;
#else
            uint32_t length = round_up(25 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5 + veno + 1, DMA_WORD_PER_BEAT);
            // uint32_t length = round_up(PLM_OUT_WORD, DMA_WORD_PER_BEAT);
#endif
            // Chunking
            for (int rem = length; rem > 0; rem -= PLM_OUT_WORD)
            {

                this->store_compute_handshake();

                store_state_req_dbg.write(1);

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

                store_state_req_dbg.write(2);

#if (DMA_WORD_PER_BEAT == 0)
                // data word is wider than NoC links
                for (uint16_t i = 0; i < len; i++)
                {
                    // Read from PLM
                    sc_dt::sc_int<DATA_WIDTH> data;
                    wait();
                    if (ping)
                        data = plm_out_ping[i];
                    else
                        data = plm_out_pong[i];
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
                        if (ping)
                            dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_ping[i + k];
                        else
                            dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_pong[i + k];
                    }
                    this->dma_write_chnl.put(dataBv);
                }
#endif
                store_state_req_dbg.write(3);
                ping = !ping;
            }
        }
    }

    // Conclude
    {
        this->accelerator_done();
        this->process_done();
    }
}

// sc_dt::sc_int<DATA_WIDTH> fusion_unopt::computeUpdatedVoxelDepthInfo(int32_t voxel[2], // voxel[0] = sdf, voxel[1] = w_depth
//                                 int32_t pt_model[4],
//                                 int32_t M_d[16],
//                                 int32_t projParams_d[4],
//                                 int32_t mu,
//                                 int32_t maxW,
//                                 // int32_t depth[IMGWIDTH * IMGHEIGHT],	// remember to change this value when changing the size of the image
//                                 int32_t depth[1],	// remember to change this value when changing the size of the image
//                                 int32_t imgSize[2],
// 								sc_dt::sc_int<DATA_WIDTH> newValue[2]) {
    
//     int32_t pt_camera[4], pt_image[2];
//     int32_t depth_measure, eta, oldF, newF;
//     // int oldW, newW;
//     int32_t oldW, newW;

//     int32_t comp;

//     // project point into image
//     for (int i = 0; i < 4; ++i) {
//         for (int j = 0; j < 4; ++j) {
//             if (j == 0) {
//                 pt_camera[i] = 0;
//             }
//             pt_camera[i] += M_d[i * 4 + j] * pt_model[j];
//         }
//     }

//     // pt_image[0] = projParams_d[0] * pt_camera[0] / pt_camera[2] + projParams_d[2];
// 	// pt_image[1] = projParams_d[1] * pt_camera[1] / pt_camera[2] + projParams_d[3];
//     pt_image[0] = projParams_d[0] * pt_camera[0] >> 4 + projParams_d[2];    // dummy constant here because of the division
// 	pt_image[1] = projParams_d[1] * pt_camera[1] >> 4 + projParams_d[3];    // dummy constant here because of the division

//     // get measured depth from image
//     // depth_measure = depth[(int)(pt_image[0] + 0.5f) + (int)((pt_image[1] + 0.5f) * imgSize[0])];
//     int depthID = (int)(pt_image[0] + 1) + (int)((pt_image[1] + 1) * imgSize[0]);
//     // if (depthID < 0) {
//     //     depth_measure = depth[0];
//     // }
//     // else if (depthID > IMGWIDTH * IMGHEIGHT - 1) {
//     //     depth_measure = depth[IMGWIDTH * IMGHEIGHT - 1];
//     // }
//     // else {
//     //     depth_measure = depth[depthID];
//     // }
//     depth_measure = depth[0];

//     // check whether voxel needs updating
//     eta = depth_measure - pt_camera[2];

//     // oldF = voxel.sdf;
//     // oldW = voxel.w_depth;

//     oldF = voxel[0];
//     oldW = voxel[1];
//     newW = 1;   // dummy constant here. fix after having the FP <-> FXP conversion

//     // comp = eta / mu;
//     mu = mu >> 2;    // dummy constant here because of the division
//     comp = eta >> 3;    // dummy constant here because of the division

//     if (comp < 100) {   // dummy constant here. fix after having the FP <-> FXP conversion
//         newF = comp;
//     }
//     else {
//         newF = 100; // dummy constant here. fix after having the FP <-> FXP conversion
//     }

//     newF = oldW * oldF + newW * newF;
// 	newW = oldW + newW;

// 	// newF /= newW;
//     newF = newF >> 3;  // dummy constant here because of the division

//     if (newW > maxW) {
//         newW = maxW;
//     }

//     // write back
// 	// voxel.sdf = newF;
// 	// voxel.w_depth = newW;

//     newValue[0] = newF;	// voxel[0] = newF;
// 	newValue[1] = newW;	// voxel[1] = newW;

//     return eta;
// }

void fusion_unopt::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        compute_state_req_dbg.write(0);

        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t veno;
    int32_t imgwidth;
    int32_t htdim;
    int32_t imgheight;
    int32_t sdf_block_size;
    int32_t sdf_block_size3;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        veno = config.veno;
        imgwidth = config.imgwidth;
        htdim = config.htdim;
        imgheight = config.imgheight;
        sdf_block_size = config.sdf_block_size;
        sdf_block_size3 = config.sdf_block_size3;
    }

    // Compute
    // bool ping = true;
    {

        int entryID, z, y, x, locID, i, j;
        int globalPos[3];
        int currentHashEntry[5];
        int32_t pt_model[4];
        int32_t imgSize[2] = {imgwidth, imgheight};

        int32_t voxel[2];
        int32_t M_d[16];
        int32_t projParams_d[4];
        int32_t mu;
        int32_t maxW;
        // int32_t depth[IMGWIDTH * IMGHEIGHT];
        int32_t depth[4];
        int32_t voxelSize;

        int32_t pt_camera[4], pt_image[2];
        int32_t depth_measure, eta, oldF, newF;
        int32_t oldW, newW;

        int32_t comp;

        int32_t depthID;

        this->compute_load_handshake();

        compute_state_req_dbg.write(1);

        for (z = 0; z < 16; ++z) {
            M_d[z] = plm_in_ping[3 + z];
        }
        for (z = 0; z < 4; ++z) {
            projParams_d[z] = plm_in_ping[19 + z];
        }
        for (z = 0; z < IMGWIDTH * IMGHEIGHT; ++z) {
            if (z < imgwidth * imgheight) {
            depth[z] = plm_in_ping[25 + z];
            }
        }

        mu = plm_in_ping[23];
        maxW = plm_in_ping[24];
        voxelSize = plm_in_ping[2];
	    
        for (entryID = 0; entryID < VENO; ++entryID) {    // hardcode to 10
            if (entryID < veno) {
            int id = plm_in_ping[25 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5 + entryID];
            for (z = 0; z < 5; ++z) {
			    currentHashEntry[z] = plm_in_ping[id * 5 + z];
		    }

            globalPos[0] = currentHashEntry[0] * sdf_block_size; // hardcode to 8
            globalPos[1] = currentHashEntry[1] * sdf_block_size; // hardcode to 8
            globalPos[2] = currentHashEntry[2] * sdf_block_size; // hardcode to 8

            for (z = 0; z < SDF_BLOCK_SIZE; ++z) {   // hardcode to 8
                if (z < sdf_block_size) {
                for (y = 0; y < SDF_BLOCK_SIZE; ++y) {   // hardcode to 8
                    if (y < sdf_block_size) {
                    for (x = 0; x < SDF_BLOCK_SIZE; ++x) {   // hardcode to 8
                        if (x < sdf_block_size) {
                        locID = x + y * sdf_block_size + z * sdf_block_size * sdf_block_size;

                        pt_model[0] = (int32_t)(globalPos[0] + x) * voxelSize;
                        pt_model[1] = (int32_t)(globalPos[1] + y) * voxelSize;
                        pt_model[2] = (int32_t)(globalPos[2] + z) * voxelSize;
                        pt_model[3] = 1.0f;

                        // *(plm_out_ping + entryID * sdf_block_size * sdf_block_size * sdf_block_size + z * sdf_block_size * sdf_block_size + y * sdf_block_size + x) = computeUpdatedVoxelDepthInfo((voxel + locID * 2),
                        // pt_model,
                        // M_d,
                        // projParams_d,
                        // mu,
                        // maxW,
                        // depth,
                        // imgSize,
                        // (plm_out_ping + entryID * sdf_block_size * sdf_block_size * sdf_block_size + z * sdf_block_size * sdf_block_size + y * sdf_block_size + x + 1));
                        // }

                        /* voxel fusion */
                        for (i = 0; i < 4; ++i) {
                            for (j = 0; j < 4; ++j) {
                                if (j == 0) {
                                    pt_camera[i] = 0;
                                }
                                // pt_camera[i] += plm_in_ping[3 + i * 4 + j] * pt_model[j];
                                pt_camera[i] += M_d[i * 4 + j] * pt_model[j];
                            }
                        }

                        // pt_image[0] = projParams_d[0] * pt_camera[0] / pt_camera[2] + projParams_d[2];
                        // pt_image[1] = projParams_d[1] * pt_camera[1] / pt_camera[2] + projParams_d[3];
                        // pt_image[0] = plm_in_ping[19 + 0] * pt_camera[0] >> 4 + plm_in_ping[19 + 2];    // dummy constant here because of the division
                        // pt_image[1] = plm_in_ping[19 + 1] * pt_camera[1] >> 4 + plm_in_ping[19 + 3];    // dummy constant here because of the division
                        pt_image[0] = projParams_d[0] * pt_camera[0] >> 4 + projParams_d[2];    // dummy constant here because of the division
                        pt_image[1] = projParams_d[1] * pt_camera[1] >> 4 + projParams_d[3];    // dummy constant here because of the division

                        // get measured depth from image
                        // depth_measure = depth[(int)(pt_image[0] + 0.5f) + (int)((pt_image[1] + 0.5f) * imgSize[0])];
                        depthID = (int)(pt_image[0] + 1) + (int)((pt_image[1] + 1) * imgSize[0]);
                        if (depthID < 0) {
                            // depth_measure = plm_in_ping[25 + 0];
                            depth_measure = depth[0];
                        }
                        else if (depthID > imgwidth * imgheight - 1) {
                            // depth_measure = plm_in_ping[25 + imgwidth * imgheight - 1];
                            depth_measure = depth[imgwidth * imgheight - 1];
                        }
                        else {
                            // depth_measure = plm_in_ping[25 + depthID];
                            depth_measure = depth[depthID];
                        }
                        // depth_measure = depth[0];

                        // check whether voxel needs updating
                        eta = depth_measure - pt_camera[2];

                        // oldF = voxel.sdf;
                        // oldW = voxel.w_depth;

                        oldF = plm_in_ping[locID];
                        oldW = plm_in_ping[locID + 1];
                        newW = 1;   // dummy constant here. fix after having the FP <-> FXP conversion

                        // comp = eta / mu;
                        mu = mu >> 2;    // dummy constant here because of the division
                        comp = eta >> 3;    // dummy constant here because of the division

                        if (comp < 100) {   // dummy constant here. fix after having the FP <-> FXP conversion
                            newF = comp;
                        }
                        else {
                            newF = 100; // dummy constant here. fix after having the FP <-> FXP conversion
                        }

                        newF = oldW * oldF + newW * newF;
                        newW = oldW + newW;

                        // newF /= newW;
                        newF = newF >> 3;  // dummy constant here because of the division

                        if (newW > maxW) {
                            newW = maxW;
                        }

                        plm_out_ping[entryID * sdf_block_size3 + z * sdf_block_size * sdf_block_size + y * sdf_block_size + x] = eta;

                        // write back
                        // voxel.sdf = newF;
                        // voxel.w_depth = newW;

                        plm_out_ping[entryID * sdf_block_size3 + z * sdf_block_size * sdf_block_size + y * sdf_block_size + x + 1] = newF;	// voxel[0] = newF;
                        plm_out_ping[entryID * sdf_block_size3 + z * sdf_block_size * sdf_block_size + y * sdf_block_size + x + 2] = newW;	// voxel[1] = newW;
                        }
                    }
                    }
                }
                }
            }
            }
        }

        this->compute_store_handshake();

        compute_state_req_dbg.write(0);

    }

    // Conclude
    {
        this->process_done();
    }
}
