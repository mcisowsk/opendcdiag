/*
 * Copyright 2024 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SANDSTONE_OPTS
#define SANDSTONE_OPTS

#include "sandstone_tests.h"

struct ParsedOpts {
    const char* seed = nullptr;
    int max_cores_per_slice = 0;
    int thread_count = -1;
    bool fatal_errors = false;
    const char* on_hang_arg = nullptr;
    const char* on_crash_arg = nullptr;

    // test selection
    std::vector<const char *> enabled_tests;
    std::vector<const char *> disabled_tests;
    const char *test_list_file_path = nullptr;

    struct test_set_cfg test_set_config = {
        .ignore_unknown_tests = false,
        .randomize = false,
        .cycle_through = false,
    };
    const char *builtin_test_list_name = nullptr;
    int starting_test_number = 1;  // One based count for user interface, not zero based
    int ending_test_number = INT_MAX;
};

int parse_cmdline(int argc, char** argv, SandstoneApplication* app, ParsedOpts& opts);

#endif /* SANDSTONE_OPTS */
