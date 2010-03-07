/*
     This file is part of libmicrohttpd
     (C) 2008 Christian Grothoff

     libmicrohttpd is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 2, or (at your
     option) any later version.

     libmicrohttpd is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with libmicrohttpd; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

/**
 * @file socat.c
 * @brief  Code to fork-exec zzuf and start the socat process
 * @author Christian Grothoff
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


/**
 * A larger loop count will run more random tests --
 * which would be good, except that it may take too
 * long for most user's patience.  So this small
 * value is the default.
 */
#define LOOP_COUNT 10

#define CURL_TIMEOUT 50L

static pid_t zzuf_pid;

static void
zzuf_socat_start ()
{
  int status;
  char *const args[] = {
    "zzuf",
    "--ratio=0.0:0.75",
    "-n",
    "-A",
    "--",
    "socat",
    "-lf",
    "/dev/null",
    "TCP4-LISTEN:11081,reuseaddr,fork",
    "TCP4:127.0.0.1:11080",
    NULL,
  };
  zzuf_pid = fork ();
  if (zzuf_pid == -1)
    {
      fprintf (stderr, "fork failed: %s\n", strerror (errno));
      exit (1);
    }
  if (zzuf_pid != 0)
    {
      sleep (1);                /* allow zzuf and socat to start */
      status = 0;
      if (0 < waitpid (zzuf_pid, &status, WNOHANG))
        {
          if (WIFEXITED (status))
            fprintf (stderr,
                     "zzuf died with status code %d!\n",
                     WEXITSTATUS (status));
          if (WIFSIGNALED (status))
            fprintf (stderr,
                     "zzuf died from signal %d!\n", WTERMSIG (status));
          exit (1);
        }
      return;
    }
  setpgrp ();
  execvp ("zzuf", args);
  fprintf (stderr, "execution of `zzuf' failed: %s\n", strerror (errno));
  zzuf_pid = 0;                 /* fork failed */
  exit (1);
}


static void
zzuf_socat_stop ()
{
  int status;
  if (zzuf_pid != 0)
    {
      if (0 != killpg (zzuf_pid, SIGINT))
        fprintf (stderr, "Failed to killpg: %s\n", strerror (errno));
      kill (zzuf_pid, SIGINT);
      waitpid (zzuf_pid, &status, 0);
      sleep (1);                /* allow socat to also die in peace */
    }
}

/* end of socat.c */
