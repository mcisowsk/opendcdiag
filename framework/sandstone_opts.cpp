/*
 * Copyright 2024 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sandstone_p.h"
#include "sandstone_opts.hpp"
#include "sandstone_utils.h"

#include <cinttypes>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace std::chrono;

using namespace std::chrono_literals;

namespace {
enum {
    invalid_option = 128,         /* not used, here just so the next option is non-zero */

    one_sec_option,
    thirty_sec_option,
    two_min_option,
    five_min_option,

    cpuset_option,
    disable_option,
    dump_cpu_info_option,
    fatal_skips_option,
    gdb_server_option,
    ignore_os_errors_option,
    ignore_unknown_tests_option,
    is_asan_option,
    is_debug_option,
    force_test_time_option,
    test_knob_option,
    longer_runtime_option,
    max_concurrent_threads_option,
    max_cores_per_slice_option,
    max_test_count_option,
    max_test_loop_count_option,
    max_messages_option,
    max_logdata_option,
    mce_check_period_option,
    mem_sample_time_option,
    mem_samples_per_log_option,
    no_mem_sampling_option,
    no_slicing_option,
    no_triage_option,
    on_crash_option,
    on_hang_option,
    output_format_option,
    quality_option,
    quick_run_option,
    raw_list_tests,
    raw_list_group_members,
    raw_list_groups,
    retest_on_failure_option,
    schedule_by_option,
#ifndef NO_SELF_TESTS
    selftest_option,
#endif
    service_option,
    shortened_runtime_option,
    strict_runtime_option,
    syslog_runtime_option,
    temperature_threshold_option,
    test_delay_option,
    test_index_range_option,
    test_list_file_option,
    test_list_randomize_option,
    test_tests_option,
    timeout_option,
    total_retest_on_failure,
    triage_option,
    ud_on_failure_option,
    use_builtin_test_list_option,
    vary_frequency,
    vary_uncore_frequency,
    version_option,
    weighted_testrun_option,
    alpha_option,
    beta_option,
};

void suggest_help(char **argv) {
    printf("Try '%s --help' for more information.\n", argv[0]);
}

void usage(char **argv)
{
    static const char usageText[] = R"(%s [options]
Common command-line options are:
 -F, --fatal-errors
     Stop execution after first failure; do not continue to run tests.
 -T <time>, --total-time=<time>
     Specify the minimum run time for the program.  A special value for <time>
     of "forever" causes the program to loop indefinitely.  The defaults for <time>
     is milliseconds, with s, m, and h available for seconds, minutes or hours.
     Example: sandstone -T 60s     # run for at least 60 seconds.
     Example: sandstone -T 5000    # run for at least 5,000 milliseconds
 --strict-runtime
     Use in conjunction with -T to force the program to stop execution after the
     specific time has elapsed.
 -t <test-time>
     Specify the execution time per test for the program in ms.
     Value for this field can also be specified with a label s, m, h for seconds,
     minutes or hours.  Example: 200ms, 2s or 2m
 --max-test-count <NUMBER>
     Specify the maximum number of tests you want to execute.  Allows you
     to run at most <NUMBER> tests in a program execution.
 --max-test-loop-count <NUMBER>
     When this option is present, test execution will be limited by the number
     of times the test executes its main execution loop. This option augments
     the time-based options in that the test will end if either the test time
     condition is exceeded, or the test-max-loop-count is exhausted.  The use
     of --max-test-loop-count disables test fracturing, the default mode of
     test execution in which individual tests are run multiple times with
     different random number seeds during the same invocation of opendcdiag.
     A value of 0 for --max-test-loop-count is interpreted as there being no
     limit to the number of loop iterations.  This special value can be
     used to disable test fracturing.  When specified tests will not be
     fractured and their execution will be time limited.
 --cpuset=<set>
     Selects the CPUs to run tests on. The <set> option may be a comma-separated
     list of either plain numbers that select based on the system's logical
     processor number, or a letter  followed by a number to select based on
     topology: p for package, c for core and t for thread.
 --dump-cpu-info
     Prints the CPU information that the tool detects (package ID, core ID,
     thread ID, microcode, and PPIN) then exit.
 -e <test>, --enable=<test>, --disable=<test>
     Selectively enable/disable a given test. Can be given multiple times.
     <test> is a test's ID (see the -l option), a wildcard matching test IDs.
     or a test group (starting with @).
 --ignore-os-error, --ignore-timeout
     Continue execution of Sandstone even if a test encounters an operating
     system error (this includes tests timing out).
 --ignore-unknown-tests
     Ignore unknown tests listed on --enable and --disable.
 -h, --help
     Print help.
 -l, --list
     Lists the tests and groups, with their descriptions, and exits.
 --list-tests
     Lists the test names.
 --list-groups
     Lists the test groups.
 --max-messages <NUMBER>
     Limits the maximum number of log messages that can be output by a single
     thread per test invocation.  A value of less than or equal to 0 means
     that there is no limit.  The default value is 5.
 --max-logdata <NUMBER>
     Limits the maximum number of bytes of binary data that can be logged
     by a single thread per test invocation.  A value of less than or equal
     to 0 means that there is no limit.  The default value is 128.
     Sandstone will not log partial data, so if the binary data would cause
     the thread to exceed this threshold it simply will not be output.
 -n <NUMBER>, --threads=<NUMBER>
     Set the number of threads to be run to <NUMBER>. If not specified or if
     0 is passed, then the test defaults to the number of CPUs in the system.
     Note the --cpuset and this parameter do not behave well together.
 -o, --output-log <FILE>
     Place all logging information in <FILE>.  By default, a file name is
     auto-generated by the program.  Use -o /dev/null to suppress creation of any file.
 -s <STATE>, --rng-state=<STATE>
     Specify the random generator state to reload. The seed is in the form:
       Engine:engine-specific-data
 -v, -q, --verbose, --quiet
     Set logging output verbosity level.  Default is quiet.
 --version
     Display program version information.
 --1sec, --30sec, --2min, --5min
     Run for the specified amount of time in the option. In this mode, the program
     prioritizes test execution based on prior detections.
     These options are intended to drive coverage over multiple runs.
     Test priority is ignored when running in combination with the
     --test-list-file option.
 --test-list-file <file path>
     Specifies the tests to run in a text file.  This will run the tests
     in the order they appear in the file and also allows you to vary the
     individual test durations.  See the User Guide for details.
 --test-range A-B
     Run tests from test number A to test number B based on their list location
     in an input file specified using --test-list-file <inputfile>.
     For example: --test-list-file mytests.list -test-range 6-10
                  runs tests 6 through 10 from the file mytests.list.
     See User Guide for more details.
 --test-list-randomize
     Randomizes the order in which tests are executed.
 --test-delay <time in ms>
     Delay between individual test executions in milliseconds.
  -Y, --yaml [<indentation>]
     Use YAML for logging. The optional argument is the number of spaces to
     indent each line by (defaults to 0).
For more options and information, please see the User Reference
Guide.
)";

    static const char restrictedUsageText[] = R"(%s [options]
Available command-line options are:
 -h, --help         Print help.
 -q, --query        Reports whether a scan service found an issue and exits.
 -s, --service      Run as a slow scan service.
     --version      Display version number.
)";

    printf(SandstoneConfig::RestrictedCommandLine ? restrictedUsageText : usageText, argv[0]);
}

void dump_cpu_info()
{
    int i;

    // find the best matching CPU
    const char *detected = "<unknown>";
    for (const auto &arch : x86_architectures) {
        if ((arch.features & cpu_features) == arch.features) {
            detected = arch.name;
            break;
        }
        if (sApp->shmem->verbosity > 1)
            printf("CPU is not %s: missing %s\n", arch.name,
                   cpu_features_to_string(arch.features & ~cpu_features).c_str());
    }
    printf("Detected CPU: %s; family-model-stepping (hex): %02x-%02x-%02x; CPU features: %s\n",
           detected, cpu_info[0].family, cpu_info[0].model, cpu_info[0].stepping,
           cpu_features_to_string(cpu_features).c_str());
    printf("# CPU\tPkgID\tCoreID\tThrdID\tMicrocode\tPPIN\n");
    for (i = 0; i < num_cpus(); ++i) {
        printf("%d\t%d\t%d\t%d\t0x%" PRIx64, cpu_info[i].cpu_number,
               cpu_info[i].package_id, cpu_info[i].core_id, cpu_info[i].thread_id,
               cpu_info[i].microcode);
        if (cpu_info[i].ppin)
            printf("\t%016" PRIx64, cpu_info[i].ppin);
        puts("");
    }
}

auto collate_test_groups(SandstoneTestSet& test_set)
{
    struct Group {
        const struct test_group *definition = nullptr;
        std::vector<const struct test *> entries;
    };
    std::map<std::string_view, Group> groups;
    for (const auto &ti : test_set) {
        for (auto ptr = ti.test->groups; ptr && *ptr; ++ptr) {
            Group &g = groups[(*ptr)->id];
            g.definition = *ptr;
            g.entries.push_back(ti.test);
        }
    }

    return groups;
}

void list_tests(SandstoneTestSet& test_set, int opt)
{
    bool include_tests = (opt != raw_list_groups);
    bool include_groups = (opt != raw_list_tests);
    bool include_descriptions = (opt == 'l');

    auto groups = collate_test_groups(test_set);
    int i = 0;

    for (const auto &ti : test_set) {
        struct test *test = ti.test;
        if (test->quality_level >= sApp->requested_quality) {
            if (include_tests) {
                if (include_descriptions) {
                    printf("%i %-20s \"%s\"\n", ++i, test->id, test->description);
                } else if (sApp->shmem->verbosity > 0) {
                    // don't report the FW minimum CPU features
                    uint64_t cpuf = test->compiler_minimum_cpu & ~_compilerCpuFeatures;
                    cpuf |= test->minimum_cpu;
                    printf("%-20s %s\n", test->id, cpu_features_to_string(cpuf).c_str());
                } else {
                    puts(test->id);
                }
            }
        }
    }

    if (include_groups && !groups.empty()) {
        if (include_descriptions)
            printf("\nGroups:\n");
        for (auto pair : groups) {
            const auto &g = pair.second;
            if (include_descriptions) {
                printf("@%-21s \"%s\"\n", g.definition->id, g.definition->description);
                for (auto test : g.entries)
                    if (test->quality_level >= sApp->requested_quality)
                        printf("  %s\n", test->id);
            } else {
                // just the group name
                printf("@%s\n", g.definition->id);
            }
        }
    }
}

void list_group_members(SandstoneTestSet& test_set, const char *groupname)
{
    auto groups = collate_test_groups(test_set);
    for (auto pair : groups) {
        const auto &g = pair.second;
        if (groupname[0] == '@' && strcmp(g.definition->id, groupname + 1) == 0) {
            for (auto test : g.entries)
                printf("%s\n", test->id);
            return;
        }
    }

    fprintf(stderr, "No such group '%s'\n", groupname);
    exit(EX_USAGE);
}

enum class OutOfRangeMode { Exit, Saturate };
template <typename Integer = int> struct ParseIntArgument
{
    static_assert(std::is_signed_v<Integer> || std::is_unsigned_v<Integer>);
    using MaxInteger = std::conditional_t<std::is_signed_v<Integer>, long long, unsigned long long>;

    const char *name = nullptr;
    const char *explanation = nullptr;
    MaxInteger min = 0;
    MaxInteger max = std::numeric_limits<Integer>::max();
    int base = 10;
    OutOfRangeMode range_mode = OutOfRangeMode::Exit;

    void print_explanation() const
    {
        // i18n style guide says to never construct sentences...
        if (explanation)
            fprintf(stderr, "%s: value is %s\n", program_invocation_name, explanation);
    }

    void print_range_error(const char *arg) const
    {
        const char *severity = "warning";
        if (range_mode == OutOfRangeMode::Exit)
            severity = "error";
        if constexpr (std::is_signed_v<Integer>) {
            fprintf(stderr,
                    "%s: %s: value out of range for option '%s': %s (minimum is %lld, maximum %lld)\n",
                    program_invocation_name, severity, name, arg, min, max);
        } else {
            fprintf(stderr,
                    "%s: %s: value out of range for option '%s': %s (minimum is %llu, maximum %llu)\n",
                    program_invocation_name, severity, name, arg, min, max);
        }
        print_explanation();
    }

    // non-const because this usually comes from optarg anyway
    Integer operator()(char *arg = optarg) const
    {
        assert(name);
        assert(arg);
        assert(min <= max);
        assert(Integer(min) == min);
        assert(Integer(max) == max);

        char *end = arg;
        errno = 0;
        MaxInteger parsed;
        if constexpr (std::is_signed_v<Integer>)
            parsed = strtoll(arg, &end, base);
        else
            parsed = strtoull(arg, &end, base);

        if (*end != '\0' || *arg == '\0') {
            // strtoll() did not consume the entire string or there wasn't anything to consume,
            // so it can't be valid
            fprintf(stderr, "%s: invalid argument for option '%s': %s\n", program_invocation_name,
                    name, arg);
            print_explanation();
            exit(EX_USAGE);
        }

        // validate range
        Integer v = Integer(parsed);
        bool erange = (errno == ERANGE);
        if (erange || v != parsed || v < min || v > max) {
            print_range_error(arg);
            if (range_mode == OutOfRangeMode::Exit)
                exit(EX_USAGE);

            if (parsed < min || (erange && parsed == std::numeric_limits<long long>::min()))
                v = Integer(min);
            else if (v > max || (erange && parsed == std::numeric_limits<long long>::max()))
                v = Integer(max);
        }
        return v;
    }
};

auto parse_testrun_range(const char *arg, int &starting_test_number, int &ending_test_number)
{
    char *end;
    errno = 0;
    starting_test_number = strtoul(arg, &end, 10);
    if (errno == 0) {
        if (*end == '-')
            ending_test_number = strtoul(end + 1, &end, 10);
        else
            errno = EINVAL;
    }
    if (errno != 0) {
        fprintf(stderr, "%s: error: --test-range requires two dash separated integer args like --test-range 1-10\n",
                program_invocation_name);
        return EXIT_FAILURE;
    }
    if (starting_test_number > ending_test_number)
        std::swap(starting_test_number, ending_test_number);
    if (starting_test_number < 1) {
        fprintf(stderr, "%s: error: The lower bound of the test range must be >= 1, %d specified\n",
                program_invocation_name, starting_test_number);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void warn_deprecated_opt(const char *opt)
{
    fprintf(stderr, "%s: option '%s' is ignored and will be removed in a future version.\n",
            program_invocation_name, opt);
}
}

int parse_cmdline(int argc, char** argv, SandstoneApplication* app, ParsedOpts& opts) {
    static constexpr int EXIT_CONTINUE = 255; // to differentiate between exiting return codes (EX_USAGE, EXIT_SUCCES), and non exiting
    static struct option long_options[]  = {
        { "1sec", no_argument, nullptr, one_sec_option },
        { "30sec", no_argument, nullptr, thirty_sec_option },
        { "2min", no_argument, nullptr, two_min_option },
        { "5min", no_argument, nullptr, five_min_option },
        { "alpha", no_argument, nullptr, alpha_option },
        { "beta", no_argument, nullptr, beta_option },
        { "cpuset", required_argument, nullptr, cpuset_option },
        { "disable", required_argument, nullptr, disable_option },
        { "dump-cpu-info", no_argument, nullptr, dump_cpu_info_option },
        { "enable", required_argument, nullptr, 'e' },
        { "fatal-errors", no_argument, nullptr, 'F'},
        { "fatal-skips", no_argument, nullptr, fatal_skips_option },
        { "fork-mode", required_argument, nullptr, 'f' },
        { "help", no_argument, nullptr, 'h' },
        { "ignore-os-errors", no_argument, nullptr, ignore_os_errors_option },
        { "ignore-timeout", no_argument, nullptr, ignore_os_errors_option },
        { "ignore-unknown-tests", no_argument, nullptr, ignore_unknown_tests_option },
        { "list", no_argument, nullptr, 'l' },
        { "list-tests", no_argument, nullptr, raw_list_tests },
        { "list-group-members", required_argument, nullptr, raw_list_group_members },
        { "list-groups", no_argument, nullptr, raw_list_groups },
        { "longer-runtime", required_argument, nullptr, longer_runtime_option },
        { "max-concurrent-threads", required_argument, nullptr, max_concurrent_threads_option },
        { "max-cores-per-slice", required_argument, nullptr, max_cores_per_slice_option },
        { "max-logdata", required_argument, nullptr, max_logdata_option },
        { "max-messages", required_argument, nullptr, max_messages_option },
        { "max-test-count", required_argument, nullptr, max_test_count_option },
        { "max-test-loop-count", required_argument, nullptr, max_test_loop_count_option },
        { "mce-check-every", required_argument, nullptr, mce_check_period_option },
        { "mem-sample-time", required_argument, nullptr, mem_sample_time_option },
        { "mem-samples-per-log", required_argument, nullptr, mem_samples_per_log_option},
        { "no-memory-sampling", no_argument, nullptr, no_mem_sampling_option },
        { "no-slicing", no_argument, nullptr, no_slicing_option },
        { "triage", no_argument, nullptr, triage_option },
        { "no-triage", no_argument, nullptr, no_triage_option },
        { "on-crash", required_argument, nullptr, on_crash_option },
        { "on-hang", required_argument, nullptr, on_hang_option },
        { "output-format", required_argument, nullptr, output_format_option},
        { "output-log", required_argument, nullptr, 'o' },
        { "quality", required_argument, nullptr, quality_option },
        { "quick", no_argument, nullptr, quick_run_option },
        { "quiet", no_argument, nullptr, 'q' },
        { "retest-on-failure", required_argument, nullptr, retest_on_failure_option },
        { "rng-state", required_argument, nullptr, 's' },
        { "schedule-by", required_argument, nullptr, schedule_by_option },
#ifndef NO_SELF_TESTS
        { "selftests", no_argument, nullptr, selftest_option },
#endif
        { "service", no_argument, nullptr, service_option },
        { "shorten-runtime", required_argument, nullptr, shortened_runtime_option },
        { "strict-runtime", no_argument, nullptr, strict_runtime_option },
        { "syslog", no_argument, nullptr, syslog_runtime_option },
        { "temperature-threshold", required_argument, nullptr, temperature_threshold_option },
        { "test-delay", required_argument, nullptr, test_delay_option },
        { "test-list-file", required_argument, nullptr, test_list_file_option },
        { "test-range", required_argument, nullptr, test_index_range_option },
        { "test-list-randomize", no_argument, nullptr, test_list_randomize_option },
        { "test-time", required_argument, nullptr, 't' },   // repeated below
        { "force-test-time", no_argument, nullptr, force_test_time_option },
        { "test-option", required_argument, nullptr, 'O'},
        { "threads", required_argument, nullptr, 'n' },
        { "time", required_argument, nullptr, 't' },        // repeated above
        { "timeout", required_argument, nullptr, timeout_option },
        { "total-retest-on-failure", required_argument, nullptr, total_retest_on_failure },
        { "total-time", required_argument, nullptr, 'T' },
        { "ud-on-failure", no_argument, nullptr, ud_on_failure_option },
        { "use-builtin-test-list", optional_argument, nullptr, use_builtin_test_list_option },
        { "vary-frequency", no_argument, nullptr, vary_frequency},
        { "vary-uncore-frequency", no_argument, nullptr, vary_uncore_frequency},
        { "verbose", no_argument, nullptr, 'v' },
        { "version", no_argument, nullptr, version_option },
        { "weighted-testrun-type", required_argument, nullptr, weighted_testrun_option },
        { "yaml", optional_argument, nullptr, 'Y' },

#if defined(__SANITIZE_ADDRESS__)
        { "is-asan-build", no_argument, nullptr, is_asan_option },
#endif
#ifndef NDEBUG
        // debug-mode only options:
        { "gdb-server", required_argument, nullptr, gdb_server_option },
        { "is-debug-build", no_argument, nullptr, is_debug_option },
        { "test-tests", no_argument, nullptr, test_tests_option },
#endif
        { nullptr, 0, nullptr, 0 }
    };

    int opt;
    int coptind = -1;
    int current_quality_value = app->DefaultQualityLevel;

    while (!SandstoneConfig::RestrictedCommandLine &&
        (opt = simple_getopt(argc, argv, long_options, &coptind)) != -1) {
        switch (opt) {
        case disable_option:
            opts.disabled_tests.push_back(optarg);
            break;
        case 'e':
            opts.enabled_tests.emplace_back(optarg, current_quality_value);
            break;
        case 'f':
            if (strcmp(optarg, "no") == 0 || strcmp(optarg, "no-fork") == 0) {
                app->fork_mode = SandstoneApplication::no_fork;
            } else if (!strcmp(optarg, "exec")) {
                app->fork_mode = SandstoneApplication::exec_each_test;
#ifndef _WIN32
            } else if (strcmp(optarg, "yes") == 0 || strcmp(optarg, "each-test") == 0) {
                app->fork_mode = SandstoneApplication::fork_each_test;
#endif
            } else {
                fprintf(stderr, "%s: unknown option to -f: %s\n", argv[0], optarg);
                usage(argv);
                return EX_USAGE;
            }
            break;
        case 'F':
            opts.fatal_errors = true;
            break;
        case 'h':
            usage(argv);
            return EXIT_SUCCESS;
        case 'l':
        case raw_list_tests:
        case raw_list_groups:
        {
            SandstoneTestSet test_set{opts.test_set_config, SandstoneTestSet::enable_all_tests};
            list_tests(test_set, opt);
            return EXIT_SUCCESS;
        }
        case raw_list_group_members:
        {
            SandstoneTestSet test_set{opts.test_set_config, SandstoneTestSet::enable_all_tests};
            list_group_members(test_set, optarg);
            return EXIT_SUCCESS;
        }
        case 'n':
            opts.thread_count = ParseIntArgument<>{
                    .name = "-n / --threads",
                    .min = 1,
                    .max = app->thread_count,
                    .range_mode = OutOfRangeMode::Saturate
            }();
            break;
        case 'o':
            app->file_log_path = optarg;
            break;
        case 'O':
            app->shmem->log_test_knobs = true;
            if ( ! set_knob_from_key_value_string(optarg)){
                fprintf(stderr, "Malformed test knob: %s (should be in the form KNOB=VALUE)\n", optarg);
                return EX_USAGE;
            }
            break;
        case 'q':
            app->shmem->verbosity = 0;
            break;
        case 's':
            opts.seed = optarg;
            break;
        case 't':
            app->test_time = string_to_millisecs(optarg);
            break;
        case force_test_time_option: /* overrides max and min duration specified by the test */
            app->force_test_time = true;
            break;
        case 'T':
            if (strcmp(optarg, "forever") == 0) {
                app->endtime = MonotonicTimePoint::max();
            } else {
                app->endtime = app->starttime + string_to_millisecs(optarg);
            }
            opts.test_set_config.cycle_through = true; /* Time controls when the execution stops as
                                                        opposed to the number of tests. */
            break;
        case 'v':
            if (app->shmem->verbosity < 0)
                app->shmem->verbosity = 1;
            else
                ++app->shmem->verbosity;
            break;
        case 'Y':
            app->shmem->output_format = SandstoneApplication::OutputFormat::yaml;
            if (optarg)
                app->shmem->output_yaml_indent = ParseIntArgument<>{
                        .name = "-Y / --yaml",
                        .max = 160,     // arbitrary
                }();
            break;
        case cpuset_option:
            apply_cpuset_param(optarg);
            break;
        case dump_cpu_info_option:
            dump_cpu_info();
            return EXIT_SUCCESS;
        case fatal_skips_option:
            app->fatal_skips = true;
            break;
#ifndef NDEBUG
        case gdb_server_option:
            app->gdb_server_comm = optarg;
            break;
#endif
        case ignore_os_errors_option:
            app->ignore_os_errors = true;
            break;
        case ignore_unknown_tests_option:
            opts.test_set_config.ignore_unknown_tests = true;
            break;
        case is_asan_option:
        case is_debug_option:
            // these options are only accessible in the command-line if the
            // corresponding functionality is active
            return EXIT_SUCCESS;
        case max_cores_per_slice_option:
            opts.max_cores_per_slice = ParseIntArgument<>{
                    .name = "--max-cores-per-slice",
                    .min = -1,
                }();
            break;
        case mce_check_period_option:
            app->mce_check_period = ParseIntArgument<>{"--mce-check-every"}();
            break;
        case no_slicing_option:
            opts.max_cores_per_slice = -1;
            break;
        case on_crash_option:
            opts.on_crash_arg = optarg;
            break;
        case on_hang_option:
            opts.on_hang_arg = optarg;
            break;
        case output_format_option:
            if (strcmp(optarg, "key-value") == 0) {
                app->shmem->output_format = SandstoneApplication::OutputFormat::key_value;
            } else if (strcmp(optarg, "tap") == 0) {
                app->shmem->output_format = SandstoneApplication::OutputFormat::tap;
            } else if (strcmp(optarg, "yaml") == 0) {
                app->shmem->output_format = SandstoneApplication::OutputFormat::yaml;
            } else if (SandstoneConfig::Debug && strcmp(optarg, "none") == 0) {
                // for testing only
                app->shmem->output_format = SandstoneApplication::OutputFormat::no_output;
                app->shmem->verbosity = -1;
            } else {
                fprintf(stderr, "%s: unknown output format: %s\n", argv[0], optarg);
                return EX_USAGE;
            }
            break;

        case alpha_option:
            current_quality_value = INT_MIN;
            break;
        case beta_option:
            current_quality_value = 0;
            break;
        case quality_option:
            current_quality_value = ParseIntArgument<>{
                    .name = "--quality",
                    .min = -1000,
                    .max = +1000,
                    .range_mode = OutOfRangeMode::Saturate
            }();
            break;

        case quick_run_option:
            app->max_test_loop_count = 1;
            app->delay_between_tests = 0ms;
            break;
        case retest_on_failure_option:
            app->retest_count = ParseIntArgument<>{
                    .name = "--retest-on-failure",
                    .max = SandstoneApplication::MaxRetestCount,
                    .range_mode = OutOfRangeMode::Saturate
            }();
            break;
        case strict_runtime_option:
            app->shmem->use_strict_runtime = true;
            break;
        case syslog_runtime_option:
            app->syslog_ident = program_invocation_name;
            break;
#ifndef NO_SELF_TESTS
        case selftest_option:
            if (app->requested_quality != SandstoneApplication::DefaultQualityLevel) {
                fprintf(stderr, "%s: --selftest is incompatible with --beta or --quality.\n", argv[0]);
                return EX_USAGE;
            }
            app->requested_quality = 0;
            app->shmem->selftest = true;
            opts.test_set_config.is_selftest = true;
            break;
#endif
        case service_option:
            // keep in sync with RestrictedCommandLine below
            opts.fatal_errors = true;
            app->endtime = MonotonicTimePoint::max();
            app->service_background_scan = true;
            break;
        case ud_on_failure_option:
            app->shmem->ud_on_failure = true;
            break;
        case use_builtin_test_list_option:
            if (!SandstoneConfig::HasBuiltinTestList) {
                fprintf(stderr, "%s: --use-builtin-test-list specified but this build does not "
                                "have a built-in test list.\n", argv[0]);
                return EX_USAGE;
            }
            opts.builtin_test_list_name = optarg ? optarg : "auto";
            break;
        case temperature_threshold_option:
            if (strcmp(optarg, "disable") == 0)
                app->thermal_throttle_temp = -1;
            else
                app->thermal_throttle_temp = ParseIntArgument<>{
                        .name = "--temperature-threshold",
                        .explanation = "value should be specified in thousandths of degrees Celsius "
                                        "(for example, 85000 is 85 degrees Celsius), or \"disable\" "
                                        "to disable monitoring",
                        .max = 160000,      // 160 C is WAAAY too high anyway
                        .range_mode = OutOfRangeMode::Saturate
                }();
            break;

        case test_delay_option:
            app->delay_between_tests = string_to_millisecs(optarg);
            break;

        case test_tests_option:
            app->enable_test_tests();
            if (app->test_tests_enabled()) {
                // disable other options that don't make sense in this mode
                app->retest_count = 0;
            }
            break;

        case timeout_option:
            app->max_test_time = string_to_millisecs(optarg);
            break;

        case total_retest_on_failure:
            app->total_retest_count = ParseIntArgument<>{
                    .name = "--total-retest-on-failure",
                    .min = -1
            }();
            break;

        case test_list_file_option:
            opts.test_list_file_path = optarg;
            break;

        case test_index_range_option:
            if (parse_testrun_range(optarg, opts.starting_test_number, opts.ending_test_number) == EXIT_FAILURE)
                return EX_USAGE;
            break;

        case test_list_randomize_option:
            opts.test_set_config.randomize = true;
            break;

        case max_logdata_option: {
            app->shmem->max_logdata_per_thread = ParseIntArgument<unsigned>{
                    .name = "--max-logdata",
                    .explanation = "maximum number of bytes of test's data to log per thread (0 is unlimited))",
                    .base = 0,      // accept hex
                    .range_mode = OutOfRangeMode::Saturate
            }();
            if (app->shmem->max_logdata_per_thread == 0)
                app->shmem->max_logdata_per_thread = UINT_MAX;
            break;
        }
        case max_messages_option:
            app->shmem->max_messages_per_thread = ParseIntArgument<>{
                    .name = "--max-messages",
                    .explanation = "maximum number of messages (per thread) to log in each test (0 is unlimited)",
                    .min = -1,
                    .range_mode = OutOfRangeMode::Saturate
            }();
            if (app->shmem->max_messages_per_thread <= 0)
                app->shmem->max_messages_per_thread = INT_MAX;
            break;

        case vary_frequency:
            if (!FrequencyManager::FrequencyManagerWorks) {
                fprintf(stderr, "%s: --vary-frequency works only on Linux\n", program_invocation_name);
                return EX_USAGE;
            }
            app->vary_frequency_mode = true;
            break;

        case vary_uncore_frequency:
            if (!FrequencyManager::FrequencyManagerWorks) {
                fprintf(stderr, "%s: --vary-uncore-frequency works only on Linux\n", program_invocation_name);
                return EX_USAGE;
            }
            app->vary_uncore_frequency_mode = true;
            break;

        case version_option:
            logging_print_version();
            return EXIT_SUCCESS;
        case one_sec_option:
            opts.test_set_config.randomize = true;
            opts.test_set_config.cycle_through = true;
            app->shmem->use_strict_runtime = true;
            app->endtime = app->starttime + 1s;
            break;
        case thirty_sec_option:
            opts.test_set_config.randomize = true;
            opts.test_set_config.cycle_through = true;
            app->shmem->use_strict_runtime = true;
            app->endtime = app->starttime + 30s;
            break;
        case two_min_option:
            opts.test_set_config.randomize = true;
            opts.test_set_config.cycle_through = true;
            app->shmem->use_strict_runtime = true;
            app->endtime = app->starttime + 2min;
            break;
        case five_min_option:
            opts.test_set_config.randomize = true;
            opts.test_set_config.cycle_through = true;
            app->shmem->use_strict_runtime = true;
            app->endtime = app->starttime + 5min;
            break;

        case max_test_count_option:
            app->max_test_count = ParseIntArgument<>{"--max-test-count"}();
            break;

        case max_test_loop_count_option:
            app->max_test_loop_count = ParseIntArgument<>{"--max-test-loop-count"}();
            if (app->max_test_loop_count == 0)
                    app->max_test_loop_count = std::numeric_limits<int>::max();
            break;

            /* deprecated options */
        case longer_runtime_option:
        case max_concurrent_threads_option:
        case mem_sample_time_option:
        case mem_samples_per_log_option:
        case no_mem_sampling_option:
        case no_triage_option:
        case schedule_by_option:
        case shortened_runtime_option:
        case triage_option:
        case weighted_testrun_option:
            warn_deprecated_opt(long_options[coptind].name);
            break;

        case 0:
            /* long option setting a value */
            continue;
        default:
            suggest_help(argv);
            return EX_USAGE;
        }
    }

    // settled at this point
    app->requested_quality = current_quality_value;

    if (SandstoneConfig::RestrictedCommandLine) {
        // Default options for the simplified OpenDCDiag cmdline
        static struct option restricted_long_options[] = {
            { "help", no_argument, nullptr, 'h' },
            { "query", no_argument, nullptr, 'q' },
            { "service", no_argument, nullptr, 's' },
            { "version", no_argument, nullptr, version_option },
            { nullptr, 0, nullptr, 0 }
        };

        while ((opt = simple_getopt(argc, argv, restricted_long_options)) != -1) {
            switch (opt) {
            case 'q':
                // ### FIXME
                fprintf(stderr, "%s: --query not implemented yet\n", argv[0]);
                abort();
            case 's':
                // keep in sync above
                app->endtime = MonotonicTimePoint::max();
                app->service_background_scan = true;
                break;
            case version_option:
                logging_print_version();
                return EXIT_SUCCESS;

            case 'h':
                usage(argv);
                return opt == 'h' ? EXIT_SUCCESS : EX_USAGE;
            default:
                suggest_help(argv);
                return EX_USAGE;
            }
        }

        if (SandstoneConfig::NoLogging) {
            app->shmem->output_format = SandstoneApplication::OutputFormat::no_output;
        } else  {
            app->shmem->verbosity = 1;
        }

        app->delay_between_tests = 50ms;
        app->thermal_throttle_temp = INT_MIN;
        opts.fatal_errors = true;
        opts.builtin_test_list_name = "auto";

        static_assert(!SandstoneConfig::RestrictedCommandLine || SandstoneConfig::HasBuiltinTestList,
                "Restricted command-line build must have a built-in test list");
    }

    if (optind < argc) {
        usage(argv);
        return EX_USAGE;
    }

    return EXIT_CONTINUE;
}