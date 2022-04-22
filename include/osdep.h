/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#ifndef _FCORO_OSDEP_H_
#define _FCORO_OSDEP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <ucontext.h>

typedef pthread_t       fcoro_pth;
typedef pthread_attr_t  fcoro_pth_flag;
typedef ucontext_t      fcontext_t;
typedef int64_t         ftime_t;

/* Parameters used to convert the timespec values */
#define MSEC_PER_SEC    1000UL
#define USEC_PER_MSEC   1000UL
#define NSEC_PER_USEC   1000UL
#define NSEC_PER_MSEC   1000000UL
#define USEC_PER_SEC    1000000UL
#define NSEC_PER_SEC    1000000000UL
#define PSEC_PER_SEC    1000000000000ULL
#define FSEC_PER_SEC    1000000000000000ULL

#define FTIME_MAX       INT64_MAX
#define FTIME_MIN       INT64_MIN
#define FTIME_SEC_MAX   (FTIME_MAX / NSEC_PER_SEC)
#define FTIME_SEC_MIN   (FTIME_MIN / NSEC_PER_SEC)

#define ftime_add(time, nsval)  ((time) + (nsval))
#define ftime_sub(time, nsval)  ((time) - (nsval))

static inline uint64_t fcoro_get_time(void)
{
    struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return (int64_t)spec.tv_sec * 1000000000 + spec.tv_nsec;
}

/**
 * ftime_to_ns/us/ms - convert ftime to time
 * @time: the ftime to convert
 */
static inline int64_t ftime_to_ns(ftime_t time)
{
    return time;
}

static inline int64_t ftime_to_us(ftime_t time)
{
    return ftime_to_ns(time) / NSEC_PER_USEC;
}

static inline int64_t ftime_to_ms(ftime_t time)
{
    return ftime_to_ns(time) / NSEC_PER_MSEC;
}

/**
 * ns/us/ms_to_ftime - convert time to ftime
 * @time: the ns/us/ms to convert
 */
static inline ftime_t ns_to_ftime(int64_t time)
{
    return time;
}

static inline ftime_t us_to_ftime(int64_t time)
{
    return ns_to_ftime(time * NSEC_PER_USEC);
}

static inline ftime_t ms_to_ftime(int64_t time)
{
    return ns_to_ftime(time * NSEC_PER_MSEC);
}

/**
 * ftime_add_ns/us/ms - add time to ftime
 * @time: the ftime to add
 * @nsec: the ns/us/ms to add
 */
static inline ftime_t ftime_add_ns(const ftime_t time, const uint64_t nsec)
{
    return ftime_add(time, nsec);
}

static inline ftime_t ftime_add_us(const ftime_t time, const uint64_t usec)
{
    return ftime_add_ns(time, usec * NSEC_PER_USEC);
}

static inline ftime_t ftime_add_ms(const ftime_t time, const uint64_t msec)
{
    return ftime_add_ns(time, msec * NSEC_PER_MSEC);
}

/**
 * ftime_sub_ns/us/ms - sub time form ftime
 * @time: the ftime to sub
 * @nsec: the ns/us/ms to sub
 */
static inline ftime_t ftime_sub_ns(const ftime_t time, const uint64_t nsec)
{
    return ftime_sub(time, nsec);
}

static inline ftime_t ftime_sub_us(const ftime_t time, const uint64_t usec)
{
    return ftime_sub_ns(time, usec * NSEC_PER_USEC);
}

static inline ftime_t ftime_sub_ms(const ftime_t time, const uint64_t msec)
{
    return ftime_sub_ns(time, msec * NSEC_PER_MSEC);
}

/**
 * ftime_compare - compares two ftime_t variables
 * @timea: variables a
 * @timeb: variables b
 */
static inline int ftime_compare(const ftime_t timea, const ftime_t timeb)
{
    if (timea < timeb)
        return -1;
    if (timea > timeb)
        return 1;
    return 0;
}

/**
 * ftime_after - compare if a ftime value is bigger than another one
 * @timea: the bigger one
 * @timeb: the smaller one
 */
static inline bool ftime_after(const ftime_t timea, const ftime_t timeb)
{
    return ftime_compare(timea, timeb) > 0;
}

/**
 * ftime_before - compare if a ftime value is smaller than another one
 * @timea: the smaller one
 * @timeb: the bigger one
 */
static inline bool ftime_before(const ftime_t timea, const ftime_t timeb)
{
    return ftime_compare(timea, timeb) < 0;
}

static inline int fcoro_debug(const char *fmt, ...)
{
    va_list args;
    int length;

    va_start(args, fmt);
    length = vfprintf(stderr, fmt, args);
    va_end(args);

    return length;
}

static inline void *fcoro_alloc(size_t size)
{
    return malloc(size);
}

static inline void *fcoro_zalloc(size_t size)
{
    void *block;

    block = fcoro_alloc(size);
    if (!block)
        return NULL;

    memset(block, 0, size);
    return block;
}

static inline void fcoro_free(void *block)
{
    return free(block);
}

static inline int fcoro_pthread_create(fcoro_pth *pth, fcoro_pth_flag* flags, void *(*fun)(void *), void *args)
{
    return pthread_create(pth, flags, fun, args);
}

static inline void fcoro_pthread_cancel(fcoro_pth pth)
{
    pthread_cancel(pth);
}

#endif  /* _fcoro_OSDEP_H_ */
