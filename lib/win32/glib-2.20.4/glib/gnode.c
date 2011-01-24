/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GNode: N-way tree implementation.
 * Copyright (C) 1998 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

void g_node_push_allocator (gpointer dummy) { /* present for binary compat only */ }
void g_node_pop_allocator  (void)           { /* present for binary compat only */ }

#define g_node_alloc0()         g_slice_new0 (GNode)
#define g_node_free(node)       g_slice_free (GNode, node)

/* --- functions --- */
/**
 * g_node_new:
 * @data: the data of the new node
 *
 * Creates a new #GNode containing the given data.
 * Used to create the first node in a tree.
 *
 * Returns: a new #GNode
 */
GNode*
g_node_new (gpointer data)
{
  GNode *node = g_node_alloc0 ();
  node->data = data;
  return node;
}

static void
g_nodes_free (GNode *node)
{
  while (node)
    {
      GNode *next = node->next;
      if (node->children)
        g_nodes_free (node->children);
      g_node_free (node);
      node = next;
    }
}

/**
 * g_node_destroy:
 * @root: the root of the tree/subtree to destroy
 *
 * Removes @root and its children from the tree, freeing any memory
 * allocated.
 */
void
g_node_destroy (GNode *root)
{
  g_return_if_fail (root != NULL);
  
  if (!G_NODE_IS_ROOT (root))
    g_node_unlink (root);
  
  g_nodes_free (root);
}

/**
 * g_node_unlink:
 * @node: the #GNode to unlink, which becomes the root of a new tree
 *
 * Unlinks a #GNode from a tree, resulting in two separate trees.
 */
void
g_node_unlink (GNode *node)
{
  g_return_if_fail (node != NULL);
  
  if (node->prev)
    node->prev->next = node->next;
  else if (node->parent)
    node->parent->children = node->next;
  node->parent = NULL;
  if (node->next)
    {
      node->next->prev = node->prev;
      node->next = NULL;
    }
  node->prev = NULL;
}

/**
 * g_node_copy_deep:
 * @node: a #GNode
 * @copy_func: the function which is called to copy the data inside each node,
 *   or %NULL to use the original data.
 * @data: data to pass to @copy_func
 * 
 * Recursively copies a #GNode and its data.
 * 
 * Return value: a new #GNode containing copies of the data in @node.
 *
 * Since: 2.4
 **/
GNode*
g_node_copy_deep (GNode     *node, 
		  GCopyFunc  copy_func,
		  gpointer   data)
{
  GNode *new_node = NULL;

  if (copy_func == NULL)
	return g_node_copy (node);

  if (node)
    {
      GNode *child, *new_child;
      
      new_node = g_node_new (copy_func (node->data, data));
      
      for (child = g_node_last_child (node); child; child = child->prev) 
	{
	  new_child = g_node_copy_deep (child, copy_func, data);
	  g_node_prepend (new_node, new_child);
	}
    }
  
  return new_node;
}

/**
 * g_node_copy:
 * @node: a #GNode
 *
 * Recursively copies a #GNode (but does not deep-copy the data inside the 
 * nodes, see g_node_copy_deep() if you need that).
 *
 * Returns: a new #GNode containing the same data pointers
 */
GNode*
g_node_copy (GNode *node)
{
  GNode *new_node = NULL;
  
  if (node)
    {
      GNode *child;
      
      new_node = g_node_new (node->data);
      
      for (child = g_node_last_child (node); child; child = child->prev)
	g_node_prepend (new_node, g_node_copy (child));
    }
  
  return new_node;
}

/**
 * g_node_insert:
 * @parent: the #GNode to place @node under
 * @position: the position to place @node at, with respect to its siblings
 *     If position is -1, @node is inserted as the last child of @parent
 * @node: the #GNode to insert
 *
 * Inserts a #GNode beneath the parent at the given position.
 *
 * Returns: the inserted #GNode
 */
GNode*
g_node_insert (GNode *parent,
	       gint   position,
	       GNode *node)
{
  g_return_val_if_fail (parent != NULL, node);
  g_return_val_if_fail (node != NULL, node);
  g_return_val_if_fail (G_NODE_IS_ROOT (node), node);
  
  if (position > 0)
    return g_node_insert_before (parent,
				 g_node_nth_child (parent, position),
				 node);
  else if (position == 0)
    return g_node_prepend (parent, node);
  else /* if (position < 0) */
    return g_node_append (parent, node);
}

/**
 * g_node_insert_before:
 * @parent: the #GNode to place @node under
 * @sibling: the sibling #GNode to place @node before. 
 *     If sibling is %NULL, the node is inserted as the last child of @parent.
 * @node: the #GNode to insert
 *
 * Inserts a #GNode beneath the parent before the given sibling.
 *
 * Returns: the inserted #GNode
 */
GNode*
g_node_insert_before (GNode *parent,
		      GNode *sibling,
		      GNode *node)
{
  g_return_val_if_fail (parent != NULL, node);
  g_return_val_if_fail (node != NULL, node);
  g_return_val_if_fail (G_NODE_IS_ROOT (node), node);
  if (sibling)
    g_return_val_if_fail (sibling->parent == parent, node);
  
  node->parent = parent;
  
  if (sibling)
    {
      if (sibling->prev)
	{
	  node->prev = sibling->prev;
	  node->prev->next = node;
	  node->next = sibling;
	  sibling->prev = node;
	}
      else
	{
	  node->parent->children = node;
	  node->next = sibling;
	  sibling->prev = node;
	}
    }
  else
    {
      if (parent->children)
	{
	  sibling = parent->children;
	  while (sibling->next)
	    sibling = sibling->next;
	  node->prev = sibling;
	  sibling->next = node;
	}
      else
	node->parent->children = node;
    }

  return node;
}

/**
 * g_node_insert_after:
 * @parent: the #GNode to place @node under
 * @sibling: the sibling #GNode to place @node after. 
 *     If sibling is %NULL, the node is inserted as the first child of @parent.
 * @node: the #GNode to insert
 *
 * Inserts a #GNode beneath the parent after the given sibling.
 *
 * Returns: the inserted #GNode
 */
GNode*
g_node_insert_after (GNode *parent,
		     GNode *sibling,
		     GNode *node)
{
  g_return_val_if_fail (parent != NULL, node);
  g_return_val_if_fail (node != NULL, node);
  g_return_val_if_fail (G_NODE_IS_ROOT (node), node);
  if (sibling)
    g_return_val_if_fail (sibling->parent == parent, node);

  node->parent = parent;

  if (sibling)
    {
      if (sibling->next)
	{
	  sibling->next->prev = node;
	}
      node->next = sibling->next;
      node->prev = sibling;
      sibling->next = node;
    }
  else
    {
      if (parent->children)
	{
	  node->next = parent->children;
	  parent->children->prev = node;
	}
      parent->children = node;
    }

  return node;
}

/**
 * g_node_prepend:
 * @parent: the #GNode to place the new #GNode under
 * @node: the #GNode to insert
 *
 * Inserts a #GNode as the first child of the given parent.
 *
 * Returns: the inserted #GNode
 */
GNode*
g_node_prepend (GNode *parent,
		GNode *node)
{
  g_return_val_if_fail (parent != NULL, node);
  
  return g_node_insert_before (parent, parent->children, node);
}

/**
 * g_node_get_root:
 * @node: a #GNode
 *
 * Gets the root of a tree.
 *
 * Returns: the root of the tree
 */
GNode*
g_node_get_root (GNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);
  
  while (node->parent)
    node = node->parent;
  
  return node;
}

/**
 * g_node_is_ancestor:
 * @node: a #GNode
 * @descendant: a #GNode
 *
 * Returns %TRUE if @node is an ancestor of @descendant.
 * This is true if node is the parent of @descendant, 
 * or if node is the grandparent of @descendant etc.
 *
 * Returns: %TRUE if @node is an ancestor of @descendant
 */
gboolean
g_node_is_ancestor (GNode *node,
		    GNode *descendant)
{
  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (descendant != NULL, FALSE);
  
  while (descendant)
    {
      if (descendant->parent == node)
	return TRUE;
      
      descendant = descendant->parent;
    }
  
  return FALSE;
}

/**
 * g_node_depth:
 * @node: a #GNode
 *
 * Gets the depth of a #GNode.
 *
 * If @node is %NULL the depth is 0. The root node has a depth of 1.
 * For the children of the root node the depth is 2. And so on.
 *
 * Returns: the depth of the #GNode
 */
guint
g_node_depth (GNode *node)
{
  guint depth = 0;
  
  while (node)
    {
      depth++;
      node = node->parent;
    }
  
  return depth;
}

/**
 * g_node_reverse_children:
 * @node: a #GNode.
 *
 * Reverses the order of the children of a #GNode.
 * (It doesn't change the order of the grandchildren.)
 */
void
g_node_reverse_children (GNode *node)
{
  GNode *child;
  GNode *last;
  
  g_return_if_fail (node != NULL);
  
  child = node->children;
  last = NULL;
  while (child)
    {
      last = child;
      child = last->next;
      last->next = last->prev;
      last->prev = child;
    }
  node->children = last;
}

/**
 * g_node_max_height:
 * @root: a #GNode
 *
 * Gets the maximum height of all branches beneath a #GNode.
 * This is the maximum distance from the #GNode to all leaf nodes.
 *
 * If @root is %NULL, 0 is returned. If @root has no children, 
 * 1 is returned. If @root has children, 2 is returned. And so on.
 *
 * Returns: the maximum height of the tree beneath @root
 */
guint
g_node_max_height (GNode *root)
{
  GNode *child;
  guint max_height = 0;
  
  if (!root)
    return 0;
  
  child = root->children;
  while (child)
    {
      guint tmp_height;
      
      tmp_height = g_node_max_height (child);
      if (tmp_height > max_height)
	max_height = tmp_height;
      child = child->next;
    }
  
  return max_height + 1;
}

static gboolean
g_node_traverse_pre_order (GNode	    *node,
			   GTraverseFlags    flags,
			   GNodeTraverseFunc func,
			   gpointer	     data)
{
  if (node->children)
    {
      GNode *child;
      
      if ((flags & G_TRAVERSE_NON_LEAFS) &&
	  func (node, data))
	return TRUE;
      
      child = node->children;
      while (child)
	{
	  GNode *current;
	  
	  current = child;
	  child = current->next;
	  if (g_node_traverse_pre_order (current, flags, func, data))
	    return TRUE;
	}
    }
  else if ((flags & G_TRAVERSE_LEAFS) &&
	   func (node, data))
    return TRUE;
  
  return FALSE;
}

static gboolean
g_node_depth_traverse_pre_order (GNode		  *node,
				 GTraverseFlags	   flags,
				 guint		   depth,
				 GNodeTraverseFunc func,
				 gpointer	   data)
{
  if (node->children)
    {
      GNode *child;
      
      if ((flags & G_TRAVERSE_NON_LEAFS) &&
	  func (node, data))
	return TRUE;
      
      depth--;
      if (!depth)
	return FALSE;
      
      child = node->children;
      while (child)
	{
	  GNode *current;
	  
	  current = child;
	  child = current->next;
	  if (g_node_depth_traverse_pre_order (current, flags, depth, func, data))
	    return TRUE;
	}
    }
  else if ((flags & G_TRAVERSE_LEAFS) &&
	   func (node, data))
    return TRUE;
  
  return FALSE;
}

static gboolean
g_node_traverse_post_order (GNode	     *node,
			    GTraverseFlags    flags,
			    GNodeTraverseFunc func,
			    gpointer	      data)
{
  if (node->children)
    {
      GNode *child;
      
      child = node->children;
      while (child)
	{
	  GNode *current;
	  
	  current = child;
	  child = current->next;
	  if (g_node_traverse_post_order (current, flags, func, data))
	    return TRUE;
	}
      
      if ((flags & G_TRAVERSE_NON_LEAFS) &&
	  func (node, data))
	return TRUE;
      
    }
  else if ((flags & G_TRAVERSE_LEAFS) &&
	   func (node, data))
    return TRUE;
  
  return FALSE;
}

static gboolean
g_node_depth_traverse_post_order (GNode		   *node,
				  GTraverseFlags    flags,
				  guint		    depth,
				  GNodeTraverseFunc func,
				  gpointer	    data)
{
  if (node->children)
    {
      depth--;
      if (depth)
	{
	  GNode *child;
	  
	  child = node->children;
	  while (child)
	    {
	      GNode *current;
	      
	      current = child;
	      child = current->next;
	      if (g_node_depth_traverse_post_order (current, flags, depth, func, data))
		return TRUE;
	    }
	}
      
      if ((flags & G_TRAVERSE_NON_LEAFS) &&
	  func (node, data))
	return TRUE;
      
    }
  else if ((flags & G_TRAVERSE_LEAFS) &&
	   func (node, data))
    return TRUE;
  
  return FALSE;
}

static gboolean
g_node_traverse_in_order (GNode		   *node,
			  GTraverseFlags    flags,
			  GNodeTraverseFunc func,
			  gpointer	    data)
{
  if (node->children)
    {
      GNode *child;
      GNode *current;
      
      child = node->children;
      current = child;
      child = current->next;
      
      if (g_node_traverse_in_order (current, flags, func, data))
	return TRUE;
      
      if ((flags & G_TRAVERSE_NON_LEAFS) &&
	  func (node, data))
	return TRUE;
      
      while (child)
	{
	  current = child;
	  child = current->next;
	  if (g_node_traverse_in_order (current, flags, func, data))
	    return TRUE;
	}
    }
  else if ((flags & G_TRAVERSE_LEAFS) &&
	   func (node, data))
    return TRUE;
  
  return FALSE;
}

static gboolean
g_node_depth_traverse_in_order (GNode		 *node,
				GTraverseFlags	  flags,
				guint		  depth,
				GNodeTraverseFunc func,
				gpointer	  data)
{
  if (node->children)
    {
      depth--;
      if (depth)
	{
	  GNode *child;
	  GNode *current;
	  
	  child = node->children;
	  current = child;
	  child = current->next;
	  
	  if (g_node_depth_traverse_in_order (current, flags, depth, func, data))
	    return TRUE;
	  
	  if ((flags & G_TRAVERSE_NON_LEAFS) &&
	      func (node, data))
	    return TRUE;
	  
	  while (child)
	    {
	      current = child;
	      child = current->next;
	      if (g_node_depth_traverse_in_order (current, flags, depth, func, data))
		return TRUE;
	    }
	}
      else if ((flags & G_TRAVERSE_NON_LEAFS) &&
	       func (node, data))
	return TRUE;
    }
  else if ((flags & G_TRAVERSE_LEAFS) &&
	   func (node, data))
    return TRUE;
  
  return FALSE;
}

static gboolean
g_node_traverse_level (GNode		 *node,
		       GTraverseFlags	  flags,
		       guint		  level,
		       GNodeTraverseFunc  func,
		       gpointer	          data,
		       gboolean          *more_levels)
{
  if (level == 0) 
    {
      if (node->children)
	{
	  *more_levels = TRUE;
	  return (flags & G_TRAVERSE_NON_LEAFS) && func (node, data);
	}
      else
	{
	  return (flags & G_TRAVERSE_LEAFS) && func (node, data);
	}
    }
  else 
    {
      node = node->children;
      
      while (node)
	{
	  if (g_node_traverse_level (node, flags, level - 1, func, data, more_levels))
	    return TRUE;

	  node = node->next;
	}
    }

  return FALSE;
}

static gboolean
g_node_depth_traverse_level (GNode             *node,
			     GTraverseFlags	flags,
			     guint		depth,
			     GNodeTraverseFunc  func,
			     gpointer	        data)
{
  guint level;
  gboolean more_levels;

  level = 0;  
  while (level != depth) 
    {
      more_levels = FALSE;
      if (g_node_traverse_level (node, flags, level, func, data, &more_levels))
	return TRUE;
      if (!more_levels)
	break;
      level++;
    }
  return FALSE;
}

/**
 * g_node_traverse:
 * @root: the root #GNode of the tree to traverse
 * @order: the order in which nodes are visited - %G_IN_ORDER, 
 *     %G_PRE_ORDER, %G_POST_ORDER, or %G_LEVEL_ORDER.
 * @flags: which types of children are to be visited, one of 
 *     %G_TRAVERSE_ALL, %G_TRAVERSE_LEAVES and %G_TRAVERSE_NON_LEAVES
 * @max_depth: the maximum depth of the traversal. Nodes below this
 *     depth will not be visited. If max_depth is -1 all nodes in 
 *     the tree are visited. If depth is 1, only the root is visited. 
 *     If depth is 2, the root and its children are visited. And so on.
 * @func: the function to call for each visited #GNode
 * @data: user data to pass to the function
 *
 * Traverses a tree starting at the given root #GNode.
 * It calls the given function for each node visited.
 * The traversal can be halted at any point by returning %TRUE from @func.
 */
void
g_node_traverse (GNode		  *root,
		 GTraverseType	   order,
		 GTraverseFlags	   flags,
		 gint		   depth,
		 GNodeTraverseFunc func,
		 gpointer	   data)
{
  g_return_if_fail (root != NULL);
  g_return_if_fail (func != NULL);
  g_return_if_fail (order <= G_LEVEL_ORDER);
  g_return_if_fail (flags <= G_TRAVERSE_MASK);
  g_return_if_fail (depth == -1 || depth > 0);
  
  switch (order)
    {
    case G_PRE_ORDER:
      if (depth < 0)
	g_node_traverse_pre_order (root, flags, func, data);
      else
	g_node_depth_traverse_pre_order (root, flags, depth, func, data);
      break;
    case G_POST_ORDER:
      if (depth < 0)
	g_node_traverse_post_order (root, flags, func, data);
      else
	g_node_depth_traverse_post_order (root, flags, depth, func, data);
      break;
    case G_IN_ORDER:
      if (depth < 0)
	g_node_traverse_in_order (root, flags, func, data);
      else
	g_node_depth_traverse_in_order (root, flags, depth, func, data);
      break;
    case G_LEVEL_ORDER:
      g_node_depth_traverse_level (root, flags, depth, func, data);
      break;
    }
}

static gboolean
g_node_find_func (GNode	   *node,
		  gpointer  data)
{
  gpointer *d = data;
  
  if (*d != node->data)
    return FALSE;
  
  *(++d) = node;
  
  return TRUE;
}

/**
 * g_node_find:
 * @root: the root #GNode of the tree to search
 * @order: the order in which nodes are visited - %G_IN_ORDER, 
 *     %G_PRE_ORDER, %G_POST_ORDER, or %G_LEVEL_ORDER
 * @flags: which types of children are to be searched, one of 
 *     %G_TRAVERSE_ALL, %G_TRAVERSE_LEAVES and %G_TRAVERSE_NON_LEAVES
 * @data: the data to find
 *
 * Finds a #GNode in a tree.
 *
 * Returns: the found #GNode, or %NULL if the data is not found
 */
GNode*
g_node_find (GNode	    *root,
	     GTraverseType   order,
	     GTraverseFlags  flags,
	     gpointer        data)
{
  gpointer d[2];
  
  g_return_val_if_fail (root != NULL, NULL);
  g_return_val_if_fail (order <= G_LEVEL_ORDER, NULL);
  g_return_val_if_fail (flags <= G_TRAVERSE_MASK, NULL);
  
  d[0] = data;
  d[1] = NULL;
  
  g_node_traverse (root, order, flags, -1, g_node_find_func, d);
  
  return d[1];
}

static void
g_node_count_func (GNode	 *node,
		   GTraverseFlags flags,
		   guint	 *n)
{
  if (node->children)
    {
      GNode *child;
      
      if (flags & G_TRAVERSE_NON_LEAFS)
	(*n)++;
      
      child = node->children;
      while (child)
	{
	  g_node_count_func (child, flags, n);
	  child = child->next;
	}
    }
  else if (flags & G_TRAVERSE_LEAFS)
    (*n)++;
}

/**
 * g_node_n_nodes:
 * @root: a #GNode
 * @flags: which types of children are to be counted, one of 
 *     %G_TRAVERSE_ALL, %G_TRAVERSE_LEAVES and %G_TRAVERSE_NON_LEAVES
 *
 * Gets the number of nodes in a tree.
 *
 * Returns: the number of nodes in the tree
 */
guint
g_node_n_nodes (GNode	       *root,
		GTraverseFlags  flags)
{
  guint n = 0;
  
  g_return_val_if_fail (root != NULL, 0);
  g_return_val_if_fail (flags <= G_TRAVERSE_MASK, 0);
  
  g_node_count_func (root, flags, &n);
  
  return n;
}

/**
 * g_node_last_child:
 * @node: a #GNode (must not be %NULL)
 *
 * Gets the last child of a #GNode.
 *
 * Returns: the last child of @node, or %NULL if @node has no children
 */
GNode*
g_node_last_child (GNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);
  
  node = node->children;
  if (node)
    while (node->next)
      node = node->next;
  
  return node;
}

/**
 * g_node_nth_child:
 * @node: a #GNode
 * @n: the index of the desired child
 *
 * Gets a child of a #GNode, using the given index.
 * The first child is at index 0. If the index is 
 * too big, %NULL is returned.
 *
 * Returns: the child of @node at index @n
 */
GNode*
g_node_nth_child (GNode *node,
		  guint	 n)
{
  g_return_val_if_fail (node != NULL, NULL);
  
  node = node->children;
  if (node)
    while ((n-- > 0) && node)
      node = node->next;
  
  return node;
}

/**
 * g_node_n_children:
 * @node: a #GNode
 *
 * Gets the number of children of a #GNode.
 *
 * Returns: the number of children of @node
 */
guint
g_node_n_children (GNode *node)
{
  guint n = 0;
  
  g_return_val_if_fail (node != NULL, 0);
  
  node = node->children;
  while (node)
    {
      n++;
      node = node->next;
    }
  
  return n;
}

/**
 * g_node_find_child:
 * @node: a #GNode
 * @flags: which types of children are to be searched, one of 
 *     %G_TRAVERSE_ALL, %G_TRAVERSE_LEAVES and %G_TRAVERSE_NON_LEAVES
 * @data: the data to find
 *
 * Finds the first child of a #GNode with the given data.
 *
 * Returns: the found child #GNode, or %NULL if the data is not found
 */
GNode*
g_node_find_child (GNode	  *node,
		   GTraverseFlags  flags,
		   gpointer	   data)
{
  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (flags <= G_TRAVERSE_MASK, NULL);
  
  node = node->children;
  while (node)
    {
      if (node->data == data)
	{
	  if (G_NODE_IS_LEAF (node))
	    {
	      if (flags & G_TRAVERSE_LEAFS)
		return node;
	    }
	  else
	    {
	      if (flags & G_TRAVERSE_NON_LEAFS)
		return node;
	    }
	}
      node = node->next;
    }
  
  return NULL;
}

/**
 * g_node_child_position:
 * @node: a #GNode
 * @child: a child of @node
 *
 * Gets the position of a #GNode with respect to its siblings.
 * @child must be a child of @node. The first child is numbered 0, 
 * the second 1, and so on.
 *
 * Returns: the position of @child with respect to its siblings
 */
gint
g_node_child_position (GNode *node,
		       GNode *child)
{
  guint n = 0;
  
  g_return_val_if_fail (node != NULL, -1);
  g_return_val_if_fail (child != NULL, -1);
  g_return_val_if_fail (child->parent == node, -1);
  
  node = node->children;
  while (node)
    {
      if (node == child)
	return n;
      n++;
      node = node->next;
    }
  
  return -1;
}

/**
 * g_node_child_index:
 * @node: a #GNode
 * @data: the data to find
 *
 * Gets the position of the first child of a #GNode 
 * which contains the given data.
 *
 * Returns: the index of the child of @node which contains 
 *     @data, or -1 if the data is not found
 */
gint
g_node_child_index (GNode    *node,
		    gpointer  data)
{
  guint n = 0;
  
  g_return_val_if_fail (node != NULL, -1);
  
  node = node->children;
  while (node)
    {
      if (node->data == data)
	return n;
      n++;
      node = node->next;
    }
  
  return -1;
}

/**
 * g_node_first_sibling:
 * @node: a #GNode
 *
 * Gets the first sibling of a #GNode.
 * This could possibly be the node itself.
 *
 * Returns: the first sibling of @node
 */
GNode*
g_node_first_sibling (GNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);
  
  if (node->parent)
    return node->parent->children;
  
  while (node->prev)
    node = node->prev;
  
  return node;
}

/**
 * g_node_last_sibling:
 * @node: a #GNode
 *
 * Gets the last sibling of a #GNode.
 * This could possibly be the node itself.
 *
 * Returns: the last sibling of @node
 */
GNode*
g_node_last_sibling (GNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);
  
  while (node->next)
    node = node->next;
  
  return node;
}

/**
 * g_node_children_foreach:
 * @node: a #GNode
 * @flags: which types of children are to be visited, one of 
 *     %G_TRAVERSE_ALL, %G_TRAVERSE_LEAVES and %G_TRAVERSE_NON_LEAVES
 * @func: the function to call for each visited node
 * @data: user data to pass to the function
 *
 * Calls a function for each of the children of a #GNode.
 * Note that it doesn't descend beneath the child nodes.
 */
void
g_node_children_foreach (GNode		  *node,
			 GTraverseFlags	   flags,
			 GNodeForeachFunc  func,
			 gpointer	   data)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (flags <= G_TRAVERSE_MASK);
  g_return_if_fail (func != NULL);
  
  node = node->children;
  while (node)
    {
      GNode *current;
      
      current = node;
      node = current->next;
      if (G_NODE_IS_LEAF (current))
	{
	  if (flags & G_TRAVERSE_LEAFS)
	    func (current, data);
	}
      else
	{
	  if (flags & G_TRAVERSE_NON_LEAFS)
	    func (current, data);
	}
    }
}

#define __G_NODE_C__
#include "galiasdef.c"
