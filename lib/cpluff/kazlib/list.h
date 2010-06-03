/*
 * List Abstract Data Type
 * Copyright (C) 1997 Kaz Kylheku <kaz@ashi.footprints.net>
 *
 * Free Software License:
 *
 * All rights are reserved by the author, with the following exceptions:
 * Permission is granted to freely reproduce and distribute this software,
 * possibly in exchange for a fee, provided that this copyright notice appears
 * intact. Permission is also granted to adapt this software to produce
 * derivative works, as long as the modified versions carry this copyright
 * notice and additional notices stating that the work has been modified.
 * This source code may be translated into executable form and incorporated
 * into proprietary software; there is no requirement for such software to
 * contain a copyright notice related to this source.
 *
 * $Id: list.h,v 1.19 1999/11/14 20:46:19 kaz Exp $
 * $Name: kazlib_1_20 $
 */

/*
 * Modified by Johannes Lehtinen in 2006-2007.
 * Included the definition of CP_HIDDEN macro and used it in declarations and
 * definitions to hide Kazlib symbols when building a shared C-Pluff library.
 */

#ifndef LIST_H
#define LIST_H

#ifdef _WIN32
#include "../libcpluff/win32/cpluffdef.h"
#else
#include "../libcpluff/cpluffdef.h"
#endif

#include <limits.h>

#ifdef KAZLIB_SIDEEFFECT_DEBUG
#include "sfx.h"
#define LIST_SFX_CHECK(E) SFX_CHECK(E)
#else
#define LIST_SFX_CHECK(E) (E)
#endif

/*
 * Blurb for inclusion into C++ translation units
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long listcount_t;
#define LISTCOUNT_T_MAX ULONG_MAX

typedef struct lnode_t {
    #if defined(LIST_IMPLEMENTATION) || !defined(KAZLIB_OPAQUE_DEBUG)
    struct lnode_t *list_next;
    struct lnode_t *list_prev;
    void *list_data;
    #else
    int list_dummy;
    #endif
} lnode_t;

typedef struct lnodepool_t {
    #if defined(LIST_IMPLEMENTATION) || !defined(KAZLIB_OPAQUE_DEBUG)
    struct lnode_t *list_pool;
    struct lnode_t *list_free;
    listcount_t list_size;
    #else
    int list_dummy;
    #endif
} lnodepool_t;

typedef struct list_t {
    #if defined(LIST_IMPLEMENTATION) || !defined(KAZLIB_OPAQUE_DEBUG)
    lnode_t list_nilnode;
    listcount_t list_nodecount;
    listcount_t list_maxcount;
    #else
    int list_dummy;
    #endif
} list_t;

CP_HIDDEN lnode_t *lnode_create(void *);
CP_HIDDEN lnode_t *lnode_init(lnode_t *, void *);
CP_HIDDEN void lnode_destroy(lnode_t *);
CP_HIDDEN void lnode_put(lnode_t *, void *);
CP_HIDDEN void *lnode_get(lnode_t *);
CP_HIDDEN int lnode_is_in_a_list(lnode_t *);

#if defined(LIST_IMPLEMENTATION) || !defined(KAZLIB_OPAQUE_DEBUG)
#define lnode_put(N, D)		((N)->list_data = (D))
#define lnode_get(N)		((N)->list_data)
#endif

CP_HIDDEN lnodepool_t *lnode_pool_init(lnodepool_t *, lnode_t *, listcount_t);
CP_HIDDEN lnodepool_t *lnode_pool_create(listcount_t);
CP_HIDDEN void lnode_pool_destroy(lnodepool_t *);
CP_HIDDEN lnode_t *lnode_borrow(lnodepool_t *, void *);
CP_HIDDEN void lnode_return(lnodepool_t *, lnode_t *);
CP_HIDDEN int lnode_pool_isempty(lnodepool_t *);
CP_HIDDEN int lnode_pool_isfrom(lnodepool_t *, lnode_t *);

CP_HIDDEN list_t *list_init(list_t *, listcount_t);
CP_HIDDEN list_t *list_create(listcount_t);
CP_HIDDEN void list_destroy(list_t *);
CP_HIDDEN void list_destroy_nodes(list_t *);
CP_HIDDEN void list_return_nodes(list_t *, lnodepool_t *);

CP_HIDDEN listcount_t list_count(list_t *);
CP_HIDDEN int list_isempty(list_t *);
CP_HIDDEN int list_isfull(list_t *);
CP_HIDDEN int list_contains(list_t *, lnode_t *);

CP_HIDDEN void list_append(list_t *, lnode_t *);
CP_HIDDEN void list_prepend(list_t *, lnode_t *);
CP_HIDDEN void list_ins_before(list_t *, lnode_t *, lnode_t *);
CP_HIDDEN void list_ins_after(list_t *, lnode_t *, lnode_t *);

CP_HIDDEN lnode_t *list_first(list_t *);
CP_HIDDEN lnode_t *list_last(list_t *);
CP_HIDDEN lnode_t *list_next(list_t *, lnode_t *);
CP_HIDDEN lnode_t *list_prev(list_t *, lnode_t *);

CP_HIDDEN lnode_t *list_del_first(list_t *);
CP_HIDDEN lnode_t *list_del_last(list_t *);
CP_HIDDEN lnode_t *list_delete(list_t *, lnode_t *);

CP_HIDDEN void list_process(list_t *, void *, void (*)(list_t *, lnode_t *, void *));

CP_HIDDEN int list_verify(list_t *);

#if defined(LIST_IMPLEMENTATION) || !defined(KAZLIB_OPAQUE_DEBUG)
#define lnode_pool_isempty(P)	((P)->list_free == 0)
#define list_count(L)		((L)->list_nodecount)
#define list_isempty(L)		((L)->list_nodecount == 0)
#define list_isfull(L)		(LIST_SFX_CHECK(L)->list_nodecount == (L)->list_maxcount)
#define list_next(L, N)		(LIST_SFX_CHECK(N)->list_next == &(L)->list_nilnode ? NULL : (N)->list_next)
#define list_prev(L, N)		(LIST_SFX_CHECK(N)->list_prev == &(L)->list_nilnode ? NULL : (N)->list_prev)
#define list_first(L)		list_next(LIST_SFX_CHECK(L), &(L)->list_nilnode)
#define list_last(L)		list_prev(LIST_SFX_CHECK(L), &(L)->list_nilnode)
#endif

#if defined(LIST_IMPLEMENTATION) || !defined(KAZLIB_OPAQUE_DEBUG)
#define list_append(L, N)	list_ins_before(LIST_SFX_CHECK(L), N, &(L)->list_nilnode)
#define list_prepend(L, N)	list_ins_after(LIST_SFX_CHECK(L), N, &(L)->list_nilnode)
#define list_del_first(L)	list_delete(LIST_SFX_CHECK(L), list_first(L))
#define list_del_last(L)	list_delete(LIST_SFX_CHECK(L), list_last(L))
#endif

/* destination list on the left, source on the right */

CP_HIDDEN void list_extract(list_t *, list_t *, lnode_t *, lnode_t *);
CP_HIDDEN void list_transfer(list_t *, list_t *, lnode_t *first);
CP_HIDDEN void list_merge(list_t *, list_t *, int (const void *, const void *));
CP_HIDDEN void list_sort(list_t *, int (const void *, const void *));
CP_HIDDEN lnode_t *list_find(list_t *, const void *, int (const void *, const void *));
CP_HIDDEN int list_is_sorted(list_t *, int (const void *, const void *));

#ifdef __cplusplus
}
#endif

#endif
