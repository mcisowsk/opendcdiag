/*
 * Copyright 2026 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INC_TEST_DATA_IDXD_H
#define INC_TEST_DATA_IDXD_H

#include "test_data.h"

namespace PerThreadData {
struct alignas(64) TestIdxd : TestCommon
{
    void init()
    {
        TestCommon::init();
    }
};

using Test = TestIdxd;

} // namespace PerThreadData

#endif /* INC_TEST_DATA_IDXD_H */
