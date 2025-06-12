/*Add commentMore actions
 * Copyright 2024 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "devicedeps/test_base.hpp"

namespace SandstoneTest {

class Cpu : public Base
{
    friend class Base;
    template <TestClass T>
    static constexpr void _apply_parameters(struct test *test)
    {
        Base::_apply_parameters_base<T>(test);
        test->compiler_minimum_cpu = _compilerCpuFeatures;
        if constexpr (requires { T::parameters; }) {
            test->minimum_cpu = T::parameters.minimum_cpu;
        }
    }
};

} // namespace SandstoneTest
