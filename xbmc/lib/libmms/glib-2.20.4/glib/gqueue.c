/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GQueue: Double ended queue implementation, piggy backed on GList.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * MT safe
 */

#include "config.h"

#include "glib.h"
#include "galias.h"

/**
 * g_queue_new:
 *
 * Creates a new #GQueue. 
 *
 * Returns: a new #GQueue.
 **/
GQueue*
g_queue_new (void)
{
  return g_slice_new0 (GQueue);
}

/**
 * g_queue_free:
 * @queue: a #GQueue.
 *
 * Frees the memory allocated for the #GQueue. Only call this function if
 * @queue was created with g_queue_new(). If queue elements contain
 * dynamically-allocated memory, they should be freed first.
 **/
void
g_queue_free (GQueue *queue)
{
  g_return_if_fail (queue != NULL);

  g_list_free (queue->head);
  g_slice_free (GQueue, queue);
}

/**
 * g_queue_init:
 * @queue: an uninitialized #GQueue
 *
 * A statically-allocated #GQueue must be initialized with this function
 * before it can be used. Alternatively you can initialize it with
 * #G_QUEUE_INIT. It is not necessary to initialize queues created with
 * g_queue_new().
 *
 * Since: 2.14
 **/
void
g_queue_init (GQueue *queue)
{
  g_return_if_fail (queue != NULL);

  queue->head = queue->tail = NULL;
  queue->length = 0;
}

/**
 * g_queue_clear:
 * @queue: a #GQueue
 *
 * Removes all the elements in @queue. If queue elements contain
 * dynamically-allocated memory, they should be freed first.
 *
 * Since: 2.14
 */
void
g_queue_clear (GQueue *queue)
{
  g_return_if_fail (queue != NULL);

  g_list_free (queue->head);
  g_queue_init (queue);
}

/**
 * g_queue_is_empty:
 * @queue: a #GQueue.
 *
 * Returns %TRUE if the queue is empty.
 *
 * Returns: %TRUE if the queue is empty.
 **/
gboolean
g_queue_is_empty (GQueue *queue)
{
  g_return_val_if_fail (queue != NULL, TRUE);

  return queue->head == NULL;
}

/**
 * g_queue_get_length:
 * @queue: a #GQueue
 * 
 * Returns the number of items in @queue.
 * 
 * Return value: The number of items in @queue.
 * 
 * Since: 2.4
 **/
guint
g_queue_get_length (GQueue *queue)
{
  g_return_val_if_fail (queue != NULL, 0);

  return queue->length;
}

/**
 * g_queue_reverse:
 * @queue: a #GQueue
 * 
 * Reverses the order of the items in @queue.
 * 
 * Since: 2.4
 **/
void
g_queue_reverse (GQueue *queue)
{
  g_return_if_fail (queue != NULL);

  queue->tail = queue->head;
  queue->head = g_list_reverse (queue->head);
}

/**
 * g_queue_copy:
 * @queue: a #GQueue
 * 
 * Copies a @queue. Note that is a shallow copy. If the elements in the
 * queue consist of pointers to data, the pointers are copied, but the
 * actual data is not.
 * 
 * Return value: A copy of @queue
 * 
 * Since: 2.4
 **/
GQueue *
g_queue_copy (GQueue *queue)
{
  GQueue *result;
  GList *list;

  g_return_val_if_fail (queue != NULL, NULL);

  result = g_queue_new ();

  for (list = queue->head; list != NULL; list = list->next)
    g_queue_push_tail (result, list->data);

  return result;
}

/**
 * g_queue_foreach:
 * @queue: a #GQueue
 * @func: the function to call for each element's data
 * @user_data: user data to pass to @func
 * 
 * Calls @func for each element in the queue passing @user_data to the
 * function.
 * 
 * Since: 2.4
 **/
void
g_queue_foreach (GQueue   *queue,
		 GFunc     func,
		 gpointer  user_data)
{
  GList *list;

  g_return_if_fail (queue != NULL);
  g_return_if_fail (func != NULL);
  
  list = queue->head;
  while (list)
    {
      GList *next = list->next;
      func (list->data, user_data);
      list = next;
    }
}

/**
 * g_queue_find:
 * @queue: a #GQueue
 * @data: data to find
 * 
 * Finds the first link in @queue which contains @data.
 * 
 * Return value: The first link in @queue which contains @data.
 * 
 * Since: 2.4
 **/
GList *
g_queue_find (GQueue        *queue,
	      gconstpointer  data)
{
  g_return_val_if_fail (queue != NULL, NULL);

  return g_list_find (queue->head, data);
}

/**
 * g_queue_find_custom:
 * @queue: a #GQueue
 * @data: user data passed to @func
 * @func: a #GCompareFunc to call for each element. It should return 0
 * when the desired element is found
 *
 * Finds an element in a #GQueue, using a supplied function to find the
 * desired element. It iterates over the queue, calling the given function
 * which should return 0 when the desired element is found. The function
 * takes two gconstpointer arguments, the #GQueue element's data as the
 * first argument and the given user data as the second argument.
 * 
 * Return value: The found link, or %NULL if it wasn't found
 * 
 * Since: 2.4
 **/
GList *
g_queue_find_custom    (GQueue        *queue,
			gconstpointer  data,
			GCompareFunc   func)
{
  g_return_val_if_fail (queue != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  return g_list_find_custom (queue->head, data, func);
}

/**
 * g_queue_sort:
 * @queue: a #GQueue
 * @compare_func: the #GCompareDataFunc used to sort @queue. This function
 *     is passed two elements of the queue and should return 0 if they are
 *     equal, a negative value if the first comes before the second, and
 *     a positive value if the second comes before the first.
 * @user_data: user data passed to @compare_func
 * 
 * Sorts @queue using @compare_func. 
 * 
 * Since: 2.4
 **/
void
g_queue_sort (GQueue           *queue,
	      GCompareDataFunc  compare_func,
	      gpointer          user_data)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (compare_func != NULL);

  queue->head = g_list_sort_with_data (queue->head, compare_func, user_data);
  queue->tail = g_list_last (queue->head);
}

/**
 * g_queue_push_head:
 * @queue: a #GQueue.
 * @data: the data for the new element.
 *
 * Adds a new element at the head of the queue.
 **/
void
g_queue_push_head (GQueue  *queue,
		   gpointer data)
{
  g_return_if_fail (queue != NULL);

  queue->head = g_list_prepend (queue->head, data);
  if (!queue->tail)
    queue->tail = queue->head;
  queue->length++;
}

/**
 * g_queue_push_nth:
 * @queue: a #GQueue
 * @data: the data for the new element
 * @n: the position to insert the new element. If @n is negative or
 *     larger than the number of elements in the @queue, the element is
 *     added to the end of the queue.
 * 
 * Inserts a new element into @queue at the given position
 * 
 * Since: 2.4
 **/
void
g_queue_push_nth (GQueue   *queue,
		  gpointer  data,
		  gint      n)
{
  g_return_if_fail (queue != NULL);

  if (n < 0 || n >= queue->length)
    {
      g_queue_push_tail (queue, data);
      return;
    }

  g_queue_insert_before (queue, g_queue_peek_nth_link (queue, n), data);
}

/**
 * g_queue_push_head_link:
 * @queue: a #GQueue.
 * @link_: a single #GList element, <emphasis>not</emphasis> a list with
 *     more than one element.
 *
 * Adds a new element at the head of the queue.
 **/
void
g_queue_push_head_link (GQueue *queue,
			GList  *link)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link != NULL);
  g_return_if_fail (link->prev == NULL);
  g_return_if_fail (link->next == NULL);

  link->next = queue->head;
  if (queue->head)
    queue->head->prev = link;
  else
    queue->tail = link;
  queue->head = link;
  queue->length++;
}

/**
 * g_queue_push_tail:
 * @queue: a #GQueue.
 * @data: the data for the new element.
 *
 * Adds a new element at the tail of the queue.
 **/
void
g_queue_push_tail (GQueue  *queue,
		   gpointer data)
{
  g_return_if_fail (queue != NULL);

  queue->tail = g_list_append (queue->tail, data);
  if (queue->tail->next)
    queue->tail = queue->tail->next;
  else
    queue->head = queue->tail;
  queue->length++;
}

/**
 * g_queue_push_tail_link:
 * @queue: a #GQueue.
 * @link_: a single #GList element, <emphasis>not</emphasis> a list with
 *   more than one element.
 *
 * Adds a new element at the tail of the queue.
 **/
void
g_queue_push_tail_link (GQueue *queue,
			GList  *link)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link != NULL);
  g_return_if_fail (link->prev == NULL);
  g_return_if_fail (link->next == NULL);

  link->prev = queue->tail;
  if (queue->tail)
    queue->tail->next = link;
  else
    queue->head = link;
  queue->tail = link;
  queue->length++;
}

/**
 * g_queue_push_nth_link:
 * @queue: a #GQueue
 * @n: the position to insert the link. If this is negative or larger than
 *     the number of elements in @queue, the link is added to the end of
 *     @queue.
 * @link_: the link to add to @queue
 * 
 * Inserts @link into @queue at the given position.
 * 
 * Since: 2.4
 **/
void
g_queue_push_nth_link  (GQueue  *queue,
			gint     n,
			GList   *link_)
{
  GList *next;
  GList *prev;
  
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link_ != NULL);

  if (n < 0 || n >= queue->length)
    {
      g_queue_push_tail_link (queue, link_);
      return;
    }

  g_assert (queue->head);
  g_assert (queue->tail);

  next = g_queue_peek_nth_link (queue, n);
  prev = next->prev;

  if (prev)
    prev->next = link_;
  next->prev = link_;

  link_->next = next;
  link_->prev = prev;

  if (queue->head->prev)
    queue->head = queue->head->prev;

  if (queue->tail->next)
    queue->tail = queue->tail->next;
  
  queue->length++;
}

/**
 * g_queue_pop_head:
 * @queue: a #GQueue.
 *
 * Removes the first element of the queue.
 *
 * Returns: the data of the first element in the queue, or %NULL if the queue
 *   is empty.
 **/
gpointer
g_queue_pop_head (GQueue *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->head)
    {
      GList *node = queue->head;
      gpointer data = node->data;

      queue->head = node->next;
      if (queue->head)
	queue->head->prev = NULL;
      else
	queue->tail = NULL;
      g_list_free_1 (node);
      queue->length--;

      return data;
    }

  return NULL;
}

/**
 * g_queue_pop_head_link:
 * @queue: a #GQueue.
 *
 * Removes the first element of the queue.
 *
 * Returns: the #GList element at the head of the queue, or %NULL if the queue
 *   is empty.
 **/
GList*
g_queue_pop_head_link (GQueue *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->head)
    {
      GList *node = queue->head;

      queue->head = node->next;
      if (queue->head)
	{
	  queue->head->prev = NULL;
	  node->next = NULL;
	}
      else
	queue->tail = NULL;
      queue->length--;

      return node;
    }

  return NULL;
}

/**
 * g_queue_peek_head_link:
 * @queue: a #GQueue
 * 
 * Returns the first link in @queue
 * 
 * Return value: the first link in @queue, or %NULL if @queue is empty
 * 
 * Since: 2.4
 **/
GList*
g_queue_peek_head_link (GQueue *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  return queue->head;
}

/**
 * g_queue_peek_tail_link:
 * @queue: a #GQueue
 * 
 * Returns the last link @queue.
 * 
 * Return value: the last link in @queue, or %NULL if @queue is empty
 * 
 * Since: 2.4
 **/
GList*
g_queue_peek_tail_link (GQueue *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  return queue->tail;
}

/**
 * g_queue_pop_tail:
 * @queue: a #GQueue.
 *
 * Removes the last element of the queue.
 *
 * Returns: the data of the last element in the queue, or %NULL if the queue
 *   is empty.
 **/
gpointer
g_queue_pop_tail (GQueue *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->tail)
    {
      GList *node = queue->tail;
      gpointer data = node->data;

      queue->tail = node->prev;
      if (queue->tail)
	queue->tail->next = NULL;
      else
	queue->head = NULL;
      queue->length--;
      g_list_free_1 (node);

      return data;
    }
  
  return NULL;
}

/**
 * g_queue_pop_nth:
 * @queue: a #GQueue
 * @n: the position of the element.
 * 
 * Removes the @n'th element of @queue.
 * 
 * Return value: the element's data, or %NULL if @n is off the end of @queue.
 * 
 * Since: 2.4
 **/
gpointer
g_queue_pop_nth (GQueue *queue,
		 guint   n)
{
  GList *nth_link;
  gpointer result;
  
  g_return_val_if_fail (queue != NULL, NULL);

  if (n >= queue->length)
    return NULL;
  
  nth_link = g_queue_peek_nth_link (queue, n);
  result = nth_link->data;

  g_queue_delete_link (queue, nth_link);

  return result;
}

/**
 * g_queue_pop_tail_link:
 * @queue: a #GQueue.
 *
 * Removes the last element of the queue.
 *
 * Returns: the #GList element at the tail of the queue, or %NULL if the queue
 *   is empty.
 **/
GList*
g_queue_pop_tail_link (GQueue *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);
  
  if (queue->tail)
    {
      GList *node = queue->tail;
      
      queue->tail = node->prev;
      if (queue->tail)
	{
	  queue->tail->next = NULL;
	  node->prev = NULL;
	}
      else
	queue->head = NULL;
      queue->length--;
      
      return node;
    }
  
  return NULL;
}

/**
 * g_queue_pop_nth_link:
 * @queue: a #GQueue
 * @n: the link's position
 * 
 * Removes and returns the link at the given position.
 * 
 * Return value: The @n'th link, or %NULL if @n is off the end of @queue.
 * 
 * Since: 2.4
 **/
GList*
g_queue_pop_nth_link (GQueue *queue,
		      guint   n)
{
  GList *link;
  
  g_return_val_if_fail (queue != NULL, NULL);

  if (n >= queue->length)
    return NULL;
  
  link = g_queue_peek_nth_link (queue, n);
  g_queue_unlink (queue, link);

  return link;
}

/**
 * g_queue_peek_nth_link:
 * @queue: a #GQueue
 * @n: the position of the link
 * 
 * Returns the link at the given position
 * 
 * Return value: The link at the @n'th position, or %NULL if @n is off the
 * end of the list
 * 
 * Since: 2.4
 **/
GList *
g_queue_peek_nth_link (GQueue *queue,
		       guint   n)
{
  GList *link;
  gint i;
  
  g_return_val_if_fail (queue != NULL, NULL);

  if (n >= queue->length)
    return NULL;
  
  if (n > queue->length / 2)
    {
      n = queue->length - n - 1;

      link = queue->tail;
      for (i = 0; i < n; ++i)
	link = link->prev;
    }
  else
    {
      link = queue->head;
      for (i = 0; i < n; ++i)
	link = link->next;
    }

  return link;
}

/**
 * g_queue_link_index:
 * @queue: a #Gqueue
 * @link_: A #GList link
 * 
 * Returns the position of @link_ in @queue.
 * 
 * Return value: The position of @link_, or -1 if the link is
 * not part of @queue
 * 
 * Since: 2.4
 **/
gint
g_queue_link_index (GQueue *queue,
		    GList  *link_)
{
  g_return_val_if_fail (queue != NULL, -1);

  return g_list_position (queue->head, link_);
}

/**
 * g_queue_unlink
 * @queue: a #GQueue
 * @link_: a #GList link that <emphasis>must</emphasis> be part of @queue
 *
 * Unlinks @link_ so that it will no longer be part of @queue. The link is
 * not freed.
 *
 * @link_ must be part of @queue,
 * 
 * Since: 2.4
 **/
void
g_queue_unlink (GQueue *queue,
		GList  *link_)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link_ != NULL);

  if (link_ == queue->tail)
    queue->tail = queue->tail->prev;
  
  queue->head = g_list_remove_link (queue->head, link_);
  queue->length--;
}

/**
 * g_queue_delete_link:
 * @queue: a #GQueue
 * @link_: a #GList link that <emphasis>must</emphasis> be part of @queue
 *
 * Removes @link_ from @queue and frees it.
 *
 * @link_ must be part of @queue.
 * 
 * Since: 2.4
 **/
void
g_queue_delete_link (GQueue *queue,
		     GList  *link_)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link_ != NULL);

  g_queue_unlink (queue, link_);
  g_list_free (link_);
}

/**
 * g_queue_peek_head:
 * @queue: a #GQueue.
 *
 * Returns the first element of the queue.
 *
 * Returns: the data of the first element in the queue, or %NULL if the queue
 *   is empty.
 **/
gpointer
g_queue_peek_head (GQueue *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  return queue->head ? queue->head->data : NULL;
}

/**
 * g_queue_peek_tail:
 * @queue: a #GQueue.
 *
 * Returns the last element of the queue.
 *
 * Returns: the data of the last element in the queue, or %NULL if the queue
 *   is empty.
 **/
gpointer
g_queue_peek_tail (GQueue *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  return queue->tail ? queue->tail->data : NULL;
}

/**
 * g_queue_peek_nth:
 * @queue: a #GQueue
 * @n: the position of the element.
 * 
 * Returns the @n'th element of @queue. 
 * 
 * Return value: The data for the @n'th element of @queue, or %NULL if @n is
 *   off the end of @queue.
 * 
 * Since: 2.4
 **/
gpointer
g_queue_peek_nth (GQueue *queue,
		  guint   n)
{
  GList *link;
  
  g_return_val_if_fail (queue != NULL, NULL);

  link = g_queue_peek_nth_link (queue, n);

  if (link)
    return link->data;

  return NULL;
}

/**
 * g_queue_index:
 * @queue: a #GQueue
 * @data: the data to find.
 * 
 * Returns the position of the first element in @queue which contains @data.
 * 
 * Return value: The position of the first element in @queue which contains @data, or -1 if no element in @queue contains @data.
 * 
 * Since: 2.4
 **/
gint
g_queue_index (GQueue        *queue,
	       gconstpointer  data)
{
  g_return_val_if_fail (queue != NULL, -1);

  return g_list_index (queue->head, data);
}

/**
 * g_queue_remove:
 * @queue: a #GQueue
 * @data: data to remove.
 * 
 * Removes the first element in @queue that contains @data. 
 * 
 * Since: 2.4
 **/
void
g_queue_remove (GQueue        *queue,
		gconstpointer  data)
{
  GList *link;
  
  g_return_if_fail (queue != NULL);

  link = g_list_find (queue->head, data);

  if (link)
    g_queue_delete_link (queue, link);
}

/**
 * g_queue_remove_all:
 * @queue: a #GQueue
 * @data: data to remove
 * 
 * Remove all elemeents in @queue which contains @data.
 * 
 * Since: 2.4
 **/
void
g_queue_remove_all (GQueue        *queue,
		    gconstpointer  data)
{
  GList *list;
  
  g_return_if_fail (queue != NULL);

  list = queue->head;
  while (list)
    {
      GList *next = list->next;

      if (list->data == data)
	g_queue_delete_link (queue, list);
      
      list = next;
    }
}

/**
 * g_queue_insert_before:
 * @queue: a #GQueue
 * @sibling: a #GList link that <emphasis>must</emphasis> be part of @queue
 * @data: the data to insert
 * 
 * Inserts @data into @queue before @sibling.
 *
 * @sibling must be part of @queue.
 * 
 * Since: 2.4
 **/
void
g_queue_insert_before (GQueue   *queue,
		       GList    *sibling,
		       gpointer  data)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (sibling != NULL);

  queue->head = g_list_insert_before (queue->head, sibling, data);
  queue->length++;
}

/**
 * g_queue_insert_after:
 * @queue: a #GQueue
 * @sibling: a #GList link that <emphasis>must</emphasis> be part of @queue
 * @data: the data to insert
 *
 * Inserts @data into @queue after @sibling
 *
 * @sibling must be part of @queue
 * 
 * Since: 2.4
 **/
void
g_queue_insert_after (GQueue   *queue,
		      GList    *sibling,
		      gpointer  data)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (sibling != NULL);

  if (sibling == queue->tail)
    g_queue_push_tail (queue, data);
  else
    g_queue_insert_before (queue, sibling->next, data);
}

/**
 * g_queue_insert_sorted:
 * @queue: a #GQueue
 * @data: the data to insert
 * @func: the #GCompareDataFunc used to compare elements in the queue. It is
 *     called with two elements of the @queue and @user_data. It should
 *     return 0 if the elements are equal, a negative value if the first
 *     element comes before the second, and a positive value if the second
 *     element comes before the first.
 * @user_data: user data passed to @func.
 * 
 * Inserts @data into @queue using @func to determine the new position.
 * 
 * Since: 2.4
 **/
void
g_queue_insert_sorted (GQueue           *queue,
		       gpointer          data,
		       GCompareDataFunc  func,
		       gpointer          user_data)
{
  GList *list;
  
  g_return_if_fail (queue != NULL);

  list = queue->head;
  while (list && func (list->data, data, user_data) < 0)
    list = list->next;

  if (list)
    g_queue_insert_before (queue, list, data);
  else
    g_queue_push_tail (queue, data);
}

#define __G_QUEUE_C__
#include "galiasdef.c"
