/* xbm.c
 * portable bitmap format (xbm) decoder
 * (c) 2002 Petr 'Brain' Kulhavy
 * This file is a part of the Links program, released under GPL
 */

#include "cfg.h"
#include "links.h"


#ifdef G

#include <stdio.h>
#include <string.h>
#include <ctype.h>


#define XBM_BUFFER_LEN 1024

#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

struct xbm_decoder{
	int width, height;	/* rozmery obrazku */
	int partnum;	/* buffer skoncil uprostred cisla */
	int *numdest;	/* kam se ma ukladat to cislo kdyz je partnum */
	int in_data_block;	/* jsme v bloku dat (uz jsme precetli otviraci slozenou zavorku) */
	int image_pos;	/* pixelova pozice v cimg->buffer */
	int pixels;	/* velikost cimg->buffer v pixelech */
	int state;	/* stav automatu na zrani komentaru */
	int actual_eight;	/* aktualni cislo, ktere se ma putnout do cimg */
	int line_pos;		/* aktualni pozice pixelu na radce (kolik pixelu v cimg->buffer je platnych na momentalne zpracovavane radce) */
	unsigned char barvicky[6];	/* 0-2 background, 3-5 foreground */
/*
 * stavy:
 *	0 - mimo komentar
 *	1 - za 1. lomitkem
 *	2 - za 1. lomitkem a hvezdickou
 *	3 - za 2. hvezdickou
 */
	int buffer_pos;	/* delka platnych dat v bufferu */
	unsigned char buffer[XBM_BUFFER_LEN];
};

extern int get_foreground(int rgb);


unsigned char *my_memmem(unsigned char *h, int hl, unsigned char *n, int nl)
{
	for (;hl>=nl;hl--,h++)
		if (*h==*n&&!memcmp(h,n,nl))return h;
	return NULL;
}


void xbm_start(struct cached_image *cimg)
{
	struct xbm_decoder *deco;
	unsigned short r,g,b;

	deco=mem_alloc(sizeof(struct xbm_decoder));
	if (!deco){img_end(cimg);return;}
	cimg->decoder=deco;
	deco->state=0;
	deco->width=-1;
	deco->height=-1;
	deco->buffer_pos=0;
	deco->partnum=0;
	deco->in_data_block=0;

	round_color_sRGB_to_48(&r,&g,&b,cimg->background_color);
	deco->barvicky[0]=apply_gamma_single_16_to_8(r,display_red_gamma);
	deco->barvicky[1]=apply_gamma_single_16_to_8(g,display_green_gamma);
	deco->barvicky[2]=apply_gamma_single_16_to_8(b,display_blue_gamma);
	
	round_color_sRGB_to_48(&r,&g,&b,get_foreground(cimg->background_color));
	deco->barvicky[3]=apply_gamma_single_16_to_8(r,display_red_gamma);
	deco->barvicky[4]=apply_gamma_single_16_to_8(g,display_green_gamma);
	deco->barvicky[5]=apply_gamma_single_16_to_8(b,display_blue_gamma);
	
			
}

/* vrati cislo, nebo -1, kdyz to neni cislo, a nastavi p a l na posledni necislici */
static inline int __read_num(unsigned char **p,int *l,int *partnum,int *digits, int *base)
{
	static int a=-1;
	static int b=10;
	static int d=0;
	int retval;
	int was_partnum=*partnum;
	*partnum=0;

	dalsi_runda:
	if (!(*l))return a;
	**p=tolower(**p);
	if (!was_partnum&&b==10&&((**p)<'0'||(**p)>'9'))goto smitec; /* tohle neni cislo, to si strc nekam... */
	if (b==16&&((**p)<'a'||(**p>'f'))&&((**p)<'0'||(**p)>'9'))goto smitec;
	if (a==-1)a=0;
	for (;*l&&(((**p)>='0'&&(**p)<='9')||(b==16&&(**p)>='a'&&(**p)<='f'));(*l)--,(*p)++){d++;a*=b;a+=((**p)>='a'?10+(**p)-'a':(**p)-'0');}
	if (b==10&&!a&&(*l)&&((**p)|32)=='x'){b=16;d=0;(*p)++;(*l)--;if (!*l)*partnum=1;goto dalsi_runda;}
	smitec:
	retval=a;
	if (!*l)*partnum=1;
	else a=-1,*base=b,b=10,*digits=d,d=0;
	return retval;
}


static inline void __skip_space_tab(unsigned char **p, int *l)
{
	for (;*l&&(**p==' '||**p==9);(*l)--,(*p)++);
}


static inline void __skip_whitespace(unsigned char **p, int *l)
{
	for (;*l&&((**p)>'9'||(**p)<'0');(*l)--,(*p)++);
}


static inline void put_eight(struct cached_image *cimg,int bits)
{
	struct xbm_decoder *deco=(struct xbm_decoder *)cimg->decoder;
	int ten_napis_v_s3_nekecal;

	for (ten_napis_v_s3_nekecal=0;ten_napis_v_s3_nekecal<bits&&deco->image_pos<deco->pixels&&deco->line_pos<cimg->width;ten_napis_v_s3_nekecal++,deco->image_pos++,deco->line_pos++)
	{
		memcpy(cimg->buffer+deco->image_pos*3,deco->barvicky+((deco->actual_eight)&1)*3,3);
		deco->actual_eight>>=1;
	}
	if (deco->line_pos==cimg->width)
		deco->line_pos=0,deco->actual_eight=0;
}


/* opravdovy dekoder xbm, data jsou bez komentaru */
/* length is always !=NULL */
void xbm_decode(struct cached_image *cimg, unsigned char *data, int length)
{
	struct xbm_decoder *deco=(struct xbm_decoder *)cimg->decoder;
	/* okurky v decu ;-) */
	int a;
	int must_return=0;

	/* This means that xbm image contains some additional info;
	 * Let's just ignore it  --karpov */
	if(!deco) return;

restart_again:
	if (must_return&&!length)return;
	must_return=0;
	a=min(length,XBM_BUFFER_LEN-deco->buffer_pos);
	memcpy(deco->buffer+deco->buffer_pos,data,a);
	length-=a;
	deco->buffer_pos+=a;
	if (!deco->buffer_pos)return; 	/* z toho nic plodnyho nevznikne */
	data+=a;
	if (!deco->in_data_block&&deco->partnum)
	{
		unsigned char *p;
		int a;
		int b,d;
		p=deco->buffer;
		a=deco->buffer_pos;
		*(deco->numdest)=__read_num(&p,&a,&(deco->partnum),&d,&b);
		/* p i a ukazuje na 1. neciselnej znak (at uz za mezerama bylo cislo nebo nebylo) */
		memmove(deco->buffer,p,a);
		deco->buffer_pos=a;
		if (deco->partnum){must_return=1;goto restart_again;}	/* zase konec bufferu */
	}
	if (deco->width<0||deco->height<0)	/* decoding header */
	{
		unsigned char *p,*q;
		int *d;
		int a;
		int base, digits;
		
		p=my_memmem(deco->buffer,deco->buffer_pos,"width",5);
		q=my_memmem(deco->buffer,deco->buffer_pos,"height",6);

		if (!p&&!q)	/* sezereme zacatek */
		{
			int a=deco->buffer_pos>5?deco->buffer_pos:0;	/* nesmime ukrast kus width/height */
			memmove(deco->buffer,deco->buffer+deco->buffer_pos-a,deco->buffer_pos-a);	/* sezereme to pred width/height */
			deco->buffer_pos-=a;
			must_return=1;
			goto restart_again;
		}

		p=p&&q?min(p,q):max(p,q);	/* bereme vetsi, protoze ten 2. je NULL */
		memmove(deco->buffer,p,(deco->buffer_pos)+(deco->buffer)-p);	/* sezereme to pred width/height */
		deco->buffer_pos-=p-deco->buffer;
		/* deco->buffer zacina height/width */
		if (deco->buffer[0]=='w'){p=deco->buffer+5;d=&(deco->width);}
		else {p=deco->buffer+6;d=&(deco->height);}

		a=deco->buffer_pos+deco->buffer-p;
		__skip_space_tab(&p,&a);
		if (!a){must_return=1;goto restart_again;}	/* v bufferu je: width/height, whitespace, konec */
		*d=__read_num(&p,&a,&(deco->partnum),&digits, &base);
		if (deco->partnum)deco->numdest=d,must_return=1;
		/* p i a ukazuje na 1. neciselnej znak (at uz za mezerama bylo cislo nebo nebylo) */
		memmove(deco->buffer,p,a);
		deco->buffer_pos=a;
		goto restart_again;
	}
	else	/* decoding data */
	{
		unsigned char *p;
		int a;
		int d,b;
		if (!deco->in_data_block)
		{
			p=memchr(deco->buffer,'{',deco->buffer_pos);
			if (!p){deco->buffer_pos=0;must_return=1;goto restart_again;}	/* sezerem celej blok a cekame na zavorku */

			cimg->width=deco->width;
			cimg->height=deco->height;
			cimg->buffer_bytes_per_pixel=3;
			cimg->red_gamma=display_red_gamma;
			cimg->green_gamma=display_green_gamma;
			cimg->blue_gamma=display_blue_gamma;
			cimg->strip_optimized=0;
			header_dimensions_known(cimg);
			
			deco->in_data_block=1;
			p++;
			memmove(deco->buffer,p,deco->buffer_pos+deco->buffer-p);	/* sezereme to pred width/height */
			deco->buffer_pos-=p-deco->buffer;
			deco->image_pos=0;
			deco->pixels=deco->width*deco->height;
			deco->line_pos=0;
		}
		p=deco->buffer;
		a=deco->buffer_pos;
		if (!deco->partnum) __skip_whitespace(&p,&a);
		if (!a){must_return=1; goto restart_again;}
		deco->actual_eight=__read_num(&p,&a,&(deco->partnum),&d,&b);
		memmove(deco->buffer,p,a);
		deco->buffer_pos=a;
		if (deco->partnum)must_return=1;
		else put_eight(cimg,(b==16&&d>2)||(b==10&&deco->actual_eight>255)?16:8);
		if (deco->image_pos>=deco->pixels) {
			img_end(cimg);
			return;
		}
		goto restart_again;
		
	}
}


/* skip comments and call real decoding function */
void xbm_restart(struct cached_image *cimg, unsigned char *data, int length)
{
	struct xbm_decoder *deco=(struct xbm_decoder*)cimg->decoder;

cycle_again:

	while(length && deco){

		switch(deco->state){
		case 0:	/* mimo komentar */
			{
				unsigned char *p;
				p=memchr(data,'/',length);
				if (!p){xbm_decode(cimg, data, length);return;}
				xbm_decode(cimg, data, p-data);
				data=p+1;	/* preskocim lomitko */
				length-=p-data+1;
				deco->state=1;
				break;
			}

		case 1: /* za 1. lomitkem */
			{
				if (*data=='*'){deco->state=2;data++;length--;goto cycle_again;}	/* zacal komentar */
				xbm_decode(cimg, "/", 1);
				deco->state=0;	/* to nebyl komentar */
				break;
			}

		case 2: /* za lomeno hvezdicka (uvnitr komentare) */
			{
				unsigned char *p;
				p=memchr(data,'*',length);
				if (!p)return;	/* furt komentar */
				data=p+1;	/* preskocim hvezdicku */
				length-=p-data+1;
				deco->state=3;
				break;
			}

		case 3: /* za 2. hvezdickou */
			{
				if (*data=='/'){data++;length--;deco->state=0;goto cycle_again;}	/* skoncil komentar */
				deco->state=2;
				data++;
				length--;
				break;
			}
		}
	}
}
#endif /* G */
