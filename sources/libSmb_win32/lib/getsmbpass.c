/* Copyright (C) 1992-1998 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

/* Modified to use with samba by Jeremy Allison, 8th July 1995. */

#include "includes.h"

#ifdef REPLACE_GETPASS

#ifdef SYSV_TERMIO 

/* SYSTEM V TERMIO HANDLING */

static struct termio t;

#define ECHO_IS_ON(t) ((t).c_lflag & ECHO)
#define TURN_ECHO_OFF(t) ((t).c_lflag &= ~ECHO)
#define TURN_ECHO_ON(t) ((t).c_lflag |= ECHO)

#ifndef TCSAFLUSH
#define TCSAFLUSH 1
#endif

#ifndef TCSANOW
#define TCSANOW 0
#endif

static int tcgetattr(int fd, struct termio *t)
{
	return ioctl(fd, TCGETA, t);
}

static int tcsetattr(int fd, int flags, struct termio *t)
{
	if(flags & TCSAFLUSH)
		ioctl(fd, TCFLSH, TCIOFLUSH);
	return ioctl(fd, TCSETS, t);
}

#elif !defined(TCSAFLUSH) && !defined(_XBOX)

/* BSD TERMIO HANDLING */

static struct sgttyb t;  

#define ECHO_IS_ON(t) ((t).sg_flags & ECHO)
#define TURN_ECHO_OFF(t) ((t).sg_flags &= ~ECHO)
#define TURN_ECHO_ON(t) ((t).sg_flags |= ECHO)

#define TCSAFLUSH 1
#define TCSANOW 0

static int tcgetattr(int fd, struct sgttyb *t)
{
	return ioctl(fd, TIOCGETP, (char *)t);
}

static int tcsetattr(int fd, int flags, struct sgttyb *t)
{
	return ioctl(fd, TIOCSETP, (char *)t);
}

#elif !defined(_XBOX)/* POSIX TERMIO HANDLING */
#define ECHO_IS_ON(t) ((t).c_lflag & ECHO)
#define TURN_ECHO_OFF(t) ((t).c_lflag &= ~ECHO)
#define TURN_ECHO_ON(t) ((t).c_lflag |= ECHO)

static struct termios t;
#endif /* SYSV_TERMIO */

static SIG_ATOMIC_T gotintr;
static int in_fd = -1;

/***************************************************************
 Signal function to tell us were ^C'ed.
****************************************************************/

static void gotintr_sig(void)
{
	gotintr = 1;
	if (in_fd != -1)
		close(in_fd); /* Safe way to force a return. */
	in_fd = -1;
}

char *getsmbpass(const char *prompt)
{
#ifdef _XBOX
        OutputDebugString("getsmbpass not supported\n");
        return "";
#else
	FILE *in, *out;
	int echo_off;
	static char buf[256];
	static size_t bufsize = sizeof(buf);
	size_t nread;

	/* Catch problematic signals */
	CatchSignal(SIGINT, SIGNAL_CAST gotintr_sig);

	/* Try to write to and read from the terminal if we can.
		If we can't open the terminal, use stderr and stdin.  */

	in = fopen ("/dev/tty", "w+");
	if (in == NULL) {
		in = stdin;
		out = stderr;
	} else {
		out = in;
	}

	setvbuf(in, NULL, _IONBF, 0);

	/* Turn echoing off if it is on now.  */

	if (tcgetattr (fileno (in), &t) == 0) {
		if (ECHO_IS_ON(t)) {
			TURN_ECHO_OFF(t);
			echo_off = tcsetattr (fileno (in), TCSAFLUSH, &t) == 0;
			TURN_ECHO_ON(t);
		} else {
			echo_off = 0;
		}
	} else {
		echo_off = 0;
	}

	/* Write the prompt.  */
	fputs(prompt, out);
	fflush(out);

	/* Read the password.  */
	buf[0] = 0;
	if (!gotintr) {
		in_fd = fileno(in);
		fgets(buf, bufsize, in);
	}
	nread = strlen(buf);
	if (nread) {
		if (buf[nread - 1] == '\n')
			buf[nread - 1] = '\0';
	}

	/* Restore echoing.  */
	if (echo_off) {
		if (gotintr && in_fd == -1)
			in = fopen ("/dev/tty", "w+");
		if (in != NULL)
			tcsetattr (fileno (in), TCSANOW, &t);
	}

	fprintf(out, "\n");
	fflush(out);

	if (in && in != stdin) /* We opened the terminal; now close it.  */
		fclose(in);

	/* Catch problematic signals */
	CatchSignal(SIGINT, SIGNAL_CAST SIG_DFL);

	if (gotintr) {
		printf("Interupted by signal.\n");
		fflush(stdout);
		exit(1);
	}
	return buf;
#endif
}

#else
 void getsmbpasswd_dummy(void);
 void getsmbpasswd_dummy(void) {;}
#endif
