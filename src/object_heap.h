/*
 *  object_heap.h - Management of VA-API resources
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

#ifndef VA_OBJECT_HEAP_H
#define VA_OBJECT_HEAP_H

#define OBJECT_HEAP_OFFSET_MASK 0x7f000000
#define OBJECT_HEAP_ID_MASK     0x00ffffff

typedef struct object_base *object_base_p;
typedef struct object_heap *object_heap_p;

struct object_base {
    int id;
    int next_free;
};

struct object_heap {
    pthread_mutex_t mutex;
    int object_size;
    int id_offset;
    int next_free;
    int heap_size;
    int heap_increment;
    void **bucket;
    int num_buckets;
};

typedef int object_heap_iterator;

/*
 * Return 0 on success, -1 on error
 */
int
object_heap_init(object_heap_p heap, int object_size, int id_offset)
    attribute_hidden;

/*
 * Allocates an object
 * Returns the object ID on success, returns -1 on error
 */
int object_heap_allocate(object_heap_p heap)
    attribute_hidden;

/*
 * Lookup an allocated object by object ID
 * Returns a pointer to the object on success, returns NULL on error
 */
object_base_p
object_heap_lookup(object_heap_p heap, int id)
    attribute_hidden;

/*
 * Iterate over all objects in the heap.
 * Returns a pointer to the first object on the heap, returns NULL if heap is empty.
 */
object_base_p
object_heap_first(object_heap_p heap, object_heap_iterator *iter)
    attribute_hidden;

/*
 * Iterate over all objects in the heap.
 * Returns a pointer to the next object on the heap, returns NULL if heap is empty.
 */
object_base_p
object_heap_next(object_heap_p heap, object_heap_iterator *iter)
    attribute_hidden;

/*
 * Frees an object
 */
void
object_heap_free(object_heap_p heap, object_base_p obj)
    attribute_hidden;

/*
 * Destroys a heap, the heap must be empty.
 */
void
object_heap_destroy(object_heap_p heap)
    attribute_hidden;

#endif /* VA_OBJECT_HEAP_H */
