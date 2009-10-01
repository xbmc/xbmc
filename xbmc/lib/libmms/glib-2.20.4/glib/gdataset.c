/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gdataset.c: Generic dataset mechanism, similar to GtkObject data.
 * Copyright (C) 1998 Tim Janik
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/* 
 * MT safe ; except for g_data*_foreach()
 */

#include "config.h"

#include <string.h>

#include "glib.h"
#include "gdatasetprivate.h"
#include "galias.h"


/* --- defines --- */
#define	G_QUARK_BLOCK_SIZE			(512)

/* datalist pointer accesses have to be carried out atomically */
#define G_DATALIST_GET_POINTER(datalist)						\
  ((GData*) ((gsize) g_atomic_pointer_get (datalist) & ~(gsize) G_DATALIST_FLAGS_MASK))

#define G_DATALIST_SET_POINTER(datalist, pointer)       G_STMT_START {                  \
  gpointer _oldv, _newv;                                                                \
  do {                                                                                  \
    _oldv = g_atomic_pointer_get (datalist);                                            \
    _newv = (gpointer) (((gsize) _oldv & G_DATALIST_FLAGS_MASK) | (gsize) pointer);     \
  } while (!g_atomic_pointer_compare_and_exchange ((void**) datalist, _oldv, _newv));   \
} G_STMT_END

/* --- structures --- */
typedef struct _GDataset GDataset;
struct _GData
{
  GData *next;
  GQuark id;
  gpointer data;
  GDestroyNotify destroy_func;
};

struct _GDataset
{
  gconstpointer location;
  GData        *datalist;
};


/* --- prototypes --- */
static inline GDataset*	g_dataset_lookup		(gconstpointer	  dataset_location);
static inline void	g_datalist_clear_i		(GData		**datalist);
static void		g_dataset_destroy_internal	(GDataset	 *dataset);
static inline gpointer	g_data_set_internal		(GData     	**datalist,
							 GQuark   	  key_id,
							 gpointer         data,
							 GDestroyNotify   destroy_func,
							 GDataset	 *dataset);
static void		g_data_initialize		(void);
static inline GQuark	g_quark_new			(gchar  	*string);


/* --- variables --- */
G_LOCK_DEFINE_STATIC (g_dataset_global);
static GHashTable   *g_dataset_location_ht = NULL;
static GDataset     *g_dataset_cached = NULL; /* should this be
						 threadspecific? */
G_LOCK_DEFINE_STATIC (g_quark_global);
static GHashTable   *g_quark_ht = NULL;
static gchar       **g_quarks = NULL;
static GQuark        g_quark_seq_id = 0;

/* --- functions --- */

/* HOLDS: g_dataset_global_lock */
static inline void
g_datalist_clear_i (GData **datalist)
{
  register GData *list;
  
  /* unlink *all* items before walking their destructors
   */
  list = G_DATALIST_GET_POINTER (datalist);
  G_DATALIST_SET_POINTER (datalist, NULL);
  
  while (list)
    {
      register GData *prev;
      
      prev = list;
      list = prev->next;
      
      if (prev->destroy_func)
	{
	  G_UNLOCK (g_dataset_global);
	  prev->destroy_func (prev->data);
	  G_LOCK (g_dataset_global);
	}
      
      g_slice_free (GData, prev);
    }
}

void
g_datalist_clear (GData **datalist)
{
  g_return_if_fail (datalist != NULL);
  
  G_LOCK (g_dataset_global);
  if (!g_dataset_location_ht)
    g_data_initialize ();

  while (G_DATALIST_GET_POINTER (datalist))
    g_datalist_clear_i (datalist);
  G_UNLOCK (g_dataset_global);
}

/* HOLDS: g_dataset_global_lock */
static inline GDataset*
g_dataset_lookup (gconstpointer	dataset_location)
{
  register GDataset *dataset;
  
  if (g_dataset_cached && g_dataset_cached->location == dataset_location)
    return g_dataset_cached;
  
  dataset = g_hash_table_lookup (g_dataset_location_ht, dataset_location);
  if (dataset)
    g_dataset_cached = dataset;
  
  return dataset;
}

/* HOLDS: g_dataset_global_lock */
static void
g_dataset_destroy_internal (GDataset *dataset)
{
  register gconstpointer dataset_location;
  
  dataset_location = dataset->location;
  while (dataset)
    {
      if (!dataset->datalist)
	{
	  if (dataset == g_dataset_cached)
	    g_dataset_cached = NULL;
	  g_hash_table_remove (g_dataset_location_ht, dataset_location);
	  g_slice_free (GDataset, dataset);
	  break;
	}
      
      g_datalist_clear_i (&dataset->datalist);
      dataset = g_dataset_lookup (dataset_location);
    }
}

void
g_dataset_destroy (gconstpointer  dataset_location)
{
  g_return_if_fail (dataset_location != NULL);
  
  G_LOCK (g_dataset_global);
  if (g_dataset_location_ht)
    {
      register GDataset *dataset;

      dataset = g_dataset_lookup (dataset_location);
      if (dataset)
	g_dataset_destroy_internal (dataset);
    }
  G_UNLOCK (g_dataset_global);
}

/* HOLDS: g_dataset_global_lock */
static inline gpointer
g_data_set_internal (GData	  **datalist,
		     GQuark         key_id,
		     gpointer       data,
		     GDestroyNotify destroy_func,
		     GDataset	   *dataset)
{
  register GData *list;
  
  list = G_DATALIST_GET_POINTER (datalist);
  if (!data)
    {
      register GData *prev;
      
      prev = NULL;
      while (list)
	{
	  if (list->id == key_id)
	    {
	      gpointer ret_data = NULL;

	      if (prev)
		prev->next = list->next;
	      else
		{
		  G_DATALIST_SET_POINTER (datalist, list->next);
		  
		  /* the dataset destruction *must* be done
		   * prior to invocation of the data destroy function
		   */
		  if (!list->next && dataset)
		    g_dataset_destroy_internal (dataset);
		}
	      
	      /* the GData struct *must* already be unlinked
	       * when invoking the destroy function.
	       * we use (data==NULL && destroy_func!=NULL) as
	       * a special hint combination to "steal"
	       * data without destroy notification
	       */
	      if (list->destroy_func && !destroy_func)
		{
		  G_UNLOCK (g_dataset_global);
		  list->destroy_func (list->data);
		  G_LOCK (g_dataset_global);
		}
	      else
		ret_data = list->data;
	      
              g_slice_free (GData, list);
	      
	      return ret_data;
	    }
	  
	  prev = list;
	  list = list->next;
	}
    }
  else
    {
      while (list)
	{
	  if (list->id == key_id)
	    {
	      if (!list->destroy_func)
		{
		  list->data = data;
		  list->destroy_func = destroy_func;
		}
	      else
		{
		  register GDestroyNotify dfunc;
		  register gpointer ddata;
		  
		  dfunc = list->destroy_func;
		  ddata = list->data;
		  list->data = data;
		  list->destroy_func = destroy_func;
		  
		  /* we need to have updated all structures prior to
		   * invocation of the destroy function
		   */
		  G_UNLOCK (g_dataset_global);
		  dfunc (ddata);
		  G_LOCK (g_dataset_global);
		}
	      
	      return NULL;
	    }
	  
	  list = list->next;
	}
      
      list = g_slice_new (GData);
      list->next = G_DATALIST_GET_POINTER (datalist);
      list->id = key_id;
      list->data = data;
      list->destroy_func = destroy_func;
      G_DATALIST_SET_POINTER (datalist, list);
    }

  return NULL;
}

void
g_dataset_id_set_data_full (gconstpointer  dataset_location,
			    GQuark         key_id,
			    gpointer       data,
			    GDestroyNotify destroy_func)
{
  register GDataset *dataset;
  
  g_return_if_fail (dataset_location != NULL);
  if (!data)
    g_return_if_fail (destroy_func == NULL);
  if (!key_id)
    {
      if (data)
	g_return_if_fail (key_id > 0);
      else
	return;
    }
  
  G_LOCK (g_dataset_global);
  if (!g_dataset_location_ht)
    g_data_initialize ();
 
  dataset = g_dataset_lookup (dataset_location);
  if (!dataset)
    {
      dataset = g_slice_new (GDataset);
      dataset->location = dataset_location;
      g_datalist_init (&dataset->datalist);
      g_hash_table_insert (g_dataset_location_ht, 
			   (gpointer) dataset->location,
			   dataset);
    }
  
  g_data_set_internal (&dataset->datalist, key_id, data, destroy_func, dataset);
  G_UNLOCK (g_dataset_global);
}

void
g_datalist_id_set_data_full (GData	  **datalist,
			     GQuark         key_id,
			     gpointer       data,
			     GDestroyNotify destroy_func)
{
  g_return_if_fail (datalist != NULL);
  if (!data)
    g_return_if_fail (destroy_func == NULL);
  if (!key_id)
    {
      if (data)
	g_return_if_fail (key_id > 0);
      else
	return;
    }

  G_LOCK (g_dataset_global);
  if (!g_dataset_location_ht)
    g_data_initialize ();
  
  g_data_set_internal (datalist, key_id, data, destroy_func, NULL);
  G_UNLOCK (g_dataset_global);
}

gpointer
g_dataset_id_remove_no_notify (gconstpointer  dataset_location,
			       GQuark         key_id)
{
  gpointer ret_data = NULL;

  g_return_val_if_fail (dataset_location != NULL, NULL);
  
  G_LOCK (g_dataset_global);
  if (key_id && g_dataset_location_ht)
    {
      GDataset *dataset;
  
      dataset = g_dataset_lookup (dataset_location);
      if (dataset)
	ret_data = g_data_set_internal (&dataset->datalist, key_id, NULL, (GDestroyNotify) 42, dataset);
    } 
  G_UNLOCK (g_dataset_global);

  return ret_data;
}

gpointer
g_datalist_id_remove_no_notify (GData	**datalist,
				GQuark    key_id)
{
  gpointer ret_data = NULL;

  g_return_val_if_fail (datalist != NULL, NULL);

  G_LOCK (g_dataset_global);
  if (key_id && g_dataset_location_ht)
    ret_data = g_data_set_internal (datalist, key_id, NULL, (GDestroyNotify) 42, NULL);
  G_UNLOCK (g_dataset_global);

  return ret_data;
}

gpointer
g_dataset_id_get_data (gconstpointer  dataset_location,
		       GQuark         key_id)
{
  g_return_val_if_fail (dataset_location != NULL, NULL);
  
  G_LOCK (g_dataset_global);
  if (key_id && g_dataset_location_ht)
    {
      register GDataset *dataset;
      
      dataset = g_dataset_lookup (dataset_location);
      if (dataset)
	{
	  register GData *list;
	  
	  for (list = dataset->datalist; list; list = list->next)
	    if (list->id == key_id)
	      {
		G_UNLOCK (g_dataset_global);
		return list->data;
	      }
	}
    }
  G_UNLOCK (g_dataset_global);
 
  return NULL;
}

gpointer
g_datalist_id_get_data (GData	 **datalist,
			GQuark     key_id)
{
  gpointer data = NULL;
  g_return_val_if_fail (datalist != NULL, NULL);
  if (key_id)
    {
      register GData *list;
      G_LOCK (g_dataset_global);
      for (list = G_DATALIST_GET_POINTER (datalist); list; list = list->next)
	if (list->id == key_id)
	  {
            data = list->data;
            break;
          }
      G_UNLOCK (g_dataset_global);
    }
  return data;
}

void
g_dataset_foreach (gconstpointer    dataset_location,
		   GDataForeachFunc func,
		   gpointer         user_data)
{
  register GDataset *dataset;
  
  g_return_if_fail (dataset_location != NULL);
  g_return_if_fail (func != NULL);

  G_LOCK (g_dataset_global);
  if (g_dataset_location_ht)
    {
      dataset = g_dataset_lookup (dataset_location);
      G_UNLOCK (g_dataset_global);
      if (dataset)
	{
	  register GData *list, *next;
	  
	  for (list = dataset->datalist; list; list = next)
	    {
	      next = list->next;
	      func (list->id, list->data, user_data);
	    }
	}
    }
  else
    {
      G_UNLOCK (g_dataset_global);
    }
}

void
g_datalist_foreach (GData	   **datalist,
		    GDataForeachFunc func,
		    gpointer         user_data)
{
  register GData *list, *next;

  g_return_if_fail (datalist != NULL);
  g_return_if_fail (func != NULL);
  
  for (list = G_DATALIST_GET_POINTER (datalist); list; list = next)
    {
      next = list->next;
      func (list->id, list->data, user_data);
    }
}

void
g_datalist_init (GData **datalist)
{
  g_return_if_fail (datalist != NULL);

  g_atomic_pointer_set (datalist, NULL);
}

/**
 * g_datalist_set_flags:
 * @datalist: pointer to the location that holds a list
 * @flags: the flags to turn on. The values of the flags are
 *   restricted by %G_DATALIST_FLAGS_MASK (currently
 *   3; giving two possible boolean flags).
 *   A value for @flags that doesn't fit within the mask is
 *   an error.
 * 
 * Turns on flag values for a data list. This function is used
 * to keep a small number of boolean flags in an object with
 * a data list without using any additional space. It is
 * not generally useful except in circumstances where space
 * is very tight. (It is used in the base #GObject type, for
 * example.)
 *
 * Since: 2.8
 **/
void
g_datalist_set_flags (GData **datalist,
		      guint   flags)
{
  gpointer oldvalue;
  g_return_if_fail (datalist != NULL);
  g_return_if_fail ((flags & ~G_DATALIST_FLAGS_MASK) == 0);
  
  do
    {
      oldvalue = g_atomic_pointer_get (datalist);
    }
  while (!g_atomic_pointer_compare_and_exchange ((void**) datalist, oldvalue,
                                                 (gpointer) ((gsize) oldvalue | flags)));
}

/**
 * g_datalist_unset_flags:
 * @datalist: pointer to the location that holds a list
 * @flags: the flags to turn off. The values of the flags are
 *   restricted by %G_DATALIST_FLAGS_MASK (currently
 *   3: giving two possible boolean flags).
 *   A value for @flags that doesn't fit within the mask is
 *   an error.
 * 
 * Turns off flag values for a data list. See g_datalist_unset_flags()
 *
 * Since: 2.8
 **/
void
g_datalist_unset_flags (GData **datalist,
			guint   flags)
{
  gpointer oldvalue;
  g_return_if_fail (datalist != NULL);
  g_return_if_fail ((flags & ~G_DATALIST_FLAGS_MASK) == 0);
  
  do
    {
      oldvalue = g_atomic_pointer_get (datalist);
    }
  while (!g_atomic_pointer_compare_and_exchange ((void**) datalist, oldvalue,
                                                 (gpointer) ((gsize) oldvalue & ~(gsize) flags)));
}

/**
 * g_datalist_get_flags:
 * @datalist: pointer to the location that holds a list
 * 
 * Gets flags values packed in together with the datalist.
 * See g_datalist_set_flags().
 * 
 * Return value: the flags of the datalist
 *
 * Since: 2.8
 **/
guint
g_datalist_get_flags (GData **datalist)
{
  g_return_val_if_fail (datalist != NULL, 0);
  
  return G_DATALIST_GET_FLAGS (datalist); /* atomic macro */
}

/* HOLDS: g_dataset_global_lock */
static void
g_data_initialize (void)
{
  g_return_if_fail (g_dataset_location_ht == NULL);

  g_dataset_location_ht = g_hash_table_new (g_direct_hash, NULL);
  g_dataset_cached = NULL;
}

GQuark
g_quark_try_string (const gchar *string)
{
  GQuark quark = 0;
  g_return_val_if_fail (string != NULL, 0);
  
  G_LOCK (g_quark_global);
  if (g_quark_ht)
    quark = GPOINTER_TO_UINT (g_hash_table_lookup (g_quark_ht, string));
  G_UNLOCK (g_quark_global);
  
  return quark;
}

/* HOLDS: g_quark_global_lock */
static inline GQuark
g_quark_from_string_internal (const gchar *string, 
			      gboolean     duplicate)
{
  GQuark quark = 0;
  
  if (g_quark_ht)
    quark = GPOINTER_TO_UINT (g_hash_table_lookup (g_quark_ht, string));
  
  if (!quark)
    quark = g_quark_new (duplicate ? g_strdup (string) : (gchar *)string);
  
  return quark;
}

GQuark
g_quark_from_string (const gchar *string)
{
  GQuark quark;
  
  if (!string)
    return 0;
  
  G_LOCK (g_quark_global);
  quark = g_quark_from_string_internal (string, TRUE);
  G_UNLOCK (g_quark_global);
  
  return quark;
}

GQuark
g_quark_from_static_string (const gchar *string)
{
  GQuark quark;
  
  if (!string)
    return 0;
  
  G_LOCK (g_quark_global);
  quark = g_quark_from_string_internal (string, FALSE);
  G_UNLOCK (g_quark_global);

  return quark;
}

G_CONST_RETURN gchar*
g_quark_to_string (GQuark quark)
{
  gchar* result = NULL;

  G_LOCK (g_quark_global);
  if (quark < g_quark_seq_id)
    result = g_quarks[quark];
  G_UNLOCK (g_quark_global);

  return result;
}

/* HOLDS: g_quark_global_lock */
static inline GQuark
g_quark_new (gchar *string)
{
  GQuark quark;
  
  if (g_quark_seq_id % G_QUARK_BLOCK_SIZE == 0)
    g_quarks = g_renew (gchar*, g_quarks, g_quark_seq_id + G_QUARK_BLOCK_SIZE);
  if (!g_quark_ht)
    {
      g_assert (g_quark_seq_id == 0);
      g_quark_ht = g_hash_table_new (g_str_hash, g_str_equal);
      g_quarks[g_quark_seq_id++] = NULL;
    }

  quark = g_quark_seq_id++;
  g_quarks[quark] = string;
  g_hash_table_insert (g_quark_ht, string, GUINT_TO_POINTER (quark));
  
  return quark;
}

/**
 * g_intern_string:
 * @string: a string
 * 
 * Returns a canonical representation for @string. Interned strings can
 * be compared for equality by comparing the pointers, instead of using strcmp().
 * 
 * Returns: a canonical representation for the string
 *
 * Since: 2.10
 */
G_CONST_RETURN gchar*
g_intern_string (const gchar *string)
{
  const gchar *result;
  GQuark quark;

  if (!string)
    return NULL;

  G_LOCK (g_quark_global);
  quark = g_quark_from_string_internal (string, TRUE);
  result = g_quarks[quark];
  G_UNLOCK (g_quark_global);

  return result;
}

/**
 * g_intern_static_string:
 * @string: a static string
 * 
 * Returns a canonical representation for @string. Interned strings can
 * be compared for equality by comparing the pointers, instead of using strcmp().
 * g_intern_static_string() does not copy the string, therefore @string must
 * not be freed or modified. 
 * 
 * Returns: a canonical representation for the string
 *
 * Since: 2.10
 */
G_CONST_RETURN gchar*
g_intern_static_string (const gchar *string)
{
  GQuark quark;
  const gchar *result;

  if (!string)
    return NULL;

  G_LOCK (g_quark_global);
  quark = g_quark_from_string_internal (string, FALSE);
  result = g_quarks[quark];
  G_UNLOCK (g_quark_global);

  return result;
}



#define __G_DATASET_C__
#include "galiasdef.c"
