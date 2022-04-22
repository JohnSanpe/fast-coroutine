/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <fcoro.h>

const unsigned int sched_prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};

const unsigned int sched_prio_to_wmult[40] = {
 /* -20 */     48388,     59856,     76040,     92818,    118348,
 /* -15 */    147320,    184698,    229616,    287308,    360437,
 /* -10 */    449829,    563644,    704093,    875809,   1099582,
 /*  -5 */   1376151,   1717300,   2157191,   2708050,   3363326,
 /*   0 */   4194304,   5237765,   6557202,   8165337,  10153587,
 /*   5 */  12820798,  15790321,  19976592,  24970740,  31350126,
 /*  10 */  39045157,  49367440,  61356676,  76695844,  95443717,
 /*  15 */ 119304647, 148102320, 186737708, 238609294, 286331153,
};

static inline uint64_t mul_u64_u32_shr(uint64_t a, uint32_t mul, unsigned int shift)
{
    uint32_t ah, al;
    uint64_t ret;

    al = a;
    ah = a >> 32;

    ret = (al * mul) >> shift;
    if (ah)
        ret += (ah * mul) << (32 - shift);

    return ret;
}

static inline uint64_t calc_delta(struct fcoro_work *task, uint64_t delta)
{
    uint64_t fact = 1024;
    int shift = 32;

    if (unlikely(fact >> 32)) {
        while(fact >> 32)
        {
            fact >>= 1;
            shift--;
        }
    }

    fact = (uint64_t)(uint32_t)fact * sched_prio_to_wmult[task->dyprio];

    while (fact >> 32) {
        fact >>= 1;
        shift--;
    }

    return mul_u64_u32_shr(delta, fact, shift);
}

static inline uint64_t fcoro_delta_fair(struct fcoro_work *task, uint64_t delta)
{
    if (unlikely(task->dyprio != 20))
        delta = calc_delta(task, delta);

    return delta;
}

static long fcoro_work_vtime_cmp(const struct rb_node *a, const struct rb_node *b)
{
    const struct fcoro_work *work_a = container_of(a, struct fcoro_work, node);
    const struct fcoro_work *work_b = container_of(b, struct fcoro_work, node);
    return work_a->vtime - work_b->vtime;
}

static inline struct fcoro_work *fcoro_work_pick(struct fcoro_worker *worker)
{
    struct rb_node *lestmost = rb_cached_first(&worker->ready);
    return container_of_safe(lestmost, struct fcoro_work, node);
}

static inline void fcoro_worker_update(struct fcoro_worker *worker)
{
    struct fcoro_work *next = fcoro_work_pick(worker);

    if (next)
        worker->min_vtime = next->vtime;
    else if (worker->curr)
        worker->min_vtime = worker->curr->vtime;
    else
        worker->min_vtime = 0;
}

static void fcoro_worker_enqueue(struct fcoro_worker *worker, struct fcoro_work *work)
{
    rb_cached_insert(&worker->ready, &work->node, fcoro_work_vtime_cmp);
    fcoro_worker_update(worker);
    work->state = FCORO_READY;
}

static void fcoro_worker_dequeue(struct fcoro_worker *worker, struct fcoro_work *work)
{
    rb_cached_delete(&worker->ready, &work->node);
    fcoro_worker_update(worker);
    work->state = FCORO_RUNNING;
}

void fcoro_work_prio(struct fcoro_work *work, int prio)
{
    struct fcoro_worker *worker = work->worker;

    if (prio < -20)
        prio = 0;
    else if (prio > 19)
        prio = 39;
    else
        prio += 20;

    worker->weight -= sched_prio_to_weight[work->prio];
    worker->weight += sched_prio_to_weight[prio];
    work->prio = work->dyprio = prio;
}

void fcoro_yield(struct fcoro_work *work)
{
    struct fcoro_worker *worker = work->worker;
    struct fcoro_work *next;
    uint64_t now;

    now = fcoro_get_time();
    work->vtime += fcoro_delta_fair(work, now - work->start);

    if (likely(work->state == FCORO_RUNNING)) {
        if (((int64_t)(work->vtime - worker->min_vtime) > 0))
            fcoro_worker_enqueue(worker, work);
        else {
            work->start = now;
            return;
        }
    }

    next = fcoro_work_pick(worker);
    if (next) {
        fcoro_worker_dequeue(worker, next);
        next->start = now;
        if (next == work)
            worker->prev = NULL;
        else {
            worker->prev = work;
            worker->curr = next;
            swapcontext(&work->context, &next->context);
        }
    }

    barrier();

    if (unlikely(worker->prev))
        work = worker->prev;

    if (unlikely(work->state == FCORO_EXIT)) {
        if (worker->prev)
            fcoro_work_destroy(work);
        else
            swapcontext(&work->context, &worker->retc);
    }
}

void fcoro_exit(struct fcoro_work *work, int retval)
{
    work->state = FCORO_EXIT;
    work->retval = retval;
    fcoro_yield(work);
}

void fcoro_dispatch(struct fcoro_worker *worker)
{
    struct fcoro_work *next;

    while ((next = fcoro_work_pick(worker))) {
        fcoro_worker_dequeue(worker, next);
        worker->curr = next;
        swapcontext(&worker->retc, &next->context);
        barrier();
        fcoro_work_destroy(worker->curr);
    }
}

struct fcoro_work *
fcoro_work_stack_vcreate(struct fcoro_worker *worker, size_t ssize, fcoro_entry_t entry,
                         void *pdata, const char *namefmt, va_list args)
{
    struct fcoro_work *work;
    void *stack;

    work = fcoro_zalloc(sizeof(*work));
    if (!work)
        return NULL;

    stack = fcoro_alloc(ssize);
    if (!stack) {
        fcoro_free(work);
        return NULL;
    }

    work->prio = 20;
    work->dyprio = 20;
    work->start = fcoro_get_time();
    work->vtime = worker->min_vtime;
    work->worker = worker;

    getcontext(&work->context);
    work->context.uc_stack.ss_sp = stack;
    work->context.uc_stack.ss_size = ssize;
    work->context.uc_link = &worker->retc;

    vsnprintf(work->name, sizeof(work->name), namefmt, args);
    makecontext(&work->context, (void (*)(void))entry, 2, work, pdata);
    fcoro_worker_enqueue(worker, work);

    return work;
}

struct fcoro_work *
fcoro_stack_work_create(struct fcoro_worker *worker, size_t ssize, fcoro_entry_t entry,
                        void *pdata, const char *namefmt, ...)
{
    struct fcoro_work *work;
    va_list args;

    va_start(args, namefmt);
    work = fcoro_work_stack_vcreate(worker, ssize, entry, pdata, namefmt, args);
    va_end(args);

    return work;
}

struct fcoro_work *
fcoro_work_vcreate(struct fcoro_worker *worker, fcoro_entry_t entry,
                   void *pdata, const char *namefmt, va_list args)
{
    return fcoro_work_stack_vcreate(worker, TASK_STACK_SIZE, entry, pdata, namefmt, args);
}

struct fcoro_work *
fcoro_work_create(struct fcoro_worker *worker, fcoro_entry_t entry,
                  void *pdata, const char *namefmt, ...)
{
    struct fcoro_work *work;
    va_list args;

    va_start(args, namefmt);
    work = fcoro_work_vcreate(worker, entry, pdata, namefmt, args);
    va_end(args);

    return work;
}

void fcoro_work_destroy(struct fcoro_work *work)
{
    struct fcoro_worker *worker = work->worker;

    if (work->state == FCORO_READY) {
        fcoro_worker_dequeue(worker, work);
    }

    fcoro_free(work->context.uc_stack.ss_sp);
    fcoro_free(work);
}
