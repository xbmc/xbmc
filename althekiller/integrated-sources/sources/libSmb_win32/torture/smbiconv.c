/* 
   Unix SMB/CIFS implementation.
   Charset module tester

   Copyright (C) Jelmer Vernooij 2003
   Based on iconv/icon_prog.c from the GNU C Library, 
      Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

static int
process_block (smb_iconv_t cd, const char *addr, size_t len, FILE *output)
{
#define OUTBUF_SIZE	32768
  const char *start = addr;
  char outbuf[OUTBUF_SIZE];
  char *outptr;
  size_t outlen;
  size_t n;

  while (len > 0)
    {
      outptr = outbuf;
      outlen = OUTBUF_SIZE;
      n = smb_iconv (cd,  &addr, &len, &outptr, &outlen);

      if (outptr != outbuf)
	{
	  /* We have something to write out.  */
	  int errno_save = errno;

	  if (fwrite (outbuf, 1, outptr - outbuf, output)
	      < (size_t) (outptr - outbuf)
	      || ferror (output))
	    {
	      /* Error occurred while printing the result.  */
	      DEBUG (0, ("conversion stopped due to problem in writing the output"));
	      return -1;
	    }

	  errno = errno_save;
	}

      if (errno != E2BIG)
	{
	  /* iconv() ran into a problem.  */
	  switch (errno)
	    {
	    case EILSEQ:
	      DEBUG(0,("illegal input sequence at position %ld", 
		     (long) (addr - start)));
	      break;
	    case EINVAL:
	      DEBUG(0, ("\
incomplete character or shift sequence at end of buffer"));
	      break;
	    case EBADF:
	      DEBUG(0, ("internal error (illegal descriptor)"));
	      break;
	    default:
	      DEBUG(0, ("unknown iconv() error %d", errno));
	      break;
	    }

	  return -1;
	}
    }

  return 0;
}


static int
process_fd (iconv_t cd, int fd, FILE *output)
{
  /* we have a problem with reading from a descriptor since we must not
     provide the iconv() function an incomplete character or shift
     sequence at the end of the buffer.  Since we have to deal with
     arbitrary encodings we must read the whole text in a buffer and
     process it in one step.  */
  static char *inbuf = NULL;
  static size_t maxlen = 0;
  char *inptr = NULL;
  size_t actlen = 0;

  while (actlen < maxlen)
    {
      ssize_t n = read (fd, inptr, maxlen - actlen);

      if (n == 0)
	/* No more text to read.  */
	break;

      if (n == -1)
	{
	  /* Error while reading.  */
	  DEBUG(0, ("error while reading the input"));
	  return -1;
	}

      inptr += n;
      actlen += n;
    }

  if (actlen == maxlen)
    while (1)
      {
	ssize_t n;
	char *new_inbuf;

	/* Increase the buffer.  */
	new_inbuf = (char *) realloc (inbuf, maxlen + 32768);
	if (new_inbuf == NULL)
	  {
	    DEBUG(0, ("unable to allocate buffer for input"));
	    return -1;
	  }
	inbuf = new_inbuf;
	maxlen += 32768;
	inptr = inbuf + actlen;

	do
	  {
	    n = read (fd, inptr, maxlen - actlen);

	    if (n == 0)
	      /* No more text to read.  */
	      break;

	    if (n == -1)
	      {
		/* Error while reading.  */
		DEBUG(0, ("error while reading the input"));
		return -1;
	      }

	    inptr += n;
	    actlen += n;
	  }
	while (actlen < maxlen);

	if (n == 0)
	  /* Break again so we leave both loops.  */
	  break;
      }

  /* Now we have all the input in the buffer.  Process it in one run.  */
  return process_block (cd, inbuf, actlen, output);
}

/* Main function */

int main(int argc, char *argv[])
{
	const char *file = NULL;
	char *from = "";
	char *to = "";
	char *output = NULL;
	const char *preload_modules[] = {NULL, NULL};
	FILE *out = stdout;
	int fd;
	smb_iconv_t cd;

	/* make sure the vars that get altered (4th field) are in
	   a fixed location or certain compilers complain */
	poptContext pc;
	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{ "from-code", 'f', POPT_ARG_STRING, &from, 0, "Encoding of original text" },
		{ "to-code", 't', POPT_ARG_STRING, &to, 0, "Encoding for output" },
		{ "output", 'o', POPT_ARG_STRING, &output, 0, "Write output to this file" },
		{ "preload-modules", 'p', POPT_ARG_STRING, &preload_modules[0], 0, "Modules to load" },
		POPT_COMMON_SAMBA
		POPT_TABLEEND
	};

	setlinebuf(stdout);

	pc = poptGetContext("smbiconv", argc, (const char **) argv,
			    long_options, 0);

	poptSetOtherOptionHelp(pc, "[FILE] ...");
	
	while(poptGetNextOpt(pc) != -1);

	/* the following functions are part of the Samba debugging
	   facilities.  See lib/debug.c */
	setup_logging("smbiconv", True);

	if (preload_modules[0]) smb_load_modules(preload_modules);

	if(output) {
		out = fopen(output, "w");

		if(!out) {
			DEBUG(0, ("Can't open output file '%s': %s, exiting...\n", output, strerror(errno)));
			return 1;
		}
	}

	cd = smb_iconv_open(to, from);
	if((int)cd == -1) {
		DEBUG(0,("unable to find from or to encoding, exiting...\n"));
		return 1;
	}

	while((file = poptGetArg(pc))) {
		if(strcmp(file, "-") == 0) fd = 0;
		else {
			fd = open(file, O_RDONLY);
			
			if(!fd) {
				DEBUG(0, ("Can't open input file '%s': %s, ignoring...\n", file, strerror(errno)));
				continue;
			}
		}

		/* Loop thru all arguments */
		process_fd(cd, fd, out);

		close(fd);
	}
	poptFreeContext(pc);

	fclose(out);

	return 0;
}
