# Copyright 2026 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

# uuid-dev, libaccel-config-dev, nlohmann-json3-dev
accel_config_dep = dependency('libaccel-config', required: true)
nlohmann_json_dep = dependency('nlohmann_json', required: true)

target_deps += [
    accel_config_dep,
    nlohmann_json_dep,
]

framework_files += files(
    'idxd_config.cpp',
    'idxd_device.cpp',
    'topology.cpp',
    'logging_idxd.cpp',
)

