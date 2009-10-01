/* Load needed message catalogs.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined STDC_HEADERS || defined _LIBC
# include <stdlib.h>
#endif

#if defined HAVE_UNISTD_H || defined _LIBC
# include <unistd.h>
#endif

#if (defined HAVE_MMAP && defined HAVE_MUNMAP) || defined _LIBC
# include <sys/mman.h>
#endif

#include "gettext.h"
#include "gettextP.h"

/* @@ end of prolog @@ */

#ifdef _LIBC
/* Rename the non ISO C functions.  This is required by the standard
   because some ISO C functions will require linking with this object
   file and the name space must not be polluted.  */
# define open   __open
# define close  __close
# define read   __read
# define mmap   __mmap
# define munmap __munmap
#endif

/* We need a sign, whether a new catalog was loaded, which can be associated
   with all translations.  This is important if the translations are
   cached by one of GCC's features.  */
int _nl_msg_cat_cntr = 0;


/* Load the message catalogs specified by FILENAME.  If it is no valid
   message catalog do nothing.  */
void
internal_function
_nl_load_domain (domain_file)
     struct loaded_l10nfile *domain_file;
{
  int fd;
  size_t size;
  struct stat st;
  struct mo_file_header *data = (struct mo_file_header *) -1;
#if (defined HAVE_MMAP && defined HAVE_MUNMAP && !defined DISALLOW_MMAP) \
    || defined _LIBC
  int use_mmap = 0;
#endif
  struct loaded_domain *domain;

  domain_file->decided = 1;
  domain_file->data = NULL;

  /* If the record does not represent a valid locale the FILENAME
     might be NULL.  This can happen when according to the given
     specification the locale file name is different for XPG and CEN
     syntax.  */
  if (domain_file->filename == NULL)
    return;

  /* Try to open the addressed file.  */
  fd = open (domain_file->filename, O_RDONLY);
  if (fd == -1)
    return;

  /* We must know about the size of the file.  */
  if (fstat (fd, &st) != 0
      || (size = (size_t) st.st_size) != st.st_size
      || size < sizeof (struct mo_file_header))
    {
      /* Something went wrong.  */
      close (fd);
      return;
    }

#if (defined HAVE_MMAP && defined HAVE_MUNMAP && !defined DISALLOW_MMAP) \
    || defined _LIBC
  /* Now we are ready to load the file.  If mmap() is available we try
     this first.  If not available or it failed we try to load it.  */
  data = (struct mo_file_header *) mmap (NULL, size, PROT_READ,
					 MAP_PRIVATE, fd, 0);

  if (data != (struct mo_file_header *) -1)
    {
      /* mmap() call was successful.  */
      close (fd);
      use_mmap = 1;
    }
#endif

  /* If the data is not yet available (i.e. mmap'ed) we try to load
     it manually.  */
  if (data == (struct mo_file_header *) -1)
    {
      size_t to_read;
      char *read_ptr;

      data = (struct mo_file_header *) malloc (size);
      if (data == NULL)
	return;

      to_read = size;
      read_ptr = (char *) data;
      do
	{
	  long int nb = (long int) read (fd, read_ptr, to_read);
	  if (nb == -1)
	    {
	      close (fd);
	      return;
	    }

	  read_ptr += nb;
	  to_read -= nb;
	}
      while (to_read > 0);

      close (fd);
    }

  /* Using the magic number we can test whether it really is a message
     catalog file.  */
  if (data->magic != _MAGIC && data->magic != _MAGIC_SWAPPED)
    {
      /* The magic number is wrong: not a message catalog file.  */
#if (defined HAVE_MMAP && defined HAVE_MUNMAP && !defined DISALLOW_MMAP) \
    || defined _LIBC
      if (use_mmap)
	munmap ((caddr_t) data, size);
      else
#endif
	free (data);
      return;
    }

  domain_file->data
    = (struct loaded_domain *) malloc (sizeof (struct loaded_domain));
  if (domain_file->data == NULL)
    return;

  domain = (struct loaded_domain *) domain_file->data;
  domain->data = (char *) data;
#if (defined HAVE_MMAP && defined HAVE_MUNMAP && !defined DISALLOW_MMAP) \
    || defined _LIBC
  domain->use_mmap = use_mmap;
#endif
  domain->mmap_size = size;
  domain->must_swap = data->magic != _MAGIC;

  /* Fill in the information about the available tables.  */
  switch (W (domain->must_swap, data->revision))
    {
    case 0:
      domain->nstrings = W (domain->must_swap, data->nstrings);
      domain->orig_tab = (struct string_desc *)
	((char *) data + W (domain->must_swap, data->orig_tab_offset));
      domain->trans_tab = (struct string_desc *)
	((char *) data + W (domain->must_swap, data->trans_tab_offset));
      domain->hash_size = W (domain->must_swap, data->hash_tab_size);
      domain->hash_tab = (nls_uint32 *)
	((char *) data + W (domain->must_swap, data->hash_tab_offset));
      break;
    default:
      /* This is an illegal revision.  */
#if (defined HAVE_MMAP && defined HAVE_MUNMAP && !defined DISALLOW_MMAP) \
    || defined _LIBC
      if (use_mmap)
	munmap ((caddr_t) data, size);
      else
#endif
	free (data);
      free (domain);
      domain_file->data = NULL;
      return;
    }

  /* Show that one domain is changed.  This might make some cached
     translations invalid.  */
  ++_nl_msg_cat_cntr;
}


#ifdef _LIBC
void
internal_function
_nl_unload_domain (domain)
     struct loaded_domain *domain;
{
  if (domain->use_mmap)
    munmap ((caddr_t) domain->data, domain->mmap_size);
  else
    free ((void *) domain->data);

  free (domain);
}
#endif
