/*
 * Copyright 2022 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Output is in "Test Anything Protocol" format, as per http://testanything.org/tap-specification.html
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#if __has_include(<malloc.h>)
#  include <malloc.h>
#endif
#include <pthread.h>
#if __has_include(<sys/auxv.h>)         // FreeBSD and Linux
#  include <sys/auxv.h>
#endif

#include "sandstone.h"
#include "sandstone_p.h"
#include "topology.h"

#if SANDSTONE_SSL_BUILD
#  include "sandstone_ssl.h"
#  include "sandstone_ssl_rand.h"
#endif

#ifdef _WIN32
#  include <ntstatus.h>
#  include <shlwapi.h>
#  include <windows.h>
#  include <psapi.h>
#  include <pdh.h>

#  ifdef ftruncate
// MinGW's ftruncate64 tries to check free disk space and that fails on Wine,
// so use the the 32-bit offset version (which calls _chsize)
#    undef ftruncate
#  endif
#endif

#if !defined(__GLIBC__) && !defined(fileno_unlocked)
#  define fileno_unlocked   fileno
#endif

// TODO main.cpp also uses this, so it must be defined in one place...
#ifdef __llvm__
thread_local int thread_num __attribute__((tls_model("initial-exec")));
#else
thread_local int thread_num = 0;
#endif

// TODO duplicate in main.cpp
static inline __attribute__((always_inline, noreturn)) void ud2()
{
    __builtin_trap();
    __builtin_unreachable();
}

static void __attribute__((noreturn)) report_fail_common()
{
    logging_mark_thread_failed(thread_num);
#ifdef _WIN32
    /* does not call the cleanup handlers */
    _endthread();
#else
    /* does call the cleanup handalers */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
    pthread_cancel(pthread_self());
#endif
    __builtin_unreachable();
}

void _report_fail(const struct test *test, const char *file, int line)
{
    /* Keep this very early */
    if (sApp->shmem->ud_on_failure)
        ud2();

    if (!SandstoneConfig::NoLogging)
        log_error("Failed at %s:%d", file, line);
    report_fail_common();
}

void _report_fail_msg(const char *file, int line, const char *fmt, ...)
{
    /* Keep this very early */
    if (sApp->shmem->ud_on_failure)
        ud2();

    if (!SandstoneConfig::NoLogging)
        log_error("Failed at %s:%d: %s", file, line, va_start_and_stdprintf(fmt).c_str());
    report_fail_common();
}

/* Like memcmp(), but returns the offset of the byte that differed (negative if equal) */
static ptrdiff_t memcmp_offset(const uint8_t *d1, const uint8_t *d2, size_t size)
{
    ptrdiff_t i = 0;

    for (; i < (ptrdiff_t) size; ++i) {
        if (d1[i] != d2[i])
            return i;
    }
    return -1;
}

void _memcmp_fail_report(const void *_actual, const void *_expected, size_t size, DataType type, const char *fmt, ...)
{
    // Execute UD2 early if we've failed
    if (sApp->shmem->ud_on_failure)
        ud2();

    if (!SandstoneConfig::NoLogging) {
        if (fmt)
            assert(strchr(fmt, '\n') == nullptr && "Data descriptions should not include a newline");

        auto actual = static_cast<const uint8_t *>(_actual);
        auto expected = static_cast<const uint8_t *>(_expected);
        ptrdiff_t offset = memcmp_offset(actual, expected, size);

        va_list va;
        va_start(va, fmt);
        logging_report_mismatched_data(type, actual, expected, size, offset, fmt, va);
        va_end(va);
    }

    report_fail_common();
}

int num_cpus()
{
    return sApp->thread_count;
}

int num_packages()
{
    return Topology::topology().packages.size();
}
