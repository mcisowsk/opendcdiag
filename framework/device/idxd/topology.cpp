/*
 * Copyright 2026 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sandstone_p.h>
#include "topology.h"
#include "idxd_config.hpp"
#include "idxd_device.h"
#include "idxd_utils.hpp"
#include "topology_idxd.hpp"

#include <accel-config/libaccel_config.h>

#include <charconv>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>

#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>

namespace fs = std::filesystem;

struct wq_info_t *device_info = nullptr;

int num_packages()
{
    return Topology::topology().nodes.size();
}

void make_rescheduler(RescheduleMode mode)
{
}

namespace {
Topology &cached_topology()
{
    static Topology cached_topology = Topology();
    return cached_topology;
}
}

const Topology &Topology::topology()
{
    return cached_topology();
}

void apply_deviceset_param(const char *param)
{
}

std::string build_failure_mask_for_topology(const struct test* test)
{
    return {};
}

uint32_t mixin_from_device_info(int thread_num)
{
    return 6789;
}

void print_temperature_of_device()
{
}

/// Collect all WQ visible in the system, whether they're enabled or not. Do not create any hierarchy of them at this point.
template <>
WorkQueueSet detect_devices<WorkQueueSet>()
{
    WorkQueueSet enabled_devices;
    AccfgCtx ctx;
    if (auto ret = ctx.init(); ret) {
        return enabled_devices;
    }

    accfg_device* device;
    accfg_device_foreach(ctx.get(), device) {
        const std::string_view dev_name = accfg_device_get_devname(device);

        dev_type_t dev_type;
        if (dev_name.starts_with("dsa"))
            dev_type = DEV_TYPE_DSA;
        else if (dev_name.starts_with("iax"))
            dev_type = DEV_TYPE_IAX;
        else
            continue;

        int device_id = accfg_device_get_id(device);

        accfg_wq* wq;
        accfg_wq_foreach(device, wq) {
            auto& enabled = enabled_devices.emplace_back();
            enabled.device_id = device_id;
            enabled.wq_id     = accfg_wq_get_id(wq);
            enabled.dev_type  = dev_type;

            printf("wq%d.%d found!\n", enabled.device_id, enabled.wq_id);
        }
    }

    sApp->thread_count = enabled_devices.size();
    sApp->user_thread_data.resize(sApp->thread_count);

    return enabled_devices;
}

/// Unlike other tools, here we want to change system's configuration.
/// Since this is called before any opt is parsed, we have to fist locate
/// out specific option here. We expect a json file.
/// This is somewhat hacky.
int apply_user_device_config(int argc, char **argv)
{
    static constexpr int idxd_user_config_option = 1; // it's the only option
    static struct option opts[] = {
        { "idxd-config", required_argument, nullptr, idxd_user_config_option },
        { nullptr, 0, nullptr, 0 }
    };

    std::string path;

    int opt;
    int coptind = -1;
    optind = 1;
    assert(opterr == 0 && "apply_user_device_config should not print any error messages from getopt");

    while ((opt = getopt_long(argc, argv, "", opts, &coptind)) != -1) {
        switch (opt) {
        case idxd_user_config_option:
            path = std::string{optarg};
            break;
        }
    }

    if (path.empty()) {
        // no config specified by user
        return EXIT_SUCCESS;
    }

    if (!fs::exists(fs::path{path})) {
        fprintf(stderr, "IDXD user config file %s does not exist\n", path.c_str());
        return EX_USAGE;
    }

    return apply_idxd_config(path.c_str());
}

void create_mock_topology(const char *topo)
{
}

namespace {
void append_topo_group(Topology::Group& group, wq_info_t* info)
{
    group.wqs.push_back(info);
}

void append_topo_device(Topology::Device& device, wq_info_t* info)
{
    if (auto it = std::find_if(device.groups.begin(), device.groups.end(), [&](const auto& g) { return g.index == info->group_id; }); it != device.groups.end()) {
        append_topo_group(*it, info);
    } else {
        auto& group = device.groups.emplace_back();
        group.index = info->group_id;
        // group.engines = // TODO
        append_topo_group(group, info);
    }
}

void append_topo_node(Topology::Node& node, wq_info_t* info)
{
    if (auto it = std::find_if(node.devices.begin(), node.devices.end(), [&](const auto& d) { return d.index == info->device_id; }); it != node.devices.end()) {
        append_topo_device(*it, info);
    } else {
        auto& device = node.devices.emplace_back();
        device.index = info->device_id;
        // device.bdf = ??here?? // TODO
        append_topo_device(device, info);
    }
}

Topology build_topology()
{
    Topology topo;
    wq_info_t* info = device_info;
    const wq_info_t* cend = device_info + thread_count();

    while (info != cend) {
        if (auto it = std::find_if(topo.nodes.begin(), topo.nodes.end(), [&](const auto& n) { return n.index == info->package_id; }); it != topo.nodes.end()) {
            append_topo_node(*it, info);
        } else {
            auto& node = topo.nodes.emplace_back();
            node.index = info->package_id;
            append_topo_node(node, info);
        }

        info++;
    }
    return topo;
}
} // end anonymous namespace

/// Update device_info and topology based on state of the WQs in the system. IDXD topology should be settled by now.
template <>
void setup_devices<WorkQueueSet>(const WorkQueueSet &enabled_devices)
{
    device_info = sApp->shmem->device_info;

    if (const char* mock_topo = getenv("SANDSTONE_MOCK_TOPOLOGY"); SandstoneConfig::Debug && mock_topo && *mock_topo) {
        create_mock_topology(mock_topo);
        return;
    }

    assert(enabled_devices.size() == thread_count());
    wq_info_t* info = device_info;
    [[maybe_unused]] const wq_info_t* cend = device_info + thread_count();

    AccfgCtx ctx;
    if (auto ret = ctx.init(); ret) {
        return;
    }
    int cnt = 0;
    for (const auto &enabled : enabled_devices) {
        const std::string dev_name = std::format("{}{}",
            enabled.dev_type == DEV_TYPE_DSA ? "dsa" : "iax",
            enabled.device_id);

        const std::string wq_name = std::format("wq{}.{}",
            enabled.device_id, enabled.wq_id);

        accfg_device* device = find_device(ctx.get(), dev_name);
        assert(device);
        if (!device) {
            continue; // TODO: this should not happen - we found it once already
        }

        if (accfg_device_get_state(device) != ACCFG_DEVICE_ENABLED)
            continue;

        accfg_wq* wq = find_wq(device, wq_name);
        assert(wq);
        if (!wq) {
            continue; // TODO: this should not happen - we found it once already
        }

        if (accfg_wq_get_state(wq) != ACCFG_WQ_ENABLED)
            continue;

        if (accfg_wq_get_type(wq) != ACCFG_WQT_USER)
            continue;

        info->dev_type   = enabled.dev_type;
        info->wq_id      = enabled.wq_id;
        info->device_id  = enabled.device_id;
        info->package_id = static_cast<int16_t>(accfg_device_get_numa_node(device));
        info->group_id   = accfg_wq_get_group_id(wq);
        info->mode       = accfg_wq_get_mode(wq) == ACCFG_WQ_SHARED
                               ? WQ_MODE_SHARED : WQ_MODE_DEDICATED;
        info->max_transfer_size = accfg_wq_get_max_transfer_size(wq);
        info->gen_cap           = accfg_device_get_gen_cap(device);

        info++;
        cnt++;
    }

    if (info != cend) {
        sApp->thread_count = cnt;
    }

    cached_topology() = build_topology();
}

void restrict_topology(DeviceRange range)
{
    assert(range.starting_device + range.device_count <= sApp->thread_count);
    auto old_wq_info = std::exchange(device_info, sApp->shmem->device_info + range.starting_device);
    int old_thread_count = std::exchange(sApp->thread_count, range.device_count);

    Topology &topo = cached_topology();
    if (old_wq_info != device_info || old_thread_count != sApp->thread_count) {
        topo = build_topology();
    }
}

void rebuild_topology()
{
    assert(device_info && sApp->thread_count && "device_info must be filled at this point");
    cached_topology() = build_topology();
}

void analyze_test_failures_for_topology(const struct test *test, const PerThreadFailures &per_thread_failures)
{
}

void slice_plan_init(int max_cores_per_slice)
{
    std::vector plan = { DeviceRange{ 0, thread_count() } };
    sApp->slice_plans.plans.fill(plan);
}
