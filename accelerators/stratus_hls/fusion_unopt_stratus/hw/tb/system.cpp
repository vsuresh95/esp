// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <sstream>
#include "system.hpp"

// Process
void system_t::config_proc()
{

    // Reset
    {
        conf_done.write(false);
        conf_info.write(conf_info_t());
        wait();
    }

    ESP_REPORT_INFO("reset done");

    // Config
    load_memory();
    {
        conf_info_t config;
        // Custom configuration
        /* <<--params-->> */
        config.veno = veno;
        config.imgwidth = imgwidth;
        config.htdim = htdim;
        config.imgheight = imgheight;
        config.sdf_block_size = sdf_block_size;
        config.sdf_block_size3 = sdf_block_size3;

        wait(); conf_info.write(config);
        conf_done.write(true);
    }

    ESP_REPORT_INFO("config done");

    // Compute
    {
        // Print information about begin time
        sc_time begin_time = sc_time_stamp();
        ESP_REPORT_TIME(begin_time, "BEGIN - fusion_unopt");

        // Wait the termination of the accelerator
        do { wait(); } while (!acc_done.read());
        debug_info_t debug_code = debug.read();

        // Print information about end time
        sc_time end_time = sc_time_stamp();
        ESP_REPORT_TIME(end_time, "END - fusion_unopt");

        esc_log_latency(sc_object::basename(), clock_cycle(end_time - begin_time));
        wait(); conf_done.write(false);
    }

    // Validate
    {
        dump_memory(); // store the output in more suitable data structure if needed
        // check the results with the golden model
        if (validate())
        {
            ESP_REPORT_ERROR("validation failed!");
        } else
        {
            ESP_REPORT_INFO("validation passed!");
        }
    }

    // Conclude
    {
        sc_stop();
    }
}

int32_t system_t::computeUpdatedVoxelDepthInfo(int32_t voxel[2], // voxel[0] = sdf, voxel[1] = w_depth
                                int32_t pt_model[4],
                                int32_t M_d[16],
                                int32_t projParams_d[4],
                                int32_t mu,
                                int32_t maxW,
                                int32_t depth[640 * 480],	// remember to change this value when changing the size of the image
                                int32_t imgSize[2],
								int32_t newValue[2]) {
    
    int32_t pt_camera[4], pt_image[2];
    int32_t depth_measure, eta, oldF, newF;
    // int oldW, newW;
    int32_t oldW, newW;

    int32_t comp;

    // project point into image
    for (int i = 0; i < 4; ++i) {
        pt_camera[i] = 0;
        for (int j = 0; j < 4; ++j) {
            pt_camera[i] += M_d[i * 4 + j] * pt_model[j];
        }
    }

    // pt_image[0] = projParams_d[0] * pt_camera[0] / pt_camera[2] + projParams_d[2];
	// pt_image[1] = projParams_d[1] * pt_camera[1] / pt_camera[2] + projParams_d[3];
    pt_image[0] = projParams_d[0] * pt_camera[0] >> 4 + projParams_d[2];    // dummy constant here because of the division
	pt_image[1] = projParams_d[1] * pt_camera[1] >> 4 + projParams_d[3];    // dummy constant here because of the division

    // get measured depth from image
    // depth_measure = depth[(int)(pt_image[0] + 0.5f) + (int)((pt_image[1] + 0.5f) * imgSize[0])];
    depth_measure = depth[(int)(pt_image[0] + 1) + (int)((pt_image[1] + 1) * imgSize[0])];

    // check whether voxel needs updating
    eta = depth_measure - pt_camera[2];

    // oldF = voxel.sdf;
    // oldW = voxel.w_depth;

    oldF = voxel[0];
    oldW = voxel[1];
    newW = 1; // dummy constant here. fix after having the FP <-> FXP conversion

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

    // write back
	// voxel.sdf = newF;
	// voxel.w_depth = newW;

    newValue[0] = newF;	// voxel[0] = newF;
	newValue[1] = newW;	// voxel[1] = newW;

    return eta;
}

// Functions
void system_t::load_memory()
{
    // Optional usage check
#ifdef CADENCE
    if (esc_argc() != 1)
    {
        ESP_REPORT_INFO("usage: %s\n", esc_argv()[0]);
        sc_stop();
    }
#endif

    // Input data and golden output (aligned to DMA_WIDTH makes your life easier)
#if (DMA_WORD_PER_BEAT == 0)
    in_words_adj = 2 + 1 + 4 * 4 + 4 + 1 + 1 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5 + veno;
    out_words_adj = veno * sdf_block_size * sdf_block_size * sdf_block_size * 3;
#else
    in_words_adj = round_up(2 + 1 + 4 * 4 + 4 + 1 + 1 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5 + veno, DMA_WORD_PER_BEAT);
    out_words_adj = round_up(veno * sdf_block_size * sdf_block_size * sdf_block_size * 3, DMA_WORD_PER_BEAT);
#endif

    in_size = in_words_adj * (1);
    out_size = out_words_adj * (1);

    // Initialize input
    in = new int32_t[in_size];
    for (int i = 0; i < 1; i++)
        for (int j = 0; j < 2 + 1 + 4 * 4 + 4 + 1 + 1 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5 + veno; j++)
            in[i * in_words_adj + j] = (int32_t) j - 1000;

    // Compute golden output
    gold = new int32_t[out_size];
    for (int i = 0; i < 1; i++) {
        int entryID, z, y, x, locID;
        int globalPos[3];
        int currentHashEntry[5];
        int32_t pt_model[4];
        int32_t imgSize[2] = {imgwidth, imgheight};
        int32_t voxelSize = in[2];


        for (entryID = 0; entryID < 10; ++entryID) {
            if (entryID < veno) {
            int id = in[310809 + entryID];
            for (z = 0; z < 5; ++z) {
                currentHashEntry[z] = in[id * 5 + z];
            }

            globalPos[0] = currentHashEntry[0] * 8;
            globalPos[1] = currentHashEntry[1] * 8;
            globalPos[2] = currentHashEntry[2] * 8;

            for (z = 0; z < 8; ++z) {
                if (z < sdf_block_size) {
                for (y = 0; y < 8; ++y) {
                    if (y < sdf_block_size) {
                    for (x = 0; x < 8; ++x) {
                        if (x < sdf_block_size) {
                        locID = x + y * 8 + z * 64;

                        pt_model[0] = (int32_t)(globalPos[0] + x) * voxelSize;
                        pt_model[1] = (int32_t)(globalPos[1] + y) * voxelSize;
                        pt_model[2] = (int32_t)(globalPos[2] + z) * voxelSize;
                        pt_model[3] = 1.0f;

                        *(gold + entryID * sdf_block_size3 + z * sdf_block_size * sdf_block_size + y * sdf_block_size + x) = computeUpdatedVoxelDepthInfo((in + 2 + 2 + 1 + 4 * 4 + 4 + 1 + 1 + imgwidth*imgheight + locID * 2),
                        pt_model, (in + 2 + 1),
                        (in + 2 + 1 + 4 * 4),
                        *(in + 2 + 1 + 4 * 4 + 4),
                        *(in + 2 + 1 + 4 * 4 + 4 + 1),
                        (in + 2 + 1 + 4 * 4 + 4 + 1 + 1),
                        imgSize,
                        gold + entryID * sdf_block_size * sdf_block_size * sdf_block_size + z * sdf_block_size * sdf_block_size + y * sdf_block_size + x + 1);
                        }
                    }
                    }
                }
                }
            }
            }
        }
    }

    // Memory initialization:
#if (DMA_WORD_PER_BEAT == 0)
    for (int i = 0; i < in_size; i++)  {
        sc_dt::sc_bv<DATA_WIDTH> data_bv(in[i]);
        for (int j = 0; j < DMA_BEAT_PER_WORD; j++)
            mem[DMA_BEAT_PER_WORD * i + j] = data_bv.range((j + 1) * DMA_WIDTH - 1, j * DMA_WIDTH);
    }
#else
    for (int i = 0; i < in_size / DMA_WORD_PER_BEAT; i++)  {
        sc_dt::sc_bv<DMA_WIDTH> data_bv(in[i]);
        for (int j = 0; j < DMA_WORD_PER_BEAT; j++)
            data_bv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH) = in[i * DMA_WORD_PER_BEAT + j];
        mem[i] = data_bv;
    }
#endif

    ESP_REPORT_INFO("load memory completed");
}

void system_t::dump_memory()
{
    // Get results from memory
    out = new int32_t[out_size];
    uint32_t offset = in_size;

#if (DMA_WORD_PER_BEAT == 0)
    offset = offset * DMA_BEAT_PER_WORD;
    for (int i = 0; i < out_size; i++)  {
        sc_dt::sc_bv<DATA_WIDTH> data_bv;

        for (int j = 0; j < DMA_BEAT_PER_WORD; j++)
            data_bv.range((j + 1) * DMA_WIDTH - 1, j * DMA_WIDTH) = mem[offset + DMA_BEAT_PER_WORD * i + j];

        out[i] = data_bv.to_int64();
    }
#else
    offset = offset / DMA_WORD_PER_BEAT;
    for (int i = 0; i < out_size / DMA_WORD_PER_BEAT; i++)
        for (int j = 0; j < DMA_WORD_PER_BEAT; j++)
            out[i * DMA_WORD_PER_BEAT + j] = mem[offset + i].range((j + 1) * DATA_WIDTH - 1, j * DATA_WIDTH).to_int64();
#endif

    ESP_REPORT_INFO("dump memory completed");
}

int system_t::validate()
{
    // Check for mismatches
    uint32_t errors = 0;

    for (int i = 0; i < 1; i++)
        for (int j = 0; j < veno * sdf_block_size * sdf_block_size * sdf_block_size * 3; j++)
            if (gold[i * out_words_adj + j] != out[i * out_words_adj + j])
                errors++;

    delete [] in;
    delete [] out;
    delete [] gold;

    return errors;
}
