/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006 Free Software Foundation
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
#include <libtasn1.h>
#include <gnutls_dh.h>

/* this is used in order to make the multi-threaded initialization call to libgcrypt */
#include <pthread.h>
#include <gcrypt.h>

/* used to set the MHD_tls logging function */
#include "internal.h"

/* TODO fix :  needed by GCRY_THREAD_OPTION_PTHREAD_IMPL but missing otherwise */
#define ENOMEM    12            /* Out of memory */

#ifdef HAVE_WINSOCK
# include <winsock2.h>
#endif


GCRY_THREAD_OPTION_PTHREAD_IMPL;

#define MHD_gnutls_log_func LOG_FUNC

/* created by asn1c */
extern const ASN1_ARRAY_TYPE MHD_gnutlsMHD__asn1_tab[];
extern const ASN1_ARRAY_TYPE MHD_pkix_asn1_tab[];

LOG_FUNC MHD__gnutls_log_func;
int MHD__gnutls_log_level = 0;  /* default log level */

ASN1_TYPE MHD__gnutls_pkix1_asn;
ASN1_TYPE MHD__gnutlsMHD__gnutls_asn;

/**
 * MHD_gtls_global_set_log_function - This function sets the logging function
 * @log_func: it's a log function
 *
 * This is the function where you set the logging function gnutls
 * is going to use. This function only accepts a character array.
 * Normally you may not use this function since it is only used
 * for debugging purposes.
 *
 * MHD_gnutls_log_func is of the form,
 * void (*MHD_gnutls_log_func)( int level, const char*);
 **/
void
MHD_gtls_global_set_log_function (MHD_gnutls_log_func log_func)
{
  MHD__gnutls_log_func = log_func;
}

/**
 * MHD_gtls_global_set_log_level - This function sets the logging level
 * @level: it's an integer from 0 to 9.
 *
 * This is the function that allows you to set the log level.
 * The level is an integer between 0 and 9. Higher values mean
 * more verbosity. The default value is 0. Larger values should
 * only be used with care, since they may reveal sensitive information.
 *
 * Use a log level over 10 to enable all debugging options.
 *
 **/
void
MHD_gtls_global_set_log_level (int level)
{
  MHD__gnutls_log_level = level;
}

int MHD__gnutls_is_secure_mem_null (const void *);

static int MHD__gnutls_init_level = 0;

/**
 * MHD__gnutls_global_init - This function initializes the global data to defaults.
 *
 * This function initializes the global data to defaults.
 * Every gnutls application has a global data which holds common parameters
 * shared by gnutls session structures.
 * You must call MHD__gnutls_global_deinit() when gnutls usage is no longer needed
 * Returns zero on success.
 *
 * Note that this function will also initialize libgcrypt, if it has not
 * been initialized before. Thus if you want to manually initialize libgcrypt
 * you must do it before calling this function. This is useful in cases you
 * want to disable libgcrypt's internal lockings etc.
 *
 * This function increment a global counter, so that
 * MHD__gnutls_global_deinit() only releases resources when it has been
 * called as many times as MHD__gnutls_global_init().  This is useful when
 * GnuTLS is used by more than one library in an application.  This
 * function can be called many times, but will only do something the
 * first time.
 *
 * Note!  This function is not thread safe.  If two threads call this
 * function simultaneously, they can cause a race between checking
 * the global counter and incrementing it, causing both threads to
 * execute the library initialization code.  That would lead to a
 * memory leak.  To handle this, your application could invoke this
 * function after aquiring a thread mutex.  To ignore the potential
 * memory leak is also an option.
 *
 **/
int
MHD__gnutls_global_init ()
{
  int result = 0;
  int res;

  if (MHD__gnutls_init_level++)
    return 0;

#if HAVE_WINSOCK
  {
    WORD requested;
    WSADATA data;
    int err;

    requested = MAKEWORD (1, 1);
    err = WSAStartup (requested, &data);
    if (err != 0)
      {
        MHD__gnutls_debug_log ("WSAStartup failed: %d.\n", err);
        return GNUTLS_E_LIBRARY_VERSION_MISMATCH;
      }

    if (data.wVersion < requested)
      {
        MHD__gnutls_debug_log ("WSAStartup version check failed (%d < %d).\n",
                               data.wVersion, requested);
        WSACleanup ();
        return GNUTLS_E_LIBRARY_VERSION_MISMATCH;
      }
  }
#endif

  /* bindtextdomain("mhd", "./"); */

  if (gcry_control (GCRYCTL_ANY_INITIALIZATION_P) == 0)
    {
      const char *p;

      /* to enable multi-threading this call must precede any other call made to libgcrypt */
      gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);

      /* set p to point at the required version of gcrypt */
      p = strchr (MHD_GCRYPT_VERSION, ':');
      if (p == NULL)
        p = MHD_GCRYPT_VERSION;
      else
        p++;

      /* this call initializes libgcrypt */
      if (gcry_check_version (p) == NULL)
        {
          MHD_gnutls_assert ();
          MHD__gnutls_debug_log ("Checking for libgcrypt failed '%s'\n", p);
          return GNUTLS_E_INCOMPATIBLE_GCRYPT_LIBRARY;
        }

      /* for gcrypt in order to be able to allocate memory */
      gcry_set_allocation_handler (MHD_gnutls_malloc,
                                   MHD_gnutls_secure_malloc,
                                   MHD__gnutls_is_secure_memory,
                                   MHD_gnutls_realloc, MHD_gnutls_free);

      /* gcry_control (GCRYCTL_DISABLE_INTERNAL_LOCKING, NULL, 0); */

      gcry_control (GCRYCTL_INITIALIZATION_FINISHED, NULL, 0);

    }

  if (MHD_gc_init () != GC_OK)
    {
      MHD_gnutls_assert ();
      MHD__gnutls_debug_log ("Initializing crypto backend failed\n");
      return GNUTLS_E_INCOMPATIBLE_CRYPTO_LIBRARY;
    }

  /* initialize parser
   * This should not deal with files in the final
   * version.
   */
  res =
    MHD__asn1_array2tree (MHD_pkix_asn1_tab, &MHD__gnutls_pkix1_asn, NULL);
  if (res != ASN1_SUCCESS)
    {
      result = MHD_gtls_asn2err (res);
      return result;
    }

  res =
    MHD__asn1_array2tree (MHD_gnutlsMHD__asn1_tab,
                          &MHD__gnutlsMHD__gnutls_asn, NULL);
  if (res != ASN1_SUCCESS)
    {
      MHD__asn1_delete_structure (&MHD__gnutls_pkix1_asn);
      result = MHD_gtls_asn2err (res);
      return result;
    }

  return result;
}

/**
 * MHD__gnutls_global_deinit - This function deinitializes the global data
 *
 * This function deinitializes the global data, that were initialized
 * using MHD__gnutls_global_init().
 *
 * Note!  This function is not thread safe.  See the discussion for
 * MHD__gnutls_global_init() for more information.
 *
 **/
void
MHD__gnutls_global_deinit ()
{
  if (MHD__gnutls_init_level == 1)
    {
#if HAVE_WINSOCK
      WSACleanup ();
#endif
      MHD__asn1_delete_structure (&MHD__gnutlsMHD__gnutls_asn);
      MHD__asn1_delete_structure (&MHD__gnutls_pkix1_asn);
      MHD_gc_done ();
    }
  MHD__gnutls_init_level--;
}

/* These functions should be elsewere. Kept here for
 * historical reasons.
 */

/**
 * MHD__gnutls_transport_set_pull_function - This function sets a read like function
 * @pull_func: a callback function similar to read()
 * @session: gnutls session
 *
 * This is the function where you set a function for gnutls
 * to receive data. Normally, if you use berkeley style sockets,
 * do not need to use this function since the default (recv(2)) will
 * probably be ok.
 *
 * PULL_FUNC is of the form,
 * ssize_t (*MHD_gtls_pull_func)(MHD_gnutls_transport_ptr_t, void*, size_t);
 **/
void
MHD__gnutls_transport_set_pull_function (MHD_gtls_session_t session,
                                         MHD_gtls_pull_func pull_func)
{
  session->internals.MHD__gnutls_pull_func = pull_func;
}

/**
 * MHD__gnutls_transport_set_push_function - This function sets the function to send data
 * @push_func: a callback function similar to write()
 * @session: gnutls session
 *
 * This is the function where you set a push function for gnutls
 * to use in order to send data. If you are going to use berkeley style
 * sockets, you do not need to use this function since
 * the default (send(2)) will probably be ok. Otherwise you should
 * specify this function for gnutls to be able to send data.
 *
 * PUSH_FUNC is of the form,
 * ssize_t (*MHD_gtls_push_func)(MHD_gnutls_transport_ptr_t, const void*, size_t);
 **/
void
MHD__gnutls_transport_set_push_function (MHD_gtls_session_t session,
                                         MHD_gtls_push_func push_func)
{
  session->internals.MHD__gnutls_push_func = push_func;
}
