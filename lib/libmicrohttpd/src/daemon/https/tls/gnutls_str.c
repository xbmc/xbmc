/*
 * Copyright (C) 2002, 2004, 2005, 2007  Free Software Foundation
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA
 *
 */

#include <gnutls_int.h>
#include <gnutls_errors.h>
#include <gnutls_num.h>
#include <gnutls_str.h>

/* These function are like strcat, strcpy. They only
 * do bound checking (they shouldn't cause buffer overruns),
 * and they always produce null terminated strings.
 *
 * They should be used only with null terminated strings.
 */
void
MHD_gtls_str_cat (char *dest, size_t dest_tot_size, const char *src)
{
  size_t str_size = strlen (src);
  size_t dest_size = strlen (dest);

  if (dest_tot_size - dest_size > str_size)
    {
      strcat (dest, src);
    }
  else
    {
      if (dest_tot_size - dest_size > 0)
        {
          strncat (dest, src, (dest_tot_size - dest_size) - 1);
          dest[dest_tot_size - 1] = 0;
        }
    }
}

void
MHD_gtls_str_cpy (char *dest, size_t dest_tot_size, const char *src)
{
  size_t str_size = strlen (src);

  if (dest_tot_size > str_size)
    {
      strcpy (dest, src);
    }
  else
    {
      if (dest_tot_size > 0)
        {
          strncpy (dest, src, (dest_tot_size) - 1);
          dest[dest_tot_size - 1] = 0;
        }
    }
}

void
MHD_gtls_string_init (MHD_gtls_string * str,
                      MHD_gnutls_alloc_function alloc_func,
                      MHD_gnutls_realloc_function realloc_func,
                      MHD_gnutls_free_function free_func)
{
  str->data = NULL;
  str->max_length = 0;
  str->length = 0;

  str->alloc_func = alloc_func;
  str->free_func = free_func;
  str->realloc_func = realloc_func;
}

void
MHD_gtls_string_clear (MHD_gtls_string * str)
{
  if (str == NULL || str->data == NULL)
    return;
  str->free_func (str->data);

  str->data = NULL;
  str->max_length = 0;
  str->length = 0;
}

#define MIN_CHUNK 256


int
MHD_gtls_string_append_data (MHD_gtls_string * dest,
                             const void *data, size_t data_size)
{
  size_t tot_len = data_size + dest->length;

  if (dest->max_length >= tot_len)
    {
      memcpy (&dest->data[dest->length], data, data_size);
      dest->length = tot_len;

      return tot_len;
    }
  else
    {
      size_t new_len =
        MAX (data_size, MIN_CHUNK) + MAX (dest->max_length, MIN_CHUNK);
      dest->data = dest->realloc_func (dest->data, new_len);
      if (dest->data == NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_MEMORY_ERROR;
        }
      dest->max_length = new_len;

      memcpy (&dest->data[dest->length], data, data_size);
      dest->length = tot_len;

      return tot_len;
    }
}

/* Converts the given string (old) to hex. A buffer must be provided
 * to hold the new hex string. The new string will be null terminated.
 * If the buffer does not have enough space to hold the string, a
 * truncated hex string is returned (always null terminated).
 */
char *
MHD_gtls_bin2hex (const void *_old,
                  size_t oldlen, char *buffer, size_t buffer_size)
{
  unsigned int i, j;
  const opaque *old = _old;

  for (i = j = 0; i < oldlen && j + 2 < buffer_size; j += 2)
    {
      sprintf (&buffer[j], "%.2x", old[i]);
      i++;
    }
  buffer[j] = '\0';

  return buffer;
}
