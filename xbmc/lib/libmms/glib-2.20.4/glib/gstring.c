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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "glib.h"
#include "gprintf.h"

#include "galias.h"

struct _GStringChunk
{
  GHashTable *const_table;
  GSList     *storage_list;
  gsize       storage_next;    
  gsize       this_size;       
  gsize       default_size;    
};

/* Hash Functions.
 */

/**
 * g_str_equal:
 * @v1: a key
 * @v2: a key to compare with @v1
 * 
 * Compares two strings for byte-by-byte equality and returns %TRUE 
 * if they are equal. It can be passed to g_hash_table_new() as the 
 * @key_equal_func parameter, when using strings as keys in a #GHashTable.
 *
 * Returns: %TRUE if the two keys match
 */
gboolean
g_str_equal (gconstpointer v1,
	     gconstpointer v2)
{
  const gchar *string1 = v1;
  const gchar *string2 = v2;
  
  return strcmp (string1, string2) == 0;
}

/**
 * g_str_hash:
 * @v: a string key
 *
 * Converts a string to a hash value.
 * It can be passed to g_hash_table_new() as the @hash_func 
 * parameter, when using strings as keys in a #GHashTable.
 *
 * Returns: a hash value corresponding to the key
 */
guint
g_str_hash (gconstpointer v)
{
  /* 31 bit hash function */
  const signed char *p = v;
  guint32 h = *p;

  if (h)
    for (p += 1; *p != '\0'; p++)
      h = (h << 5) - h + *p;

  return h;
}

#define MY_MAXSIZE ((gsize)-1)

static inline gsize
nearest_power (gsize base, gsize num)    
{
  if (num > MY_MAXSIZE / 2)
    {
      return MY_MAXSIZE;
    }
  else
    {
      gsize n = base;

      while (n < num)
	n <<= 1;
      
      return n;
    }
}

/* String Chunks.
 */

/**
 * g_string_chunk_new:
 * @size: the default size of the blocks of memory which are 
 *        allocated to store the strings. If a particular string 
 *        is larger than this default size, a larger block of 
 *        memory will be allocated for it.
 * 
 * Creates a new #GStringChunk. 
 * 
 * Returns: a new #GStringChunk
 */
GStringChunk*
g_string_chunk_new (gsize size)    
{
  GStringChunk *new_chunk = g_new (GStringChunk, 1);
  gsize actual_size = 1;    

  actual_size = nearest_power (1, size);

  new_chunk->const_table       = NULL;
  new_chunk->storage_list      = NULL;
  new_chunk->storage_next      = actual_size;
  new_chunk->default_size      = actual_size;
  new_chunk->this_size         = actual_size;

  return new_chunk;
}

/**
 * g_string_chunk_free:
 * @chunk: a #GStringChunk 
 * 
 * Frees all memory allocated by the #GStringChunk.
 * After calling g_string_chunk_free() it is not safe to
 * access any of the strings which were contained within it.
 */
void
g_string_chunk_free (GStringChunk *chunk)
{
  GSList *tmp_list;

  g_return_if_fail (chunk != NULL);

  if (chunk->storage_list)
    {
      for (tmp_list = chunk->storage_list; tmp_list; tmp_list = tmp_list->next)
	g_free (tmp_list->data);

      g_slist_free (chunk->storage_list);
    }

  if (chunk->const_table)
    g_hash_table_destroy (chunk->const_table);

  g_free (chunk);
}

/**
 * g_string_chunk_clear:
 * @chunk: a #GStringChunk
 *
 * Frees all strings contained within the #GStringChunk.
 * After calling g_string_chunk_clear() it is not safe to
 * access any of the strings which were contained within it.
 *
 * Since: 2.14
 */
void
g_string_chunk_clear (GStringChunk *chunk)
{
  GSList *tmp_list;

  g_return_if_fail (chunk != NULL);

  if (chunk->storage_list)
    {
      for (tmp_list = chunk->storage_list; tmp_list; tmp_list = tmp_list->next)
        g_free (tmp_list->data);

      g_slist_free (chunk->storage_list);

      chunk->storage_list       = NULL;
      chunk->storage_next       = chunk->default_size;
      chunk->this_size          = chunk->default_size;
    }

  if (chunk->const_table)
      g_hash_table_remove_all (chunk->const_table);
}

/**
 * g_string_chunk_insert:
 * @chunk: a #GStringChunk
 * @string: the string to add
 * 
 * Adds a copy of @string to the #GStringChunk.
 * It returns a pointer to the new copy of the string 
 * in the #GStringChunk. The characters in the string 
 * can be changed, if necessary, though you should not 
 * change anything after the end of the string.
 *
 * Unlike g_string_chunk_insert_const(), this function 
 * does not check for duplicates. Also strings added 
 * with g_string_chunk_insert() will not be searched 
 * by g_string_chunk_insert_const() when looking for 
 * duplicates.
 * 
 * Returns: a pointer to the copy of @string within 
 *          the #GStringChunk
 */
gchar*
g_string_chunk_insert (GStringChunk *chunk,
		       const gchar  *string)
{
  g_return_val_if_fail (chunk != NULL, NULL);

  return g_string_chunk_insert_len (chunk, string, -1);
}

/**
 * g_string_chunk_insert_const:
 * @chunk: a #GStringChunk
 * @string: the string to add
 *
 * Adds a copy of @string to the #GStringChunk, unless the same
 * string has already been added to the #GStringChunk with
 * g_string_chunk_insert_const().
 *
 * This function is useful if you need to copy a large number
 * of strings but do not want to waste space storing duplicates.
 * But you must remember that there may be several pointers to
 * the same string, and so any changes made to the strings
 * should be done very carefully.
 *
 * Note that g_string_chunk_insert_const() will not return a
 * pointer to a string added with g_string_chunk_insert(), even
 * if they do match.
 *
 * Returns: a pointer to the new or existing copy of @string
 *          within the #GStringChunk
 */
gchar*
g_string_chunk_insert_const (GStringChunk *chunk,
			     const gchar  *string)
{
  char* lookup;

  g_return_val_if_fail (chunk != NULL, NULL);

  if (!chunk->const_table)
    chunk->const_table = g_hash_table_new (g_str_hash, g_str_equal);

  lookup = (char*) g_hash_table_lookup (chunk->const_table, (gchar *)string);

  if (!lookup)
    {
      lookup = g_string_chunk_insert (chunk, string);
      g_hash_table_insert (chunk->const_table, lookup, lookup);
    }

  return lookup;
}

/**
 * g_string_chunk_insert_len:
 * @chunk: a #GStringChunk
 * @string: bytes to insert
 * @len: number of bytes of @string to insert, or -1 to insert a
 *     nul-terminated string
 *
 * Adds a copy of the first @len bytes of @string to the #GStringChunk.
 * The copy is nul-terminated.
 *
 * Since this function does not stop at nul bytes, it is the caller's
 * responsibility to ensure that @string has at least @len addressable
 * bytes.
 *
 * The characters in the returned string can be changed, if necessary,
 * though you should not change anything after the end of the string.
 *
 * Return value: a pointer to the copy of @string within the #GStringChunk
 *
 * Since: 2.4
 */
gchar*
g_string_chunk_insert_len (GStringChunk *chunk,
			   const gchar  *string,
			   gssize        len)
{
  gssize size;
  gchar* pos;

  g_return_val_if_fail (chunk != NULL, NULL);

  if (len < 0)
    size = strlen (string);
  else
    size = len;

  if ((chunk->storage_next + size + 1) > chunk->this_size)
    {
      gsize new_size = nearest_power (chunk->default_size, size + 1);

      chunk->storage_list = g_slist_prepend (chunk->storage_list,
					     g_new (gchar, new_size));

      chunk->this_size = new_size;
      chunk->storage_next = 0;
    }

  pos = ((gchar *) chunk->storage_list->data) + chunk->storage_next;

  *(pos + size) = '\0';

  memcpy (pos, string, size);

  chunk->storage_next += size + 1;

  return pos;
}

/* Strings.
 */
static void
g_string_maybe_expand (GString* string,
		       gsize    len) 
{
  if (string->len + len >= string->allocated_len)
    {
      string->allocated_len = nearest_power (1, string->len + len + 1);
      string->str = g_realloc (string->str, string->allocated_len);
    }
}

/**
 * g_string_sized_new:
 * @dfl_size: the default size of the space allocated to 
 *            hold the string
 *
 * Creates a new #GString, with enough space for @dfl_size 
 * bytes. This is useful if you are going to add a lot of 
 * text to the string and don't want it to be reallocated 
 * too often.
 *
 * Returns: the new #GString
 */
GString*
g_string_sized_new (gsize dfl_size)    
{
  GString *string = g_slice_new (GString);

  string->allocated_len = 0;
  string->len   = 0;
  string->str   = NULL;

  g_string_maybe_expand (string, MAX (dfl_size, 2));
  string->str[0] = 0;

  return string;
}

/**
 * g_string_new:
 * @init: the initial text to copy into the string
 * 
 * Creates a new #GString, initialized with the given string.
 *
 * Returns: the new #GString
 */
GString*
g_string_new (const gchar *init)
{
  GString *string;

  if (init == NULL || *init == '\0')
    string = g_string_sized_new (2);
  else 
    {
      gint len;

      len = strlen (init);
      string = g_string_sized_new (len + 2);

      g_string_append_len (string, init, len);
    }

  return string;
}

/**
 * g_string_new_len:
 * @init: initial contents of the string
 * @len: length of @init to use
 *
 * Creates a new #GString with @len bytes of the @init buffer.  
 * Because a length is provided, @init need not be nul-terminated,
 * and can contain embedded nul bytes.
 *
 * Since this function does not stop at nul bytes, it is the caller's
 * responsibility to ensure that @init has at least @len addressable 
 * bytes.
 *
 * Returns: a new #GString
 */
GString*
g_string_new_len (const gchar *init,
                  gssize       len)    
{
  GString *string;

  if (len < 0)
    return g_string_new (init);
  else
    {
      string = g_string_sized_new (len);
      
      if (init)
        g_string_append_len (string, init, len);
      
      return string;
    }
}

/**
 * g_string_free:
 * @string: a #GString
 * @free_segment: if %TRUE the actual character data is freed as well
 *
 * Frees the memory allocated for the #GString.
 * If @free_segment is %TRUE it also frees the character data.
 *
 * Returns: the character data of @string 
 *          (i.e. %NULL if @free_segment is %TRUE)
 */
gchar*
g_string_free (GString *string,
	       gboolean free_segment)
{
  gchar *segment;

  g_return_val_if_fail (string != NULL, NULL);

  if (free_segment)
    {
      g_free (string->str);
      segment = NULL;
    }
  else
    segment = string->str;

  g_slice_free (GString, string);

  return segment;
}

/**
 * g_string_equal:
 * @v: a #GString
 * @v2: another #GString
 *
 * Compares two strings for equality, returning %TRUE if they are equal. 
 * For use with #GHashTable.
 *
 * Returns: %TRUE if they strings are the same length and contain the 
 *   same bytes
 */
gboolean
g_string_equal (const GString *v,
                const GString *v2)
{
  gchar *p, *q;
  GString *string1 = (GString *) v;
  GString *string2 = (GString *) v2;
  gsize i = string1->len;    

  if (i != string2->len)
    return FALSE;

  p = string1->str;
  q = string2->str;
  while (i)
    {
      if (*p != *q)
	return FALSE;
      p++;
      q++;
      i--;
    }
  return TRUE;
}

/**
 * g_string_hash:
 * @str: a string to hash
 *
 * Creates a hash code for @str; for use with #GHashTable.
 *
 * Returns: hash code for @str
 */
/* 31 bit hash function */
guint
g_string_hash (const GString *str)
{
  const gchar *p = str->str;
  gsize n = str->len;    
  guint h = 0;

  while (n--)
    {
      h = (h << 5) - h + *p;
      p++;
    }

  return h;
}

/**
 * g_string_assign:
 * @string: the destination #GString. Its current contents 
 *          are destroyed.
 * @rval: the string to copy into @string
 *
 * Copies the bytes from a string into a #GString, 
 * destroying any previous contents. It is rather like 
 * the standard strcpy() function, except that you do not 
 * have to worry about having enough space to copy the string.
 *
 * Returns: @string
 */
GString*
g_string_assign (GString     *string,
		 const gchar *rval)
{
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (rval != NULL, string);

  /* Make sure assigning to itself doesn't corrupt the string.  */
  if (string->str != rval)
    {
      /* Assigning from substring should be ok since g_string_truncate
	 does not realloc.  */
      g_string_truncate (string, 0);
      g_string_append (string, rval);
    }

  return string;
}

/**
 * g_string_truncate:
 * @string: a #GString
 * @len: the new size of @string
 *
 * Cuts off the end of the GString, leaving the first @len bytes. 
 *
 * Returns: @string
 */
GString*
g_string_truncate (GString *string,
		   gsize    len)    
{
  g_return_val_if_fail (string != NULL, NULL);

  string->len = MIN (len, string->len);
  string->str[string->len] = 0;

  return string;
}

/**
 * g_string_set_size:
 * @string: a #GString
 * @len: the new length
 * 
 * Sets the length of a #GString. If the length is less than
 * the current length, the string will be truncated. If the
 * length is greater than the current length, the contents
 * of the newly added area are undefined. (However, as
 * always, string->str[string->len] will be a nul byte.) 
 * 
 * Return value: @string
 **/
GString*
g_string_set_size (GString *string,
		   gsize    len)    
{
  g_return_val_if_fail (string != NULL, NULL);

  if (len >= string->allocated_len)
    g_string_maybe_expand (string, len - string->len);
  
  string->len = len;
  string->str[len] = 0;

  return string;
}

/**
 * g_string_insert_len:
 * @string: a #GString
 * @pos: position in @string where insertion should 
 *       happen, or -1 for at the end
 * @val: bytes to insert
 * @len: number of bytes of @val to insert
 * 
 * Inserts @len bytes of @val into @string at @pos.  
 * Because @len is provided, @val may contain embedded 
 * nuls and need not be nul-terminated. If @pos is -1, 
 * bytes are inserted at the end of the string.
 *
 * Since this function does not stop at nul bytes, it is 
 * the caller's responsibility to ensure that @val has at 
 * least @len addressable bytes.
 *
 * Returns: @string
 */
GString*
g_string_insert_len (GString     *string,
		     gssize       pos,    
		     const gchar *val,
		     gssize       len)    
{
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (val != NULL, string);

  if (len < 0)
    len = strlen (val);

  if (pos < 0)
    pos = string->len;
  else
    g_return_val_if_fail (pos <= string->len, string);

  /* Check whether val represents a substring of string.  This test
     probably violates chapter and verse of the C standards, since
     ">=" and "<=" are only valid when val really is a substring.
     In practice, it will work on modern archs.  */
  if (val >= string->str && val <= string->str + string->len)
    {
      gsize offset = val - string->str;
      gsize precount = 0;

      g_string_maybe_expand (string, len);
      val = string->str + offset;
      /* At this point, val is valid again.  */

      /* Open up space where we are going to insert.  */
      if (pos < string->len)
	g_memmove (string->str + pos + len, string->str + pos, string->len - pos);

      /* Move the source part before the gap, if any.  */
      if (offset < pos)
	{
	  precount = MIN (len, pos - offset);
	  memcpy (string->str + pos, val, precount);
	}

      /* Move the source part after the gap, if any.  */
      if (len > precount)
	memcpy (string->str + pos + precount,
		val + /* Already moved: */ precount + /* Space opened up: */ len,
		len - precount);
    }
  else
    {
      g_string_maybe_expand (string, len);

      /* If we aren't appending at the end, move a hunk
       * of the old string to the end, opening up space
       */
      if (pos < string->len)
	g_memmove (string->str + pos + len, string->str + pos, string->len - pos);

      /* insert the new string */
      if (len == 1)
        string->str[pos] = *val;
      else
        memcpy (string->str + pos, val, len);
    }

  string->len += len;

  string->str[string->len] = 0;

  return string;
}

#define SUB_DELIM_CHARS  "!$&'()*+,;="

static gboolean
is_valid (char c, const char *reserved_chars_allowed)
{
  if (g_ascii_isalnum (c) ||
      c == '-' ||
      c == '.' ||
      c == '_' ||
      c == '~')
    return TRUE;

  if (reserved_chars_allowed &&
      strchr (reserved_chars_allowed, c) != NULL)
    return TRUE;
  
  return FALSE;
}

static gboolean 
gunichar_ok (gunichar c)
{
  return
    (c != (gunichar) -2) &&
    (c != (gunichar) -1);
}

/**
 * g_string_append_uri_escaped:
 * @string: a #GString
 * @unescaped: a string
 * @reserved_chars_allowed: a string of reserved characters allowed to be used
 * @allow_utf8: set %TRUE if the escaped string may include UTF8 characters
 * 
 * Appends @unescaped to @string, escaped any characters that
 * are reserved in URIs using URI-style escape sequences.
 * 
 * Returns: @string
 *
 * Since: 2.16
 **/
GString *
g_string_append_uri_escaped (GString *string,
			     const char *unescaped,
			     const char *reserved_chars_allowed,
			     gboolean allow_utf8)
{
  unsigned char c;
  const char *end;
  static const gchar hex[16] = "0123456789ABCDEF";

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (unescaped != NULL, NULL);

  end = unescaped + strlen (unescaped);
  
  while ((c = *unescaped) != 0)
    {
      if (c >= 0x80 && allow_utf8 &&
	  gunichar_ok (g_utf8_get_char_validated (unescaped, end - unescaped)))
	{
	  int len = g_utf8_skip [c];
	  g_string_append_len (string, unescaped, len);
	  unescaped += len;
	}
      else if (is_valid (c, reserved_chars_allowed))
	{
	  g_string_append_c (string, c);
	  unescaped++;
	}
      else
	{
	  g_string_append_c (string, '%');
	  g_string_append_c (string, hex[((guchar)c) >> 4]);
	  g_string_append_c (string, hex[((guchar)c) & 0xf]);
	  unescaped++;
	}
    }

  return string;
}

/**
 * g_string_append:
 * @string: a #GString
 * @val: the string to append onto the end of @string
 * 
 * Adds a string onto the end of a #GString, expanding 
 * it if necessary.
 *
 * Returns: @string
 */
GString*
g_string_append (GString     *string,
		 const gchar *val)
{  
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (val != NULL, string);

  return g_string_insert_len (string, -1, val, -1);
}

/**
 * g_string_append_len:
 * @string: a #GString
 * @val: bytes to append
 * @len: number of bytes of @val to use
 * 
 * Appends @len bytes of @val to @string. Because @len is 
 * provided, @val may contain embedded nuls and need not 
 * be nul-terminated.
 * 
 * Since this function does not stop at nul bytes, it is 
 * the caller's responsibility to ensure that @val has at 
 * least @len addressable bytes.
 *
 * Returns: @string
 */
GString*
g_string_append_len (GString	 *string,
                     const gchar *val,
                     gssize       len)    
{
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (val != NULL, string);

  return g_string_insert_len (string, -1, val, len);
}

/**
 * g_string_append_c:
 * @string: a #GString
 * @c: the byte to append onto the end of @string
 *
 * Adds a byte onto the end of a #GString, expanding 
 * it if necessary.
 * 
 * Returns: @string
 */
#undef g_string_append_c
GString*
g_string_append_c (GString *string,
		   gchar    c)
{
  g_return_val_if_fail (string != NULL, NULL);

  return g_string_insert_c (string, -1, c);
}

/**
 * g_string_append_unichar:
 * @string: a #GString
 * @wc: a Unicode character
 * 
 * Converts a Unicode character into UTF-8, and appends it
 * to the string.
 * 
 * Return value: @string
 **/
GString*
g_string_append_unichar (GString  *string,
			 gunichar  wc)
{  
  g_return_val_if_fail (string != NULL, NULL);
  
  return g_string_insert_unichar (string, -1, wc);
}

/**
 * g_string_prepend:
 * @string: a #GString
 * @val: the string to prepend on the start of @string
 *
 * Adds a string on to the start of a #GString, 
 * expanding it if necessary.
 *
 * Returns: @string
 */
GString*
g_string_prepend (GString     *string,
		  const gchar *val)
{
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (val != NULL, string);
  
  return g_string_insert_len (string, 0, val, -1);
}

/**
 * g_string_prepend_len:
 * @string: a #GString
 * @val: bytes to prepend
 * @len: number of bytes in @val to prepend
 *
 * Prepends @len bytes of @val to @string. 
 * Because @len is provided, @val may contain 
 * embedded nuls and need not be nul-terminated.
 *
 * Since this function does not stop at nul bytes, 
 * it is the caller's responsibility to ensure that 
 * @val has at least @len addressable bytes.
 *
 * Returns: @string
 */
GString*
g_string_prepend_len (GString	  *string,
                      const gchar *val,
                      gssize       len)    
{
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (val != NULL, string);

  return g_string_insert_len (string, 0, val, len);
}

/**
 * g_string_prepend_c:
 * @string: a #GString
 * @c: the byte to prepend on the start of the #GString
 *
 * Adds a byte onto the start of a #GString, 
 * expanding it if necessary.
 *
 * Returns: @string
 */
GString*
g_string_prepend_c (GString *string,
		    gchar    c)
{  
  g_return_val_if_fail (string != NULL, NULL);
  
  return g_string_insert_c (string, 0, c);
}

/**
 * g_string_prepend_unichar:
 * @string: a #GString
 * @wc: a Unicode character
 * 
 * Converts a Unicode character into UTF-8, and prepends it
 * to the string.
 * 
 * Return value: @string
 **/
GString*
g_string_prepend_unichar (GString  *string,
			  gunichar  wc)
{  
  g_return_val_if_fail (string != NULL, NULL);
  
  return g_string_insert_unichar (string, 0, wc);
}

/**
 * g_string_insert:
 * @string: a #GString
 * @pos: the position to insert the copy of the string
 * @val: the string to insert
 *
 * Inserts a copy of a string into a #GString, 
 * expanding it if necessary.
 *
 * Returns: @string
 */
GString*
g_string_insert (GString     *string,
		 gssize       pos,    
		 const gchar *val)
{
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (val != NULL, string);
  if (pos >= 0)
    g_return_val_if_fail (pos <= string->len, string);
  
  return g_string_insert_len (string, pos, val, -1);
}

/**
 * g_string_insert_c:
 * @string: a #GString
 * @pos: the position to insert the byte
 * @c: the byte to insert
 *
 * Inserts a byte into a #GString, expanding it if necessary.
 * 
 * Returns: @string
 */
GString*
g_string_insert_c (GString *string,
		   gssize   pos,    
		   gchar    c)
{
  g_return_val_if_fail (string != NULL, NULL);

  g_string_maybe_expand (string, 1);

  if (pos < 0)
    pos = string->len;
  else
    g_return_val_if_fail (pos <= string->len, string);
  
  /* If not just an append, move the old stuff */
  if (pos < string->len)
    g_memmove (string->str + pos + 1, string->str + pos, string->len - pos);

  string->str[pos] = c;

  string->len += 1;

  string->str[string->len] = 0;

  return string;
}

/**
 * g_string_insert_unichar:
 * @string: a #GString
 * @pos: the position at which to insert character, or -1 to
 *       append at the end of the string
 * @wc: a Unicode character
 * 
 * Converts a Unicode character into UTF-8, and insert it
 * into the string at the given position.
 * 
 * Return value: @string
 **/
GString*
g_string_insert_unichar (GString *string,
			 gssize   pos,    
			 gunichar wc)
{
  gint charlen, first, i;
  gchar *dest;

  g_return_val_if_fail (string != NULL, NULL);

  /* Code copied from g_unichar_to_utf() */
  if (wc < 0x80)
    {
      first = 0;
      charlen = 1;
    }
  else if (wc < 0x800)
    {
      first = 0xc0;
      charlen = 2;
    }
  else if (wc < 0x10000)
    {
      first = 0xe0;
      charlen = 3;
    }
   else if (wc < 0x200000)
    {
      first = 0xf0;
      charlen = 4;
    }
  else if (wc < 0x4000000)
    {
      first = 0xf8;
      charlen = 5;
    }
  else
    {
      first = 0xfc;
      charlen = 6;
    }
  /* End of copied code */

  g_string_maybe_expand (string, charlen);

  if (pos < 0)
    pos = string->len;
  else
    g_return_val_if_fail (pos <= string->len, string);

  /* If not just an append, move the old stuff */
  if (pos < string->len)
    g_memmove (string->str + pos + charlen, string->str + pos, string->len - pos);

  dest = string->str + pos;
  /* Code copied from g_unichar_to_utf() */
  for (i = charlen - 1; i > 0; --i)
    {
      dest[i] = (wc & 0x3f) | 0x80;
      wc >>= 6;
    }
  dest[0] = wc | first;
  /* End of copied code */
  
  string->len += charlen;

  string->str[string->len] = 0;

  return string;
}

/**
 * g_string_overwrite:
 * @string: a #GString
 * @pos: the position at which to start overwriting
 * @val: the string that will overwrite the @string starting at @pos
 * 
 * Overwrites part of a string, lengthening it if necessary.
 * 
 * Return value: @string
 *
 * Since: 2.14
 **/
GString *
g_string_overwrite (GString     *string,
		    gsize        pos,
		    const gchar *val)
{
  g_return_val_if_fail (val != NULL, string);
  return g_string_overwrite_len (string, pos, val, strlen (val));
}

/**
 * g_string_overwrite_len:
 * @string: a #GString
 * @pos: the position at which to start overwriting
 * @val: the string that will overwrite the @string starting at @pos
 * @len: the number of bytes to write from @val
 * 
 * Overwrites part of a string, lengthening it if necessary. 
 * This function will work with embedded nuls.
 * 
 * Return value: @string
 *
 * Since: 2.14
 **/
GString *
g_string_overwrite_len (GString     *string,
			gsize        pos,
			const gchar *val,
			gssize       len)
{
  gsize end;

  g_return_val_if_fail (string != NULL, NULL);

  if (!len)
    return string;

  g_return_val_if_fail (val != NULL, string);
  g_return_val_if_fail (pos <= string->len, string);

  if (len < 0)
    len = strlen (val);

  end = pos + len;

  if (end > string->len)
    g_string_maybe_expand (string, end - string->len);

  memcpy (string->str + pos, val, len);

  if (end > string->len)
    {
      string->str[end] = '\0';
      string->len = end;
    }

  return string;
}

/**
 * g_string_erase:
 * @string: a #GString
 * @pos: the position of the content to remove
 * @len: the number of bytes to remove, or -1 to remove all
 *       following bytes
 *
 * Removes @len bytes from a #GString, starting at position @pos.
 * The rest of the #GString is shifted down to fill the gap.
 *
 * Returns: @string
 */
GString*
g_string_erase (GString *string,
		gssize   pos,
		gssize   len)
{
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (pos >= 0, string);
  g_return_val_if_fail (pos <= string->len, string);

  if (len < 0)
    len = string->len - pos;
  else
    {
      g_return_val_if_fail (pos + len <= string->len, string);

      if (pos + len < string->len)
	g_memmove (string->str + pos, string->str + pos + len, string->len - (pos + len));
    }

  string->len -= len;
  
  string->str[string->len] = 0;

  return string;
}

/**
 * g_string_ascii_down:
 * @string: a GString
 * 
 * Converts all upper case ASCII letters to lower case ASCII letters.
 * 
 * Return value: passed-in @string pointer, with all the upper case
 *               characters converted to lower case in place, with
 *               semantics that exactly match g_ascii_tolower().
 **/
GString*
g_string_ascii_down (GString *string)
{
  gchar *s;
  gint n;

  g_return_val_if_fail (string != NULL, NULL);

  n = string->len;
  s = string->str;

  while (n)
    {
      *s = g_ascii_tolower (*s);
      s++;
      n--;
    }

  return string;
}

/**
 * g_string_ascii_up:
 * @string: a GString
 * 
 * Converts all lower case ASCII letters to upper case ASCII letters.
 * 
 * Return value: passed-in @string pointer, with all the lower case
 *               characters converted to upper case in place, with
 *               semantics that exactly match g_ascii_toupper().
 **/
GString*
g_string_ascii_up (GString *string)
{
  gchar *s;
  gint n;

  g_return_val_if_fail (string != NULL, NULL);

  n = string->len;
  s = string->str;

  while (n)
    {
      *s = g_ascii_toupper (*s);
      s++;
      n--;
    }

  return string;
}

/**
 * g_string_down:
 * @string: a #GString
 *  
 * Converts a #GString to lowercase.
 *
 * Returns: the #GString.
 *
 * Deprecated:2.2: This function uses the locale-specific 
 *   tolower() function, which is almost never the right thing. 
 *   Use g_string_ascii_down() or g_utf8_strdown() instead.
 */
GString*
g_string_down (GString *string)
{
  guchar *s;
  glong n;

  g_return_val_if_fail (string != NULL, NULL);

  n = string->len;    
  s = (guchar *) string->str;

  while (n)
    {
      if (isupper (*s))
	*s = tolower (*s);
      s++;
      n--;
    }

  return string;
}

/**
 * g_string_up:
 * @string: a #GString 
 * 
 * Converts a #GString to uppercase.
 * 
 * Return value: @string
 *
 * Deprecated:2.2: This function uses the locale-specific 
 *   toupper() function, which is almost never the right thing. 
 *   Use g_string_ascii_up() or g_utf8_strup() instead.
 **/
GString*
g_string_up (GString *string)
{
  guchar *s;
  glong n;

  g_return_val_if_fail (string != NULL, NULL);

  n = string->len;
  s = (guchar *) string->str;

  while (n)
    {
      if (islower (*s))
	*s = toupper (*s);
      s++;
      n--;
    }

  return string;
}

/**
 * g_string_append_vprintf:
 * @string: a #GString
 * @format: the string format. See the printf() documentation
 * @args: the list of arguments to insert in the output
 *
 * Appends a formatted string onto the end of a #GString.
 * This function is similar to g_string_append_printf()
 * except that the arguments to the format string are passed
 * as a va_list.
 *
 * Since: 2.14
 */
void
g_string_append_vprintf (GString     *string,
			 const gchar *format,
			 va_list      args)
{
  gchar *buf;
  gint len;
  
  g_return_if_fail (string != NULL);
  g_return_if_fail (format != NULL);

  len = g_vasprintf (&buf, format, args);

  if (len >= 0)
    {
      g_string_maybe_expand (string, len);
      memcpy (string->str + string->len, buf, len + 1);
      string->len += len;
      g_free (buf);
    }
}

/**
 * g_string_vprintf:
 * @string: a #GString
 * @format: the string format. See the printf() documentation
 * @args: the parameters to insert into the format string
 *
 * Writes a formatted string into a #GString. 
 * This function is similar to g_string_printf() except that 
 * the arguments to the format string are passed as a va_list.
 *
 * Since: 2.14
 */
void
g_string_vprintf (GString     *string,
		  const gchar *format,
		  va_list      args)
{
  g_string_truncate (string, 0);
  g_string_append_vprintf (string, format, args);
}

/**
 * g_string_sprintf:
 * @string: a #GString
 * @format: the string format. See the sprintf() documentation
 * @Varargs: the parameters to insert into the format string
 *
 * Writes a formatted string into a #GString.
 * This is similar to the standard sprintf() function,
 * except that the #GString buffer automatically expands 
 * to contain the results. The previous contents of the 
 * #GString are destroyed. 
 *
 * Deprecated: This function has been renamed to g_string_printf().
 */

/**
 * g_string_printf:
 * @string: a #GString
 * @format: the string format. See the printf() documentation
 * @Varargs: the parameters to insert into the format string
 *
 * Writes a formatted string into a #GString.
 * This is similar to the standard sprintf() function,
 * except that the #GString buffer automatically expands 
 * to contain the results. The previous contents of the 
 * #GString are destroyed.
 */
void
g_string_printf (GString     *string,
		 const gchar *format,
		 ...)
{
  va_list args;

  g_string_truncate (string, 0);

  va_start (args, format);
  g_string_append_vprintf (string, format, args);
  va_end (args);
}

/**
 * g_string_sprintfa:
 * @string: a #GString
 * @format: the string format. See the sprintf() documentation
 * @Varargs: the parameters to insert into the format string
 *
 * Appends a formatted string onto the end of a #GString.
 * This function is similar to g_string_sprintf() except that
 * the text is appended to the #GString. 
 *
 * Deprecated: This function has been renamed to g_string_append_printf()
 */

/**
 * g_string_append_printf:
 * @string: a #GString
 * @format: the string format. See the printf() documentation
 * @Varargs: the parameters to insert into the format string
 *
 * Appends a formatted string onto the end of a #GString.
 * This function is similar to g_string_printf() except 
 * that the text is appended to the #GString.
 */
void
g_string_append_printf (GString     *string,
			const gchar *format,
			...)
{
  va_list args;

  va_start (args, format);
  g_string_append_vprintf (string, format, args);
  va_end (args);
}

#define __G_STRING_C__
#include "galiasdef.c"
