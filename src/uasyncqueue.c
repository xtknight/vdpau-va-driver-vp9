/*
 *  uasyncqueue.c - Asynchronous queues
 *
 *  libva-vdpau-driver (C) 2009-2011 Splitted-Desktop Systems
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "sysdeps.h"
#include "uasyncqueue.h"
#include "uqueue.h"
#include <pthread.h>

struct _UAsyncQueue {
    UQueue             *queue;
    pthread_mutex_t     mutex;
    pthread_cond_t      cond;
    unsigned int        is_waiting;
};

UAsyncQueue *async_queue_new(void)
{
    UAsyncQueue *queue = malloc(sizeof(*queue));

    if (!queue)
        return NULL;

    queue->queue = queue_new();
    if (!queue->queue)
        goto error;

    if (pthread_cond_init(&queue->cond, NULL) != 0)
        goto error;

    pthread_mutex_init(&queue->mutex, NULL);
    queue->is_waiting = 0;
    return queue;

error:
    async_queue_free(queue);
    return NULL;
}

void async_queue_free(UAsyncQueue *queue)
{
    if (!queue)
        return;

    pthread_mutex_unlock(&queue->mutex);
    queue_free(queue->queue);
    free(queue);
}

int async_queue_is_empty(UAsyncQueue *queue)
{
    return queue && queue_is_empty(queue->queue);
}

static UAsyncQueue *async_queue_push_unlocked(UAsyncQueue *queue, void *data)
{
    queue_push(queue->queue, data);
    if (queue->is_waiting)
        pthread_cond_signal(&queue->cond);
    return queue;
}

UAsyncQueue *async_queue_push(UAsyncQueue *queue, void *data)
{
    if (!queue)
        return NULL;

    pthread_mutex_lock(&queue->mutex);
    async_queue_push_unlocked(queue, data);
    pthread_mutex_unlock(&queue->mutex);
    return queue;
}

static void *
async_queue_timed_pop_unlocked(UAsyncQueue *queue, uint64_t end_time)
{
    if (queue_is_empty(queue->queue)) {
        assert(!queue->is_waiting);
        ++queue->is_waiting;
        if (!end_time)
            pthread_cond_wait(&queue->cond, &queue->mutex);
        else {
            struct timespec timeout;
            timeout.tv_sec  = end_time / 1000000;
            timeout.tv_nsec = 1000 * (end_time % 1000000);
            pthread_cond_timedwait(&queue->cond, &queue->mutex, &timeout);
        }
        --queue->is_waiting;
        if (queue_is_empty(queue->queue))
            return NULL;
    }
    return queue_pop(queue->queue);
}

void *async_queue_timed_pop(UAsyncQueue *queue, uint64_t end_time)
{
    void *data;

    if (!queue)
        return NULL;

    pthread_mutex_lock(&queue->mutex);
    data = async_queue_timed_pop_unlocked(queue, end_time);
    pthread_mutex_unlock(&queue->mutex);
    return data;
}
