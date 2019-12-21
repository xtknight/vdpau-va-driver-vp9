/*
 *  uqueue.h - Queues
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

#ifndef UQUEUE_H
#define UQUEUE_H

typedef struct _UQueue UQueue;

UQueue *queue_new(void)
    attribute_hidden;

void queue_free(UQueue *queue)
    attribute_hidden;

int queue_is_empty(UQueue *queue)
    attribute_hidden;

UQueue *queue_push(UQueue *queue, void *data)
    attribute_hidden;

void *queue_pop(UQueue *queue)
    attribute_hidden;

#endif /* UQUEUE_H */
