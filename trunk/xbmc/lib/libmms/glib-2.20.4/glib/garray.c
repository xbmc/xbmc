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

#include <string.h>
#include <stdlib.h>

#include "garray.h"

#include "gmem.h"
#include "gthread.h"
#include "gmessages.h"
#include "gqsort.h"

#include "galias.h"


#define MIN_ARRAY_SIZE  16

typedef struct _GRealArray  GRealArray;

struct _GRealArray
{
  guint8 *data;
  guint   len;
  guint   alloc;
  guint   elt_size;
  guint   zero_terminated : 1;
  guint   clear : 1;
};

#define g_array_elt_len(array,i) ((array)->elt_size * (i))
#define g_array_elt_pos(array,i) ((array)->data + g_array_elt_len((array),(i)))
#define g_array_elt_zero(array, pos, len) 				\
  (memset (g_array_elt_pos ((array), pos), 0,  g_array_elt_len ((array), len)))
#define g_array_zero_terminate(array) G_STMT_START{			\
  if ((array)->zero_terminated)						\
    g_array_elt_zero ((array), (array)->len, 1);			\
}G_STMT_END

static gint g_nearest_pow        (gint        num) G_GNUC_CONST;
static void g_array_maybe_expand (GRealArray *array,
				  gint        len);

GArray*
g_array_new (gboolean zero_terminated,
	     gboolean clear,
	     guint    elt_size)
{
  return (GArray*) g_array_sized_new (zero_terminated, clear, elt_size, 0);
}

GArray* g_array_sized_new (gboolean zero_terminated,
			   gboolean clear,
			   guint    elt_size,
			   guint    reserved_size)
{
  GRealArray *array = g_slice_new (GRealArray);

  array->data            = NULL;
  array->len             = 0;
  array->alloc           = 0;
  array->zero_terminated = (zero_terminated ? 1 : 0);
  array->clear           = (clear ? 1 : 0);
  array->elt_size        = elt_size;

  if (array->zero_terminated || reserved_size != 0)
    {
      g_array_maybe_expand (array, reserved_size);
      g_array_zero_terminate(array);
    }

  return (GArray*) array;
}

gchar*
g_array_free (GArray   *array,
	      gboolean  free_segment)
{
  gchar* segment;

  g_return_val_if_fail (array, NULL);

  if (free_segment)
    {
      g_free (array->data);
      segment = NULL;
    }
  else
    segment = array->data;

  g_slice_free1 (sizeof (GRealArray), array);

  return segment;
}

GArray*
g_array_append_vals (GArray       *farray,
		     gconstpointer data,
		     guint         len)
{
  GRealArray *array = (GRealArray*) farray;

  g_array_maybe_expand (array, len);

  memcpy (g_array_elt_pos (array, array->len), data, 
	  g_array_elt_len (array, len));

  array->len += len;

  g_array_zero_terminate (array);

  return farray;
}

GArray*
g_array_prepend_vals (GArray        *farray,
		      gconstpointer  data,
		      guint          len)
{
  GRealArray *array = (GRealArray*) farray;

  g_array_maybe_expand (array, len);

  g_memmove (g_array_elt_pos (array, len), g_array_elt_pos (array, 0), 
	     g_array_elt_len (array, array->len));

  memcpy (g_array_elt_pos (array, 0), data, g_array_elt_len (array, len));

  array->len += len;

  g_array_zero_terminate (array);

  return farray;
}

GArray*
g_array_insert_vals (GArray        *farray,
		     guint          index_,
		     gconstpointer  data,
		     guint          len)
{
  GRealArray *array = (GRealArray*) farray;

  g_array_maybe_expand (array, len);

  g_memmove (g_array_elt_pos (array, len + index_), 
	     g_array_elt_pos (array, index_), 
	     g_array_elt_len (array, array->len - index_));

  memcpy (g_array_elt_pos (array, index_), data, g_array_elt_len (array, len));

  array->len += len;

  g_array_zero_terminate (array);

  return farray;
}

GArray*
g_array_set_size (GArray *farray,
		  guint   length)
{
  GRealArray *array = (GRealArray*) farray;
  if (length > array->len)
    {
      g_array_maybe_expand (array, length - array->len);
      
      if (array->clear)
	g_array_elt_zero (array, array->len, length - array->len);
    }
  else if (G_UNLIKELY (g_mem_gc_friendly) && length < array->len)
    g_array_elt_zero (array, length, array->len - length);
  
  array->len = length;
  
  g_array_zero_terminate (array);
  
  return farray;
}

GArray*
g_array_remove_index (GArray *farray,
		      guint   index_)
{
  GRealArray* array = (GRealArray*) farray;

  g_return_val_if_fail (array, NULL);

  g_return_val_if_fail (index_ < array->len, NULL);

  if (index_ != array->len - 1)
    g_memmove (g_array_elt_pos (array, index_),
	       g_array_elt_pos (array, index_ + 1),
	       g_array_elt_len (array, array->len - index_ - 1));
  
  array->len -= 1;

  if (G_UNLIKELY (g_mem_gc_friendly))
    g_array_elt_zero (array, array->len, 1);
  else
    g_array_zero_terminate (array);

  return farray;
}

GArray*
g_array_remove_index_fast (GArray *farray,
			   guint   index_)
{
  GRealArray* array = (GRealArray*) farray;

  g_return_val_if_fail (array, NULL);

  g_return_val_if_fail (index_ < array->len, NULL);

  if (index_ != array->len - 1)
    memcpy (g_array_elt_pos (array, index_), 
	    g_array_elt_pos (array, array->len - 1),
	    g_array_elt_len (array, 1));
  
  array->len -= 1;

  if (G_UNLIKELY (g_mem_gc_friendly))
    g_array_elt_zero (array, array->len, 1);
  else
    g_array_zero_terminate (array);

  return farray;
}

GArray*
g_array_remove_range (GArray *farray,
                      guint   index_,
                      guint   length)
{
  GRealArray *array = (GRealArray*) farray;

  g_return_val_if_fail (array, NULL);
  g_return_val_if_fail (index_ < array->len, NULL);
  g_return_val_if_fail (index_ + length <= array->len, NULL);

  if (index_ + length != array->len)
    g_memmove (g_array_elt_pos (array, index_), 
               g_array_elt_pos (array, index_ + length), 
               (array->len - (index_ + length)) * array->elt_size);

  array->len -= length;
  if (G_UNLIKELY (g_mem_gc_friendly))
    g_array_elt_zero (array, array->len, length);
  else
    g_array_zero_terminate (array);

  return farray;
}

void
g_array_sort (GArray       *farray,
	      GCompareFunc  compare_func)
{
  GRealArray *array = (GRealArray*) farray;

  g_return_if_fail (array != NULL);

  qsort (array->data,
	 array->len,
	 array->elt_size,
	 compare_func);
}

void
g_array_sort_with_data (GArray           *farray,
			GCompareDataFunc  compare_func,
			gpointer          user_data)
{
  GRealArray *array = (GRealArray*) farray;

  g_return_if_fail (array != NULL);

  g_qsort_with_data (array->data,
		     array->len,
		     array->elt_size,
		     compare_func,
		     user_data);
}


static gint
g_nearest_pow (gint num)
{
  gint n = 1;

  while (n < num)
    n <<= 1;

  return n;
}

static void
g_array_maybe_expand (GRealArray *array,
		      gint        len)
{
  guint want_alloc = g_array_elt_len (array, array->len + len + 
				      array->zero_terminated);

  if (want_alloc > array->alloc)
    {
      want_alloc = g_nearest_pow (want_alloc);
      want_alloc = MAX (want_alloc, MIN_ARRAY_SIZE);

      array->data = g_realloc (array->data, want_alloc);

      if (G_UNLIKELY (g_mem_gc_friendly))
        memset (array->data + array->alloc, 0, want_alloc - array->alloc);

      array->alloc = want_alloc;
    }
}

/* Pointer Array
 */

typedef struct _GRealPtrArray  GRealPtrArray;

struct _GRealPtrArray
{
  gpointer *pdata;
  guint     len;
  guint     alloc;
};

static void g_ptr_array_maybe_expand (GRealPtrArray *array,
				      gint           len);

GPtrArray*
g_ptr_array_new (void)
{
  return g_ptr_array_sized_new (0);
}

GPtrArray*  
g_ptr_array_sized_new (guint reserved_size)
{
  GRealPtrArray *array = g_slice_new (GRealPtrArray);

  array->pdata = NULL;
  array->len = 0;
  array->alloc = 0;

  if (reserved_size != 0)
    g_ptr_array_maybe_expand (array, reserved_size);

  return (GPtrArray*) array;  
}

gpointer*
g_ptr_array_free (GPtrArray *array,
		  gboolean   free_segment)
{
  gpointer* segment;

  g_return_val_if_fail (array, NULL);

  if (free_segment)
    {
      g_free (array->pdata);
      segment = NULL;
    }
  else
    segment = array->pdata;

  g_slice_free1 (sizeof (GRealPtrArray), array);

  return segment;
}

static void
g_ptr_array_maybe_expand (GRealPtrArray *array,
			  gint           len)
{
  if ((array->len + len) > array->alloc)
    {
      guint old_alloc = array->alloc;
      array->alloc = g_nearest_pow (array->len + len);
      array->alloc = MAX (array->alloc, MIN_ARRAY_SIZE);
      array->pdata = g_realloc (array->pdata, sizeof (gpointer) * array->alloc);
      if (G_UNLIKELY (g_mem_gc_friendly))
        for ( ; old_alloc < array->alloc; old_alloc++)
          array->pdata [old_alloc] = NULL;
    }
}

void
g_ptr_array_set_size  (GPtrArray *farray,
		       gint	  length)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;

  g_return_if_fail (array);

  if (length > array->len)
    {
      int i;
      g_ptr_array_maybe_expand (array, (length - array->len));
      /* This is not 
       *     memset (array->pdata + array->len, 0,
       *            sizeof (gpointer) * (length - array->len));
       * to make it really portable. Remember (void*)NULL needn't be
       * bitwise zero. It of course is silly not to use memset (..,0,..).
       */
      for (i = array->len; i < length; i++)
	array->pdata[i] = NULL;
    }
  if (G_UNLIKELY (g_mem_gc_friendly) && length < array->len)
    {
      int i;
      for (i = length; i < array->len; i++)
	array->pdata[i] = NULL;
    }

  array->len = length;
}

gpointer
g_ptr_array_remove_index (GPtrArray *farray,
			  guint      index_)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;
  gpointer result;

  g_return_val_if_fail (array, NULL);

  g_return_val_if_fail (index_ < array->len, NULL);

  result = array->pdata[index_];
  
  if (index_ != array->len - 1)
    g_memmove (array->pdata + index_, array->pdata + index_ + 1, 
	       sizeof (gpointer) * (array->len - index_ - 1));
  
  array->len -= 1;

  if (G_UNLIKELY (g_mem_gc_friendly))
    array->pdata[array->len] = NULL;

  return result;
}

gpointer
g_ptr_array_remove_index_fast (GPtrArray *farray,
			       guint      index_)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;
  gpointer result;

  g_return_val_if_fail (array, NULL);

  g_return_val_if_fail (index_ < array->len, NULL);

  result = array->pdata[index_];
  
  if (index_ != array->len - 1)
    array->pdata[index_] = array->pdata[array->len - 1];

  array->len -= 1;

  if (G_UNLIKELY (g_mem_gc_friendly))
    array->pdata[array->len] = NULL;

  return result;
}

void
g_ptr_array_remove_range (GPtrArray *farray,
                          guint      index_,
                          guint      length)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;

  g_return_if_fail (array);
  g_return_if_fail (index_ < array->len);
  g_return_if_fail (index_ + length <= array->len);

  if (index_ + length != array->len)
    g_memmove (&array->pdata[index_],
               &array->pdata[index_ + length], 
               (array->len - (index_ + length)) * sizeof (gpointer));

  array->len -= length;
  if (G_UNLIKELY (g_mem_gc_friendly))
    {
      guint i;
      for (i = 0; i < length; i++)
        array->pdata[array->len + i] = NULL;
    }
}

gboolean
g_ptr_array_remove (GPtrArray *farray,
		    gpointer   data)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;
  guint i;

  g_return_val_if_fail (array, FALSE);

  for (i = 0; i < array->len; i += 1)
    {
      if (array->pdata[i] == data)
	{
	  g_ptr_array_remove_index (farray, i);
	  return TRUE;
	}
    }

  return FALSE;
}

gboolean
g_ptr_array_remove_fast (GPtrArray *farray,
			 gpointer   data)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;
  guint i;

  g_return_val_if_fail (array, FALSE);

  for (i = 0; i < array->len; i += 1)
    {
      if (array->pdata[i] == data)
	{
	  g_ptr_array_remove_index_fast (farray, i);
	  return TRUE;
	}
    }

  return FALSE;
}

void
g_ptr_array_add (GPtrArray *farray,
		 gpointer   data)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;

  g_return_if_fail (array);

  g_ptr_array_maybe_expand (array, 1);

  array->pdata[array->len++] = data;
}

void
g_ptr_array_sort (GPtrArray    *array,
		  GCompareFunc  compare_func)
{
  g_return_if_fail (array != NULL);

  qsort (array->pdata,
	 array->len,
	 sizeof (gpointer),
	 compare_func);
}

void
g_ptr_array_sort_with_data (GPtrArray        *array,
			    GCompareDataFunc  compare_func,
			    gpointer          user_data)
{
  g_return_if_fail (array != NULL);

  g_qsort_with_data (array->pdata,
		     array->len,
		     sizeof (gpointer),
		     compare_func,
		     user_data);
}

/**
 * g_ptr_array_foreach:
 * @array: a #GPtrArray
 * @func: the function to call for each array element
 * @user_data: user data to pass to the function
 * 
 * Calls a function for each element of a #GPtrArray.
 *
 * Since: 2.4
 **/
void
g_ptr_array_foreach (GPtrArray *array,
                     GFunc      func,
                     gpointer   user_data)
{
  guint i;

  g_return_if_fail (array);

  for (i = 0; i < array->len; i++)
    (*func) (array->pdata[i], user_data);
}

/* Byte arrays 
 */

GByteArray* g_byte_array_new (void)
{
  return (GByteArray*) g_array_sized_new (FALSE, FALSE, 1, 0);
}

GByteArray* g_byte_array_sized_new (guint reserved_size)
{
  return (GByteArray*) g_array_sized_new (FALSE, FALSE, 1, reserved_size);
}

guint8*	    g_byte_array_free     (GByteArray *array,
			           gboolean    free_segment)
{
  return (guint8*) g_array_free ((GArray*) array, free_segment);
}

GByteArray* g_byte_array_append   (GByteArray   *array,
				   const guint8 *data,
				   guint         len)
{
  g_array_append_vals ((GArray*) array, (guint8*)data, len);

  return array;
}

GByteArray* g_byte_array_prepend  (GByteArray   *array,
				   const guint8 *data,
				   guint         len)
{
  g_array_prepend_vals ((GArray*) array, (guint8*)data, len);

  return array;
}

GByteArray* g_byte_array_set_size (GByteArray *array,
				   guint       length)
{
  g_array_set_size ((GArray*) array, length);

  return array;
}

GByteArray* g_byte_array_remove_index (GByteArray *array,
				       guint       index_)
{
  g_array_remove_index ((GArray*) array, index_);

  return array;
}

GByteArray* g_byte_array_remove_index_fast (GByteArray *array,
					    guint       index_)
{
  g_array_remove_index_fast ((GArray*) array, index_);

  return array;
}

GByteArray*
g_byte_array_remove_range (GByteArray *array,
                           guint       index_,
                           guint       length)
{
  g_return_val_if_fail (array, NULL);
  g_return_val_if_fail (index_ < array->len, NULL);
  g_return_val_if_fail (index_ + length <= array->len, NULL);

  return (GByteArray *)g_array_remove_range ((GArray*) array, index_, length);
}

void
g_byte_array_sort (GByteArray   *array,
		   GCompareFunc  compare_func)
{
  g_array_sort ((GArray *) array, compare_func);
}

void
g_byte_array_sort_with_data (GByteArray       *array,
			     GCompareDataFunc  compare_func,
			     gpointer          user_data)
{
  g_array_sort_with_data ((GArray *) array, compare_func, user_data);
}

#define __G_ARRAY_C__
#include "galiasdef.c"
