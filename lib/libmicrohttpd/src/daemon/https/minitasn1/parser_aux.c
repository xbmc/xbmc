/*
 *      Copyright (C) 2004, 2006, 2007 Free Software Foundation
 *      Copyright (C) 2000,2001 Fabio Fiorina
 *
 * This file is part of LIBTASN1.
 *
 * The LIBTASN1 library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#include <int.h>
#include "parser_aux.h"
#include "gstr.h"
#include "structure.h"
#include "element.h"



char MHD__asn1_identifierMissing[MAX_NAME_SIZE + 1];    /* identifier name not found */

/***********************************************/
/* Type: list_type                             */
/* Description: type used in the list during   */
/* the structure creation.                     */
/***********************************************/
typedef struct list_struct
{
  node_asn *node;
  struct list_struct *next;
} list_type;


/* Pointer to the first element of the list */
list_type *MHD_firstElement = NULL;

/******************************************************/
/* Function : MHD__asn1_add_node                          */
/* Description: creates a new NODE_ASN element and    */
/* puts it in the list pointed by MHD_firstElement.       */
/* Parameters:                                        */
/*   type: type of the new element (see TYPE_         */
/*         and CONST_ constants).                     */
/* Return: pointer to the new element.                */
/******************************************************/
node_asn *
MHD__asn1_add_node (unsigned int type)
{
  list_type *listElement;
  node_asn *punt;

  punt = (node_asn *) MHD__asn1_calloc (1, sizeof (node_asn));
  if (punt == NULL)
    return NULL;

  listElement = (list_type *) MHD__asn1_malloc (sizeof (list_type));
  if (listElement == NULL)
    {
      MHD__asn1_free (punt);
      return NULL;
    }

  listElement->node = punt;
  listElement->next = MHD_firstElement;
  MHD_firstElement = listElement;

  punt->type = type;

  return punt;
}

/**
 * MHD__asn1_find_node:
 * @pointer: NODE_ASN element pointer.
 * @name: null terminated string with the element's name to find.
 *
 * Searches for an element called NAME starting from POINTER.  The
 * name is composed by differents identifiers separated by dots.  When
 * *POINTER has a name, the first identifier must be the name of
 * *POINTER, otherwise it must be the name of one child of *POINTER.
 *
 * Return value: the searching result. NULL if not found.
 **/
ASN1_TYPE
MHD__asn1_find_node (ASN1_TYPE pointer, const char *name)
{
  node_asn *p;
  char *n_end, n[MAX_NAME_SIZE + 1];
  const char *n_start;

  if (pointer == NULL)
    return NULL;

  if (name == NULL)
    return NULL;

  p = pointer;
  n_start = name;

  if (p->name != NULL)
    {                           /* has *pointer got a name ? */
      n_end = strchr (n_start, '.');    /* search the first dot */
      if (n_end)
        {
          memcpy (n, n_start, n_end - n_start);
          n[n_end - n_start] = 0;
          n_start = n_end;
          n_start++;
        }
      else
        {
          MHD__asn1_str_cpy (n, sizeof (n), n_start);
          n_start = NULL;
        }

      while (p)
        {
          if ((p->name) && (!strcmp (p->name, n)))
            break;
          else
            p = p->right;
        }                       /* while */

      if (p == NULL)
        return NULL;
    }
  else
    {                           /* *pointer doesn't have a name */
      if (n_start[0] == 0)
        return p;
    }

  while (n_start)
    {                           /* Has the end of NAME been reached? */
      n_end = strchr (n_start, '.');    /* search the next dot */
      if (n_end)
        {
          memcpy (n, n_start, n_end - n_start);
          n[n_end - n_start] = 0;
          n_start = n_end;
          n_start++;
        }
      else
        {
          MHD__asn1_str_cpy (n, sizeof (n), n_start);
          n_start = NULL;
        }

      if (p->down == NULL)
        return NULL;

      p = p->down;

      /* The identifier "?LAST" indicates the last element
         in the right chain. */
      if (!strcmp (n, "?LAST"))
        {
          if (p == NULL)
            return NULL;
          while (p->right)
            p = p->right;
        }
      else
        {                       /* no "?LAST" */
          while (p)
            {
              if ((p->name) && (!strcmp (p->name, n)))
                break;
              else
                p = p->right;
            }
          if (p == NULL)
            return NULL;
        }
    }                           /* while */

  return p;
}


/******************************************************************/
/* Function : MHD__asn1_set_value                                     */
/* Description: sets the field VALUE in a NODE_ASN element. The   */
/*              previous value (if exist) will be lost            */
/* Parameters:                                                    */
/*   node: element pointer.                                       */
/*   value: pointer to the value that you want to set.            */
/*   len: character number of value.                              */
/* Return: pointer to the NODE_ASN element.                       */
/******************************************************************/
node_asn *
MHD__asn1_set_value (node_asn * node, const void *_value, unsigned int len)
{
  const unsigned char *value = _value;

  if (node == NULL)
    return node;
  if (node->value)
    {
      MHD__asn1_free (node->value);
      node->value = NULL;
      node->value_len = 0;
    }
  if (!len)
    return node;
  node->value = (unsigned char *) MHD__asn1_malloc (len);
  if (node->value == NULL)
    return NULL;
  node->value_len = len;

  memcpy (node->value, value, len);
  return node;
}

/******************************************************************/
/* Function : MHD__asn1_set_name                                      */
/* Description: sets the field NAME in a NODE_ASN element. The    */
/*              previous value (if exist) will be lost            */
/* Parameters:                                                    */
/*   node: element pointer.                                       */
/*   name: a null terminated string with the name that you want   */
/*         to set.                                                */
/* Return: pointer to the NODE_ASN element.                       */
/******************************************************************/
node_asn *
MHD__asn1_set_name (node_asn * node, const char *name)
{
  if (node == NULL)
    return node;

  if (node->name)
    {
      MHD__asn1_free (node->name);
      node->name = NULL;
    }

  if (name == NULL)
    return node;

  if (strlen (name))
    {
      node->name = (char *) MHD__asn1_strdup (name);
      if (node->name == NULL)
        return NULL;
    }
  else
    node->name = NULL;
  return node;
}

/******************************************************************/
/* Function : MHD__asn1_set_right                                     */
/* Description: sets the field RIGHT in a NODE_ASN element.       */
/* Parameters:                                                    */
/*   node: element pointer.                                       */
/*   right: pointer to a NODE_ASN element that you want be pointed*/
/*          by NODE.                                              */
/* Return: pointer to *NODE.                                      */
/******************************************************************/
node_asn *
MHD__asn1_set_right (node_asn * node, node_asn * right)
{
  if (node == NULL)
    return node;
  node->right = right;
  if (right)
    right->left = node;
  return node;
}

/******************************************************************/
/* Function : MHD__asn1_set_down                                      */
/* Description: sets the field DOWN in a NODE_ASN element.        */
/* Parameters:                                                    */
/*   node: element pointer.                                       */
/*   down: pointer to a NODE_ASN element that you want be pointed */
/*          by NODE.                                              */
/* Return: pointer to *NODE.                                      */
/******************************************************************/
node_asn *
MHD__asn1_set_down (node_asn * node, node_asn * down)
{
  if (node == NULL)
    return node;
  node->down = down;
  if (down)
    down->left = node;
  return node;
}

/******************************************************************/
/* Function : MHD__asn1_remove_node                                   */
/* Description: gets free the memory allocated for an NODE_ASN    */
/*              element (not the elements pointed by it).         */
/* Parameters:                                                    */
/*   node: NODE_ASN element pointer.                              */
/******************************************************************/
void
MHD__asn1_remove_node (node_asn * node)
{
  if (node == NULL)
    return;

  if (node->name != NULL)
    MHD__asn1_free (node->name);
  if (node->value != NULL)
    MHD__asn1_free (node->value);
  MHD__asn1_free (node);
}

/******************************************************************/
/* Function : MHD__asn1_find_up                                       */
/* Description: return the father of the NODE_ASN element.        */
/* Parameters:                                                    */
/*   node: NODE_ASN element pointer.                              */
/* Return: Null if not found.                                     */
/******************************************************************/
node_asn *
MHD__asn1_find_up (node_asn * node)
{
  node_asn *p;

  if (node == NULL)
    return NULL;

  p = node;

  while ((p->left != NULL) && (p->left->right == p))
    p = p->left;

  return p->left;
}

/******************************************************************/
/* Function : MHD__asn1_delete_list                                   */
/* Description: deletes the list elements (not the elements       */
/*  pointed by them).                                             */
/******************************************************************/
void
MHD__asn1_delete_list (void)
{
  list_type *listElement;

  while (MHD_firstElement)
    {
      listElement = MHD_firstElement;
      MHD_firstElement = MHD_firstElement->next;
      MHD__asn1_free (listElement);
    }
}

/******************************************************************/
/* Function : MHD__asn1_delete_list_and nodes                         */
/* Description: deletes the list elements and the elements        */
/*  pointed by them.                                              */
/******************************************************************/
void
MHD__asn1_delete_list_and_nodes (void)
{
  list_type *listElement;

  while (MHD_firstElement)
    {
      listElement = MHD_firstElement;
      MHD_firstElement = MHD_firstElement->next;
      MHD__asn1_remove_node (listElement->node);
      MHD__asn1_free (listElement);
    }
}


char *
MHD__asn1_ltostr (long v, char *str)
{
  long d, r;
  char temp[20];
  int count, k, start;

  if (v < 0)
    {
      str[0] = '-';
      start = 1;
      v = -v;
    }
  else
    start = 0;

  count = 0;
  do
    {
      d = v / 10;
      r = v - d * 10;
      temp[start + count] = '0' + (char) r;
      count++;
      v = d;
    }
  while (v);

  for (k = 0; k < count; k++)
    str[k + start] = temp[start + count - k - 1];
  str[count + start] = 0;
  return str;
}


/******************************************************************/
/* Function : MHD__asn1_change_integer_value                          */
/* Description: converts into DER coding the value assign to an   */
/*   INTEGER constant.                                            */
/* Parameters:                                                    */
/*   node: root of an ASN1element.                                */
/* Return:                                                        */
/*   ASN1_ELEMENT_NOT_FOUND if NODE is NULL,                       */
/*   otherwise ASN1_SUCCESS                                             */
/******************************************************************/
MHD__asn1_retCode
MHD__asn1_change_integer_value (ASN1_TYPE node)
{
  node_asn *p;
  unsigned char val[SIZEOF_UNSIGNED_LONG_INT];
  unsigned char val2[SIZEOF_UNSIGNED_LONG_INT + 1];
  int len;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = node;
  while (p)
    {
      if ((type_field (p->type) == TYPE_INTEGER) && (p->type & CONST_ASSIGN))
        {
          if (p->value)
            {
              MHD__asn1_convert_integer ((const char *) p->value, val,
                                         sizeof (val), &len);
              MHD__asn1_octet_der (val, len, val2, &len);
              MHD__asn1_set_value (p, val2, len);
            }
        }

      if (p->down)
        {
          p = p->down;
        }
      else
        {
          if (p == node)
            p = NULL;
          else if (p->right)
            p = p->right;
          else
            {
              while (1)
                {
                  p = MHD__asn1_find_up (p);
                  if (p == node)
                    {
                      p = NULL;
                      break;
                    }
                  if (p->right)
                    {
                      p = p->right;
                      break;
                    }
                }
            }
        }
    }

  return ASN1_SUCCESS;
}


/******************************************************************/
/* Function : MHD__asn1_expand_object_id                              */
/* Description: expand the IDs of an OBJECT IDENTIFIER constant.  */
/* Parameters:                                                    */
/*   node: root of an ASN1 element.                               */
/* Return:                                                        */
/*   ASN1_ELEMENT_NOT_FOUND if NODE is NULL,                       */
/*   otherwise ASN1_SUCCESS                                             */
/******************************************************************/
MHD__asn1_retCode
MHD__asn1_expand_object_id (ASN1_TYPE node)
{
  node_asn *p, *p2, *p3, *p4, *p5;
  char name_root[MAX_NAME_SIZE], name2[2 * MAX_NAME_SIZE + 1];
  int move, tlen;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  MHD__asn1_str_cpy (name_root, sizeof (name_root), node->name);

  p = node;
  move = DOWN;

  while (!((p == node) && (move == UP)))
    {
      if (move != UP)
        {
          if ((type_field (p->type) == TYPE_OBJECT_ID)
              && (p->type & CONST_ASSIGN))
            {
              p2 = p->down;
              if (p2 && (type_field (p2->type) == TYPE_CONSTANT))
                {
                  if (p2->value && !isdigit (p2->value[0]))
                    {
                      MHD__asn1_str_cpy (name2, sizeof (name2), name_root);
                      MHD__asn1_str_cat (name2, sizeof (name2), ".");
                      MHD__asn1_str_cat (name2, sizeof (name2),
                                         (const char *) p2->value);
                      p3 = MHD__asn1_find_node (node, name2);
                      if (!p3 || (type_field (p3->type) != TYPE_OBJECT_ID) ||
                          !(p3->type & CONST_ASSIGN))
                        return ASN1_ELEMENT_NOT_FOUND;
                      MHD__asn1_set_down (p, p2->right);
                      MHD__asn1_remove_node (p2);
                      p2 = p;
                      p4 = p3->down;
                      while (p4)
                        {
                          if (type_field (p4->type) == TYPE_CONSTANT)
                            {
                              p5 = MHD__asn1_add_node_only (TYPE_CONSTANT);
                              MHD__asn1_set_name (p5, p4->name);
                              tlen = strlen ((const char *) p4->value);
                              if (tlen > 0)
                                MHD__asn1_set_value (p5, p4->value, tlen + 1);
                              if (p2 == p)
                                {
                                  MHD__asn1_set_right (p5, p->down);
                                  MHD__asn1_set_down (p, p5);
                                }
                              else
                                {
                                  MHD__asn1_set_right (p5, p2->right);
                                  MHD__asn1_set_right (p2, p5);
                                }
                              p2 = p5;
                            }
                          p4 = p4->right;
                        }
                      move = DOWN;
                      continue;
                    }
                }
            }
          move = DOWN;
        }
      else
        move = RIGHT;

      if (move == DOWN)
        {
          if (p->down)
            p = p->down;
          else
            move = RIGHT;
        }

      if (p == node)
        {
          move = UP;
          continue;
        }

      if (move == RIGHT)
        {
          if (p->right)
            p = p->right;
          else
            move = UP;
        }
      if (move == UP)
        p = MHD__asn1_find_up (p);
    }


  /*******************************/
  /*       expand DEFAULT        */
  /*******************************/
  p = node;
  move = DOWN;

  while (!((p == node) && (move == UP)))
    {
      if (move != UP)
        {
          if ((type_field (p->type) == TYPE_OBJECT_ID) &&
              (p->type & CONST_DEFAULT))
            {
              p2 = p->down;
              if (p2 && (type_field (p2->type) == TYPE_DEFAULT))
                {
                  MHD__asn1_str_cpy (name2, sizeof (name2), name_root);
                  MHD__asn1_str_cat (name2, sizeof (name2), ".");
                  MHD__asn1_str_cat (name2, sizeof (name2),
                                     (const char *) p2->value);
                  p3 = MHD__asn1_find_node (node, name2);
                  if (!p3 || (type_field (p3->type) != TYPE_OBJECT_ID) ||
                      !(p3->type & CONST_ASSIGN))
                    return ASN1_ELEMENT_NOT_FOUND;
                  p4 = p3->down;
                  name2[0] = 0;
                  while (p4)
                    {
                      if (type_field (p4->type) == TYPE_CONSTANT)
                        {
                          if (name2[0])
                            MHD__asn1_str_cat (name2, sizeof (name2), ".");
                          MHD__asn1_str_cat (name2, sizeof (name2),
                                             (const char *) p4->value);
                        }
                      p4 = p4->right;
                    }
                  tlen = strlen (name2);
                  if (tlen > 0)
                    MHD__asn1_set_value (p2, name2, tlen + 1);
                }
            }
          move = DOWN;
        }
      else
        move = RIGHT;

      if (move == DOWN)
        {
          if (p->down)
            p = p->down;
          else
            move = RIGHT;
        }

      if (p == node)
        {
          move = UP;
          continue;
        }

      if (move == RIGHT)
        {
          if (p->right)
            p = p->right;
          else
            move = UP;
        }
      if (move == UP)
        p = MHD__asn1_find_up (p);
    }

  return ASN1_SUCCESS;
}

/******************************************************************/
/* Function : MHD__asn1_check_identifier                              */
/* Description: checks the definitions of all the identifiers     */
/*   and the first element of an OBJECT_ID (e.g. {pkix 0 4}).     */
/*   The MHD__asn1_identifierMissing global variable is filled if     */
/*   necessary.                                                   */
/* Parameters:                                                    */
/*   node: root of an ASN1 element.                               */
/* Return:                                                        */
/*   ASN1_ELEMENT_NOT_FOUND      if NODE is NULL,                 */
/*   ASN1_IDENTIFIER_NOT_FOUND   if an identifier is not defined, */
/*   otherwise ASN1_SUCCESS                                       */
/******************************************************************/
MHD__asn1_retCode
MHD__asn1_check_identifier (ASN1_TYPE node)
{
  node_asn *p, *p2;
  char name2[MAX_NAME_SIZE * 2 + 2];

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = node;
  while (p)
    {
      if (type_field (p->type) == TYPE_IDENTIFIER)
        {
          MHD__asn1_str_cpy (name2, sizeof (name2), node->name);
          MHD__asn1_str_cat (name2, sizeof (name2), ".");
          MHD__asn1_str_cat (name2, sizeof (name2), (const char *) p->value);
          p2 = MHD__asn1_find_node (node, name2);
          if (p2 == NULL)
            {
              strcpy (MHD__asn1_identifierMissing, (const char *) p->value);
              return ASN1_IDENTIFIER_NOT_FOUND;
            }
        }
      else if ((type_field (p->type) == TYPE_OBJECT_ID) &&
               (p->type & CONST_DEFAULT))
        {
          p2 = p->down;
          if (p2 && (type_field (p2->type) == TYPE_DEFAULT))
            {
              MHD__asn1_str_cpy (name2, sizeof (name2), node->name);
              MHD__asn1_str_cat (name2, sizeof (name2), ".");
              MHD__asn1_str_cat (name2, sizeof (name2),
                                 (const char *) p2->value);
              strcpy (MHD__asn1_identifierMissing, (const char *) p2->value);
              p2 = MHD__asn1_find_node (node, name2);
              if (!p2 || (type_field (p2->type) != TYPE_OBJECT_ID) ||
                  !(p2->type & CONST_ASSIGN))
                return ASN1_IDENTIFIER_NOT_FOUND;
              else
                MHD__asn1_identifierMissing[0] = 0;
            }
        }
      else if ((type_field (p->type) == TYPE_OBJECT_ID) &&
               (p->type & CONST_ASSIGN))
        {
          p2 = p->down;
          if (p2 && (type_field (p2->type) == TYPE_CONSTANT))
            {
              if (p2->value && !isdigit (p2->value[0]))
                {
                  MHD__asn1_str_cpy (name2, sizeof (name2), node->name);
                  MHD__asn1_str_cat (name2, sizeof (name2), ".");
                  MHD__asn1_str_cat (name2, sizeof (name2),
                                     (const char *) p2->value);
                  strcpy (MHD__asn1_identifierMissing,
                          (const char *) p2->value);
                  p2 = MHD__asn1_find_node (node, name2);
                  if (!p2 || (type_field (p2->type) != TYPE_OBJECT_ID) ||
                      !(p2->type & CONST_ASSIGN))
                    return ASN1_IDENTIFIER_NOT_FOUND;
                  else
                    MHD__asn1_identifierMissing[0] = 0;
                }
            }
        }

      if (p->down)
        {
          p = p->down;
        }
      else if (p->right)
        p = p->right;
      else
        {
          while (1)
            {
              p = MHD__asn1_find_up (p);
              if (p == node)
                {
                  p = NULL;
                  break;
                }
              if (p->right)
                {
                  p = p->right;
                  break;
                }
            }
        }
    }

  return ASN1_SUCCESS;
}
