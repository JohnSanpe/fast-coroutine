/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <fcoro.h>

void fcoro_nsleep(struct fcoro_work *work, unsigned int nsec)
{
    ftime_t timeout = ftime_add_ns(fcoro_get_time(), nsec);
    while (ftime_before(fcoro_get_time(), timeout)) {
        fcoro_dyprio_inc(work);
        fcoro_yield(work);
    }
    fcoro_dyprio_reset(work);
}

void fcoro_usleep(struct fcoro_work *work, unsigned int usec)
{
    ftime_t timeout = ftime_add_us(fcoro_get_time(), usec);
    while (ftime_before(fcoro_get_time(), timeout)) {
        fcoro_dyprio_inc(work);
        fcoro_yield(work);
    }
    fcoro_dyprio_reset(work);
}

void fcoro_msleep(struct fcoro_work *work, unsigned int msec)
{
    ftime_t timeout = ftime_add_ms(fcoro_get_time(), msec);
    while (ftime_before(fcoro_get_time(), timeout)) {
        fcoro_dyprio_inc(work);
        fcoro_yield(work);
    }
    fcoro_dyprio_reset(work);
}
