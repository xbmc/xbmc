/*
  @(#) $Id: convert_recode.c,v 1.13 2003/11/17 12:27:39 yeti Exp $
  interface to GNU recode library (`librecode')

  Copyright (C) 2000-2003 David Necas (Yeti) <yeti@physics.muni.cz>

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as published
  by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/
#include "common.h"
#ifdef HAVE_LIBRECODE

#if HAVE_STDBOOL_H
#  include <stdbool.h>
#else /* HAVE_STDBOOL_H */
#  if ! HAVE__BOOL
typedef unsigned char _Bool;
#  endif /* HAVE__BOOL */
#  define bool _Bool
#  define false 0
#  define true 1
#  define __bool_true_false_are_defined 1
#endif /* HAVE_STDBOOL_H */

#include <recodext.h>

#define enca_recode_fail_level RECODE_NOT_CANONICAL

/* request list struct
   (they are cached between convert_recode() calls)
   auto-deallocated at exit */
typedef struct _RecRequest RecRequest;

struct _RecRequest {
  RECODE_REQUEST request; /* the recode request itself */
  char *request_string; /* request string */
  unsigned long int count; /* count, for caching optimization */
  RecRequest *next;
};

/* recode outer (allocated only once, auto-deallocated at exit) */
static RECODE_OUTER outer = NULL;

/* Local prototypes */
static RECODE_REQUEST get_recode_request(const char *encreq);
static void print_recode_warning(enum recode_error err,
                                 const char *fname);

/* convert file using GNU recode library
   returns 0 on success, nonzero error code otherwise */
int
convert_recode(File *file,
               EncaEncoding from_enc)
{
  RECODE_REQUEST request;
  RECODE_TASK task;
  File *tempfile = NULL;
  bool success;
  const char *encreq;

  /* Allocate librecode outer if we are called first time. */
  if (outer == NULL) {
    if ((outer = recode_new_outer(false)) == NULL) {
      fprintf(stderr, "%s: recode library doesn't like us\n",
                      program_name);
      return ERR_LIBCOM;
    }
  }

  /* Construct recode request string,
     try to mimic surfaceless converter now. */
  {
    EncaEncoding enc;

    enc.charset = from_enc.charset;
    enc.surface = from_enc.surface | ENCA_SURFACE_REMOVE;
    encreq = format_request_string(enc, options.target_enc,
                                   ENCA_SURFACE_EOL_LF);
  }
  /* Create a recode request from it. */
  request = get_recode_request(encreq);
  if (request == NULL)
    return ERR_CANNOT;

  /* Now we have to distinguish between file and stdin, namely because
   * in case of stdin, it's first part is already loaded in the buffer. */
  if (file->name != NULL) {
    /* File is a regular file.
       Since recode doesn't recode files in place, we make a temporary file
       and copy contents of file fname to it. */
    if (file_seek(file, 0, SEEK_SET) != 0)
      return ERR_IOFAIL;
    file->buffer->pos = 0;

    if ((tempfile = file_temporary(file->buffer, 1)) == NULL
        || copy_and_convert(file, tempfile, NULL) != 0
        || file_seek(file, 0, SEEK_SET) != 0
        || file_seek(tempfile, 0, SEEK_SET) != 0
        || file_truncate(file, 0) != 0) {
      file_free(tempfile);
      return ERR_IOFAIL;
    }

    /* Create a task from the request. */
    task = recode_new_task(request);
    task->fail_level = enca_recode_fail_level;
    task->abort_level = RECODE_SYSTEM_ERROR;
    task->input.name = NULL;
    task->input.file = tempfile->stream;
    task->output.name = NULL;
    task->output.file = file->stream;

    /* Now run conversion temporary file -> original. */
    success = recode_perform_task(task);

    /* If conversion wasn't successfull, original file is probably damaged
       (damned librecode!) try to restore it from the temporary copy. */
    if (!success) {
      if (task->error_so_far >= RECODE_SYSTEM_ERROR) {
        fprintf(stderr, "%s: librecode probably damaged file `%s'. "
                        "Trying to recover... ",
                        program_name,
                        file->name);
        tempfile->buffer->pos = 0;
        if (file_seek(tempfile, 0, SEEK_SET) != -1
            && file_seek(file, 0, SEEK_SET) != -1
            && file_truncate(file, file->size) == 0
            && copy_and_convert(tempfile, file, NULL) == 0)
          fprintf(stderr, "succeeded.\n");
        else
          fprintf(stderr, "failed\n");
      }
      else
        print_recode_warning(task->error_so_far, file->name);
    }

    recode_delete_task(task);
    file_free(tempfile);
  }
  else {
    /* File is stdin.
       First recode begining saved in io_buffer, then append rest of stdin. */
    enum recode_error errmax = RECODE_NO_ERROR;

    /* Create a task from the request.
     * Set it up for buffer -> stdout conversion */
    task = recode_new_task(request);
    task->fail_level = enca_recode_fail_level;
    task->abort_level = RECODE_SYSTEM_ERROR;
    task->input.name = NULL;
    task->input.file = NULL;
    task->input.buffer = (char*)file->buffer->data;
    task->input.cursor = (char*)file->buffer->data;
    task->input.limit = (char*)file->buffer->data + file->buffer->pos;
    task->output.name = NULL;
    task->output.file = stdout;

    success = recode_perform_task(task);
    if (!success) {
      if (task->error_so_far >= RECODE_SYSTEM_ERROR) {
        fprintf(stderr, "%s: librecode probably damaged `%s'. "
                        "No way to recover in a pipe.\n",
                        program_name,
                        ffname_r(NULL));
        recode_delete_task(task);
        return ERR_IOFAIL;
      }
      else
        errmax = task->error_so_far;
    }
    recode_delete_task(task);

    /* Create a task from the request.
     * Set it up for stdin -> stdout conversion */
    task = recode_new_task(request);
    task->fail_level = enca_recode_fail_level;
    task->abort_level = RECODE_SYSTEM_ERROR;
    task->input.name = NULL;
    task->input.file = stdin;
    task->output.name = NULL;
    task->output.file = stdout;

    success = recode_perform_task(task);
    if (!success) {
      if (task->error_so_far >= RECODE_SYSTEM_ERROR) {
        fprintf(stderr, "%s: librecode probably damaged `%s'. "
                        "No way to recover in a pipe.\n",
                        program_name,
                        ffname_r(NULL));
        recode_delete_task(task);
        return ERR_IOFAIL;
      }
      else {
        if (errmax < task->error_so_far)
          errmax = task->error_so_far;
      }
    }
    if (errmax >= enca_recode_fail_level)
      print_recode_warning(errmax, ffname_r(NULL));

    recode_delete_task(task);
  }

  /* return ERR_IOFAIL on failure since it means file-related problems */
  return success ? ERR_OK : ERR_IOFAIL;
}

/* caching request creator
   returns recode request either found in cache or, if not found, a newly
   created (and immediately put into the cache)
   returns NULL on failure */
static RECODE_REQUEST
get_recode_request(const char *encreq)
{
  static RecRequest *request_cache = NULL; /* recode request cache */

  RECODE_REQUEST request;
  RecRequest *req;

  /* try to find the request in cache (bubble sorting it meanwhile) */
  for (req = request_cache; req != NULL; req = req->next) {
    if (strcmp(req->request_string, encreq) == 0)
      break;

    if (req->next != NULL && req->count < req->next->count) {
      RecRequest tmpreq;
      /* it's easier to exchange contents instead of pointers here */
      tmpreq.request = req->request;
      tmpreq.count = req->count;
      tmpreq.request_string = req->request_string;

      req->request = req->next->request;
      req->count = req->next->count;
      req->request_string = req->next->request_string;

      req->next->request = tmpreq.request;
      req->next->count = tmpreq.count;
      req->next->request_string = tmpreq.request_string;
    }
  }
  /* found, increment usage count and return it */
  if (req != NULL) {
    req->count++;
    return req->request;
  }
  /* request not found, ask for a new one */
  if ((request = recode_new_request(outer)) == NULL) {
    fprintf(stderr, "%s: recode library doesn't accept new requests\n",
            program_name);
    return NULL; /* maybe we could simply abort */
  }
  /* Set some options. */
  request->diacritics_only = request->ascii_graphics = true;
  /* create request from request string */
  if (!recode_scan_request(request, encreq)) {
    if (options.verbosity_level) {
      fprintf(stderr, "%s: errorneous recoding request `%s'\n",
                     program_name,encreq);
    }
    recode_delete_request(request);
    return NULL;
  }
  /* add it to end of cache */
  if ((req = request_cache) != NULL) {
    while (req->next != NULL) req = req->next;
    req->next = NEW(RecRequest, 1);
    req = req->next;
  }
  else {
    req = NEW(RecRequest, 1);
    request_cache = req;
  }
  req->request = request;
  req->request_string = enca_strdup(encreq);
  req->count = 1;
  req->next = NULL;

  return request;
}

static void
print_recode_warning(enum recode_error err,
                     const char *fname)
{
  const char *msg;

  if (options.verbosity_level < 1)
    return;

  switch (err) {
    case RECODE_NOT_CANONICAL:
    msg = "Input is not canonical";
    break;

    case RECODE_AMBIGUOUS_OUTPUT:
    msg = "Conversion leads to ambiguous output";
    break;

    case RECODE_UNTRANSLATABLE:
    msg = "Untranslatable input";
    break;

    case RECODE_INVALID_INPUT:
    msg = "Invalid input";
    break;

    default:
    msg = "Unknown error";
    break;
  }

  fprintf(stderr, "%s: librecode warning: %s in `%s'\n",
                  program_name,
                  msg,
                  fname);
}
#endif /* HAVE_LIBRECODE */

/* vim: ts=2
 */
