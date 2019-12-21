/*
 *  uasyncqueue.h - Asynchronous queues
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

#ifndef UASYNCQUEUE_H
#define UASYNCQUEUE_H

typedef struct _UAsyncQueue UAsyncQueue;

UAsyncQueue *async_queue_new(void)
    attribute_hidden;

void async_queue_free(UAsyncQueue *queue)
    attribute_hidden;

int async_queue_is_empty(UAsyncQueue *queue)
    attribute_hidden;

UAsyncQueue *async_queue_push(UAsyncQueue *queue, void *data)
    attribute_hidden;

void *async_queue_timed_pop(UAsyncQueue *queue, uint64_t end_time)
    attribute_hidden;

#define async_queue_pop(queue) \
    async_queue_timed_pop(queue, 0)

#endif /* UASYNCQUEUE_H */
