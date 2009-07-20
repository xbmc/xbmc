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


void g_slist_push_allocator (gpointer dummy) { /* present for binary compat only */ }
void g_slist_pop_allocator  (void)           { /* present for binary compat only */ }

#define _g_slist_alloc0()       g_slice_new0 (GSList)
#define _g_slist_alloc()        g_slice_new (GSList)
#define _g_slist_free1(slist)   g_slice_free (GSList, slist)

GSList*
g_slist_alloc (void)
{
  return _g_slist_alloc0 ();
}

/**
 * g_slist_free:
 * @list: a #GSList
 *
 * Frees all of the memory used by a #GSList.
 * The freed elements are returned to the slice allocator.
 */
void
g_slist_free (GSList *list)
{
  g_slice_free_chain (GSList, list, next);
}

/**
 * g_slist_free_1:
 * @list: a #GSList element
 * 
 * Frees one #GSList element.
 * It is usually used after g_slist_remove_link().
 */
void
g_slist_free_1 (GSList *list)
{
  _g_slist_free1 (list);
}

/**
 * g_slist_append:
 * @list: a #GSList
 * @data: the data for the new element
 *
 * Adds a new element on to the end of the list.
 *
 * <note><para>
 * The return value is the new start of the list, which may 
 * have changed, so make sure you store the new value.
 * </para></note>
 *
 * <note><para>
 * Note that g_slist_append() has to traverse the entire list 
 * to find the end, which is inefficient when adding multiple 
 * elements. A common idiom to avoid the inefficiency is to prepend 
 * the elements and reverse the list when all elements have been added.
 * </para></note>
 *
 * |[
 * /&ast; Notice that these are initialized to the empty list. &ast;/
 * GSList *list = NULL, *number_list = NULL;
 *
 * /&ast; This is a list of strings. &ast;/
 * list = g_slist_append (list, "first");
 * list = g_slist_append (list, "second");
 *
 * /&ast; This is a list of integers. &ast;/
 * number_list = g_slist_append (number_list, GINT_TO_POINTER (27));
 * number_list = g_slist_append (number_list, GINT_TO_POINTER (14));
 * ]|
 *
 * Returns: the new start of the #GSList
 */
GSList*
g_slist_append (GSList   *list,
		gpointer  data)
{
  GSList *new_list;
  GSList *last;

  new_list = _g_slist_alloc ();
  new_list->data = data;
  new_list->next = NULL;

  if (list)
    {
      last = g_slist_last (list);
      /* g_assert (last != NULL); */
      last->next = new_list;

      return list;
    }
  else
    return new_list;
}

/**
 * g_slist_prepend:
 * @list: a #GSList
 * @data: the data for the new element
 *
 * Adds a new element on to the start of the list.
 *
 * <note><para>
 * The return value is the new start of the list, which 
 * may have changed, so make sure you store the new value.
 * </para></note>
 *
 * |[
 * /&ast; Notice that it is initialized to the empty list. &ast;/
 * GSList *list = NULL;
 * list = g_slist_prepend (list, "last");
 * list = g_slist_prepend (list, "first");
 * ]|
 *
 * Returns: the new start of the #GSList
 */
GSList*
g_slist_prepend (GSList   *list,
		 gpointer  data)
{
  GSList *new_list;

  new_list = _g_slist_alloc ();
  new_list->data = data;
  new_list->next = list;

  return new_list;
}

/**
 * g_slist_insert:
 * @list: a #GSList
 * @data: the data for the new element
 * @position: the position to insert the element. 
 *     If this is negative, or is larger than the number 
 *     of elements in the list, the new element is added on
 *     to the end of the list.
 *
 * Inserts a new element into the list at the given position.
 *
 * Returns: the new start of the #GSList
 */
GSList*
g_slist_insert (GSList   *list,
		gpointer  data,
		gint      position)
{
  GSList *prev_list;
  GSList *tmp_list;
  GSList *new_list;

  if (position < 0)
    return g_slist_append (list, data);
  else if (position == 0)
    return g_slist_prepend (list, data);

  new_list = _g_slist_alloc ();
  new_list->data = data;

  if (!list)
    {
      new_list->next = NULL;
      return new_list;
    }

  prev_list = NULL;
  tmp_list = list;

  while ((position-- > 0) && tmp_list)
    {
      prev_list = tmp_list;
      tmp_list = tmp_list->next;
    }

  if (prev_list)
    {
      new_list->next = prev_list->next;
      prev_list->next = new_list;
    }
  else
    {
      new_list->next = list;
      list = new_list;
    }

  return list;
}

/**
 * g_slist_insert_before:
 * @slist: a #GSList
 * @sibling: node to insert @data before
 * @data: data to put in the newly-inserted node
 *
 * Inserts a node before @sibling containing @data. 
 * 
 * Returns: the new head of the list.
 */
GSList*
g_slist_insert_before (GSList  *slist,
		       GSList  *sibling,
		       gpointer data)
{
  if (!slist)
    {
      slist = _g_slist_alloc ();
      slist->data = data;
      slist->next = NULL;
      g_return_val_if_fail (sibling == NULL, slist);
      return slist;
    }
  else
    {
      GSList *node, *last = NULL;

      for (node = slist; node; last = node, node = last->next)
	if (node == sibling)
	  break;
      if (!last)
	{
	  node = _g_slist_alloc ();
	  node->data = data;
	  node->next = slist;

	  return node;
	}
      else
	{
	  node = _g_slist_alloc ();
	  node->data = data;
	  node->next = last->next;
	  last->next = node;

	  return slist;
	}
    }
}

/**
 * g_slist_concat:
 * @list1: a #GSList
 * @list2: the #GSList to add to the end of the first #GSList
 *
 * Adds the second #GSList onto the end of the first #GSList.
 * Note that the elements of the second #GSList are not copied.
 * They are used directly.
 *
 * Returns: the start of the new #GSList
 */
GSList *
g_slist_concat (GSList *list1, GSList *list2)
{
  if (list2)
    {
      if (list1)
	g_slist_last (list1)->next = list2;
      else
	list1 = list2;
    }

  return list1;
}

/**
 * g_slist_remove:
 * @list: a #GSList
 * @data: the data of the element to remove
 *
 * Removes an element from a #GSList.
 * If two elements contain the same data, only the first is removed.
 * If none of the elements contain the data, the #GSList is unchanged.
 *
 * Returns: the new start of the #GSList
 */
GSList*
g_slist_remove (GSList        *list,
		gconstpointer  data)
{
  GSList *tmp, *prev = NULL;

  tmp = list;
  while (tmp)
    {
      if (tmp->data == data)
	{
	  if (prev)
	    prev->next = tmp->next;
	  else
	    list = tmp->next;

	  g_slist_free_1 (tmp);
	  break;
	}
      prev = tmp;
      tmp = prev->next;
    }

  return list;
}

/**
 * g_slist_remove_all:
 * @list: a #GSList
 * @data: data to remove
 *
 * Removes all list nodes with data equal to @data. 
 * Returns the new head of the list. Contrast with 
 * g_slist_remove() which removes only the first node 
 * matching the given data.
 *
 * Returns: new head of @list
 */
GSList*
g_slist_remove_all (GSList        *list,
		    gconstpointer  data)
{
  GSList *tmp, *prev = NULL;

  tmp = list;
  while (tmp)
    {
      if (tmp->data == data)
	{
	  GSList *next = tmp->next;

	  if (prev)
	    prev->next = next;
	  else
	    list = next;
	  
	  g_slist_free_1 (tmp);
	  tmp = next;
	}
      else
	{
	  prev = tmp;
	  tmp = prev->next;
	}
    }

  return list;
}

static inline GSList*
_g_slist_remove_link (GSList *list,
		      GSList *link)
{
  GSList *tmp;
  GSList *prev;

  prev = NULL;
  tmp = list;

  while (tmp)
    {
      if (tmp == link)
	{
	  if (prev)
	    prev->next = tmp->next;
	  if (list == tmp)
	    list = list->next;

	  tmp->next = NULL;
	  break;
	}

      prev = tmp;
      tmp = tmp->next;
    }

  return list;
}

/**
 * g_slist_remove_link:
 * @list: a #GSList
 * @link_: an element in the #GSList
 *
 * Removes an element from a #GSList, without 
 * freeing the element. The removed element's next 
 * link is set to %NULL, so that it becomes a
 * self-contained list with one element.
 *
 * Returns: the new start of the #GSList, without the element
 */
GSList* 
g_slist_remove_link (GSList *list,
		     GSList *link_)
{
  return _g_slist_remove_link (list, link_);
}

/**
 * g_slist_delete_link:
 * @list: a #GSList
 * @link_: node to delete
 *
 * Removes the node link_ from the list and frees it. 
 * Compare this to g_slist_remove_link() which removes the node 
 * without freeing it.
 *
 * Returns: the new head of @list
 */
GSList*
g_slist_delete_link (GSList *list,
		     GSList *link_)
{
  list = _g_slist_remove_link (list, link_);
  _g_slist_free1 (link_);

  return list;
}

/**
 * g_slist_copy:
 * @list: a #GSList
 * 
 * Copies a #GSList.
 * 
 * <note><para>
 * Note that this is a "shallow" copy. If the list elements 
 * consist of pointers to data, the pointers are copied but 
 * the actual data isn't.
 * </para></note>
 *
 * Returns: a copy of @list
 */
GSList*
g_slist_copy (GSList *list)
{
  GSList *new_list = NULL;

  if (list)
    {
      GSList *last;

      new_list = _g_slist_alloc ();
      new_list->data = list->data;
      last = new_list;
      list = list->next;
      while (list)
	{
	  last->next = _g_slist_alloc ();
	  last = last->next;
	  last->data = list->data;
	  list = list->next;
	}
      last->next = NULL;
    }

  return new_list;
}

/**
 * g_slist_reverse:
 * @list: a #GSList
 *
 * Reverses a #GSList.
 *
 * Returns: the start of the reversed #GSList
 */
GSList*
g_slist_reverse (GSList *list)
{
  GSList *prev = NULL;
  
  while (list)
    {
      GSList *next = list->next;

      list->next = prev;
      
      prev = list;
      list = next;
    }
  
  return prev;
}

/**
 * g_slist_nth:
 * @list: a #GSList
 * @n: the position of the element, counting from 0
 *
 * Gets the element at the given position in a #GSList.
 *
 * Returns: the element, or %NULL if the position is off 
 *     the end of the #GSList
 */
GSList*
g_slist_nth (GSList *list,
	     guint   n)
{
  while (n-- > 0 && list)
    list = list->next;

  return list;
}

/**
 * g_slist_nth_data:
 * @list: a #GSList
 * @n: the position of the element
 *
 * Gets the data of the element at the given position.
 *
 * Returns: the element's data, or %NULL if the position 
 *     is off the end of the #GSList
 */
gpointer
g_slist_nth_data (GSList   *list,
		  guint     n)
{
  while (n-- > 0 && list)
    list = list->next;

  return list ? list->data : NULL;
}

/**
 * g_slist_find:
 * @list: a #GSList
 * @data: the element data to find
 *
 * Finds the element in a #GSList which 
 * contains the given data.
 *
 * Returns: the found #GSList element, 
 *     or %NULL if it is not found
 */
GSList*
g_slist_find (GSList        *list,
	      gconstpointer  data)
{
  while (list)
    {
      if (list->data == data)
	break;
      list = list->next;
    }

  return list;
}


/**
 * g_slist_find_custom:
 * @list: a #GSList
 * @data: user data passed to the function
 * @func: the function to call for each element. 
 *     It should return 0 when the desired element is found
 *
 * Finds an element in a #GSList, using a supplied function to 
 * find the desired element. It iterates over the list, calling 
 * the given function which should return 0 when the desired 
 * element is found. The function takes two #gconstpointer arguments, 
 * the #GSList element's data as the first argument and the 
 * given user data.
 *
 * Returns: the found #GSList element, or %NULL if it is not found
 */
GSList*
g_slist_find_custom (GSList        *list,
		     gconstpointer  data,
		     GCompareFunc   func)
{
  g_return_val_if_fail (func != NULL, list);

  while (list)
    {
      if (! func (list->data, data))
	return list;
      list = list->next;
    }

  return NULL;
}

/**
 * g_slist_position:
 * @list: a #GSList
 * @llink: an element in the #GSList
 *
 * Gets the position of the given element 
 * in the #GSList (starting from 0).
 *
 * Returns: the position of the element in the #GSList, 
 *     or -1 if the element is not found
 */
gint
g_slist_position (GSList *list,
		  GSList *llink)
{
  gint i;

  i = 0;
  while (list)
    {
      if (list == llink)
	return i;
      i++;
      list = list->next;
    }

  return -1;
}

/**
 * g_slist_index:
 * @list: a #GSList
 * @data: the data to find
 *
 * Gets the position of the element containing 
 * the given data (starting from 0).
 *
 * Returns: the index of the element containing the data, 
 *     or -1 if the data is not found
 */
gint
g_slist_index (GSList        *list,
	       gconstpointer  data)
{
  gint i;

  i = 0;
  while (list)
    {
      if (list->data == data)
	return i;
      i++;
      list = list->next;
    }

  return -1;
}

/**
 * g_slist_last:
 * @list: a #GSList 
 *
 * Gets the last element in a #GSList.
 *  
 * <note><para>
 * This function iterates over the whole list.
 * </para></note>
 *
 * Returns: the last element in the #GSList, 
 *     or %NULL if the #GSList has no elements
 */
GSList*
g_slist_last (GSList *list)
{
  if (list)
    {
      while (list->next)
	list = list->next;
    }

  return list;
}

/**
 * g_slist_length:
 * @list: a #GSList
 *
 * Gets the number of elements in a #GSList.
 *
 * <note><para>
 * This function iterates over the whole list to 
 * count its elements.
 * </para></note>
 *
 * Returns: the number of elements in the #GSList
 */
guint
g_slist_length (GSList *list)
{
  guint length;

  length = 0;
  while (list)
    {
      length++;
      list = list->next;
    }

  return length;
}

/**
 * g_slist_foreach:
 * @list: a #GSList
 * @func: the function to call with each element's data
 * @user_data: user data to pass to the function
 *
 * Calls a function for each element of a #GSList.
 */
void
g_slist_foreach (GSList   *list,
		 GFunc     func,
		 gpointer  user_data)
{
  while (list)
    {
      GSList *next = list->next;
      (*func) (list->data, user_data);
      list = next;
    }
}

static GSList*
g_slist_insert_sorted_real (GSList   *list,
			    gpointer  data,
			    GFunc     func,
			    gpointer  user_data)
{
  GSList *tmp_list = list;
  GSList *prev_list = NULL;
  GSList *new_list;
  gint cmp;
 
  g_return_val_if_fail (func != NULL, list);

  if (!list)
    {
      new_list = _g_slist_alloc ();
      new_list->data = data;
      new_list->next = NULL;
      return new_list;
    }
 
  cmp = ((GCompareDataFunc) func) (data, tmp_list->data, user_data);
 
  while ((tmp_list->next) && (cmp > 0))
    {
      prev_list = tmp_list;
      tmp_list = tmp_list->next;

      cmp = ((GCompareDataFunc) func) (data, tmp_list->data, user_data);
    }

  new_list = _g_slist_alloc ();
  new_list->data = data;

  if ((!tmp_list->next) && (cmp > 0))
    {
      tmp_list->next = new_list;
      new_list->next = NULL;
      return list;
    }
  
  if (prev_list)
    {
      prev_list->next = new_list;
      new_list->next = tmp_list;
      return list;
    }
  else
    {
      new_list->next = list;
      return new_list;
    }
}

/**
 * g_slist_insert_sorted:
 * @list: a #GSList
 * @data: the data for the new element
 * @func: the function to compare elements in the list. 
 *     It should return a number > 0 if the first parameter 
 *     comes after the second parameter in the sort order.
 *
 * Inserts a new element into the list, using the given 
 * comparison function to determine its position.
 *
 * Returns: the new start of the #GSList
 */
GSList*
g_slist_insert_sorted (GSList       *list,
                       gpointer      data,
                       GCompareFunc  func)
{
  return g_slist_insert_sorted_real (list, data, (GFunc) func, NULL);
}

/**
 * g_slist_insert_sorted_with_data:
 * @list: a #GSList
 * @data: the data for the new element
 * @func: the function to compare elements in the list. 
 *     It should return a number > 0 if the first parameter 
 *     comes after the second parameter in the sort order.
 * @user_data: data to pass to comparison function
 *
 * Inserts a new element into the list, using the given 
 * comparison function to determine its position.
 *
 * Returns: the new start of the #GSList
 *
 * Since: 2.10
 */
GSList*
g_slist_insert_sorted_with_data (GSList           *list,
				 gpointer          data,
				 GCompareDataFunc  func,
				 gpointer          user_data)
{
  return g_slist_insert_sorted_real (list, data, (GFunc) func, user_data);
}

static GSList *
g_slist_sort_merge (GSList   *l1, 
		    GSList   *l2,
		    GFunc     compare_func,
		    gpointer  user_data)
{
  GSList list, *l;
  gint cmp;

  l=&list;

  while (l1 && l2)
    {
      cmp = ((GCompareDataFunc) compare_func) (l1->data, l2->data, user_data);

      if (cmp <= 0)
        {
	  l=l->next=l1;
	  l1=l1->next;
        } 
      else 
	{
	  l=l->next=l2;
	  l2=l2->next;
        }
    }
  l->next= l1 ? l1 : l2;
  
  return list.next;
}

static GSList *
g_slist_sort_real (GSList   *list,
		   GFunc     compare_func,
		   gpointer  user_data)
{
  GSList *l1, *l2;

  if (!list) 
    return NULL;
  if (!list->next) 
    return list;

  l1 = list; 
  l2 = list->next;

  while ((l2 = l2->next) != NULL)
    {
      if ((l2 = l2->next) == NULL) 
	break;
      l1=l1->next;
    }
  l2 = l1->next; 
  l1->next = NULL;

  return g_slist_sort_merge (g_slist_sort_real (list, compare_func, user_data),
			     g_slist_sort_real (l2, compare_func, user_data),
			     compare_func,
			     user_data);
}

/**
 * g_slist_sort:
 * @list: a #GSList
 * @compare_func: the comparison function used to sort the #GSList.
 *     This function is passed the data from 2 elements of the #GSList 
 *     and should return 0 if they are equal, a negative value if the 
 *     first element comes before the second, or a positive value if 
 *     the first element comes after the second.
 *
 * Sorts a #GSList using the given comparison function.
 *
 * Returns: the start of the sorted #GSList
 */
GSList *
g_slist_sort (GSList       *list,
	      GCompareFunc  compare_func)
{
  return g_slist_sort_real (list, (GFunc) compare_func, NULL);
}

/**
 * g_slist_sort_with_data:
 * @list: a #GSList
 * @compare_func: comparison function
 * @user_data: data to pass to comparison function
 *
 * Like g_slist_sort(), but the sort function accepts a user data argument.
 *
 * Returns: new head of the list
 */
GSList *
g_slist_sort_with_data (GSList           *list,
			GCompareDataFunc  compare_func,
			gpointer          user_data)
{
  return g_slist_sort_real (list, (GFunc) compare_func, user_data);
}

#define __G_SLIST_C__
#include "galiasdef.c"
