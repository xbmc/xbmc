/* Emulation for select(2)
   Contributed by Paolo Bonzini.

   Copyright 2008 Free Software Foundation, Inc.

   This file is part of gnulib.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <config.h>
#include <alloca.h>

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
#include <sys/types.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <conio.h>
#include <time.h>

struct bitset {
  unsigned char in[FD_SETSIZE / CHAR_BIT];
  unsigned char out[FD_SETSIZE / CHAR_BIT];
};

/* Declare data structures for ntdll functions.  */
typedef struct _FILE_PIPE_LOCAL_INFORMATION {
  ULONG NamedPipeType;
  ULONG NamedPipeConfiguration;
  ULONG MaximumInstances;
  ULONG CurrentInstances;
  ULONG InboundQuota;
  ULONG ReadDataAvailable;
  ULONG OutboundQuota;
  ULONG WriteQuotaAvailable;
  ULONG NamedPipeState;
  ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _IO_STATUS_BLOCK
{
  union {
    DWORD Status;
    PVOID Pointer;
  } u;
  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef enum _FILE_INFORMATION_CLASS {
  FilePipeLocalInformation = 24
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef DWORD (WINAPI *PNtQueryInformationFile)
	 (HANDLE, IO_STATUS_BLOCK *, VOID *, ULONG, FILE_INFORMATION_CLASS);

#ifndef PIPE_BUF
#define PIPE_BUF	512
#endif

/* Compute output fd_sets for libc descriptor FD (whose Win32 handle is H).  */

static int
win32_poll_handle (HANDLE h, int fd, struct bitset *rbits, struct bitset *wbits,
		   struct bitset *xbits)
{
  BOOL read, write, except;
  int i, ret;
  INPUT_RECORD *irbuffer;
  DWORD avail, nbuffer;
  BOOL bRet;
  IO_STATUS_BLOCK iosb;
  FILE_PIPE_LOCAL_INFORMATION fpli;
  static PNtQueryInformationFile NtQueryInformationFile;
  static BOOL once_only;

  read = write = except = FALSE;
  switch (GetFileType (h))
    {
    case FILE_TYPE_DISK:
      read = TRUE;
      write = TRUE;
      break;

    case FILE_TYPE_PIPE:
      if (!once_only)
	{
	  NtQueryInformationFile = (PNtQueryInformationFile)
	    GetProcAddress (GetModuleHandle ("ntdll.dll"),
			    "NtQueryInformationFile");
	  once_only = TRUE;
	}

      if (PeekNamedPipe (h, NULL, 0, NULL, &avail, NULL) != 0)
	{
	  if (avail)
	    read = TRUE;
	}

      else
	{
	  /* It was the write-end of the pipe.  Check if it is writable.
	     If NtQueryInformationFile fails, optimistically assume the pipe is
	     writable.  This could happen on Win9x, where NtQueryInformationFile
	     is not available, or if we inherit a pipe that doesn't permit
	     FILE_READ_ATTRIBUTES access on the write end (I think this should
	     not happen since WinXP SP2; WINE seems fine too).  Otherwise,
	     ensure that enough space is available for atomic writes.  */
          memset (&iosb, 0, sizeof (iosb));
          memset (&fpli, 0, sizeof (fpli));

          if (!NtQueryInformationFile
              || NtQueryInformationFile (h, &iosb, &fpli, sizeof (fpli),
				         FilePipeLocalInformation)
	      || fpli.WriteQuotaAvailable >= PIPE_BUF
	      || (fpli.OutboundQuota < PIPE_BUF &&
	          fpli.WriteQuotaAvailable == fpli.OutboundQuota))
	    write = TRUE;
	}
      break;

    case FILE_TYPE_CHAR:
      ret = WaitForSingleObject (h, 0);
      write = TRUE;
      if (ret == WAIT_OBJECT_0)
        {
	  nbuffer = avail = 0;
	  bRet = GetNumberOfConsoleInputEvents (h, &nbuffer);
	  if (!bRet || nbuffer == 0)
	    except = TRUE;

	  irbuffer = (INPUT_RECORD *) alloca (nbuffer * sizeof (INPUT_RECORD));
	  bRet = PeekConsoleInput (h, irbuffer, nbuffer, &avail);
	  if (!bRet || avail == 0)
	    except = TRUE;

	  for (i = 0; i < avail; i++)
	    if (irbuffer[i].EventType == KEY_EVENT)
	      read = TRUE;
	}
      break;

    default:
      ret = WaitForSingleObject (h, 0);
      write = TRUE;
      if (ret == WAIT_OBJECT_0)
        read = TRUE;

      break;
    }

  ret = 0;
  if (read && (rbits->in[fd / CHAR_BIT] & (1 << (fd & (CHAR_BIT - 1)))))
    {
      rbits->out[fd / CHAR_BIT] |= (1 << (fd & (CHAR_BIT - 1)));
      ret++;
    }

  if (write && (wbits->in[fd / CHAR_BIT] & (1 << (fd & (CHAR_BIT - 1)))))
    {
      wbits->out[fd / CHAR_BIT] |= (1 << (fd & (CHAR_BIT - 1)));
      ret++;
    }

  if (except && (xbits->in[fd / CHAR_BIT] & (1 << (fd & (CHAR_BIT - 1)))))
    {
      xbits->out[fd / CHAR_BIT] |= (1 << (fd & (CHAR_BIT - 1)));
      ret++;
    }

  return ret;
}

int
rpl_select (int nfds, fd_set *rfds, fd_set *wfds, fd_set *xfds,
            struct timeval *timeout)
{
  static struct timeval tv0;
  static HANDLE hEvent;
  HANDLE h, handle_array[FD_SETSIZE + 2];
  fd_set handle_rfds, handle_wfds, handle_xfds;
  struct bitset rbits, wbits, xbits;
  unsigned char anyfds_in[FD_SETSIZE / CHAR_BIT];
  DWORD ret, wait_timeout, nhandles, nsock;
  MSG msg;
  int i, fd, rc;

  if (nfds > FD_SETSIZE)
    nfds = FD_SETSIZE;

  if (!timeout)
    wait_timeout = INFINITE;
  else
    {
      wait_timeout = timeout->tv_sec + timeout->tv_usec / 1000;

      /* select is also used as a portable usleep.  */
      if (!rfds && !wfds && !xfds)
        {
          Sleep (wait_timeout);
          return 0;
        }
    }

  if (!hEvent)
    hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

  handle_array[0] = hEvent;
  nhandles = 1;
  nsock = 0;

  /* Copy descriptors to bitsets.  */
  memset (&rbits, 0, sizeof (rbits));
  memset (&wbits, 0, sizeof (wbits));
  memset (&xbits, 0, sizeof (xbits));
  memset (anyfds_in, 0, sizeof (anyfds_in));
  if (rfds)
    for (i = 0; i < rfds->fd_count; i++)
      {
        fd = rfds->fd_array[i];
        rbits.in[fd / CHAR_BIT] |= 1 << (fd & (CHAR_BIT - 1));
        anyfds_in[fd / CHAR_BIT] |= 1 << (fd & (CHAR_BIT - 1));
      }
  else
    rfds = (fd_set *) alloca (sizeof (fd_set));

  if (wfds)
    for (i = 0; i < wfds->fd_count; i++)
      {
        fd = wfds->fd_array[i];
        wbits.in[fd / CHAR_BIT] |= 1 << (fd & (CHAR_BIT - 1));
        anyfds_in[fd / CHAR_BIT] |= 1 << (fd & (CHAR_BIT - 1));
      }
  else
    wfds = (fd_set *) alloca (sizeof (fd_set));

  if (xfds)
    for (i = 0; i < xfds->fd_count; i++)
      {
        fd = xfds->fd_array[i];
        xbits.in[fd / CHAR_BIT] |= 1 << (fd & (CHAR_BIT - 1));
        anyfds_in[fd / CHAR_BIT] |= 1 << (fd & (CHAR_BIT - 1));
      }
  else
    xfds = (fd_set *) alloca (sizeof (fd_set));

  /* Zero all the fd_sets, including the application's.  */
  FD_ZERO (rfds);
  FD_ZERO (wfds);
  FD_ZERO (xfds);
  FD_ZERO (&handle_rfds);
  FD_ZERO (&handle_wfds);
  FD_ZERO (&handle_xfds);

  /* Classify handles.  Create fd sets for sockets, poll the others. */
  for (i = 0; i < nfds; i++)
    {
      WSANETWORKEVENTS ev;

      if ((anyfds_in[i / CHAR_BIT] & (1 << (i & (CHAR_BIT - 1)))) == 0)
	continue;

      h = (HANDLE) _get_osfhandle (i);
      if (!h)
        {
	  errno = EBADF;
	  return -1;
        }

      /* Under Wine, it seems that getsockopt returns 0 for pipes too.
	 WSAEnumNetworkEvents instead distinguishes the two correctly.  */
      ev.lNetworkEvents = 0xDEADBEEF;
      WSAEnumNetworkEvents ((SOCKET) h, NULL, &ev);
      if (ev.lNetworkEvents != 0xDEADBEEF)
        {
          int requested = FD_CLOSE;

          /* See above; socket handles are mapped onto select, but we
	     need to map descriptors to handles.  */
          if (rbits.in[i / CHAR_BIT] & (1 << (i & (CHAR_BIT - 1))))
	    {
              requested |= FD_READ | FD_ACCEPT;
	      FD_SET ((SOCKET) h, rfds);
	      FD_SET ((SOCKET) h, &handle_rfds);
	    }
          if (wbits.in[i / CHAR_BIT] & (1 << (i & (CHAR_BIT - 1))))
	    {
              requested |= FD_WRITE | FD_CONNECT;
	      FD_SET ((SOCKET) h, wfds);
	      FD_SET ((SOCKET) h, &handle_wfds);
	    }
          if (xbits.in[i / CHAR_BIT] & (1 << (i & (CHAR_BIT - 1))))
	    {
              requested |= FD_OOB;
	      FD_SET ((SOCKET) h, xfds);
	      FD_SET ((SOCKET) h, &handle_xfds);
	    }

          WSAEventSelect ((SOCKET) h, hEvent, requested);
	  nsock++;
        }
      else
        {
          handle_array[nhandles++] = h;

	  /* Poll now.  If we get an event, do not wait below.  */
          if (wait_timeout != 0
	      && win32_poll_handle (h, i, &rbits, &wbits, &xbits))
	    wait_timeout = 0;
        }
    }

  if (wait_timeout == 0 || nsock == 0)
    rc = 0;
  else
    {
      /* See if we need to wait in the loop below.  If any select is ready,
         do MsgWaitForMultipleObjects anyway to dispatch messages, but
         no need to call select again.  */
      rc = select (0, &handle_rfds, &handle_wfds, &handle_xfds, &tv0);
      if (rc == 0)
	{
	  /* Restore the fd_sets for the other select we do below.  */
          memcpy (&handle_rfds, rfds, sizeof (fd_set));
          memcpy (&handle_wfds, wfds, sizeof (fd_set));
          memcpy (&handle_xfds, xfds, sizeof (fd_set));
	}
      else
        wait_timeout = 0;
    }

  for (;;)
    {
      ret = MsgWaitForMultipleObjects (nhandles, handle_array, FALSE,
				       wait_timeout, QS_ALLINPUT);

      if (ret == WAIT_OBJECT_0 + nhandles)
	{
          /* new input of some other kind */
	  BOOL bRet;
          while ((bRet = PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) != 0)
            {
              TranslateMessage (&msg);
              DispatchMessage (&msg);
            }
	}
      else
	break;
    }

  /* If we haven't done it yet, check the status of the sockets.  */
  if (rc == 0 && nsock > 0)
    rc = select (0, &handle_rfds, &handle_wfds, &handle_xfds, &tv0);

  /* Now fill in the results.  */
  FD_ZERO (rfds);
  FD_ZERO (wfds);
  FD_ZERO (xfds);

  /* Place a sentinel at the end of the array.  */
  handle_array[nhandles] = NULL;
  nhandles = 1;
  for (i = 0; i < nfds; i++)
    {
      if ((anyfds_in[i / CHAR_BIT] & (1 << (i & (CHAR_BIT - 1)))) == 0)
	continue;

      h = (HANDLE) _get_osfhandle (i);
      if (h != handle_array[nhandles])
	{
	  /* Perform handle->descriptor mapping.  Don't update rc, as these
	     results are counted in the return value of Winsock's select.  */
          WSAEventSelect ((SOCKET) h, NULL, 0);
          if (FD_ISSET (h, &handle_rfds))
            FD_SET (i, rfds);
          if (FD_ISSET (h, &handle_wfds))
            FD_SET (i, wfds);
          if (FD_ISSET (h, &handle_xfds))
            FD_SET (i, xfds);
	}
      else
        {
          /* Not a socket.  */
          nhandles++;
          win32_poll_handle (h, i, &rbits, &wbits, &xbits);
          if (rbits.out[i / CHAR_BIT] & (1 << (i & (CHAR_BIT - 1))))
            {
	      rc++;
	      FD_SET (i, rfds);
	    }
          if (wbits.out[i / CHAR_BIT] & (1 << (i & (CHAR_BIT - 1))))
            {
	      rc++;
	      FD_SET (i, wfds);
	    }
          if (xbits.out[i / CHAR_BIT] & (1 << (i & (CHAR_BIT - 1))))
            {
	      rc++;
	      FD_SET (i, xfds);
	    }
        }
    }

  return rc;
}

#endif /* Native Win32.  */
