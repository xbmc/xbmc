/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
/* modified for FLAC support by Steven Richman (2003) */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <glib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <xmms/util.h>
#include <xmms/plugin.h>

#include "FLAC/format.h"
#include "configure.h"
#include "locale_hack.h"
#include "plugin.h"

/* on FreeBSD we get socklen_t from <sys/socket.h> */
#if (!defined HAVE_SOCKLEN_T) && !defined(__FreeBSD__)
typedef unsigned int socklen_t;
#endif

#define min(x,y) ((x)<(y)?(x):(y))
#define min3(x,y,z) (min(x,y)<(z)?min(x,y):(z))
#define min4(x,y,z,w) (min3(x,y,z)<(w)?min3(x,y,z):(w))

static gchar *icy_name = NULL;
static gint icy_metaint = 0;

extern InputPlugin flac_ip;

#undef DEBUG_UDP

/* Static udp channel functions */
static int udp_establish_listener (gint *sock);
static int udp_check_for_data(gint sock);

static char *flac_http_get_title(char *url);

static gboolean prebuffering, going, eof = FALSE;
static gint sock, rd_index, wr_index, buffer_length, prebuffer_length;
static guint64 buffer_read = 0;
static gchar *buffer;
static guint64 offset;
static pthread_t thread;
static GtkWidget *error_dialog = NULL;

static FILE *output_file = NULL;

#define BASE64_LENGTH(len) (4 * (((len) + 2) / 3))

/* Encode the string S of length LENGTH to base64 format and place it
   to STORE.  STORE will be 0-terminated, and must point to a writable
   buffer of at least 1+BASE64_LENGTH(length) bytes.  */
static void base64_encode (const gchar *s, gchar *store, gint length)
{
	/* Conversion table.  */
	static gchar tbl[64] = {
		'A','B','C','D','E','F','G','H',
		'I','J','K','L','M','N','O','P',
		'Q','R','S','T','U','V','W','X',
		'Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n',
		'o','p','q','r','s','t','u','v',
		'w','x','y','z','0','1','2','3',
		'4','5','6','7','8','9','+','/'
	};
	gint i;
	guchar *p = (guchar *)store;

	/* Transform the 3x8 bits to 4x6 bits, as required by base64.  */
	for (i = 0; i < length; i += 3)
	{
		*p++ = tbl[s[0] >> 2];
		*p++ = tbl[((s[0] & 3) << 4) + (s[1] >> 4)];
		*p++ = tbl[((s[1] & 0xf) << 2) + (s[2] >> 6)];
		*p++ = tbl[s[2] & 0x3f];
		s += 3;
	}
	/* Pad the result if necessary...  */
	if (i == length + 1)
		*(p - 1) = '=';
	else if (i == length + 2)
		*(p - 1) = *(p - 2) = '=';
	/* ...and zero-terminate it.  */
	*p = '\0';
}

/* Create the authentication header contents for the `Basic' scheme.
   This is done by encoding the string `USER:PASS' in base64 and
   prepending `HEADER: Basic ' to it.  */
static gchar *basic_authentication_encode (const gchar *user, const gchar *passwd, const gchar *header)
{
	gchar *t1, *t2, *res;
	gint len1 = strlen (user) + 1 + strlen (passwd);
	gint len2 = BASE64_LENGTH (len1);

	t1 = g_strdup_printf("%s:%s", user, passwd);
	t2 = g_malloc0(len2 + 1);
	base64_encode (t1, t2, len1);
	res = g_strdup_printf("%s: Basic %s\r\n", header, t2);
	g_free(t2);
	g_free(t1);
	
	return res;
}

static void parse_url(const gchar * url, gchar ** user, gchar ** pass, gchar ** host, int *port, gchar ** filename)
{
	gchar *h, *p, *pt, *f, *temp, *ptr;

	temp = g_strdup(url);
	ptr = temp;

	if (!strncasecmp("http://", ptr, 7))
		ptr += 7;
	h = strchr(ptr, '@');
	f = strchr(ptr, '/');
	if (h != NULL && (!f || h < f))
	{
		*h = '\0';
		p = strchr(ptr, ':');
		if (p != NULL && p < h)
		{
			*p = '\0';
			p++;
			*pass = g_strdup(p);
		}
		else
			*pass = NULL;
		*user = g_strdup(ptr);
		h++;
		ptr = h;
	}
	else
	{
		*user = NULL;
		*pass = NULL;
		h = ptr;
	}
	pt = strchr(ptr, ':');
	if (pt != NULL && (f == NULL || pt < f))
	{
		*pt = '\0';
		*port = atoi(pt + 1);
	}
	else
	{
		if (f)
			*f = '\0';
		*port = 80;
	}
	*host = g_strdup(h);
	
	if (f)
		*filename = g_strdup(f + 1);
	else
		*filename = NULL;
	g_free(temp);
}

void flac_http_close(void)
{
	going = FALSE;

	pthread_join(thread, NULL);
	g_free(icy_name);
	icy_name = NULL;
}


static gint http_used(void)
{
	if (wr_index >= rd_index)
		return wr_index - rd_index;
	return buffer_length - (rd_index - wr_index);
}

static gint http_free(void)
{
	if (rd_index > wr_index)
		return (rd_index - wr_index) - 1;
	return (buffer_length - (wr_index - rd_index)) - 1;
}

static void http_wait_for_data(gint bytes)
{
	while ((prebuffering || http_used() < bytes) && !eof && going)
		xmms_usleep(10000);
}

static void show_error_message(gchar *error)
{
	if(!error_dialog)
	{
		GDK_THREADS_ENTER();
		error_dialog = xmms_show_message(_("Error"), error, _("Ok"), FALSE,
						 NULL, NULL);
		gtk_signal_connect(GTK_OBJECT(error_dialog),
				   "destroy",
				   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
				   &error_dialog);
		GDK_THREADS_LEAVE();
	}
}

int flac_http_read(gpointer data, gint length)
{
	gint len, cnt, off = 0, meta_len, meta_off = 0, i;
	gchar *meta_data, **tags, *temp, *title;
	if (length > buffer_length) {
		length = buffer_length;
	}

	http_wait_for_data(length);

	if (!going)
		return 0;
	len = min(http_used(), length);

	while (len && http_used())
	{
		if ((flac_cfg.stream.cast_title_streaming) && (icy_metaint > 0) && (buffer_read % icy_metaint) == 0 && (buffer_read > 0))
		{
			meta_len = *((guchar *) buffer + rd_index) * 16;
			rd_index = (rd_index + 1) % buffer_length;
			if (meta_len > 0)
			{
				http_wait_for_data(meta_len);
				meta_data = g_malloc0(meta_len);
				if (http_used() >= meta_len)
				{
					while (meta_len)
					{
						cnt = min(meta_len, buffer_length - rd_index);
						memcpy(meta_data + meta_off, buffer + rd_index, cnt);
						rd_index = (rd_index + cnt) % buffer_length;
						meta_len -= cnt;
						meta_off += cnt;
					}
					tags = g_strsplit(meta_data, "';", 0);

					for (i = 0; tags[i]; i++)
					{
						if (!strncasecmp(tags[i], "StreamTitle=", 12))
						{
							temp = g_strdup(tags[i] + 13);
							title = g_strdup_printf("%s (%s)", temp, icy_name);
							set_track_info(title, -1);
							g_free(title);
							g_free(temp);
						}

					}
					g_strfreev(tags);

				}
				g_free(meta_data);
			}
			if (!http_used())
				http_wait_for_data(length - off);
			cnt = min3(len, buffer_length - rd_index, http_used());
		}
		else if ((icy_metaint > 0) && (flac_cfg.stream.cast_title_streaming))
			cnt = min4(len, buffer_length - rd_index, http_used(), icy_metaint - (gint) (buffer_read % icy_metaint));
		else
			cnt = min3(len, buffer_length - rd_index, http_used());
		if (output_file)
			fwrite(buffer + rd_index, 1, cnt, output_file);

		memcpy((gchar *)data + off, buffer + rd_index, cnt);
		rd_index = (rd_index + cnt) % buffer_length;
		buffer_read += cnt;
		len -= cnt;
		off += cnt;
	}
	if (!off) {
		fprintf(stderr, "returning zero\n");
	}
	return off;
}

static gboolean http_check_for_data(void)
{

	fd_set set;
	struct timeval tv;
	gint ret;

	tv.tv_sec = 0;
	tv.tv_usec = 20000;
	FD_ZERO(&set);
	FD_SET(sock, &set);
	ret = select(sock + 1, &set, NULL, NULL, &tv);
	if (ret > 0)
		return TRUE;
	return FALSE;
}

gint flac_http_read_line(gchar * buf, gint size)
{
	gint i = 0;

	while (going && i < size - 1)
	{
		if (http_check_for_data())
		{
			if (read(sock, buf + i, 1) <= 0)
				return -1;
			if (buf[i] == '\n')
				break;
			if (buf[i] != '\r')
				i++;
		}
	}
	if (!going)
		return -1;
	buf[i] = '\0';
	return i;
}

/* returns the file descriptor of the socket, or -1 on error */
static int http_connect (gchar *url_, gboolean head, guint64 offset)
{
	gchar line[1024], *user, *pass, *host, *filename,
	     *status, *url, *temp, *file;
	gchar *chost;
	gint cnt, error, port, cport;
	socklen_t err_len;
	gboolean redirect;
	int udp_sock = 0;
	fd_set set;
	struct hostent *hp;
	struct sockaddr_in address;
	struct timeval tv;

	url = g_strdup (url_);

	do
	{
		redirect=FALSE;
	
		g_strstrip(url);

		parse_url(url, &user, &pass, &host, &port, &filename);

		if ((!filename || !*filename) && url[strlen(url) - 1] != '/')
			temp = g_strconcat(url, "/", NULL);
		else
			temp = g_strdup(url);
		g_free(url);
		url = temp;

		chost = flac_cfg.stream.use_proxy ? flac_cfg.stream.proxy_host : host;
		cport = flac_cfg.stream.use_proxy ? flac_cfg.stream.proxy_port : port;

		sock = socket(AF_INET, SOCK_STREAM, 0);
		fcntl(sock, F_SETFL, O_NONBLOCK);
		address.sin_family = AF_INET;

		status = g_strdup_printf(_("LOOKING UP %s"), chost);
		flac_ip.set_info_text(status);
		g_free(status);

		if (!(hp = gethostbyname(chost)))
		{
			status = g_strdup_printf(_("Couldn't look up host %s"), chost);
			show_error_message(status);
			g_free(status);

			flac_ip.set_info_text(NULL);
			eof = TRUE;
		}

		if (!eof)
		{
			memcpy(&address.sin_addr.s_addr, *(hp->h_addr_list), sizeof (address.sin_addr.s_addr));
			address.sin_port = (gint) g_htons(cport);

			status = g_strdup_printf(_("CONNECTING TO %s:%d"), chost, cport);
			flac_ip.set_info_text(status);
			g_free(status);
			if (connect(sock, (struct sockaddr *) &address, sizeof (struct sockaddr_in)) == -1)
			{
				if (errno != EINPROGRESS)
				{
					status = g_strdup_printf(_("Couldn't connect to host %s"), chost);
					show_error_message(status);
					g_free(status);

					flac_ip.set_info_text(NULL);
					eof = TRUE;
				}
			}
			while (going)
			{
				tv.tv_sec = 0;
				tv.tv_usec = 10000;
				FD_ZERO(&set);
				FD_SET(sock, &set);
				if (select(sock + 1, NULL, &set, NULL, &tv) > 0)
				{
					err_len = sizeof (error);
					getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &err_len);
					if (error)
					{
						status = g_strdup_printf(_("Couldn't connect to host %s"),
									 chost);
						show_error_message(status);
						g_free(status);

						flac_ip.set_info_text(NULL);
						eof = TRUE;
						
					}
					break;
				}
			}
			if (!eof)
			{
				gchar *auth = NULL, *proxy_auth = NULL;
				gchar udpspace[30];
				int udp_port;

				if (flac_cfg.stream.use_udp_channel)
				{
					udp_port = udp_establish_listener (&udp_sock);
					if (udp_port > 0) 
						sprintf (udpspace, "x-audiocast-udpport: %d\r\n", udp_port);
					else
						udp_sock = 0;
				}
					
				if(user && pass)
					auth = basic_authentication_encode(user, pass, "Authorization");

				if (flac_cfg.stream.use_proxy)
				{
					file = g_strdup(url);
					if(flac_cfg.stream.proxy_use_auth && flac_cfg.stream.proxy_user && flac_cfg.stream.proxy_pass)
					{
						proxy_auth = basic_authentication_encode(flac_cfg.stream.proxy_user,
											 flac_cfg.stream.proxy_pass,
											 "Proxy-Authorization");
					}
				}
				else
					file = g_strconcat("/", filename, NULL);

				temp = g_strdup_printf("GET %s HTTP/1.0\r\n"
						       "Host: %s\r\n"
						       "User-Agent: %s/%s\r\n"
						       "%s%s%s%s",
						       file, host, "Reference FLAC Player", FLAC__VERSION_STRING, 
						       proxy_auth ? proxy_auth : "", auth ? auth : "",
						       flac_cfg.stream.cast_title_streaming ?  "Icy-MetaData:1\r\n" : "",
						       flac_cfg.stream.use_udp_channel ? udpspace : "");
				if (offset && !head) {
					gchar *temp_dead = temp;
					temp = g_strdup_printf ("%sRange: %llu-\r\n", temp, offset);
					fprintf (stderr, "%s", temp);
					g_free (temp_dead);
				}
				
				g_free(file);
				if(proxy_auth)
					g_free(proxy_auth);
				if(auth)
					g_free(auth);
				write(sock, temp, strlen(temp));
				write(sock, "\r\n", 2);
				g_free(temp);
				flac_ip.set_info_text(_("CONNECTED: WAITING FOR REPLY"));
				while (going && !eof)
				  {
					if (http_check_for_data())
					{
						if (flac_http_read_line(line, 1024))
						{
							status = strchr(line, ' ');
							if (status)
							{
								if (status[1] == '2')
									break;
								else if(status[1] == '3' && status[2] == '0' && status[3] == '2')
								{
									while(going)
									{
										if(http_check_for_data())
										{
											if((cnt = flac_http_read_line(line, 1024)) != -1)
											{
												if(!cnt)
													break;
												if(!strncmp(line, "Location:", 9))
												{
													g_free(url);
													url = g_strdup(line+10);
												}
											}
											else
											{
												eof=TRUE;
												flac_ip.set_info_text(NULL);
												break;
											}
										}
									}			
									redirect=TRUE;
									break;
								}
								else
								{
									status = g_strdup_printf(_("Couldn't connect to host %s\nServer reported: %s"), chost, status);
									show_error_message(status);
									g_free(status);
									break;
								}
							}
						}
						else
						{
							eof = TRUE;
							flac_ip.set_info_text(NULL);
						}
					}
				}

				while (going && !redirect)
				{
					if (http_check_for_data())
					{
						if ((cnt = flac_http_read_line(line, 1024)) != -1)
						{
							if (!cnt)
								break;
							if (!strncmp(line, "icy-name:", 9))
								icy_name = g_strdup(line + 9);
							else if (!strncmp(line, "x-audiocast-name:", 17))
								icy_name = g_strdup(line + 17);
							if (!strncmp(line, "icy-metaint:", 12))
								icy_metaint = atoi(line + 12);
							if (!strncmp(line, "x-audiocast-udpport:", 20)) {
#ifdef DEBUG_UDP
								fprintf (stderr, "Server wants udp messages on port %d\n", atoi (line + 20));
#endif
								/*udp_serverport = atoi (line + 20);*/
							}
							
						}
						else
						{
							eof = TRUE;
							flac_ip.set_info_text(NULL);
							break;
						}
					}
				}
			}
		}
	
		if(redirect)
		{
			if (output_file)
			{
				fclose(output_file);
				output_file = NULL;
			}
			close(sock);
		}

		g_free(user);
		g_free(pass);
		g_free(host);
		g_free(filename);
	} while(redirect);

	g_free(url);
	return eof ? -1 : sock;
}

static void *http_buffer_loop(void *arg)
{
	gchar *status, *url, *temp, *file;
	gint cnt, written;
	int udp_sock = 0;

	url = (gchar *) arg;
	sock = http_connect (url, false, offset);
	
	if (sock >= 0 && flac_cfg.stream.save_http_stream) {
		gchar *output_name;
		file = flac_http_get_title(url);
		output_name = file;
		if (!strncasecmp(output_name, "http://", 7))
			output_name += 7;
		temp = strrchr(output_name, '.');
		if (temp && (!strcasecmp(temp, ".fla") || !strcasecmp(temp, ".flac")))
			*temp = '\0';

		while ((temp = strchr(output_name, '/')))
			*temp = '_';
		output_name = g_strdup_printf("%s/%s.flac", flac_cfg.stream.save_http_path, output_name);

		g_free(file);

		output_file = fopen(output_name, "wb");
		g_free(output_name);
	}

	while (going)
	{

		if (!http_used() && !flac_ip.output->buffer_playing())
			prebuffering = TRUE;
		if (http_free() > 0 && !eof)
		{
			if (http_check_for_data())
			{
				cnt = min(http_free(), buffer_length - wr_index);
				if (cnt > 1024)
					cnt = 1024;
				written = read(sock, buffer + wr_index, cnt);
				if (written <= 0)
				{
					eof = TRUE;
					if (prebuffering)
					{
						prebuffering = FALSE;

						flac_ip.set_info_text(NULL);
					}

				}
				else
					wr_index = (wr_index + written) % buffer_length;
			}

			if (prebuffering)
			{
				if (http_used() > prebuffer_length)
				{
					prebuffering = FALSE;
					flac_ip.set_info_text(NULL);
				}
				else
				{
					status = g_strdup_printf(_("PRE-BUFFERING: %dKB/%dKB"), http_used() / 1024, prebuffer_length / 1024);
					flac_ip.set_info_text(status);
					g_free(status);
				}

			}
		}
		else
			xmms_usleep(10000);

		if (flac_cfg.stream.use_udp_channel && udp_sock != 0)
			if (udp_check_for_data(udp_sock) < 0)
			{
				close(udp_sock);
				udp_sock = 0;
			}
	}
	if (output_file)
	{
		fclose(output_file);
		output_file = NULL;
	}
	if (sock >= 0) {
		close(sock);
	}
	if (udp_sock != 0)
		close(udp_sock);

	g_free(buffer);
	g_free(url);
	
	pthread_exit(NULL);
	return NULL; /* avoid compiler warning */
}

int flac_http_open(const gchar * _url, guint64 _offset)
{
	gchar *url;

	url = g_strdup(_url);

	rd_index = 0;
	wr_index = 0;
	buffer_length = flac_cfg.stream.http_buffer_size * 1024;
	prebuffer_length = (buffer_length * flac_cfg.stream.http_prebuffer) / 100;
	buffer_read = 0;
	icy_metaint = 0;
	prebuffering = TRUE;
	going = TRUE;
	eof = FALSE;
	buffer = g_malloc(buffer_length);
	offset = _offset;

	pthread_create(&thread, NULL, http_buffer_loop, url);

	return 0;
}

char *flac_http_get_title(char *url)
{
	if (icy_name)
		return g_strdup(icy_name);
	if (g_basename(url) && strlen(g_basename(url)) > 0)
		return g_strdup(g_basename(url));
	return g_strdup(url);
}

/* Start UDP Channel specific stuff */

/* Find a good local udp port and bind udp_sock to it, return the port */
static int udp_establish_listener(int *sock)
{
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof (struct sockaddr_in);
	
#ifdef DEBUG_UDP
	fprintf (stderr,"Establishing udp listener\n");
#endif
	
	if ((*sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		g_log(NULL, G_LOG_LEVEL_CRITICAL,
		      "udp_establish_listener(): unable to create socket");
		return -1;
	}

	memset(&sin, 0, sinlen);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = g_htonl(INADDR_ANY);
			
	if (bind(*sock, (struct sockaddr *)&sin, sinlen) < 0)
	{
		g_log(NULL, G_LOG_LEVEL_CRITICAL,
		      "udp_establish_listener():  Failed to bind socket to localhost: %s", strerror(errno));
		close(*sock);
		return -1;
	}
	if (fcntl(*sock, F_SETFL, O_NONBLOCK) < 0)
	{
		g_log(NULL, G_LOG_LEVEL_CRITICAL,
		      "udp_establish_listener():  Failed to set flags: %s", strerror(errno));
		close(*sock);
		return -1;
	}

	memset(&sin, 0, sinlen);
	if (getsockname(*sock, (struct sockaddr *)&sin, &sinlen) < 0)
	{
		g_log(NULL, G_LOG_LEVEL_CRITICAL,
		      "udp_establish_listener():  Failed to retrieve socket info: %s", strerror(errno));
		close(*sock);
		return -1;
	}

#ifdef DEBUG_UDP
	fprintf (stderr,"Listening on local %s:%d\n", inet_ntoa(sin.sin_addr), g_ntohs(sin.sin_port));
#endif
	
	return g_ntohs(sin.sin_port);
}

static int udp_check_for_data(int sock)
{
	char buf[1025], **lines;
	char *valptr;
	gchar *title;
	gint len, i;
	struct sockaddr_in from;
	socklen_t fromlen;

	fromlen = sizeof(struct sockaddr_in);
	
	if ((len = recvfrom(sock, buf, 1024, 0, (struct sockaddr *)&from, &fromlen)) < 0)
	{
		if (errno != EAGAIN)
		{
			g_log(NULL, G_LOG_LEVEL_CRITICAL,
			      "udp_read_data(): Error reading from socket: %s", strerror(errno));
			return -1;
		}
		return 0;
	}
	buf[len] = '\0';
#ifdef DEBUG_UDP
	fprintf (stderr,"Received: [%s]\n", buf);
#endif
	lines = g_strsplit(buf, "\n", 0);
	if (!lines)
		return 0;
	
	for (i = 0; lines[i]; i++)
	{
		while ((lines[i][strlen(lines[i]) - 1] == '\n') ||
		       (lines[i][strlen(lines[i]) - 1] == '\r'))
			lines[i][strlen(lines[i]) - 1] = '\0';
		
		valptr = strchr(lines[i], ':');
		
		if (!valptr)
			continue;
		else
			valptr++;
		
		g_strstrip(valptr);
		if (!strlen(valptr))
			continue;
		
		if (strstr(lines[i], "x-audiocast-streamtitle") != NULL)
		{
			title = g_strdup_printf ("%s (%s)", valptr, icy_name);
			if (going)
				set_track_info(title, -1);
			g_free (title);
		}

#if 0
		else if (strstr(lines[i], "x-audiocast-streamlength") != NULL)
		{
			if (atoi(valptr) != -1)
				set_track_info(NULL, atoi(valptr));
		}
#endif
				
		else if (strstr(lines[i], "x-audiocast-streammsg") != NULL)
		{
			/* set_track_info(title, -1); */
/*  			xmms_show_message(_("Message"), valptr, _("Ok"), */
/*  					  FALSE, NULL, NULL); */
			g_message("Stream_message: %s", valptr);
		}

#if 0
		/* Use this to direct your webbrowser.. yeah right.. */
		else if (strstr(lines[i], "x-audiocast-streamurl") != NULL)
		{
			if (lasturl && g_strcmp (valptr, lasturl))
			{
				c_message (stderr, "Song URL: %s\n", valptr);
				g_free (lasturl);
				lasturl = g_strdup (valptr);
			}
		}
#endif
		else if (strstr(lines[i], "x-audiocast-udpseqnr:") != NULL)
		{
			gchar obuf[60];
			sprintf(obuf, "x-audiocast-ack: %ld \r\n", atol(valptr));
			if (sendto(sock, obuf, strlen(obuf), 0, (struct sockaddr *) &from, fromlen) < 0)
			{
				g_log(NULL, G_LOG_LEVEL_WARNING,
				      "udp_check_for_data(): Unable to send ack to server: %s", strerror(errno));
			}
#ifdef DEBUG_UDP
			else
				fprintf(stderr,"Sent ack: %s", obuf);
			fprintf (stderr,"Remote: %s:%d\n", inet_ntoa(from.sin_addr), g_ntohs(from.sin_port));
#endif
		}
	}
	g_strfreev(lines);
	return 0;
}
