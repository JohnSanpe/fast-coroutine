/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#ifndef _FCORO_H_
#define _FCORO_H_

#include <macro.h>
#include <osdep.h>
#include <list.h>
#include <rbtree.h>

#define TASK_NAME_LEN       32
#define TASK_STACK_SIZE     0x80000

enum fcoro_state {
    FCORO_RUNNING,
    FCORO_READY,
    FCORO_EXIT,
};

/**
 * fcoro_work - Describe a coro work
 * @node: node add to worker
 * @worker: bound worker
 * @name: kcoro work name
 * @state: running state
 * @context: the work context
 * @start: work start time
 * @prio: static priority
 * @dyprio: dynamic priority
 * @vtime: relative running time
 */
struct fcoro_work {
    struct rb_node node;
    struct fcoro_worker *worker;
    char name[TASK_NAME_LEN];

    enum fcoro_state state;
    fcontext_t context;
    int retval;

    ftime_t start;
    uint64_t vtime;
    int prio;
    int dyprio;
};

/**
 * fcoro_worker - Describe a coro worker
 * @list: list node for system management
 * @task: point to the parent task
 * @ready: ready queue ont this worker
 * @retc: context use for task return
 * @curr: current running kcoro work
 * @min_vtime: minimum virtual time in this worker
 * @weight: worker weight
 */
struct fcoro_worker {
    struct list_head list;
    struct rb_root_cached ready;
    struct list_head zombies;

    fcontext_t retc;
    struct fcoro_work *prev;
    struct fcoro_work *curr;
    uint64_t min_vtime;
    uint64_t weight;
};

typedef void (*fcoro_entry_t)(struct fcoro_work *, void *pdata);

static inline void fcoro_dyprio_inc(struct fcoro_work *work)
{
    if (work->dyprio < 39)
        work->dyprio++;
}

static inline void fcoro_dyprio_dec(struct fcoro_work *work)
{
    if (work->dyprio > 0)
        work->dyprio--;
}

static inline void fcoro_dyprio_reset(struct fcoro_work *work)
{
    if (work->dyprio != work->prio)
        work->dyprio = work->prio;
}

extern struct fcoro_worker *fcoro_worker_create(const char *name, ...);
extern void fcoro_worker_destroy(struct fcoro_worker *worker);

extern struct fcoro_work *fcoro_work_stack_vcreate(struct fcoro_worker *worker, size_t ssize, fcoro_entry_t entry, void *pdata, const char *namefmt, va_list args);
extern struct fcoro_work *fcoro_work_stack_create(struct fcoro_worker *worker, size_t ssize, fcoro_entry_t entry, void *pdata, const char *namefmt, ...);
extern struct fcoro_work *fcoro_work_vcreate(struct fcoro_worker *worker, fcoro_entry_t entry, void *pdata, const char *namefmt, va_list args);
extern struct fcoro_work *fcoro_work_create(struct fcoro_worker *worker, fcoro_entry_t entry, void *pdata, const char *namefmt, ...);
extern void fcoro_work_destroy(struct fcoro_work *work);

extern void fcoro_work_prio(struct fcoro_work *work, int prio);
extern void fcoro_dispatch(struct fcoro_worker *worker);
extern void fcoro_yield(struct fcoro_work *work);
extern void fcoro_exit(struct fcoro_work *work, int retval);

extern void fcoro_nsleep(struct fcoro_work *work, unsigned int nsec);
extern void fcoro_usleep(struct fcoro_work *work, unsigned int usec);
extern void fcoro_msleep(struct fcoro_work *work, unsigned int msec);

#endif  /* _FCORO_H_ */
