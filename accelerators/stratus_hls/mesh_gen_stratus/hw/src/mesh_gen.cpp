// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "mesh_gen.hpp"
#include "init_table.hpp"
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
        uint32_t length = round_up(num_hash_table*HASH_ENTRY_LEN, DMA_WORD_PER_BEAT);

        // Fetch hash entries one by one
        for (uint16_t offset = 0; offset < length; offset += HASH_ENTRY_LEN)
        {
            dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, HASH_ENTRY_LEN / DMA_WORD_PER_BEAT, DMA_SIZE);

            this->dma_read_ctrl.put(dma_info);

            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            // HashEntry.pos.x
            dataBv = this->dma_read_chnl.get();
            wait();
            plm_hashtable[offset/HASH_ENTRY_LEN].pos.x = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            wait();

            // HashEntry.pos.y
            dataBv = this->dma_read_chnl.get();
            wait();
            plm_hashtable[offset/HASH_ENTRY_LEN].pos.y = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            wait();

            // HashEntry.pos.z
            dataBv = this->dma_read_chnl.get();
            wait();
            plm_hashtable[offset/HASH_ENTRY_LEN].pos.z = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            wait();

            // HashEntry.ptr
            dataBv = this->dma_read_chnl.get();
            wait();
            plm_hashtable[offset/HASH_ENTRY_LEN].ptr = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            wait();

            int64_t voxel_offset = plm_hashtable[offset/HASH_ENTRY_LEN].ptr;

            // Read voxel block
            uint32_t len = round_up(SDF_BLOCK_SIZE3, DMA_WORD_PER_BEAT);

            dma_info_t dma_info(voxel_offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);

            this->dma_read_ctrl.put(dma_info);

            for (uint16_t j = 0; j < len; j += DMA_WORD_PER_BEAT)
            {
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                dataBv = this->dma_read_chnl.get();
                wait();

                plm_voxelblock[j] = dataBv.range(DATA_WIDTH - 1, 0).to_int64();                
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
        uint32_t store_offset = num_hash_table * HASH_ENTRY_LEN;

        // Store back computed triangles for hash entries read
        for (uint16_t offset = 0; offset < length; offset++)
        {
            wait();
            uint32_t len = round_up(SDF_BLOCK_SIZE3*MAX_TRIANGLE, DMA_WORD_PER_BEAT);

            // Chunking
            this->store_compute_handshake();

            for (uint16_t i = 0; i < len; i++)
            {
                // Configure DMA transaction
                dma_info_t dma_info(store_offset / DMA_WORD_PER_BEAT, TRIANGLE_LEN / DMA_WORD_PER_BEAT, DMA_SIZE);
                store_offset += TRIANGLE_LEN;

                this->dma_write_ctrl.put(dma_info);

                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                // Triangle.p0.x
                wait();
                dataBv.range(DATA_WIDTH - 1, 0) = (int64_t) plm_triangles[i].p0.x;
                this->dma_write_chnl.put(dataBv);

                // Triangle.p0.y
                wait();
                dataBv.range(DATA_WIDTH - 1, 0) = (int64_t) plm_triangles[i].p0.y;
                this->dma_write_chnl.put(dataBv);

                // Triangle.p0.z
                wait();
                dataBv.range(DATA_WIDTH - 1, 0) = (int64_t) plm_triangles[i].p0.z;
                this->dma_write_chnl.put(dataBv);

                // Triangle.p1.x
                wait();
                dataBv.range(DATA_WIDTH - 1, 0) = (int64_t) plm_triangles[i].p1.x;
                this->dma_write_chnl.put(dataBv);

                // Triangle.p1.y
                wait();
                dataBv.range(DATA_WIDTH - 1, 0) = (int64_t) plm_triangles[i].p1.y;
                this->dma_write_chnl.put(dataBv);

                // Triangle.p1.z
                wait();
                dataBv.range(DATA_WIDTH - 1, 0) = (int64_t) plm_triangles[i].p1.z;
                this->dma_write_chnl.put(dataBv);

                // Triangle.p2.x
                wait();
                dataBv.range(DATA_WIDTH - 1, 0) = (int64_t) plm_triangles[i].p2.x;
                this->dma_write_chnl.put(dataBv);

                // Triangle.p2.y
                wait();
                dataBv.range(DATA_WIDTH - 1, 0) = (int64_t) plm_triangles[i].p2.y;
                this->dma_write_chnl.put(dataBv);

                // Triangle.p2.z
                wait();
                dataBv.range(DATA_WIDTH - 1, 0) = (int64_t) plm_triangles[i].p2.z;
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

Vector3f sdfInterp(Vector3f &p1, Vector3f &p2, double valp1, double valp2)
{
	if (fabs(0.0f - valp1) < 0.00001f) return p1;
	if (fabs(0.0f - valp2) < 0.00001f) return p2;
	if (fabs(valp1 - valp2) < 0.00001f) return p1;

    Vector3f p;
    p.x = p2.x - p1.x; 
    p.y = p2.y - p1.y; 
    p.z = p2.z - p1.z; 

    double val = ((0.0f - valp1) / (valp2 - valp1));
    p.x = val * p.x; 
    p.y = val * p.y; 
    p.z = val * p.z; 

    p.x = p1.x + p.x; 
    p.y = p1.y + p.y; 
    p.z = p1.z + p.z; 

	return p;
}

void mesh_gen::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        // explicit PLM ports reset if any

        // User-defined reset code
        initialize_edgetable(plm_edgeTable);
        initialize_triangletable(plm_triangleTable);

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
        for (uint16_t offset = 0; offset < length; offset++)
        {
            this->compute_load_handshake();

            // take hash entry and get the pointer to voxel block, and global position
            Vector3i globalPos;
            globalPos = plm_hashtable[offset].pos;

            uint16_t noTriangles = 0;

            // Calculate cube index and vert list for each voxel in voxel block
            for (uint16_t z = 0; z < SDF_BLOCK_SIZE; z++)
            {
                uint16_t sdf_offset_z = z*SDF_BLOCK_SIZE*SDF_BLOCK_SIZE*NUM_VERT;

                for (uint16_t y = 0; y < SDF_BLOCK_SIZE; y++)
                {
                    uint16_t sdf_offset_y = y*SDF_BLOCK_SIZE*NUM_VERT;

                    for (uint16_t x = 0; x < SDF_BLOCK_SIZE; x++)
                    {
                        uint16_t sdf_offset_x = x*NUM_VERT;

                        double sdf[NUM_VERT];
                        Vector3f p[NUM_VERT];

                        // assign sdf value for each vertex of the cube to register
                        for (uint16_t vert = 0; vert < NUM_VERT; vert++)
                        {
                            sdf[vert] = plm_voxelblock[sdf_offset_z + sdf_offset_y + sdf_offset_x + vert];
                        }

                        // assign location value for each vertex of the cube to register
                        {
                            p[0].x = globalPos.x + x + 0; p[0].y = globalPos.y + y + 0; p[0].z = globalPos.z + z + 0;
                            p[1].x = globalPos.x + x + 1; p[1].y = globalPos.y + y + 0; p[1].z = globalPos.z + z + 0;
                            p[2].x = globalPos.x + x + 1; p[2].y = globalPos.y + y + 1; p[2].z = globalPos.z + z + 0;
                            p[3].x = globalPos.x + x + 0; p[3].y = globalPos.y + y + 1; p[3].z = globalPos.z + z + 0; 
                            p[4].x = globalPos.x + x + 0; p[4].y = globalPos.y + y + 0; p[4].z = globalPos.z + z + 1;
                            p[5].x = globalPos.x + x + 1; p[5].y = globalPos.y + y + 0; p[5].z = globalPos.z + z + 1;
                            p[6].x = globalPos.x + x + 1; p[6].y = globalPos.y + y + 1; p[6].z = globalPos.z + z + 1;
                            p[7].x = globalPos.x + x + 0; p[7].y = globalPos.y + y + 1; p[7].z = globalPos.z + z + 1;
                        }

                        // calculate cube index
                        int cubeIndex = 0;

                        for (uint16_t vert = 0; vert < NUM_VERT; vert++)
                        {
                            if (sdf[vert] < 0) cubeIndex |= 1 << vert;
                        }

			            if (plm_edgeTable[cubeIndex] == 0) continue;

                        Vector3f vertList[12];

                        {
                            if (plm_edgeTable[cubeIndex] & 1) vertList[0] = sdfInterp(p[0], p[1], sdf[0], sdf[1]);
                            if (plm_edgeTable[cubeIndex] & 2) vertList[1] = sdfInterp(p[1], p[2], sdf[1], sdf[2]);
                            if (plm_edgeTable[cubeIndex] & 4) vertList[2] = sdfInterp(p[2], p[3], sdf[2], sdf[3]);
                            if (plm_edgeTable[cubeIndex] & 8) vertList[3] = sdfInterp(p[3], p[0], sdf[3], sdf[0]);
                            if (plm_edgeTable[cubeIndex] & 16) vertList[4] = sdfInterp(p[4], p[5], sdf[4], sdf[5]);
                            if (plm_edgeTable[cubeIndex] & 32) vertList[5] = sdfInterp(p[5], p[6], sdf[5], sdf[6]);
                            if (plm_edgeTable[cubeIndex] & 64) vertList[6] = sdfInterp(p[6], p[7], sdf[6], sdf[7]);
                            if (plm_edgeTable[cubeIndex] & 128) vertList[7] = sdfInterp(p[7], p[4], sdf[7], sdf[4]);
                            if (plm_edgeTable[cubeIndex] & 256) vertList[8] = sdfInterp(p[0], p[4], sdf[0], sdf[4]);
                            if (plm_edgeTable[cubeIndex] & 512) vertList[9] = sdfInterp(p[1], p[5], sdf[1], sdf[5]);
                            if (plm_edgeTable[cubeIndex] & 1024) vertList[10] = sdfInterp(p[2], p[6], sdf[2], sdf[6]);
                            if (plm_edgeTable[cubeIndex] & 2048) vertList[11] = sdfInterp(p[3], p[7], sdf[3], sdf[7]);
                        }

			            for (int i = 0; plm_triangleTable[cubeIndex][i] != -1; i += 3)
			            {
			            	plm_triangles[noTriangles].p0 = vertList[plm_triangleTable[cubeIndex][i]] * VOXEL_SIZE;
			            	plm_triangles[noTriangles].p1 = vertList[plm_triangleTable[cubeIndex][i + 1]] * VOXEL_SIZE;
			            	plm_triangles[noTriangles].p2 = vertList[plm_triangleTable[cubeIndex][i + 2]] * VOXEL_SIZE;

                            noTriangles++;
			            }
                    }
                }
            }

            // Triangle calculation

            this->compute_store_handshake();
        }

        // Conclude
        {
            this->process_done();
        }
    }
}
