/*
 * Copyright 2026 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INC_TOPOLOGY_IDXD_HPP
#define INC_TOPOLOGY_IDXD_HPP

#include "topology.h"
#include "idxd_device.h"

#include <vector>


/// Immutable id of a queue: device_id and wq_id, as in qw<device_id>.<wq_id>.
struct WorkQueueId
{
    // int node_id;
    int device_id;
    // int group_id;
    int wq_id;

    dev_type_t dev_type;
    // wq_mode_t mode;
    // uint64_t max_transfer_size;
    // uint64_t gen_cap;
};

using WorkQueueSet = std::vector<WorkQueueId>;
using EnabledDevices = WorkQueueSet;

class Topology
{
public:
    using Thread = struct wq_info_t;

    struct Engine
    {
        // char* name; // TODO: std::string?
        int index;
    };

    struct Group
    {
        // char* name;
        int index;

        std::vector<const Thread*> wqs;
        std::vector<Engine> engines; // TODO: I wonder if num_engines would suffice, since we cannot target them.
    };

    struct Device
    {
        // char *name;
        int index;

        //// TODO: where to keep them? In device_info? Or in topo? In device_info some would be duplicated (gen_cap, bdf)
        // int numa_node;
        // char *bdf;
        // uint64_t gen_cap;
        // idxd_op_cap_t op_cap;

        std::vector<Group> groups;
    };

    struct Node // a.k.a socket
    {
        int index;
        // TODO:
        // Decide:
        //   - std::vector<std::variant<DsaDevice, IaxDevice>>
        //   - std::vector<DsaDevice> and std::vector<IaxDevice>
        //   - one std::vector<Device> and a type member to Device?
        std::vector<Device> devices;
    };

    std::vector<Node> nodes;

    static const Topology &topology();
};

struct HardwareInfo
{};

#endif // INC_TOPOLOGY_IDXD_HPP
