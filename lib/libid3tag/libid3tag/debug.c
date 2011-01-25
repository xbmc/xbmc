/*
 * libid3tag - ID3 tag manipulation library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: debug.c,v 1.8 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# undef malloc
# undef calloc
# undef realloc
# undef free

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include "debug.h"

# if defined(DEBUG)

# define DEBUG_MAGIC  0xdeadbeefL

struct debug {
  char const *file;
  unsigned int line;
  size_t size;
  struct debug *next;
  struct debug *prev;
  long int magic;
};

static struct debug *allocated;
static int registered;

static
void check(void)
{
  struct debug *debug;

  for (debug = allocated; debug; debug = debug->next) {
    if (debug->magic != DEBUG_MAGIC) {
      fprintf(stderr, "memory corruption\n");
      break;
    }

    fprintf(stderr, "%s:%u: leaked %lu bytes\n",
	    debug->file, debug->line, debug->size);
  }
}

void *id3_debug_malloc(size_t size, char const *file, unsigned int line)
{
  struct debug *debug;

  if (!registered) {
    atexit(check);
    registered = 1;
  }

  if (size == 0)
    fprintf(stderr, "%s:%u: malloc(0)\n", file, line);

  debug = malloc(sizeof(*debug) + size);
  if (debug == 0) {
    fprintf(stderr, "%s:%u: malloc(%lu) failed\n", file, line, size);
    return 0;
  }

  debug->magic = DEBUG_MAGIC;

  debug->file = file;
  debug->line = line;
  debug->size = size;

  debug->next = allocated;
  debug->prev = 0;

  if (allocated)
    allocated->prev = debug;

  allocated = debug;

  return ++debug;
}

void *id3_debug_calloc(size_t nmemb, size_t size,
		       char const *file, unsigned int line)
{
  void *ptr;

  ptr = id3_debug_malloc(nmemb * size, file, line);
  if (ptr)
    memset(ptr, 0, nmemb * size);

  return ptr;
}

void *id3_debug_realloc(void *ptr, size_t size,
			char const *file, unsigned int line)
{
  struct debug *debug, *new;

  if (size == 0) {
    id3_debug_free(ptr, file, line);
    return 0;
  }

  if (ptr == 0)
    return id3_debug_malloc(size, file, line);

  debug = ptr;
  --debug;

  if (debug->magic != DEBUG_MAGIC) {
    fprintf(stderr, "%s:%u: realloc(%p, %lu) memory not allocated\n",
	    file, line, ptr, size);
    return 0;
  }

  new = realloc(debug, sizeof(*debug) + size);
  if (new == 0) {
    fprintf(stderr, "%s:%u: realloc(%p, %lu) failed\n", file, line, ptr, size);
    return 0;
  }

  if (allocated == debug)
    allocated = new;

  debug = new;

  debug->file = file;
  debug->line = line;
  debug->size = size;

  if (debug->next)
    debug->next->prev = debug;
  if (debug->prev)
    debug->prev->next = debug;

  return ++debug;
}

void id3_debug_free(void *ptr, char const *file, unsigned int line)
{
  struct debug *debug;

  if (ptr == 0) {
    fprintf(stderr, "%s:%u: free(0)\n", file, line);
    return;
  }

  debug = ptr;
  --debug;

  if (debug->magic != DEBUG_MAGIC) {
    fprintf(stderr, "%s:%u: free(%p) memory not allocated\n", file, line, ptr);
    return;
  }

  debug->magic = 0;

  if (debug->next)
    debug->next->prev = debug->prev;
  if (debug->prev)
    debug->prev->next = debug->next;

  if (allocated == debug)
    allocated = debug->next;

  free(debug);
}

void *id3_debug_release(void *ptr, char const *file, unsigned int line)
{
  struct debug *debug;

  if (ptr == 0)
    return 0;

  debug = ptr;
  --debug;

  if (debug->magic != DEBUG_MAGIC) {
    fprintf(stderr, "%s:%u: release(%p) memory not allocated\n",
	    file, line, ptr);
    return ptr;
  }

  if (debug->next)
    debug->next->prev = debug->prev;
  if (debug->prev)
    debug->prev->next = debug->next;

  if (allocated == debug)
    allocated = debug->next;

  memmove(debug, debug + 1, debug->size);

  return debug;
}

# endif
