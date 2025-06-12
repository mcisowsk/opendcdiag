/*
 * Copyright 2022 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INC_TOPOLOGY_H
#define INC_TOPOLOGY_H

#include "sandstone.h"
#include "devicedeps/devices.h"

#include <array>
#include <bit>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <functional>
#include <barrier>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "gettid.h"

class LogicalProcessorSet;

class CpuTopology
{
public:
    using Thread = struct cpu_info;
    struct Core {
        std::span<const Thread> threads;
    };
    struct Module {
        std::span<const Thread> threads;
    };

    struct CoreGrouping {
        std::vector<Core> cores;
        // std::vector<Module> modules;
    };

    struct NumaNode : CoreGrouping {
        int id() const
        { return cores.size() ? cores.front().threads.front().numa_id : -1; }
    };

    struct Package : CoreGrouping {
        std::vector<NumaNode> numa_domains;
        int id() const
        { return cores.size() ? cores.front().threads.front().package_id : -1; }
    };

    std::vector<Package> packages;

    CpuTopology(std::vector<Package> pkgs)
    {
        packages = std::move(pkgs);
    }

    bool isValid() const        { return !packages.empty(); }
    std::string build_falure_mask(const struct test *test) const;

    static const CpuTopology &topology();
    struct Data;
    Data clone() const;
};

using DeviceTopologyThread = CpuTopology::Thread;

struct CpuTopology::Data
{
    // this type is move-only (not copyable)
    Data() = default;
    Data(const Data &) = delete;
    Data(Data &&) = default;
    Data &operator=(const Data &) = delete;
    Data &operator=(Data &&) = default;

    std::vector<Package> packages;
    std::vector<CpuTopology::Thread> all_threads;
};

enum class LogicalProcessor : int {};

struct LogicalProcessorSetOps
{
    using Word = unsigned long long;
    static constexpr int ProcessorsPerWord = CHAR_BIT * sizeof(Word);

    static constexpr Word bitFor(LogicalProcessor n)
    {
        return 1ULL << (unsigned(n) % ProcessorsPerWord);
    }

    static void setInArray(std::span<Word> array, LogicalProcessor n)
    {
        wordForInArray(array, n) |= bitFor(n);
    }

    static Word &wordForInArray(std::span<Word> array, LogicalProcessor n)
    {
        return array[int(n) / ProcessorsPerWord];
    }

    static Word constWordForInArray(std::span<const Word> array, LogicalProcessor n)
    {
        int idx = int(n) / ProcessorsPerWord;
        return idx < array.size() ? array[idx] : 0;
    }
};

class LogicalProcessorSet : private LogicalProcessorSetOps
{
    // a possibly non-contiguous range
public:
    using LogicalProcessorSetOps::Word;
    using LogicalProcessorSetOps::ProcessorsPerWord;
    static constexpr int MinSize = 1024;
    std::vector<Word> array;

    LogicalProcessorSet() noexcept = default;
    LogicalProcessorSet(int minimumSize)
    {
        ensureSize(minimumSize - 1);
    }

    void clear()
    { *this = LogicalProcessorSet{}; }
    size_t size_bytes() const
    { return unsigned(array.size()) * sizeof(Word); }

    void set(LogicalProcessor n)
    { wordFor(n) |= bitFor(n); }
    void unset(LogicalProcessor n)
    { wordFor(n) &= ~bitFor(n); }
    bool is_set(LogicalProcessor n) const
    { return wordFor(n) & bitFor(n); }

    int count() const
    {
        int total = 0;
        for (const Word &w : array)
            total += std::popcount(w);
        return total;
    }

    bool empty() const
    {
        for (const Word &w: array)
            if (w)
                return false;
        return true;
    }

    void limit_to(int limit)
    {
        // find the first Word we need to change
        auto it = std::begin(array);
        for ( ; it != std::end(array) && limit > 0; ++it) {
            int n = std::popcount(*it);
            limit -= n;
        }

        if (limit < 0) {
            // clear enough upper bits on the last Word
            Word &x = it[-1];
            for ( ; limit < 0; ++limit) {
                Word bit = std::bit_floor(x);
                x &= ~bit;
            }
        }

        if (it != std::end(array))
            std::fill(it, std::end(array), 0);      // clear to the end
    }

private:
    void ensureSize(int n)
    {
        static_assert((MinSize % ProcessorsPerWord) == 0);
        static constexpr size_t MinSizeCount = MinSize / ProcessorsPerWord;
        size_t idx = size_t(n) / ProcessorsPerWord;
        if (idx >= array.size())
            array.resize(std::max(idx + 1, MinSizeCount));
    }
    Word &wordFor(LogicalProcessor n)
    {
        ensureSize(int(n));
        return wordForInArray(array, n);
    }
    Word wordFor(LogicalProcessor n) const noexcept
    {
        return constWordForInArray(array, n);
    }
};

LogicalProcessorSet ambient_logical_processor_set();
bool pin_to_logical_processor(LogicalProcessor, const char *thread_name = nullptr);
bool pin_thread_to_logical_processor(LogicalProcessor n, tid_t thread_id, const char *thread_name = nullptr);
bool pin_to_logical_processors(DeviceRange, const char *thread_name);

void slice_plan_init(int max_cores_per_slice);
void reschedule();

// TODO move to devices.h once implemented in gpu as well
void print_temperature_and_throttle();

class DeviceSchedule {
public:
    virtual void reschedule_to_next_device() = 0;
    virtual void finish_reschedule() = 0;
    virtual ~DeviceSchedule() = default;
protected:
    void pin_to_next_cpu(int next_cpu, tid_t thread_id = 0);
};

class BarrierDeviceSchedule : public DeviceSchedule
{
public:
    void reschedule_to_next_device() override;
    void finish_reschedule() override;

private:
    struct GroupInfo {
        std::barrier<std::function<void()>> *barrier;
        std::vector<pid_t> tid;     // Keep track of all members tid
        std::vector<int> next_cpu;  // Keep track of cpus on the group

        GroupInfo(int members_per_group, std::function<void()> on_completion)
        {
            barrier = new std::barrier<std::function<void()>>(members_per_group, std::move(on_completion));
            tid.resize(members_per_group);
            next_cpu.resize(members_per_group);
        }

        ~GroupInfo()
        {
            delete barrier;
        }
    };

    const int members_per_group = 2; // TODO: Make it configurable
    std::vector<GroupInfo> groups;
    std::mutex groups_mutex;
};

class QueueDeviceSchedule : public DeviceSchedule
{
public:
    void reschedule_to_next_device() override;
    void finish_reschedule() override {}

private:
    void shuffle_queue();

    int q_idx = 0;
    std::vector<int> queue;
    std::mutex q_mutex;
};

class RandomDeviceSchedule : public DeviceSchedule
{
public:
    void reschedule_to_next_device() override;
    void finish_reschedule() override {}
};

#endif /* INC_TOPOLOGY_H */
