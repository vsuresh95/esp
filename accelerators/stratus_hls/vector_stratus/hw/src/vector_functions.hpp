// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "vector.hpp"

// Optional application-specific helper functions

inline uint32_t vector::cheap_divider(uint32_t dividend, uint32_t divisor)
{
    HLS_PROTO("cheap-divider");
    uint32_t quotient = 0;
    uint32_t used = 0;
    while (used + divisor <= dividend) {
        used += divisor;
        quotient++;
        wait();
    }
    return quotient;
}
