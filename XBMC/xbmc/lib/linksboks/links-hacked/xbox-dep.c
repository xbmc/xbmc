/*
 * LinksBoks
 * Copyright (c) 2003-2005 ysbox
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "cfg.h"
#include "links.h"

#ifdef __XBOX__

//#include "xntdll.h"

IN_ADDR localAddress;
BOOL bHaveLocalAddress = FALSE;


typedef struct   
{   
	DWORD	Data_00;            // Check Block Start   
	DWORD	Data_04;   
	DWORD	Data_08;   
	DWORD	Data_0c;   
	DWORD	Data_10;            // Check Block End   
  
	DWORD	V1_IP;              // 0x14   
	DWORD	V1_Subnetmask;      // 0x18   
	DWORD	V1_Defaultgateway;  // 0x1c   
	DWORD	V1_DNS1;            // 0x20   
	DWORD	V1_DNS2;            // 0x24   

	DWORD	Data_28;            // Check Block Start   
	DWORD	Data_2c;   
	DWORD	Data_30;   
	DWORD	Data_34;   
	DWORD	Data_38;            // Check Block End   

	DWORD	V2_Tag;             // V2 Tag "XBV2"   
 
	DWORD	Flag;				// 0x40   
	DWORD	Data_44;   

	DWORD	V2_IP;              // 0x48   
	DWORD	V2_Subnetmask;      // 0x4c   
	DWORD	V2_Defaultgateway;  // 0x50   
	DWORD	V2_DNS1;            // 0x54   
	DWORD	V2_DNS2;            // 0x58   

	DWORD   Data_xx[0x200-0x5c];

} TXNetConfigParams,*PTXNetConfigParams;   


struct in_addr XBNet_getLocalAddress( )
{
	//DWORD dwStatus;
	//XNADDR xnTitleAddress;

 //   // Only need to acquire the local address one time
 //   while( !bHaveLocalAddress )
 //   {
 //       dwStatus = XNetGetTitleXnAddr( &xnTitleAddress );
 //       if( dwStatus != XNET_GET_XNADDR_PENDING )
	//	{
	//		localAddress = xnTitleAddress.ina;
 //           bHaveLocalAddress = TRUE;
	//	}
 //   }

	//return( localAddress );

	struct sockaddr_in saHost;
	TXNetConfigParams configParams; 
	int bXboxVersion2;

	saHost.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	//XNetLoadConfigParams( (LPBYTE) &configParams );

	//bXboxVersion2 = (configParams.V2_Tag == 0x58425632 ); // "XBV2"

	//if (bXboxVersion2)
	//{
	//	saHost.sin_addr.S_un.S_addr = configParams.V2_IP;
	//}
	//else
	//{
	//	saHost.sin_addr.S_un.S_addr = configParams.V1_IP;
	//}

	return saHost.sin_addr;
}








/**********************
 * os_dep.c functions *
 **********************/

//int set_nonblocking_fd(int fd)
//{
//	int flag = 1;
//	return ioctlsocket( fd - SHS, FIONBIO, &flag );
//}
//
//int set_blocking_fd(int fd)
//{
//	int flag = 0;
//	return ioctlsocket( fd - SHS, FIONBIO, &flag );
//}


int set_nonblocking_fd(int fd)
{
	int flag = 1;
	switch( xbox_get_fd_type( fd ) )
	{
		case FD_TYPE_SOCKET:
			return ioctlsocket( xbox_get_socket( fd ), FIONBIO, &flag );
		default:
			return 0;
	}

	return 0;
}

int set_blocking_fd(int fd)
{
	int flag = 0;
	switch( xbox_get_fd_type( fd ) )
	{
		case FD_TYPE_SOCKET:
			return ioctlsocket( xbox_get_socket( fd ), FIONBIO, &flag );
		default:
			return 0;
	}

	return 0;
}

void set_highpri()
{
}

char *get_clipboard_text()
{
	return NULL;
}

void set_clipboard_text(char *data)
{
}

void set_bin(int fd)
{
	setmode(xbox_get_file(fd), O_BINARY);
}

#define B_SZ	65536
char snprtintf_buffer[B_SZ];

int my_snprintf(char *str, int n, char *f, ...)
{
	int i;
	va_list l;
	if (!n) return -1;
	va_start(l, f);
	vsprintf(snprtintf_buffer, f, l);
	va_end(l);
	i = strlen(snprtintf_buffer);
	if (i >= B_SZ) {
		error("String size too large!");
		va_end(l);
		exit(1);
	}
	if (i >= n) {
		memcpy(str, snprtintf_buffer, n);
		str[n - 1] = 0;
		va_end(l);
		return -1;
	}
	strcpy(str, snprtintf_buffer);
	va_end(l);
	return i;
}

int is_safe_in_shell(unsigned char c)
{
	return c == '@' || c == '+' || c == '-' || c == '.' || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= 'a' && c <= 'z');
}

void check_shell_security(unsigned char **cmd)
{
	unsigned char *c = *cmd;
	while (*c) {
		if (!is_safe_in_shell(*c)) *c = '_';
		c++;
	}
}


void open_in_new_tab(struct terminal *term, unsigned char *exe, unsigned char *param)
{
	void *info;
	int len;
	int base = 0;
	unsigned char *url = "";
	if (!cmpbeg(param, "-base-session ")) {
		base = atoi(param + strlen("-base-session "));
	} else {
		url = param;
        }
	{
		/* Now lets create new session... */
		struct event ev = {EV_RESIZE, 0, 0, 0};
		struct window *	win = mem_alloc(sizeof (struct window));

		win->handler = get_root_window(term)->handler;
		win->data = NULL;
		win->term = term;
		/* Root window */
		win->type = 1;

		add_to_list(term->windows, win);

                win->data = create_basic_session(win);

                if (!options_get_bool("tabs_new_in_background"))
                        switch_to_tab(term, get_tab_number(win));

                if (!F) redraw_terminal_cls(term);
#ifdef G
                else if(options_get_bool("tabs_show") &&
                        number_of_tabs(term)==2 &&
                        !options_get_bool("tabs_show_if_single"))
                    t_resize(term->dev);
                else{
                    //t_resize(term->dev);
                    struct window *win;
                    struct event ev = {EV_REDRAW, 0, 0, 0};
                    term->x = ev.x = term->dev->size.x2;
                    term->y = ev.y = term->dev->size.y2;
                    drv->set_clip_area(term->dev, &term->dev->size);
                    foreach(win, term->windows) {
                        set_window_pos(win, 0, 0, ev.x, ev.y);
                        win->handler(win, &ev, 0);
                    }
                    drv->set_clip_area(term->dev, &term->dev->size);
                }
#endif

                if(*url) {
                        unsigned char *u = decode_url(url);

			if (u) {
				goto_url((struct session *)win->data, u);
			        mem_free(u);
                        }
                }
		else {
			unsigned char *h = options_get("homepage");
			if (h && *h)
				goto_url((struct session *)win->data, h);

		}
        }

}

struct open_in_new *get_open_in_new(int environment)
{
	struct open_in_new *oin = DUMMY;

	if (!(oin = mem_alloc(2 * sizeof(struct open_in_new))))
		return NULL;

	oin[0].text = TXT(T_NEW_TAB);
	oin[0].hk = TXT(T_HK_NEW_TAB);
	oin[0].fn = open_in_new_tab;
	oin[1].text = NULL;
	oin[1].hk = NULL;
	oin[1].fn = NULL;

	return oin;


/*	int i;
	struct open_in_new *oin = DUMMY;
	int noin = 0;
	if (environment & ENV_G) environment = ENV_G;
        for (i = 0; oinw[i].fn; i++) if ((environment & oinw[i].env) == oinw[i].env) {
		struct open_in_new *x;
		if (!(x = mem_realloc(oin, (noin + 2) * sizeof(struct open_in_new)))) continue;
		oin = x;
		oin[noin].text = oinw[i].text;
		oin[noin].hk = oinw[i].hk;
		oin[noin].fn = oinw[i].fn;
		noin++;
		oin[noin].text = NULL;
		oin[noin].hk = NULL;
		oin[noin].fn = NULL;
	}
	if (oin == DUMMY) return NULL;
	return oin;
*/
}


int can_resize_window(int environment)
{
	return 0;
}

int can_open_os_shell(int environment)
{
	return 0;
}

void want_draw()
{
}

void done_draw()
{
}

int exe(char *path)
{
	return -1;
}

int get_input_handle()
{
	return 0;
}

int get_output_handle()
{
	return 0;
}

int get_ctl_handle()
{
	return 0;
}

void terminate_osdep()
{
}

struct tdata {
	void (*fn)(void *, int);
	int h;
	unsigned char data[1];
};

void bgt(struct tdata *t)
{
	t->fn(t->data, t->h);
	write(t->h, "x", 1);
	close(t->h);
	free(t);
}

DWORD WINAPI ThreadFunc( LPVOID lpParam ) 
{ 
	bgt( (struct tdata *)lpParam );
	return 0;
}

int start_thread(void (*fn)(void *, int), void *ptr, int l)
{
    DWORD dwThreadId; 
    HANDLE hThread;

	struct tdata *t;
	int p[2];

	if (c_pipe(p) < 0) return -1;
	fcntl(p[0], F_SETFL, O_NONBLOCK);
	fcntl(p[1], F_SETFL, O_NONBLOCK);
	if (!(t = malloc(sizeof(struct tdata) + l))) return -1;
	t->fn = fn;
	t->h = p[1];
	memcpy(t->data, ptr, l);
	if (!(hThread = CreateThread(NULL, 0, ThreadFunc, t, 0, &dwThreadId))) {
		close(p[0]);
		close(p[1]);
		mem_free(t);
		return -1;
	}
	return p[0];
}



#endif