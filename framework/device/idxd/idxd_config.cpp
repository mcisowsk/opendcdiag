/*
 * Copyright 2026 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "idxd_config.hpp"
#include "idxd_utils.hpp"

#include <accel-config/libaccel_config.h>

#include <nlohmann/json.hpp>

#include <fstream>

namespace {
using json = nlohmann::json;

#define CHECK_IDXD(field, ...) \
    do { \
        auto rc = (__VA_ARGS__); \
        if (rc < 0) { \
            fprintf(stderr, "Failed to set %s on %s\n", field, dev_name.c_str()); \
            return EXIT_FAILURE; \
        } \
    } while (0)

#define EXPECT_STRING(it, key) \
    if (!it->is_string()) { \
        fprintf(stderr, "Configuring %s error: `%s` is not a string\n", dev_name.c_str(), key); \
        return EXIT_FAILURE; \
    }

#define EXPECT_INT(it, key) \
    if (!it->is_number_integer()) { \
        fprintf(stderr, "Configuring %s error: `%s` is not an int\n", dev_name.c_str(), key); \
        return EXIT_FAILURE; \
    }

#define EXPECT_UINT(it, key) \
    if (!it->is_number_unsigned()) { \
        fprintf(stderr, "Configuring %s error: `%s` is not an unsigned int\n", dev_name.c_str(), key); \
        return EXIT_FAILURE; \
    }

#define EXPECT_ARRAY(it, key) \
    if (!it->is_array()) { \
        fprintf(stderr, "Configuring %s error: `%s` is not an array\n", dev_name.c_str(), key); \
        return EXIT_FAILURE; \
    }

accfg_group* find_group(accfg_device* device, const std::string& name)
{
    accfg_group* group;
    accfg_group_foreach(device, group) {
        if (name == accfg_group_get_devname(group)) {
            return group;
        }
    }
    fprintf(stderr, "Group not found: %s\n", name.c_str());
    return nullptr;
}

accfg_engine* find_engine(accfg_device* device, const std::string& name)
{
    accfg_engine* engine;
    accfg_engine_foreach(device, engine) {
        if (name == accfg_engine_get_devname(engine)) {
            return engine;
        }
    }
    fprintf(stderr, "Engine not found: %s\n", name.c_str());
    return nullptr;
}

int configure_engine(accfg_device* device, const json& j_engine)
{
    std::string dev_name;
    {
        auto it = j_engine.find("dev");
        if (it == j_engine.end()) {
            fprintf(stderr, "No `dev` key for engine found\n");
            return EXIT_FAILURE;
        }
        dev_name = it->get<std::string>();
    }
    accfg_engine* engine = find_engine(device, dev_name);
    if (!engine) {
        return EXIT_FAILURE;
    }

    if (auto it = j_engine.find("group_id"); it != j_engine.end()) {
        EXPECT_INT(it, "group_id");
        if (accfg_engine_set_group_id(engine, it->get<int>()) < 0) {
            fprintf(stderr, "Failed to set group for engine: %s", dev_name.c_str());
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int configure_wq(accfg_device* device, const json& j_wq)
{
    std::string dev_name;
    {
        auto it = j_wq.find("dev");
        if (it == j_wq.end()) {
            fprintf(stderr, "No `dev` key for wq found\n");
            return EXIT_FAILURE;
        }
        dev_name = it->get<std::string>();
    }
    accfg_wq* wq = find_wq(device, dev_name);
    if (!wq) {
        return EXIT_FAILURE;
    }

    if (auto it = j_wq.find("mode"); it != j_wq.end()) {
        EXPECT_STRING(it, "mode");
        auto mode_str = it->get<std::string>();
        accfg_wq_mode mode;
        if (mode_str == "shared") {
            mode = ACCFG_WQ_SHARED;
        } else if(mode_str == "dedicated") {
            mode = ACCFG_WQ_DEDICATED;
        } else {
            fprintf(stderr, "Unknown WQ mode: %s\n", mode_str.c_str());
            return EXIT_FAILURE;
        }
        CHECK_IDXD("mode", accfg_wq_set_mode(wq, mode));
    }

    if (auto it = j_wq.find("group_id"); it != j_wq.end()) {
        EXPECT_INT(it, "group_id");
        CHECK_IDXD("group_id", accfg_wq_set_group_id(wq, it->get<int>()));
    }

    if (auto it = j_wq.find("priority"); it != j_wq.end()) {
        EXPECT_INT(it, "priority");
        CHECK_IDXD("priority", accfg_wq_set_priority(wq, it->get<int>()));
    }

    if (auto it = j_wq.find("size"); it != j_wq.end()) {
        EXPECT_INT(it, "size");
        CHECK_IDXD("size", accfg_wq_set_size(wq, it->get<int>()));
    }

    if (auto it = j_wq.find("threshold"); it != j_wq.end()) {
        EXPECT_INT(it, "threshold");
        CHECK_IDXD("threshold", accfg_wq_set_threshold(wq, it->get<int>()));
    }

    if (auto it = j_wq.find("max_batch_size"); it != j_wq.end()) {
        EXPECT_INT(it, "max_batch_size");
        CHECK_IDXD("max_batch_size", accfg_wq_set_max_batch_size(wq, it->get<int>()));
    }

    if (auto it = j_wq.find("max_transfer_size"); it != j_wq.end()) {
        EXPECT_UINT(it, "max_transfer_size");
        CHECK_IDXD("max_transfer_size", accfg_wq_set_max_transfer_size(wq, it->get<uint64_t>()));
    }

    if (auto it = j_wq.find("block_on_fault"); it != j_wq.end()) {
        EXPECT_INT(it, "block_on_fault");
        CHECK_IDXD("block_on_fault", accfg_wq_set_block_on_fault(wq, it->get<int>()));
    }

    if (auto it = j_wq.find("ats_disable"); it != j_wq.end() && accfg_wq_get_ats_disable(wq) != -ENOENT) {
        EXPECT_INT(it, "ats_disable");
        CHECK_IDXD("ats_disable", accfg_wq_set_ats_disable(wq, it->get<int>()));
    }

    if (auto it = j_wq.find("prs_disable"); it != j_wq.end() && accfg_wq_get_prs_disable(wq) != -ENOENT) {
        EXPECT_INT(it, "prs_disable");
        CHECK_IDXD("prs_disable", accfg_wq_set_prs_disable(wq, it->get<int>()));
    }

    if (auto it = j_wq.find("driver_name"); it != j_wq.end()) {
        EXPECT_STRING(it, "driver_name");
        CHECK_IDXD("driver_name", accfg_wq_set_str_driver_name(wq, it->get<std::string>().c_str()));
    }

    if (auto it = j_wq.find("type"); it != j_wq.end()) {
        EXPECT_STRING(it, "type");
        printf("type=%s\n", it->get<std::string>().c_str());
        CHECK_IDXD("type", accfg_wq_set_str_type(wq, it->get<std::string>().c_str()));
    }

    if (auto it = j_wq.find("op_config"); it != j_wq.end()) {
        EXPECT_STRING(it, "op_config");
        CHECK_IDXD("op_config", accfg_wq_set_op_config_str(wq, it->get<std::string>().c_str()));
    }

    if (auto it = j_wq.find("name"); it != j_wq.end()) {
        EXPECT_STRING(it, "name");
        printf("name=%s\n", it->get<std::string>().c_str());
        CHECK_IDXD("name", accfg_wq_set_str_name(wq, it->get<std::string>().c_str()));
    }

    return EXIT_SUCCESS;
}

int enable_wq_if_disabled(accfg_wq* wq)
{
    if (accfg_wq_get_state(wq) == ACCFG_WQ_ENABLED) {
        return EXIT_SUCCESS;
    }

    if (auto rc = accfg_wq_enable(wq); rc < 0 || accfg_wq_get_state(wq) != ACCFG_WQ_ENABLED) {
        fprintf(stderr, "Failed to enable WQ: %s\n", accfg_wq_get_devname(wq));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int disable_wq_if_enabled(accfg_wq* wq)
{
    if (accfg_wq_get_state(wq) == ACCFG_WQ_DISABLED) {
        return EXIT_SUCCESS;
    }

    if (auto rc = accfg_wq_disable(wq, true); rc < 0 || accfg_wq_get_state(wq) != ACCFG_WQ_DISABLED) {
        fprintf(stderr, "Failed to disable WQ: %s\n", accfg_wq_get_devname(wq));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int configure_group(accfg_device* device, const json& j_group)
{
    std::string dev_name;
    {
        auto it = j_group.find("dev");
        if (it == j_group.end()) {
            fprintf(stderr, "No `dev` key for group found\n");
            return EXIT_FAILURE;
        }
        dev_name = it->get<std::string>();
    }
    accfg_group* group = find_group(device, dev_name);
    if (!group) {
        return EXIT_FAILURE;
    }

    if (auto it = j_group.find("read_buffers_reserved"); it != j_group.end()) {
        EXPECT_INT(it, "read_buffers_reserved");
        CHECK_IDXD("read_buffers_reserved", accfg_group_set_read_buffers_reserved(group, it->get<int>()));
    }

    bool use_read_buffer_limit = false;
    if (auto it = j_group.find("use_read_buffer_limit"); it != j_group.end()) {
        EXPECT_INT(it, "use_read_buffer_limit");
        use_read_buffer_limit = it->get<int>();
        CHECK_IDXD("use_read_buffer_limit", accfg_group_set_use_read_buffer_limit(group, it->get<int>()));
    }

    if (auto it = j_group.find("read_buffers_allowed"); it != j_group.end() && use_read_buffer_limit) {
        EXPECT_INT(it, "read_buffers_allowed");
        CHECK_IDXD("read_buffers_allowed", accfg_group_set_read_buffers_allowed(group, it->get<int>()));
    } else {
        // TODO: set to something sane as otherwise it's defaulted to max_read_buffers, and drievr then fails at enable time.
        CHECK_IDXD("read_buffers_allowed", accfg_group_set_read_buffers_allowed(group, 8));
    }

    if (accfg_device_get_version(device) >= ACCFG_DEVICE_VERSION_2) {
        if (auto it = j_group.find("traffic_class_a"); it != j_group.end()) {
            EXPECT_INT(it, "traffic_class_a");
            CHECK_IDXD("traffic_class_a", accfg_group_set_traffic_class_a(group, it->get<int>()));
        }

        if (auto it = j_group.find("traffic_class_b"); it != j_group.end()) {
            EXPECT_INT(it, "traffic_class_b");
            CHECK_IDXD("traffic_class_b", accfg_group_set_traffic_class_b(group, it->get<int>()));
        }
    }

    if (auto it = j_group.find("desc_progress_limit"); it != j_group.end()) {
        EXPECT_INT(it, "desc_progress_limit");
        CHECK_IDXD("desc_progress_limit", accfg_group_set_desc_progress_limit(group, it->get<int>()));
    }

    if (auto it = j_group.find("batch_progress_limit"); it != j_group.end()) {
        EXPECT_INT(it, "batch_progress_limit");
        CHECK_IDXD("batch_progress_limit", accfg_group_set_batch_progress_limit(group, it->get<int>()));
    }

    // engines first, then WQs
    if (auto it = j_group.find("grouped_engines"); it != j_group.end()) {
        EXPECT_ARRAY(it, "grouped_engines");
        for (const auto& j_engine : *it) {
            if (auto ret = configure_engine(device, j_engine); ret != EXIT_SUCCESS) {
                return ret;
            }
        }
    }

    if (auto it = j_group.find("grouped_workqueues"); it != j_group.end()) {
        EXPECT_ARRAY(it, "grouped_workqueues");
        for (const auto& j_wq : *it) {
            if (auto ret = configure_wq(device, j_wq); ret != EXIT_SUCCESS) {
                return ret;
            }
        }
    }

    return EXIT_SUCCESS;
}

int configure_device(accfg_ctx* ctx, const json& j_dev)
{
    std::string dev_name;
    {
        auto it = j_dev.find("dev");
        if (it == j_dev.end()) {
            fprintf(stderr, "No `dev` key for device found\n");
            return EXIT_FAILURE;
        }
        // EXPECT_STRING(it); // TODO: we could use try catch and remove all the type validation?
        dev_name = it->get<std::string>();
    }
    accfg_device* device = find_device(ctx, dev_name);
    if (!device) {
        return EXIT_FAILURE;
    }

    // Most device-level attributes are writable only when the device and WQs are disabled.
    {
        accfg_wq* wq;
        accfg_wq_foreach(device, wq) {
            if (auto ret = disable_wq_if_enabled(wq); ret != EXIT_SUCCESS) {
                return ret;
            }
        }
    }
    if (accfg_device_get_state(device) == ACCFG_DEVICE_ENABLED) {
        if (accfg_device_disable(device, true) < 0) {
            fprintf(stderr, "Failed to disable device: %s\n", dev_name.c_str());
            return EXIT_FAILURE;
        }
    }

    if (auto it = j_dev.find("read_buffer_limit"); it != j_dev.end()) {
        EXPECT_INT(it, "read_buffer_limit");
        CHECK_IDXD("read_buffer_limit", accfg_device_set_read_buffer_limit(device, it->get<int>()));
    }

    if (auto it = j_dev.find("event_log_size"); it != j_dev.end()) {
        EXPECT_INT(it, "event_log_size");
        CHECK_IDXD("event_log_size", accfg_device_set_event_log_size(device, it->get<int>()));
    }

    // Configure groups (which configure engines + WQs inside)
    if (auto it = j_dev.find("groups"); it != j_dev.end()) {
        EXPECT_ARRAY(it, "groups");
        for (const auto& j_group : *it) {
            if (auto ret = configure_group(device, j_group); ret != EXIT_SUCCESS) {
                return ret;
            }
        }
    }

    // Enable device last
    if (auto rc = accfg_device_enable(device); rc < 0) {
        fprintf(stderr, "Failed to enable device\n");
        return EXIT_FAILURE;
    }

    // WQs can be enabled only after the device is enabled.
    if (auto it = j_dev.find("groups"); it != j_dev.end()) {
        for (const auto& j_group : *it) {
            auto it_group = j_group.find("grouped_workqueues");
            if (it_group == j_group.end()) {
                continue;
            }
            // sanity check as asserts - we already checked all that in configure_group
            assert(it_group->is_array());
            for (const auto& j_wq : *it_group) {
                assert(j_wq.contains("dev"));
                const std::string wq_dev_name = j_wq["dev"].get<std::string>();
                accfg_wq* wq = find_wq(device, wq_dev_name);
                assert(wq);
                if (auto ret = enable_wq_if_disabled(wq); ret != EXIT_SUCCESS) {
                    return ret;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

json snapshot_wq(accfg_wq* wq)
{
    json j;

    // TODO: do not save "empty" properties?
    j["dev"]               = accfg_wq_get_devname(wq);
    j["mode"]              = accfg_wq_get_mode(wq) == ACCFG_WQ_SHARED ? "shared" : "dedicated";
    j["size"]              = accfg_wq_get_size(wq);
    j["group_id"]          = accfg_wq_get_group_id(wq);
    j["priority"]          = accfg_wq_get_priority(wq);
    j["block_on_fault"]    = accfg_wq_get_block_on_fault(wq);
    j["max_batch_size"]    = accfg_wq_get_max_batch_size(wq);
    j["max_transfer_size"] = accfg_wq_get_max_transfer_size(wq);
    j["name"]              = accfg_wq_get_type_name(wq); // watch out for this confusing name
    j["type"]              = accfg_wq_get_type(wq) == ACCFG_WQT_USER ? "user" : "kernel"; // TODO: can be unknown as well TODO2: maybe we should discard anything non-user?
    j["driver_name"]       = accfg_wq_get_driver_name(wq);

    // threshold is optional — only present on some WQs
    int threshold = accfg_wq_get_threshold(wq);
    if (threshold > 0) {
        j["threshold"] = threshold;
    }

    if (int ats = accfg_wq_get_ats_disable(wq); ats != -ENOENT) {
        j["ats_disable"] = ats;
    }
    if (int prs = accfg_wq_get_prs_disable(wq); prs != -ENOENT) {
        j["prs_disable"] = prs;
    }

    return j;
}

json snapshot_engine(accfg_engine* engine)
{
    return {
        {"dev",      accfg_engine_get_devname(engine)},
        {"group_id", accfg_engine_get_group_id(engine)},
    };
}

json snapshot_group(accfg_device* device, accfg_group* group)
{
    json j;

    j["dev"]                   = accfg_group_get_devname(group);
    j["read_buffers_reserved"] = accfg_group_get_read_buffers_reserved(group);
    j["use_read_buffer_limit"] = accfg_group_get_use_read_buffer_limit(group);
    j["read_buffers_allowed"]  = accfg_group_get_read_buffers_allowed(group);

    auto version = accfg_device_get_version(device);
    if (version >= ACCFG_DEVICE_VERSION_2) {
        int tc_a = accfg_group_get_traffic_class_a(group);
        int tc_b = accfg_group_get_traffic_class_b(group);
        if (tc_a >= 0) j["traffic_class_a"] = tc_a;
        if (tc_b >= 0) j["traffic_class_b"] = tc_b;
    }

    int dpl = accfg_group_get_desc_progress_limit(group);
    int bpl = accfg_group_get_batch_progress_limit(group);
    if (dpl > 0) j["desc_progress_limit"]  = dpl;
    if (bpl > 0) j["batch_progress_limit"] = bpl;

    // only emit if this group actually has engines/wqs assigned
    json engines = json::array();
    accfg_engine* engine;
    accfg_engine_foreach(device, engine) {
        if (accfg_engine_get_group_id(engine) == accfg_group_get_id(group)) {
            engines.push_back(snapshot_engine(engine));
        }
    }
    if (!engines.empty()) {
        j["grouped_engines"] = engines;
    }

    json wqs = json::array();
    accfg_wq* wq;
    accfg_wq_foreach(device, wq) {
        if (accfg_wq_get_group_id(wq) == accfg_group_get_id(group)) {
            wqs.push_back(snapshot_wq(wq));
        }
    }
    if (!wqs.empty()) {
        j["grouped_workqueues"] = wqs;
    }

    return j;
}

json snapshot_device(accfg_ctx* ctx, accfg_device* device)
{
    json j;

    j["dev"]               = accfg_device_get_devname(device);
    j["read_buffer_limit"] = accfg_device_get_read_buffer_limit(device);
    j["event_log_size"]    = accfg_device_get_event_log_size(device);

    json groups = json::array();
    accfg_group* group;
    accfg_group_foreach(device, group) {
        groups.push_back(snapshot_group(device, group));
    }
    j["groups"] = groups;

    return j;
}

} // end anonymous namespace

int apply_idxd_config(const char* json_path)
{
    std::ifstream f(json_path);
    auto config = nlohmann::json::parse(f, nullptr, false);
    if (config.is_discarded()) {
        fprintf(stderr, "Config parse error\n"); // TODO: throwing an exception is more informative
        return EXIT_FAILURE;
    }

    if (!config.is_array()) {
        fprintf(stderr, "Config parse error: is not an array\n");
        return EXIT_FAILURE;
    }

    AccfgCtx ctx;
    if (auto ret = ctx.init(); ret) {
        return ret;
    }

    // take snapshots for all devices in the current config before touching anything
    std::vector<json> backups;
    accfg_device* device;
    accfg_device_foreach(ctx.get(), device) {
        backups.push_back(snapshot_device(ctx.get(), device));
    }

    bool do_rollback = false;
    for (const auto& j_dev : config) {
        if (auto ret = configure_device(ctx.get(), j_dev); ret != EXIT_SUCCESS) {
            do_rollback = true;
            break;
        }
    }

    if (do_rollback) {
        fprintf(stderr, "Config apply error, rolling back previous config\n");
        for (const auto& j_dev : backups) {
            configure_device(ctx.get(), j_dev); // discard errors?
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
