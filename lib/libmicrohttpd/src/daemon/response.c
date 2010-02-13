/*
     This file is part of libmicrohttpd
     (C) 2007, 2009 Daniel Pittman and Christian Grothoff

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * @file response.c
 * @brief  Methods for managing response objects
 * @author Daniel Pittman
 * @author Christian Grothoff
 */

#include "internal.h"
#include "response.h"

/**
 * Add a header line to the response.
 *
 * @return MHD_NO on error (i.e. invalid header or content format).
 */
int
MHD_add_response_header (struct MHD_Response *response,
                         const char *header, const char *content)
{
  struct MHD_HTTP_Header *hdr;

  if ((response == NULL) ||
      (header == NULL) ||
      (content == NULL) ||
      (strlen (header) == 0) ||
      (strlen (content) == 0) ||
      (NULL != strstr (header, "\t")) ||
      (NULL != strstr (header, "\r")) ||
      (NULL != strstr (header, "\n")) ||
      (NULL != strstr (content, "\t")) ||
      (NULL != strstr (content, "\r")) || (NULL != strstr (content, "\n")))
    return MHD_NO;
  hdr = malloc (sizeof (struct MHD_HTTP_Header));
  if (hdr == NULL)
    return MHD_NO;
  hdr->header = strdup (header);
  if (hdr->header == NULL)
    {
      free (hdr);
      return MHD_NO;
    }
  hdr->value = strdup (content);
  if (hdr->value == NULL)
    {
      free (hdr->header);
      free (hdr);
      return MHD_NO;
    }
  hdr->kind = MHD_HEADER_KIND;
  hdr->next = response->first_header;
  response->first_header = hdr;
  return MHD_YES;
}

/**
 * Delete a header line from the response.
 *
 * @return MHD_NO on error (no such header known)
 */
int
MHD_del_response_header (struct MHD_Response *response,
                         const char *header, const char *content)
{
  struct MHD_HTTP_Header *pos;
  struct MHD_HTTP_Header *prev;

  if ((header == NULL) || (content == NULL))
    return MHD_NO;
  prev = NULL;
  pos = response->first_header;
  while (pos != NULL)
    {
      if ((0 == strcmp (header, pos->header)) &&
          (0 == strcmp (content, pos->value)))
        {
          free (pos->header);
          free (pos->value);
          if (prev == NULL)
            response->first_header = pos->next;
          else
            prev->next = pos->next;
          free (pos);
          return MHD_YES;
        }
      prev = pos;
      pos = pos->next;
    }
  return MHD_NO;
}

/**
 * Get all of the headers added to a response.
 *
 * @param iterator callback to call on each header;
 *        maybe NULL (then just count headers)
 * @param iterator_cls extra argument to iterator
 * @return number of entries iterated over
 */
int
MHD_get_response_headers (struct MHD_Response *response,
                          MHD_KeyValueIterator iterator, void *iterator_cls)
{
  struct MHD_HTTP_Header *pos;
  int numHeaders = 0;
  pos = response->first_header;
  while (pos != NULL)
    {
      numHeaders++;
      if ((iterator != NULL) &&
          (MHD_YES != iterator (iterator_cls,
                                pos->kind, pos->header, pos->value)))
        break;
      pos = pos->next;
    }
  return numHeaders;
}


/**
 * Get a particular header from the response.
 *
 * @param key which header to get
 * @return NULL if header does not exist
 */
const char *
MHD_get_response_header (struct MHD_Response *response, const char *key)
{
  struct MHD_HTTP_Header *pos;

  if (key == NULL)
    return NULL;
  pos = response->first_header;
  while (pos != NULL)
    {
      if (0 == strcmp (key, pos->header))
        return pos->value;
      pos = pos->next;
    }
  return NULL;
}


/**
 * Create a response object.  The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param size size of the data portion of the response, -1 for unknown
 * @param block_size preferred block size for querying crc (advisory only,
 *                   MHD may still call crc using smaller chunks); this
 *                   is essentially the buffer size used for IO, clients
 *                   should pick a value that is appropriate for IO and
 *                   memory performance requirements
 * @param crc callback to use to obtain response data
 * @param crc_cls extra argument to crc
 * @param crfc callback to call to free crc_cls resources
 * @return NULL on error (i.e. invalid arguments, out of memory)
 */
struct MHD_Response *
MHD_create_response_from_callback (uint64_t size,
                                   size_t block_size,
                                   MHD_ContentReaderCallback crc,
                                   void *crc_cls,
                                   MHD_ContentReaderFreeCallback crfc)
{
  struct MHD_Response *retVal;

  if ((crc == NULL) || (block_size == 0))
    return NULL;
  retVal = malloc (sizeof (struct MHD_Response) + block_size);
  if (retVal == NULL)
    return NULL;
  memset (retVal, 0, sizeof (struct MHD_Response));
  retVal->data = (void *) &retVal[1];
  retVal->data_buffer_size = block_size;
  if (pthread_mutex_init (&retVal->mutex, NULL) != 0)
    {
      free (retVal);
      return NULL;
    }
  retVal->crc = crc;
  retVal->crfc = crfc;
  retVal->crc_cls = crc_cls;
  retVal->reference_count = 1;
  retVal->total_size = size;
  return retVal;
}

/**
 * Create a response object.  The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param size size of the data portion of the response
 * @param data the data itself
 * @param must_free libmicrohttpd should free data when done
 * @param must_copy libmicrohttpd must make a copy of data
 *        right away, the data maybe released anytime after
 *        this call returns
 * @return NULL on error (i.e. invalid arguments, out of memory)
 */
struct MHD_Response *
MHD_create_response_from_data (size_t size,
                               void *data, int must_free, int must_copy)
{
  struct MHD_Response *retVal;
  void *tmp;

  if ((data == NULL) && (size > 0))
    return NULL;
  retVal = malloc (sizeof (struct MHD_Response));
  if (retVal == NULL)
    return NULL;
  memset (retVal, 0, sizeof (struct MHD_Response));
  if (pthread_mutex_init (&retVal->mutex, NULL) != 0)
    {
      free (retVal);
      return NULL;
    }
  if ((must_copy) && (size > 0))
    {
      tmp = malloc (size);
      if (tmp == NULL)
        {
          free (retVal);
          return NULL;
        }
      memcpy (tmp, data, size);
      must_free = 1;
      data = tmp;
    }
  retVal->crc = NULL;
  retVal->crfc = must_free ? &free : NULL;
  retVal->crc_cls = must_free ? data : NULL;
  retVal->reference_count = 1;
  retVal->total_size = size;
  retVal->data = data;
  retVal->data_size = size;
  return retVal;
}

/**
 * Destroy a response object and associated resources.  Note that
 * libmicrohttpd may keep some of the resources around if the response
 * is still in the queue for some clients, so the memory may not
 * necessarily be freed immediatley.
 */
void
MHD_destroy_response (struct MHD_Response *response)
{
  struct MHD_HTTP_Header *pos;

  if (response == NULL)
    return;
  pthread_mutex_lock (&response->mutex);
  if (0 != --response->reference_count)
    {
      pthread_mutex_unlock (&response->mutex);
      return;
    }
  pthread_mutex_unlock (&response->mutex);
  pthread_mutex_destroy (&response->mutex);
  if (response->crfc != NULL)
    response->crfc (response->crc_cls);
  while (response->first_header != NULL)
    {
      pos = response->first_header;
      response->first_header = pos->next;
      free (pos->header);
      free (pos->value);
      free (pos);
    }
  free (response);
}


void
MHD_increment_response_rc (struct MHD_Response *response)
{
  pthread_mutex_lock (&response->mutex);
  response->reference_count++;
  pthread_mutex_unlock (&response->mutex);
}


/* end of response.c */
