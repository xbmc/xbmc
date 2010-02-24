#ifndef __GNUC__
#pragma code_seg( "RTV_TEXT" )
#pragma data_seg( "RTV_DATA" )
#pragma bss_seg( "RTV_BSS" )
#pragma const_seg( "RTV_RD" )
#endif

/*
 * libRTV
 * by rtvguy
 * A library encapsulating most of the functionality of John Todd Larason's
 * ReplayPC project and Lee Thompson's GuideParser project.
 * 
 * libRTV includes much of the original source code by the authors of the
 * ReplayPC and GuideParser projects.  GuideParser was modified under the
 * terms of its license to remove support for parsing ReplayChannels and
 * add support for parsing all current versions of ReplayGuide data.  ReplayPC
 * was modified under the terms of its license to improve efficiency of the
 * network code for the purposes of streaming video to a calling program, as
 * well as to remove reliance on the OpenSSL library for MD5 functions.
 *
 * ReplayPC Copyright (C) 2002 John Todd Larason <jtl@molehill.org>
 * GuideParser Copyright (C) 2002-2003 Lee Thompson <thompsonl@logh.net>,
 *    Dan Frumin <rtv@frumin.com>, Todd Larason <jtl@molehill.org>
 * libRTV Copyright (C) 2003-2004 rtvguy
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 */

#include <string.h>
#include <stdio.h>

#include "httpfsclient.h"
#include "guideclient.h"
#include "GuideParser.h"
#include "interface.h"

#ifdef _XBOX
#include <Xtl.h>
typedef SOCKET socket_fd;
#elif defined _WIN32
#include <WS2tcpip.h>
typedef SOCKET socket_fd;
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdlib.h>
#define closesocket(fd) close(fd)
typedef int socket_fd;
typedef  socket_fd SOCKET;
#define INVALID_SOCKET (-1)
#endif

//*********************************************************************************************
u64 rtv_get_filesize(const char* strHostName, const char* strFileName)
{
    char * data = NULL;
    char ** lines;
    int num_lines;
    int i;
    u64 size;
    unsigned long status;

    status = hfs_do_simple(&data, strHostName,
                           "fstat",
                           "name", strFileName,
                           NULL);
    if (status != 0) {
        //fprintf(stderr, "Error fstat %ld\n", status);
        free(data);
        return 0;
    }

    num_lines = rtv_split_lines(data, &lines);
    for (i = 0; i < num_lines; i++) {
        if (strncmp(lines[i], "size=", 5) == 0) {
            sscanf(lines[i]+5, "%llu", &size);
			break;
        }
    }

    rtv_free_lines(num_lines, lines);
    free(data);
	return size;
}
//*********************************************************************************************
unsigned long rtv_get_guide(unsigned char ** result, const char * address)
{
    char *          orig_timestamp = "0";
    unsigned char * timestamp = NULL;
    unsigned long   size;
    unsigned long   status;

	status = guide_client_get_snapshot(result, &timestamp, &size,
									address, orig_timestamp);
	if (timestamp)
		free(timestamp);
	if (status != 0) {
		//fprintf(stderr, "Error get_snapshot %ld\n", status);
		return 0;
	}

	return size;
}
//*********************************************************************************************
int rtv_parse_guide(char * szOutputBuffer, const char * szInput, const size_t InputSize)
{
	return(GuideParser(szOutputBuffer, szInput, InputSize));
}
//*********************************************************************************************
int rtv_get_guide_xml(unsigned char ** result, const char * address)
{
	unsigned long gsize;
	unsigned char * lresult = NULL;

	gsize = rtv_get_guide(&lresult, address);
	if (!gsize)
	{
		if (lresult)
			free (lresult);
		return 0;
	}

	*result = malloc(gsize * 1.5);
	if (!*result)
	{
		if (lresult)
			free (lresult);
		return 0;
	}

	rtv_parse_guide((char*)*result, (char*)lresult, gsize);

	if (lresult)
		free (lresult);
	return 1;
}
//*********************************************************************************************
int rtv_list_files(unsigned char ** result, const char * address, const char * path)
{
    unsigned long status;

    status = hfs_do_simple((char **)result, address,
                           "ls",
                           "name", path,
                           NULL);
    if (status != 0) {
        //fprintf(stderr, "Error %ld\n", status);
		if (result)
			free(result);
        return 0;
    }

	return 1;
}
//*********************************************************************************************
RTVD rtv_open_file(const char * address, const char * strFileName, u64 filePos)
{
	RTVD rtvd;
	char szOffset[32];

	rtvd = malloc(sizeof *rtvd);

	rtvd->firstReadDone = 0;
#ifdef _LINUX
	sprintf(szOffset,"%llu",filePos);
#else
	_i64toa(filePos,szOffset,10);
#endif

	rtvd->hc = hfs_do_chunked_open(	address,
								"readfile",
								"pos",  szOffset,
								"name", strFileName,
								NULL);
	return rtvd;
}
//*********************************************************************************************
size_t rtv_read_file(RTVD rtvd, char * lpBuf, size_t uiBufSize)
{
	size_t lenread;

	lenread = hfs_do_chunked_read(rtvd->hc, (char *) lpBuf, rtvd->firstReadDone, uiBufSize);
	rtvd->firstReadDone = 1;

	return lenread;
}
//*********************************************************************************************
void rtv_close_file(RTVD rtvd)
{
	hfs_do_chunked_close(rtvd->hc);
	free(rtvd);
}
//*********************************************************************************************
int rtv_discovery(struct RTV ** result, unsigned long msTimeout)
{
	const char upnpQuery[] = "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nST: urn:replaytv-com:device:ReplayDevice:1\r\nMX: 3\r\n\r\n";
	const char httpQuery[] = "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n";
	char sendBuf[128];
	char upnpFile[32];
	char * p1, * p2;
	char msg[2048];
	int opt;
	int r, len, numRTV;
	SOCKET s1, s2;
	struct timeval tv;
	struct RTV * rtv = NULL;
	fd_set fds1, fds2;
	struct sockaddr_in sin;  /* send address structure */
	struct sockaddr_in sin2; /* receive address structure */

	// Need to initialize Winsock on Win32
#if defined(_WIN32) && !defined(_XBOX)
	WSADATA wd;
	if (WSAStartup(MAKEWORD(2,2), &wd) == -1)
	{
		goto error;
	}
#endif
  s1 = 0;
  s2 = 0;

	// Set up the information for the UPNP port connection
	sin.sin_family = AF_INET;
	sin.sin_port = htons(1900);

	// For some reason, the UPNP address doesn't work on Xbox; so we
	// use the broadcast address, which seems to work.
#if defined(_XBOX) || defined(_WIN32)
	sin.sin_addr.S_un.S_addr = htonl(INADDR_BROADCAST);
#else
	inet_aton("239.255.255.250",(struct in_addr*)&sin.sin_addr.s_addr);
#endif

	if ((s1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		//fprintf(stderr,"error: socket(): %d\n", errno);
		goto error;
	}

	opt = 1;
	if ((setsockopt(s1, SOL_SOCKET, SO_BROADCAST, (const void*)&opt, sizeof(opt))) < 0) {
		goto error;
	}

	// Send the UPNP query
	r = sendto(s1, upnpQuery, (int) strlen(upnpQuery), 0, (struct sockaddr *)&sin, sizeof(sin));
	
	// Initialize variables in preparation to receive responses from the ReplayTV(s)
	numRTV = 0;
	rtv = calloc(1, sizeof(struct RTV));
	if (!rtv) goto error;
	//memset(rtv,0,sizeof(struct RTV));
	tv.tv_sec = msTimeout / 1000;
	tv.tv_usec = (msTimeout % 1000) * 1000;
	FD_ZERO(&fds1);
	FD_SET(s1, &fds1);

	// Read the response from a ReplayTV, loop while there are more responses waiting
	while (select((int) s1 + 1, &fds1, NULL, NULL, &tv))
	{
		len = sizeof(struct sockaddr);
		r = recvfrom(s1, msg, sizeof(msg), 0, (struct sockaddr *)&sin2, (socklen_t*)&len);
		if (r < 0)
		{
			//fprintf(stderr, "recvfrom error: %d\n", errno);
		} else {
			numRTV++;

			// Reallocate memory to accomodate the additional ReplayTVs found
			if (numRTV > 1)
			{
				rtv = realloc(rtv, sizeof(struct RTV) * numRTV);
				if (!rtv) goto error;
			}
			//printf("ip: %s\n", inet_ntoa(sin2.sin_addr));

			// Extract IP part and copy to hostname
			p1 = strstr(msg, "LOCATION: http://") + 17;
			p2 = strchr(p1, '/');
			*p2 = '\0';
			strcpy(rtv[numRTV-1].hostname, p1);

			// Extract UPNP device description file part and copy to upnpFile
			p1 = p2 + 1;
			p2 = strchr(p1, '\r');
			*p2 = '\0';
			strcpy(upnpFile, p1);

      strcpy(rtv[numRTV-1].friendlyName, rtv[numRTV-1].hostname);

			// Open socket for connecting to ReplayTV to get its friendlyName
      if ((s2 = socket(AF_INET, SOCK_STREAM, 0)) >= 0) {
				// Connect to ReplayTV
				sin2.sin_port = htons(80);
				r = connect(s2, (struct sockaddr *)&sin2, sizeof(sin2));
				sprintf(sendBuf, httpQuery, upnpFile, rtv[numRTV-1].hostname);
				r = send(s2, sendBuf, (int) strlen(sendBuf), 0);
				FD_ZERO(&fds2);
				FD_SET(s2, &fds2);
				// Read response if there is one
				if (select((int) s2 + 1, &fds2, NULL, NULL, &tv)) {
					p1 = msg;
          p2 = p1 + sizeof(msg);
          r = 0;
          do {
            p1+=r;
            r = recv(s2, p1, p2-p1, 0);
          } while(r>0 && (p2-p1) > 0);
          msg[sizeof(msg)-1] = 0;

					p1 = strstr(msg, "<friendlyName>");
          if(p1) {
            p1 += 14;
					  p2 = strchr(p1, '<');
            if(p2)
            {
					    *p2 = '\0';
					    strcpy(rtv[numRTV-1].friendlyName, p1);
            }
          }
				}
        
				r = closesocket(s2);
				s2 = 0;
			}
		}
	}
	r = closesocket(s1);
	s1 = 0;

	//Need to clean up the Winsock on Win32
#if defined(_WIN32) && !defined(_XBOX)
	WSACleanup();
#endif

	// Pass back the pointer to the array of rtv and return number of ReplayTV(s) found
	*result = rtv;
	return numRTV;

	// Clean up and return error
error:
	if (rtv) free(rtv);
	if (s1) closesocket(s1);
	if (s2) closesocket(s2);
#if defined(_WIN32) && !defined(_XBOX)
	WSACleanup();
#endif
	return -1;
}
//*********************************************************************************************

