/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <fcoro.h>
#include <unistd.h>
#include <sys/select.h>

#define TEST_LOOP 100

static void msleep(unsigned int msecs)
{
    struct timeval tval = {
        .tv_sec = msecs / 1000,
        .tv_usec = (msecs * 1000) % 1000000,
    };

    select(0, NULL, NULL, NULL, &tval);
}

static void fcoro_task_a(struct fcoro_work *work, void *pdata)
{
    unsigned int count = TEST_LOOP;

    while (count--) {
        printf("fcoro task a running...\n");
        fcoro_yield(work);
    }

    fcoro_exit(work, 1);
}

static void fcoro_task_b(struct fcoro_work *work, void *pdata)
{
    unsigned int count = TEST_LOOP;

    while (count--) {
        printf("fcoro task b running...\n");
        msleep(10);
        fcoro_yield(work);
    }

    fcoro_exit(work, 2);
}

static void fcoro_task_c(struct fcoro_work *work, void *pdata)
{
    unsigned int count = TEST_LOOP;

    while (count--) {
        printf("fcoro task c running...\n");
        fcoro_msleep(work, 10);
        fcoro_yield(work);
    }

    fcoro_exit(work, 3);
}

int main(void)
{
    struct fcoro_worker *worker;
    struct fcoro_work *work_a;
    struct fcoro_work *work_b;
    struct fcoro_work *work_c;

    worker = fcoro_worker_create("fcoro worker");
    if (!worker)
        return -ENOMEM;

    work_a = fcoro_work_create(worker, fcoro_task_a, NULL, "fcoro work a");
    work_b = fcoro_work_create(worker, fcoro_task_b, NULL, "fcoro work b");
    work_c = fcoro_work_create(worker, fcoro_task_c, NULL, "fcoro work b");

    if (!work_a || !work_b || !work_c) {
        fcoro_worker_destroy(worker);
        return -ENOMEM;
    }

    fcoro_dispatch(worker);
    fcoro_worker_destroy(worker);

    return 0;
}
