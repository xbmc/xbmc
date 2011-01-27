/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/* 
 * MT safe
 */

#include "config.h"

#include "glib.h"
#include "galias.h"


typedef struct _GCacheNode  GCacheNode;

struct _GCacheNode
{
  /* A reference counted node */
  gpointer value;
  gint ref_count;
};

struct _GCache
{
  /* Called to create a value from a key */
  GCacheNewFunc value_new_func;

  /* Called to destroy a value */
  GCacheDestroyFunc value_destroy_func;

  /* Called to duplicate a key */
  GCacheDupFunc key_dup_func;

  /* Called to destroy a key */
  GCacheDestroyFunc key_destroy_func;

  /* Associates keys with nodes */
  GHashTable *key_table;

  /* Associates nodes with keys */
  GHashTable *value_table;
};

static inline GCacheNode*
g_cache_node_new (gpointer value)
{
  GCacheNode *node = g_slice_new (GCacheNode);
  node->value = value;
  node->ref_count = 1;
  return node;
}

static inline void
g_cache_node_destroy (GCacheNode *node)
{
  g_slice_free (GCacheNode, node);
}

GCache*
g_cache_new (GCacheNewFunc      value_new_func,
	     GCacheDestroyFunc  value_destroy_func,
	     GCacheDupFunc      key_dup_func,
	     GCacheDestroyFunc  key_destroy_func,
	     GHashFunc          hash_key_func,
	     GHashFunc          hash_value_func,
	     GEqualFunc         key_equal_func)
{
  GCache *cache;

  g_return_val_if_fail (value_new_func != NULL, NULL);
  g_return_val_if_fail (value_destroy_func != NULL, NULL);
  g_return_val_if_fail (key_dup_func != NULL, NULL);
  g_return_val_if_fail (key_destroy_func != NULL, NULL);
  g_return_val_if_fail (hash_key_func != NULL, NULL);
  g_return_val_if_fail (hash_value_func != NULL, NULL);
  g_return_val_if_fail (key_equal_func != NULL, NULL);

  cache = g_slice_new (GCache);
  cache->value_new_func = value_new_func;
  cache->value_destroy_func = value_destroy_func;
  cache->key_dup_func = key_dup_func;
  cache->key_destroy_func = key_destroy_func;
  cache->key_table = g_hash_table_new (hash_key_func, key_equal_func);
  cache->value_table = g_hash_table_new (hash_value_func, NULL);

  return cache;
}

void
g_cache_destroy (GCache *cache)
{
  g_return_if_fail (cache != NULL);

  g_hash_table_destroy (cache->key_table);
  g_hash_table_destroy (cache->value_table);
  g_slice_free (GCache, cache);
}

gpointer
g_cache_insert (GCache   *cache,
		gpointer  key)
{
  GCacheNode *node;
  gpointer value;

  g_return_val_if_fail (cache != NULL, NULL);

  node = g_hash_table_lookup (cache->key_table, key);
  if (node)
    {
      node->ref_count += 1;
      return node->value;
    }

  key = (* cache->key_dup_func) (key);
  value = (* cache->value_new_func) (key);
  node = g_cache_node_new (value);

  g_hash_table_insert (cache->key_table, key, node);
  g_hash_table_insert (cache->value_table, value, key);

  return node->value;
}

void
g_cache_remove (GCache        *cache,
		gconstpointer  value)
{
  GCacheNode *node;
  gpointer key;

  g_return_if_fail (cache != NULL);

  key = g_hash_table_lookup (cache->value_table, value);
  node = g_hash_table_lookup (cache->key_table, key);

  g_return_if_fail (node != NULL);

  node->ref_count -= 1;
  if (node->ref_count == 0)
    {
      g_hash_table_remove (cache->value_table, value);
      g_hash_table_remove (cache->key_table, key);

      (* cache->key_destroy_func) (key);
      (* cache->value_destroy_func) (node->value);
      g_cache_node_destroy (node);
    }
}

void
g_cache_key_foreach (GCache   *cache,
		     GHFunc    func,
		     gpointer  user_data)
{
  g_return_if_fail (cache != NULL);
  g_return_if_fail (func != NULL);

  g_hash_table_foreach (cache->value_table, func, user_data);
}

void
g_cache_value_foreach (GCache   *cache,
		       GHFunc    func,
		       gpointer  user_data)
{
  g_return_if_fail (cache != NULL);
  g_return_if_fail (func != NULL);

  g_hash_table_foreach (cache->key_table, func, user_data);
}

#define __G_CACHE_C__
#include "galiasdef.c"
