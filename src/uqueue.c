/*
 *  uqueue.c - Queues
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
#include "uqueue.h"
#include "ulist.h"

struct _UQueue {
    UList       *head;
    UList       *tail;
    unsigned int size;
};

UQueue *queue_new(void)
{
    return calloc(1, sizeof(UQueue));
}

void queue_free(UQueue *queue)
{
    if (!queue)
        return;

    list_free(queue->head);
    free(queue);
}

int queue_is_empty(UQueue *queue)
{
    return !queue || queue->size == 0;
}

UQueue *queue_push(UQueue *queue, void *data)
{
    if (!queue)
        return NULL;

    queue->tail = list_last(list_append(queue->tail, data));

    if (!queue->head)
        queue->head = queue->tail;

    ++queue->size;
    return queue;
}

void *queue_pop(UQueue *queue)
{
    UList *list;
    void *data;

    if (!queue || !queue->head)
        return NULL;

    list = queue->head;
    data = list->data;
    queue->head = list->next;
    if (--queue->size == 0)
        queue->tail = NULL;
    list_free_1(list);
    return data;
}
