/*
 *  ulist.h - Lists
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

#ifndef ULIST_H
#define ULIST_H

typedef struct _UList UList;
struct _UList {
    void       *data;
    UList      *prev;
    UList      *next;
};

typedef int (*UListCompareFunc)(const void *a, const void *b);

void list_free_1(UList *list)
    attribute_hidden;

void list_free(UList *list)
    attribute_hidden;

UList *list_append(UList *list, void *data)
    attribute_hidden;

UList *list_prepend(UList *list, void *data)
    attribute_hidden;

UList *list_first(UList *list)
    attribute_hidden;

UList *list_last(UList *list)
    attribute_hidden;

unsigned int list_size(UList *list)
    attribute_hidden;

UList *list_lookup_full(UList *list, const void *data, UListCompareFunc compare)
    attribute_hidden;

#define list_lookup(list, data) \
    list_lookup_full(list, data, NULL)

#endif /* ULIST_H */
