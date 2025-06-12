/*
 * Copyright 2024 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INC_DEVICES_H
#define INC_DEVICES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // a contiguous range
    int starting_device;
    int device_count;
} DeviceRange;

/// @brief Generic device discovery and initialization function. Need to be
/// implemented by the device-specific code. The function is called from the
/// framework's main() function.
void init_topology();

void init_num_devices(); // TODO it's because of init_shmem()...

int num_devices();

void apply_deviceset_param(char *param);

void restrict_topology(DeviceRange range);

#ifdef __cplusplus
}

// Base device type
class DeviceBase {
public:
    DeviceBase(int index) : index{index} {}
    virtual ~DeviceBase() {}
    int index;
};
#endif


#endif // INC_DEVICES_H
