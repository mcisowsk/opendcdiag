/*
 * Copyright 2026 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INC_IDXD_UTILS_HPP
#define INC_IDXD_UTILS_HPP

#include <accel-config/libaccel_config.h>

#include <string>

struct AccfgCtx
{
    accfg_ctx* ctx = nullptr;

    AccfgCtx() = default;
    ~AccfgCtx() {
        if (ctx) accfg_unref(ctx);
    }
    int init() {
        if (accfg_new(&ctx) < 0) {
            printf("Failed to create AccfgCtx\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // non-copyable
    AccfgCtx(const AccfgCtx&) = delete;
    AccfgCtx& operator=(const AccfgCtx&) = delete;

    accfg_ctx* get() const { return ctx; }
};

inline accfg_device* find_device(accfg_ctx* ctx, const std::string& name)
{
    accfg_device* device;
    accfg_device_foreach(ctx, device) {
        if (name == accfg_device_get_devname(device)) {
            return device;
        }
    }
    printf("Device not found: %s\n", name.c_str());
    return nullptr;
}

inline accfg_wq* find_wq(accfg_device* device, const std::string& name)
{
    accfg_wq* wq;
    accfg_wq_foreach(device, wq) {
        if (name == accfg_wq_get_devname(wq)) {
            return wq;
        }
    }
    printf("WQ not found: %s\n", name.c_str());
    return nullptr;
}

#endif // INC_IDXD_UTILS_HPP
