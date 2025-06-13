/*Add commentMore actions
 * Copyright 2024 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sandstone.h>

#include <concepts>
#include <type_traits>

#ifdef __cpp_lib_format
#  include <format>
#endif

namespace SandstoneTest {
class Base;
template <typename T> concept TestClass =
        // must derive from Base
        std::derived_from<T, Base> &&
        // must be default constructible or constructible from `struct test *`
        (std::is_default_constructible_v<T> || std::is_constructible_v<T, struct test *>) &&
        // must provide a const char *description member
        requires { static_cast<const char *>(T::description); } &&
        // must provide a quality level
        std::is_enum_v<decltype(T::quality_level)>;

class Base
{
public:
    struct Device {
        // generic, no information
        using TestClass = Base;
        int id;
    };
    struct Parameters {
        // common
        /*std::chrono::milliseconds*/ int desired_duration;
        int fracture_loop_count;

        // cpu part
        decltype(_compilerCpuFeatures) minimum_cpu;

        // gpu part

    };

    enum class TestQuality {
        Skipped,
        Beta,
        Production,
    };

    // defaults:
    static constexpr struct test_group* const* groups = nullptr;

protected:
    template <int N, typename Lambda> void test_loop(Lambda &&l)
    {
        static_assert(N > 0, "N must be positive");     \
        test_loop_start();                              \
        do {
            for (int i = 0; i < N; ++i)
                l();
        } while (_internal_loop_continue());
        test_loop_end();
    }

    struct Failed : std::exception {
        const char* msg;

        Failed(const char* msg) : msg{msg} {};

        const char *what() const noexcept override
        {
            return msg;
        }
    };

    struct Skipped : std::exception {
        const char* msg;

        Skipped(const char* msg) : msg{msg} {};

        const char *what() const noexcept override
        {
            return msg;
        }
    };

    bool _internal_loop_continue() noexcept {
        return test_time_condition();
    }

    template <TestClass T>
    static constexpr void _apply_parameters_base(struct test *test/*, const Parameters &params*/)
    {
        // handles only base parameters
        if constexpr (requires { T::parameters; }) {
            test->desired_duration = T::parameters.desired_duration;
            test->fracture_loop_count = T::parameters.fracture_loop_count;
        }
    }

    template <TestClass T> struct CallbackAdapter
    {
        static Base *factory(struct test *test)
        {
            if constexpr (std::is_constructible_v<T, struct test *>) {
                return new T(test);
            } else {
                return new T;
            }
        }

        static int init(struct test *test)
        {
            Base* this_test;
            try {
                this_test = factory(test); // can throw but why would it
            } catch (Skipped &e) {
                log_skip(RuntimeSkipCategory, "%s", e.what());
                return EXIT_SKIP;
            } catch (Failed &e) {
                log_error("%s", e.what());
                return EXIT_FAILURE;
            }
            test->data = this_test;
            if constexpr (requires { static_cast<T*>(this_test)->init(test); }) {
                return static_cast<T*>(this_test)->init(test);
            } else if constexpr (requires { static_cast<T*>(this_test)->init(); }) {
                return static_cast<T*>(this_test)->init();
            } else {
                return EXIT_SUCCESS;
            }
        }

        static int cleanup(struct test *test)
        {
            T *this_test = static_cast<T *>(test->data);
            auto ret = EXIT_SUCCESS;
            if constexpr (requires { this_test->cleanup(test); }) {
                ret = this_test->cleanup(test);
            } else if constexpr (requires { this_test->cleanup(); }) {
                ret = this_test->cleanup();
            }
            delete this_test;
            return ret;
        }

        static int run(struct test *test, int device_id)
        {
            T* this_test = static_cast<T*>(test->data);
            Device this_device{device_id}; //??
            if constexpr (requires { this_test->run(test, this_device); }) {
                return this_test->run(test, this_device);
            } else if constexpr (requires { this_test->run(this_device); }) {
                return this_test->run(this_device);
            } else { // We expect that any run() definition would exist
                return this_test->run();
            }
        }
    };

public:
    template <TestClass T> static consteval struct test createTestClass(const char *id);
};

template <TestClass T> consteval struct test Base::createTestClass(const char *id)
{
    struct test res = {};
    res.id = id;
    res.description = T::description;
    res.groups = T::groups;
    res.test_preinit = nullptr;
    res.test_init = &CallbackAdapter<T>::init;
    res.test_cleanup = &CallbackAdapter<T>::cleanup;
    res.test_run = &CallbackAdapter<T>::run;
    if (T::quality_level == Base::TestQuality::Skipped)
        res.quality_level = TEST_QUALITY_SKIP;
    else if (T::quality_level == Base::TestQuality::Beta)
        res.quality_level = TEST_QUALITY_BETA;
    else
        res.quality_level = TEST_QUALITY_PROD;

    T::template _apply_parameters<T>(&res);
    return res;
}

} // namespace SandstoneTest

#ifndef SANDSTONE_TEST_STRINGIFY
#  define SANDSTONE_TEST_STRINGIFY(x)       SANDSTONE_STRINGIFY(x)
#endif

#define DECLARE_TEST_CLASS(test_id, ...)            \
    __attribute__((aligned(alignof(void*)), used, section(SANDSTONE_SECTION_PREFIX "tests"))) \
    constinit struct test _test_ ## test_id =       \
        SandstoneTest::Base::createTestClass<__VA_ARGS__>(SANDSTONE_TEST_STRINGIFY(test_id))
