/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2022 Sanpe <sanpeqf@gmail.com>
 */

#include <fcoro.h>

static LIST_HEAD(worker_list);

struct fcoro_worker *fcoro_worker_vcreate(const char *name, va_list args)
{
    struct fcoro_worker *worker;

    worker = fcoro_zalloc(sizeof(*worker));
    if (!worker)
        return NULL;

    list_add(&worker_list, &worker->list);
    return worker;
}

struct fcoro_worker *fcoro_worker_create(const char *namefmt, ...)
{
    struct fcoro_worker *worker;
    va_list args;

    va_start(args, namefmt);
    worker = fcoro_worker_vcreate(namefmt, args);
    va_end(args);

    return worker;
}

void fcoro_worker_destroy(struct fcoro_worker *worker)
{
    list_del(&worker->list);
    fcoro_free(worker);
}
