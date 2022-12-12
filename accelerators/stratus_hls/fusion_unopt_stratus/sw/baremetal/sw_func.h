token_t computeUpdatedVoxelDepthInfo(token_t* voxel, // voxel[0] = sdf, voxel[1] = w_depth
                                token_t* pt_model,
                                token_t* M_d,
                                token_t* projParams_d,
                                token_t mu,
                                token_t maxW,
                                token_t* depth,
                                token_t* imgSize,
                                token_t* newValue) {
    token_t pt_camera[4], pt_image[2];
    token_t depth_measure, eta, oldF, newF;
    // int oldW, newW;
    token_t oldW, newW;

    token_t comp;

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
    // depth_measure = depth[(int)(pt_image[0] + 0.5f) + (int)(pt_image[1] + 0.5f) * (int)imgSize[0]];
    int depthID = (int)(pt_image[0] + 1) + (int)((pt_image[1] + 1) * imgSize[0]);
    if (depthID < 0) {
        depth_measure = depth[0];
    }
    else if (depthID > imgwidth * imgheight - 1) {
        depth_measure = depth[imgwidth * imgheight - 1];
    }
    else {
        depth_measure = depth[depthID];
    }
    // depth_measure = depth[0];

    // check whether voxel needs updating
    eta = depth_measure - pt_camera[2];

    // oldF = voxel.sdf;
    // oldW = voxel.w_depth;

    oldF = voxel[0];
    oldW = voxel[1];

    // comp = eta / mu;
    mu = mu >> 2;    // dummy constant here because of the division
    comp = eta >> 3;    // dummy constant here because of the division

    if (comp < 100) {
        newF = comp;
    }
    else {
        newF = 100;
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

    newValue[0] = newF;
	newValue[1] = newW;

    return eta;
}

void IntegrateIntoScene_depth_s(token_t* depthImgSize,
		                        token_t voxelSize,
		                        token_t* M_d,
		                        token_t* projParams_d,
		                        token_t mu,
		                        token_t maxW,
		                        token_t* depth,
		                        token_t* localVoxelBlock,
		                        token_t* hashTable,    // hashTable[i][j] = pos[j], hashTable[i][3] = ofset, hashTable[i][4] = ptr
		                        token_t* visibleEntryIds,
                                token_t* etaOut) {
    int entryID, z, y, x, locID;
	int globalPos[3];
    int currentHashEntry[5];
    token_t pt_model[4];
    token_t imgSize[2] = {imgwidth, imgheight};

    for (entryID = 0; entryID < VENO; ++entryID) {
        if (entryID < veno) {
		for (z = 0; z < 5; ++z) {
			currentHashEntry[z] = hashTable[(int)(visibleEntryIds[entryID] + z)];
		}

		globalPos[0] = currentHashEntry[0] * sdf_block_size;
        globalPos[1] = currentHashEntry[1] * sdf_block_size;
        globalPos[2] = currentHashEntry[2] * sdf_block_size;

		for (z = 0; z < SDF_BLOCK_SIZE; ++z) {
            if (z < sdf_block_size) {
            for (y = 0; y < SDF_BLOCK_SIZE; ++y) {
                if (y < sdf_block_size) {
                for (x = 0; x < SDF_BLOCK_SIZE; ++x) {
                    if (x < sdf_block_size) {
                    locID = x + y * sdf_block_size + z * sdf_block_size * sdf_block_size;

                    pt_model[0] = (int32_t)(globalPos[0] + x) * voxelSize;
                    pt_model[1] = (int32_t)(globalPos[1] + y) * voxelSize;
                    pt_model[2] = (int32_t)(globalPos[2] + z) * voxelSize;
                    pt_model[3] = 1.0f;

                    etaOut[entryID * sdf_block_size * sdf_block_size * sdf_block_size + z * sdf_block_size * sdf_block_size + y * sdf_block_size + x] = computeUpdatedVoxelDepthInfo(localVoxelBlock + locID,
                    pt_model, M_d,
                    projParams_d,
                    mu,
                    maxW,
                    depth,
                    imgSize,
                    (etaOut + entryID * sdf_block_size * sdf_block_size * sdf_block_size + z * sdf_block_size * sdf_block_size + y * sdf_block_size + x + 1));
                    }
                }
                }
            }
            }
        }
        }
	}
}
