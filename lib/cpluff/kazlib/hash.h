/*
 * Hash Table Data Type
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
 * $Id: hash.h,v 1.22.2.7 2000/11/13 01:36:45 kaz Exp $
 * $Name: kazlib_1_20 $
 */

/*
 * Modified by Johannes Lehtinen in 2006-2007.
 * Included the definition of CP_HIDDEN macro and used it in declarations and
 * definitions to hide Kazlib symbols when building a shared C-Pluff library.
 */

#ifndef HASH_H
#define HASH_H

#ifdef _WIN32
#include "../libcpluff/win32/cpluffdef.h"
#else
#include "../libcpluff/cpluffdef.h"
#endif

#include <limits.h>
#ifdef KAZLIB_SIDEEFFECT_DEBUG
#include "sfx.h"
#endif

/*
 * Blurb for inclusion into C++ translation units
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long hashcount_t;
#define HASHCOUNT_T_MAX ULONG_MAX

typedef unsigned long hash_val_t;
#define HASH_VAL_T_MAX ULONG_MAX

CP_HIDDEN extern int hash_val_t_bit;

#ifndef HASH_VAL_T_BIT
#define HASH_VAL_T_BIT ((int) hash_val_t_bit)
#endif

/*
 * Hash chain node structure.
 * Notes:
 * 1. This preprocessing directive is for debugging purposes.  The effect is
 *    that if the preprocessor symbol KAZLIB_OPAQUE_DEBUG is defined prior to the
 *    inclusion of this header,  then the structure shall be declared as having
 *    the single member   int __OPAQUE__.   This way, any attempts by the
 *    client code to violate the principles of information hiding (by accessing
 *    the structure directly) can be diagnosed at translation time. However,
 *    note the resulting compiled unit is not suitable for linking.
 * 2. This is a pointer to the next node in the chain. In the last node of a
 *    chain, this pointer is null.
 * 3. The key is a pointer to some user supplied data that contains a unique
 *    identifier for each hash node in a given table. The interpretation of
 *    the data is up to the user. When creating or initializing a hash table,
 *    the user must supply a pointer to a function for comparing two keys,
 *    and a pointer to a function for hashing a key into a numeric value.
 * 4. The value is a user-supplied pointer to void which may refer to
 *    any data object. It is not interpreted in any way by the hashing
 *    module.
 * 5. The hashed key is stored in each node so that we don't have to rehash
 *    each key when the table must grow or shrink.
 */

typedef struct hnode_t {
    #if defined(HASH_IMPLEMENTATION) || !defined(KAZLIB_OPAQUE_DEBUG)	/* 1 */
    struct hnode_t *hash_next;		/* 2 */
    const void *hash_key;		/* 3 */
    void *hash_data;			/* 4 */
    hash_val_t hash_hkey;		/* 5 */
    #else
    int hash_dummy;
    #endif
} hnode_t;

/*
 * The comparison function pointer type. A comparison function takes two keys
 * and produces a value of -1 if the left key is less than the right key, a
 * value of 0 if the keys are equal, and a value of 1 if the left key is
 * greater than the right key.
 */

typedef int (*hash_comp_t)(const void *, const void *);

/*
 * The hashing function performs some computation on a key and produces an
 * integral value of type hash_val_t based on that key. For best results, the
 * function should have a good randomness properties in *all* significant bits
 * over the set of keys that are being inserted into a given hash table. In
 * particular, the most significant bits of hash_val_t are most significant to
 * the hash module. Only as the hash table expands are less significant bits
 * examined. Thus a function that has good distribution in its upper bits but
 * not lower is preferrable to one that has poor distribution in the upper bits
 * but not the lower ones.
 */

typedef hash_val_t (*hash_fun_t)(const void *);

/*
 * allocator functions
 */

typedef hnode_t *(*hnode_alloc_t)(void *);
typedef void (*hnode_free_t)(hnode_t *, void *);

/*
 * This is the hash table control structure. It keeps track of information
 * about a hash table, as well as the hash table itself.
 * Notes:
 * 1.  Pointer to the hash table proper. The table is an array of pointers to
 *     hash nodes (of type hnode_t). If the table is empty, every element of
 *     this table is a null pointer. A non-null entry points to the first
 *     element of a chain of nodes.
 * 2.  This member keeps track of the size of the hash table---that is, the
 *     number of chain pointers.
 * 3.  The count member maintains the number of elements that are presently
 *     in the hash table.
 * 4.  The maximum count is the greatest number of nodes that can populate this
 *     table. If the table contains this many nodes, no more can be inserted,
 *     and the hash_isfull() function returns true.
 * 5.  The high mark is a population threshold, measured as a number of nodes,
 *     which, if exceeded, will trigger a table expansion. Only dynamic hash
 *     tables are subject to this expansion.
 * 6.  The low mark is a minimum population threshold, measured as a number of
 *     nodes. If the table population drops below this value, a table shrinkage
 *     will occur. Only dynamic tables are subject to this reduction.  No table
 *     will shrink beneath a certain absolute minimum number of nodes.
 * 7.  This is the a pointer to the hash table's comparison function. The
 *     function is set once at initialization or creation time.
 * 8.  Pointer to the table's hashing function, set once at creation or
 *     initialization time.
 * 9.  The current hash table mask. If the size of the hash table is 2^N,
 *     this value has its low N bits set to 1, and the others clear. It is used
 *     to select bits from the result of the hashing function to compute an
 *     index into the table.
 * 10. A flag which indicates whether the table is to be dynamically resized. It
 *     is set to 1 in dynamically allocated tables, 0 in tables that are
 *     statically allocated.
 */

typedef struct hash_t {
    #if defined(HASH_IMPLEMENTATION) || !defined(KAZLIB_OPAQUE_DEBUG)
    struct hnode_t **hash_table;		/* 1 */
    hashcount_t hash_nchains;			/* 2 */
    hashcount_t hash_nodecount;			/* 3 */
    hashcount_t hash_maxcount;			/* 4 */
    hashcount_t hash_highmark;			/* 5 */
    hashcount_t hash_lowmark;			/* 6 */
    hash_comp_t hash_compare;			/* 7 */
    hash_fun_t hash_function;			/* 8 */
    hnode_alloc_t hash_allocnode;
    hnode_free_t hash_freenode;
    void *hash_context;
    hash_val_t hash_mask;			/* 9 */
    int hash_dynamic;				/* 10 */
    #else
    int hash_dummy;
    #endif
} hash_t;

/*
 * Hash scanner structure, used for traversals of the data structure.
 * Notes:
 * 1. Pointer to the hash table that is being traversed.
 * 2. Reference to the current chain in the table being traversed (the chain
 *    that contains the next node that shall be retrieved).
 * 3. Pointer to the node that will be retrieved by the subsequent call to
 *    hash_scan_next().
 */

typedef struct hscan_t {
    #if defined(HASH_IMPLEMENTATION) || !defined(KAZLIB_OPAQUE_DEBUG)
    hash_t *hash_table;		/* 1 */
    hash_val_t hash_chain;	/* 2 */
    hnode_t *hash_next;		/* 3 */
    #else
    int hash_dummy;
    #endif
} hscan_t;

CP_HIDDEN extern hash_t *hash_create(hashcount_t, hash_comp_t, hash_fun_t);
CP_HIDDEN extern void hash_set_allocator(hash_t *, hnode_alloc_t, hnode_free_t, void *);
CP_HIDDEN extern void hash_destroy(hash_t *);
CP_HIDDEN extern void hash_free_nodes(hash_t *);
CP_HIDDEN extern void hash_free(hash_t *);
CP_HIDDEN extern hash_t *hash_init(hash_t *, hashcount_t, hash_comp_t,
	hash_fun_t, hnode_t **, hashcount_t);
CP_HIDDEN extern void hash_insert(hash_t *, hnode_t *, const void *);
CP_HIDDEN extern hnode_t *hash_lookup(hash_t *, const void *);
CP_HIDDEN extern hnode_t *hash_delete(hash_t *, hnode_t *);
CP_HIDDEN extern int hash_alloc_insert(hash_t *, const void *, void *);
CP_HIDDEN extern void hash_delete_free(hash_t *, hnode_t *);

CP_HIDDEN extern void hnode_put(hnode_t *, void *);
CP_HIDDEN extern void *hnode_get(hnode_t *);
CP_HIDDEN extern const void *hnode_getkey(hnode_t *);
CP_HIDDEN extern hashcount_t hash_count(hash_t *);
CP_HIDDEN extern hashcount_t hash_size(hash_t *);

CP_HIDDEN extern int hash_isfull(hash_t *);
CP_HIDDEN extern int hash_isempty(hash_t *);

CP_HIDDEN extern void hash_scan_begin(hscan_t *, hash_t *);
CP_HIDDEN extern hnode_t *hash_scan_next(hscan_t *);
CP_HIDDEN extern hnode_t *hash_scan_delete(hash_t *, hnode_t *);
CP_HIDDEN extern void hash_scan_delfree(hash_t *, hnode_t *);

CP_HIDDEN extern int hash_verify(hash_t *);

CP_HIDDEN extern hnode_t *hnode_create(void *);
CP_HIDDEN extern hnode_t *hnode_init(hnode_t *, void *);
CP_HIDDEN extern void hnode_destroy(hnode_t *);

#if defined(HASH_IMPLEMENTATION) || !defined(KAZLIB_OPAQUE_DEBUG)
#ifdef KAZLIB_SIDEEFFECT_DEBUG
#define hash_isfull(H) (SFX_CHECK(H)->hash_nodecount == (H)->hash_maxcount)
#else
#define hash_isfull(H) ((H)->hash_nodecount == (H)->hash_maxcount)
#endif
#define hash_isempty(H) ((H)->hash_nodecount == 0)
#define hash_count(H) ((H)->hash_nodecount)
#define hash_size(H) ((H)->hash_nchains)
#define hnode_get(N) ((N)->hash_data)
#define hnode_getkey(N) ((N)->hash_key)
#define hnode_put(N, V) ((N)->hash_data = (V))
#endif

#ifdef __cplusplus
}
#endif

#endif
