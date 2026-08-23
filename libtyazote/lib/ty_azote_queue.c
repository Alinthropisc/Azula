#include <stdlib.h>
#include <threads.h>
#include <time.h>

#include "ty_azote_queue.h"


typedef struct ty_azote_QueueNode {
    void *data;
    struct ty_azote_QueueNode *next;
} ty_azote_QueueNode;

struct ty_azote_Queue {
    ty_azote_QueueNode *front, *back;
    size_t size;
    mtx_t lock;
    cnd_t not_empty;
};

ty_azote_Queue *ty_azote_queue_new(void)
{
    ty_azote_Queue *q = calloc(1, sizeof *q);

    if (!q)
    {
        return nullptr;
    }
    mtx_init(&q->lock, mtx_plain);
    cnd_init(&q->not_empty);
    return q;
}

void ty_azote_queue_free(ty_azote_Queue *q, ty_azote_QueueItemFree item_free)
{
    if (!q)
    {
        return;
    }
    ty_azote_QueueNode *n = q->front;

    while (n)
    {
        ty_azote_QueueNode *next = n->next;

        if (item_free)
        {
            item_free(n->data);
        }
        free(n);
        n = next;
    }
    mtx_destroy(&q->lock);
    cnd_destroy(&q->not_empty);
    free(q);
}

bool ty_azote_queue_is_empty(ty_azote_Queue *q)
{
    mtx_lock(&q->lock);
    bool empty = q->size == 0;
    mtx_unlock(&q->lock);
    return empty;
}

size_t ty_azote_queue_size(ty_azote_Queue *q)
{
    mtx_lock(&q->lock);
    size_t s = q->size;
    mtx_unlock(&q->lock);
    return s;
}

ty_azote_QueueStatus ty_azote_queue_push_back(ty_azote_Queue *q, void *data)
{
    ty_azote_QueueNode *n = malloc(sizeof *n);

    if (!n)
    {
        return TY_AZOTE_QUEUE_ERR_NOMEM;
    }
    n->data = data;
    n->next = nullptr;
    mtx_lock(&q->lock);

    if (q->back)
    {
        q->back->next = n; else q->front = n;
    }
    q->back = n;
    ++q->size;
    cnd_signal(&q->not_empty);
    mtx_unlock(&q->lock);
    return TY_AZOTE_QUEUE_OK;
}

static void *pop_locked(ty_azote_Queue *q)
{
    ty_azote_QueueNode *n = q->front;
    q->front = n->next;

    if (!q->front)
    {
        q->back = nullptr;
    }
    --q->size;
    void *data = n->data;
    free(n);
    return data;
}

void *ty_azote_queue_pop_front(ty_azote_Queue *q)
{
    mtx_lock(&q->lock);
    while (q->size == 0)
    {
        cnd_wait(&q->not_empty, &q->lock);
    }
    void *data = pop_locked(q);
    mtx_unlock(&q->lock);
    return data;
}

bool ty_azote_queue_try_pop_front(ty_azote_Queue *q, void **out)
{
    mtx_lock(&q->lock);

    if (q->size == 0)
    {
        mtx_unlock(&q->lock);
        return false;
    }
    *out = pop_locked(q);
    mtx_unlock(&q->lock);
    return true;
}

bool ty_azote_queue_pop_front_timed(ty_azote_Queue *q, void **out, uint32_t timeout_ms)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    ts.tv_sec  += (time_t)(timeout_ms / 1000);
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;

    if (ts.tv_nsec >= 1000000000L)
    {
        ts.tv_nsec -= 1000000000L;
        ts.tv_sec += 1;
    }
    mtx_lock(&q->lock);

    while (q->size == 0)
    {
        if (cnd_timedwait(&q->not_empty, &q->lock, &ts) == thrd_timedout)
        {
            mtx_unlock(&q->lock);
            return false;
        }
    }
    *out = pop_locked(q);
    mtx_unlock(&q->lock);
    return true;
}



















