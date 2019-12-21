/*
 *  ulist.c - Lists
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
#include "ulist.h"

static UList *list_new(void *data, UList *prev, UList *next)
{
    UList *list = malloc(sizeof(*list));

    if (list) {
        list->data = data;
        list->prev = prev;
        list->next = next;
        if (prev)
            prev->next = list;
        if (next)
            next->prev = list;
    }
    return list;
}

void list_free_1(UList *list)
{
    free(list);
}

void list_free(UList *list)
{
    while (list) {
        UList * const next = list->next;
        list_free_1(list);
        list = next;
    }
}

UList *list_append(UList *list, void *data)
{
    UList * const node = list_new(data, list_last(list), NULL);

    return list ? list : node;
}

UList *list_prepend(UList *list, void *data)
{
    return list_new(data, list ? list->prev : NULL, list);
}

UList *list_first(UList *list)
{
    if (list) {
        while (list->prev)
            list = list->prev;
    }
    return list;
}

UList *list_last(UList *list)
{
    if (list) {
        while (list->next)
            list = list->next;
    }
    return list;
}

unsigned int list_size(UList *list)
{
    unsigned int size = 0;

    while (list) {
        ++size;
        list = list->next;
    }
    return size;
}

UList *list_lookup_full(UList *list, const void *data, UListCompareFunc compare)
{
    if (!list)
        return NULL;

    if (compare) {
        for (; list; list = list->next)
            if (compare(list->data, data))
                return list;
    }
    else {
        for (; list; list = list->next)
            if (list->data == data)
                return list;
    }
    return NULL;
}
