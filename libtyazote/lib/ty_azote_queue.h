#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ty_azote_Queue ty_azote_Queue; /* opaque */
typedef void (*ty_azote_QueueItemFree)(void *item);

typedef enum ty_azote_QueueStatus : uint8_t {
    TY_AZOTE_QUEUE_OK = 0,
    TY_AZOTE_QUEUE_ERR_NOMEM,
} ty_azote_QueueStatus;

[[nodiscard]]
ty_azote_Queue *ty_azote_queue_new(void);

/* Frees the queue; if `item_free` is non-null, it is invoked for every
 * item still queued (Composite cleanup — no manual draining required). */
void ty_azote_queue_free(ty_azote_Queue *q, ty_azote_QueueItemFree item_free);

[[nodiscard]]
bool ty_azote_queue_is_empty(ty_azote_Queue *q);

[[nodiscard]]
size_t ty_azote_queue_size(ty_azote_Queue *q);

[[nodiscard]]
ty_azote_QueueStatus ty_azote_queue_push_back(ty_azote_Queue *q, void *data);

/* Blocks until an item becomes available. */
[[nodiscard]]
void *ty_azote_queue_pop_front(ty_azote_Queue *q);

/* Non-blocking: returns false immediately if the queue is empty. */
[[nodiscard]]
bool ty_azote_queue_try_pop_front(ty_azote_Queue *q, void **out);

/* Blocks up to `timeout_ms`; returns false on timeout. */
[[nodiscard]]
bool ty_azote_queue_pop_front_timed(ty_azote_Queue *q, void **out,uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif







