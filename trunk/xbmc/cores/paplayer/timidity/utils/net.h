/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef ___NET_H_
#define ___NET_H_

#if defined(WINSOCK)
#include <winsock.h>
#else
typedef int SOCKET;
#define closesocket(fd) close(fd)
#define WSADATA int
#define MAKEWORD(a,b)
#define WSAStartup(a,b)
#define WSACleanup()
#endif

extern SOCKET open_socket(char *host, unsigned short port);
extern int socket_shutdown(SOCKET fd, int how);
extern long socket_write(SOCKET fd, char *buff, long bufsiz);

extern FILE *socket_fdopen(SOCKET fd, char *mode);
extern char *socket_fgets(char *buff, int n, FILE *fp);
extern int socket_fgetc(FILE *fp);
extern long socket_fread(void *buff, long bufsiz, FILE *fp);
extern long socket_fwrite(void *buff, long bufsiz, FILE *fp);
extern int socket_fflush(FILE *fp);
extern int socket_fclose(FILE *fp);

#endif /* ___NET_H_ */
