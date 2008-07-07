/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: filestream.c,v 1.3 2003/07/29 08:20:11 menno Exp $
**/

/* Not very portable yet */

#include <winsock2.h> // Note: Must be *before* windows.h
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "filestream.h"
#include "aacinfo.h"

/* TEMPROARY HACK */
#define CommonExit(A) MessageBox(NULL, A, "FAAD Plugin", MB_OK)

int winsock_init=0; // 0=winsock not initialized, 1=success
long m_local_buffer_size = 64;
long m_stream_buffer_size = 128;

FILE_STREAM *open_filestream(char *filename)
{
    FILE_STREAM *fs;

    if(StringComp(filename,"http://", 7) == 0)
    {
        fs = (FILE_STREAM *)LocalAlloc(LPTR, sizeof(FILE_STREAM) + m_stream_buffer_size * 1024);

        if(fs == NULL)
            return NULL;

        fs->data = (unsigned char *)&fs[1];

        if(http_file_open(filename, fs) < 0)
        {
            LocalFree(fs);
            return NULL;
        }

        fs->http = 1;
    }
    else
    {
        fs = (FILE_STREAM*)LocalAlloc(LPTR, sizeof(FILE_STREAM) + m_local_buffer_size * 1024);

        if(fs == NULL)
            return NULL;

        fs->data = (unsigned char *)&fs[1];

        fs->stream = CreateFile(filename, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
        if (fs->stream == INVALID_HANDLE_VALUE)
        {
            LocalFree(fs);
            return NULL;
        }

        fs->http = 0;
    }

    fs->buffer_length = 0;
    fs->buffer_offset = 0;
    fs->file_offset = 0;

    return fs;
}

int read_byte_filestream(FILE_STREAM *fs)
{
    if(fs->buffer_offset == fs->buffer_length)
    {
        fs->buffer_offset = 0;

        if(fs->http)
            fs->buffer_length = recv(fs->inetStream, fs->data, m_stream_buffer_size * 1024, 0);
        else
            ReadFile(fs->stream, fs->data, m_local_buffer_size * 1024, &fs->buffer_length, 0);

        if(fs->buffer_length <= 0)
        {
            if(fs->http)
            {
                int x;
                x = WSAGetLastError();

                if(x == 0)
                {
                    /* Equivalent of a successful EOF for HTTP */
                }
            }

            fs->buffer_length = 0;
            return -1;
        }
    }

    fs->file_offset++;

    return fs->data[fs->buffer_offset++];
}

int read_buffer_filestream(FILE_STREAM *fs, void *data, int length)
{
    int i, tmp;
    unsigned char *data2 = (unsigned char *)data;

    for(i=0; i < length; i++)
    {
        if((tmp = read_byte_filestream(fs)) < 0)
        {
            if(i)
			{
                break;
            }
			else
			{
                return -1;
            }
        }
        data2[i] = tmp;
    }

    return i;
}

unsigned long filelength_filestream(FILE_STREAM *fs)
{
    unsigned long fsize;

    if (fs->http)
    {
        fsize = fs->http_file_length;
    }
	else
	{
        fsize = GetFileSize(fs->stream, NULL);
    }

    return fsize;
}

void seek_filestream(FILE_STREAM *fs, unsigned long offset, int mode)
{
    if(fs->http)
    {
        return;
    }

	SetFilePointer(fs->stream, offset, NULL, mode);

	if(mode == FILE_CURRENT)
		fs->file_offset += offset;
	else if(mode == FILE_END)
		fs->file_offset = filelength_filestream(fs) + offset;
	else
		fs->file_offset = offset;

    fs->buffer_length = 0;
    fs->buffer_offset = 0;
}

unsigned long tell_filestream(FILE_STREAM *fs)
{
    return fs->file_offset;
}

void close_filestream(FILE_STREAM *fs)
{
    if(fs)
    {
        if (fs->http)
        {
            if (fs->inetStream)
            {
                /* The 'proper' way to close a TCP connection */
                if(fs->inetStream)
                {
                    CloseTCP(fs->inetStream);
                }
            }
        }
        else
        {
            if(fs->stream)
                CloseHandle(fs->stream);
        }

        LocalFree(fs);
        fs = NULL;
    }
}

int WinsockInit()
{
    /* Before using winsock, you must load the DLL... */
    WSADATA wsaData;

    /* Load version 2.0 */
    if (WSAStartup( MAKEWORD( 2, 0 ), &wsaData ))
    {
        /* Disable streaming */
        return -1;
    }

    winsock_init = 1;

    return 0;
}

void WinsockDeInit()
{
    /* Unload the DLL */

    if(winsock_init)
        WSACleanup();
}

int FindCRLF(char *str)
{
    int i;

    for(i=0; i != lstrlen(str) && str[i] != '\r'; i++);

    return i;
}

void CloseTCP(int s)
{
    char tempbuf[1024];

    /* Set the socket to ignore any new incoming data */
    shutdown(s, 1);

    /* Get any old remaining data */
    while(recv(s, tempbuf, 1024, 0) > 0);

    /* Deallocate the socket */
    closesocket(s);
}

int resolve_host(char *host, SOCKADDR_IN *sck_addr, unsigned short remote_port)
{
    HOSTENT *hp;

    if (isalpha(host[0]))
    {
        /* server address is a name */
        hp = gethostbyname(host);
    }
    else
    {
        unsigned long addr;
        /* Convert nnn.nnn address to a usable one */
        addr = inet_addr(host);
        hp = gethostbyaddr((char *)&addr, 4, AF_INET);
    }

    if (hp == NULL)
    {
        char tmp[128];
        wsprintf(tmp, "Error resolving host address [%s]!\n", host);
        CommonExit(tmp);
        return -1;
    }

    ZeroMemory(sck_addr, sizeof(SOCKADDR_IN));
    sck_addr->sin_family = AF_INET;
    sck_addr->sin_port = htons(remote_port);
    CopyMemory(&sck_addr->sin_addr, hp->h_addr, hp->h_length);

    return 0;
}

int http_file_open(char *url, FILE_STREAM *fs)
{
    SOCKET sck;
    SOCKADDR_IN host;
    char server[1024], file[1024], request[1024], *temp = NULL, *tmpfile = NULL;
    int i, j, port = 80, bytes_recv, http_code;

    /* No winsock, no streaming */
    if(!winsock_init)
    {
        return -1;
    }

    url += 7; // Skip over http://

    /* Extract data from the URL */
    for(i=0; url[i] != '/' && url[i] != ':' && url[i] != 0; i++);

    ZeroMemory(server, 1024);
    CopyMemory(server, url, i);

    if(url[i] == ':')
    {
        /* A certain port was specified */
        port = atol(url + (i + 1));
    }

    for(; url[i] != '/' && url[i] != 0; i++);

    ZeroMemory(file, 1024);

    CopyMemory(file, url + i, lstrlen(url));

    /* END OF URL PARSING */

    /* Create a TCP/IP socket */
    sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(sck == INVALID_SOCKET)
    {
        CommonExit("Error creating TCP/IP new socket");
        return -1;
    }

    /* Resolve the host address (turn www.blah.com into an IP) */
    if(resolve_host(server, &host, (unsigned short)port))
    {
        CommonExit("Error resolving host address");
        CloseTCP(sck);
        return -1;
    }

    /* Connect to the server */
    if(connect(sck, (SOCKADDR *)&host, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        CommonExit("Error connecting to remote server");
        CloseTCP(sck);
        return -1;
    }

	tmpfile = calloc(1, (strlen(file) * 3) + 1);

	/* Encode URL */
	for(i=0, j=0; i < (int)strlen(file); i++)
	{
		if((unsigned char)file[i] <= 31 || (unsigned char)file[i] >= 127)
		{
			/* encode ASCII-control characters */
			wsprintf(tmpfile + j, "%%%X", (unsigned char)file[i]);
			j += 3;
			continue;
		}
		else
		{
			switch(file[i])
			{
				/* encode characters that could confuse some servers */
				case ' ':
				case '"':
				case '>':
				case '<':
				case '#':
				case '%':
				case '{':
				case '}':
				case '|':
				case '\\':
				case '^':
				case '~':
				case '[':
				case ']':
				case '`':

				wsprintf(tmpfile + j, "%%%X", (unsigned char)file[i]);
				j += 3;
				continue;
			}
		}
		
		tmpfile[j] = file[i];
		j++;
	}

    wsprintf(request, "GET %s\r\n\r\n", tmpfile);

	free(tmpfile);

    /* Send the request */
    if(send(sck, request, lstrlen(request), 0) <= 0)
    {
        /* Error sending data */
        CloseTCP(sck);
        return -1;
    }

    ZeroMemory(request, 1024);

    /* Send the request */
    if((bytes_recv = recv(sck, request, 1024, 0)) <= 0)
    {
        /* Error sending data */
        CloseTCP(sck);
        return -1;
    }

    if(StringComp(request,"HTTP/1.", 7) != 0)
    {
        /* Invalid header */
        CloseTCP(sck);
        return -1;
    }

    http_code = atol(request + 9);

    if(http_code < 200 || http_code > 299)
    {
        /* HTTP error */
        CloseTCP(sck);
        return -1;
    }

	// Search for a length field
	fs->http_file_length = 0;

    /* Limit search to only 20 loops */
    if((temp = strstr(request, "Content-Length: ")) != NULL)
    {
		/* Has a content-length field, copy into structure */
		fs->http_file_length = atol(temp + 16);
	}

    /* Copy the handle data into the structure */
    fs->inetStream = sck;

    /* Copy any excess data beyond the header into the filestream buffers */
	temp = strstr(request, "\r\n\r\n");

	if(temp)
	{
		temp += 4;
	}

    if(temp - request < bytes_recv)
    {
        memcpy(fs->data, temp, (temp - request) - bytes_recv);
        fs->buffer_length = (temp - request) - bytes_recv;
        fs->buffer_offset = 0;
    }

    return 0;
}
