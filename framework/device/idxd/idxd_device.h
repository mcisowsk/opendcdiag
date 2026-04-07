/*
 * Copyright 2026 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INC_IDXD_DEVICE_H
#define INC_IDXD_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    DEV_TYPE_DSA,
    DEV_TYPE_IAX,
} dev_type_t;

typedef enum
{
    WQ_MODE_DEDICATED,
    WQ_MODE_SHARED,
    WQ_MODE_UNKNOWN,
} wq_mode_t;

// Work Queue is an atomic piece of a programmable DSA HW,
// therefore device_info should store WQs, not DSA devices.
struct wq_info_t
{
    /// Logical OS processor number.
    /// On Unix systems, this is a sequential ID; on Windows, it encodes
    /// 64 * ProcessorGroup + ProcessorNumber
    int cpu_number;

    /// Package ID in the system. TODO: What is it for us? numa node? socket?
    int16_t package_id;

    /// WQ unique index within a device.
    int wq_id;
    /// Device that this WQ belongs to.
    int device_id;
    /// Group that this WQ belongs to.
    int group_id;

    /// Device type that this WQ belongs to (DSA/IAX).
    dev_type_t dev_type;
    /// Mode of this WQ (dedicated/shared).
    wq_mode_t mode;

    /// Features per device
    uint64_t gen_cap; // General Capabilities register contents

    /// Features per WQ
    bool block_on_fault;
    uint64_t max_transfer_size;

#ifdef __cplusplus
    int wq() const;        ///! Internal WQ number
#endif
};

// Alias for use in common framework code
typedef struct wq_info_t device_info_t;

extern struct wq_info_t *device_info;

#ifdef __cplusplus
inline int wq_info_t::wq() const
{
    return this - ::device_info;
}
#endif

// Not used at the moment
typedef unsigned __int128 device_features_t;
static const device_features_t device_compiler_features = 0;
#define cpu_has_feature(f)      ((device_compiler_features & (f)) == (f) || (device_features & (f)) == (f))

#endif // INC_IDXD_DEVICE_H
