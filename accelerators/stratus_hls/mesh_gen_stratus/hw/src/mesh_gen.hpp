// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __MESH_GEN_HPP__
#define __MESH_GEN_HPP__

#include "mesh_gen_conf_info.hpp"
#include "mesh_gen_debug_info.hpp"

#include "esp_templates.hpp"

#include "mesh_gen_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 64
#define DMA_SIZE SIZE_DWORD
#define PLM_OUT_WORD 8192
#define PLM_IN_WORD 8192

#define HASH_ENTRY_LEN 4
#define TRIANGLE_LEN 9
#define MAX_TRIANGLE 5
#define NUM_VERT 8
#define NUM_EDGE 12
#define SDF_BLOCK_SIZE 8
#define SDF_BLOCK_SIZE3 256
#define VOXEL_SIZE 5

struct Vector3f {
    double x;
    double y;
    double z;

    // scalar multiply
    friend Vector3f operator * (Vector3f &lhs, double rhs) {
        lhs.x *= rhs;
        lhs.y *= rhs;
        lhs.z *= rhs;
        return lhs;
    }
};

struct Vector3i {
   int64_t x;
   int64_t y;
   int64_t z;
};

struct Triangle {
    Vector3f p0;
    Vector3f p1;
    Vector3f p2;
};

struct HashEntry {
    Vector3i pos;
    int64_t ptr;
};

class mesh_gen : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Constructor
    SC_HAS_PROCESS(mesh_gen);
    mesh_gen(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
    {
        // Signal binding
        cfg.bind_with(*this);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(plm_hashtable, PLM_TRIANGLETABLE_NAME);
        HLS_MAP_plm(plm_triangles, PLM_TRIANGLES_NAME);
        HLS_MAP_plm(plm_voxelblock, PLM_VOXELBLOCK_NAME);
        HLS_MAP_plm(plm_edgeTable, PLM_EDGETABLE_NAME);
        HLS_MAP_plm(plm_triangleTable, PLM_TRIANGLETABLE_NAME);
    }

    // Processes

    // Load the input data
    void load_input();

    // Computation
    Vector3f sdfInterp(Vector3f p1, Vector3f p2, float valp1, float valp2);
    void compute_kernel();

    // Store the output data
    void store_output();

    // Configure mesh_gen
    esp_config_proc cfg;

    // Functions
    void initialize_edgetable(sc_dt::sc_int<DATA_WIDTH> *plm_input);
    void initialize_triangletable(sc_dt::sc_int<DATA_WIDTH> *plm_input);

    // Private local memories
    HashEntry plm_hashtable[PLM_IN_WORD];
    Triangle plm_triangles[SDF_BLOCK_SIZE3*MAX_TRIANGLE];
    double plm_voxelblock[SDF_BLOCK_SIZE3*NUM_VERT];
    sc_dt::sc_int<DATA_WIDTH> plm_edgeTable[256];
    sc_dt::sc_int<DATA_WIDTH> plm_triangleTable[256*16];
};


#endif /* __MESH_GEN_HPP__ */
