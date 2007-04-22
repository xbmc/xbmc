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

#ifdef __XBOX__

#include "../linksboks.h"

extern LPDIRECT3DDEVICE8	g_pLBd3dDevice;
extern LinksBoksWindow		*g_NewLinksBoksWindow;


extern "C" {
#include "links.h"
}

#define LINKSBOKS_REFRESH_RATE		((g_ViewPort.standard == XC_VIDEO_STANDARD_PAL_I) ? 15 : 12)

BOOL					g_bWantFlip  = FALSE;
BOOL					g_bResized   = FALSE;

extern "C" struct graphics_driver xbox_driver;

struct graphics_driver *xb_get_driver( void );

static int event_timer = 0;

char *clipboard;


/*****************************************************************************/
/***************************** LINKS FUNCTIONS *******************************/
/*****************************************************************************/





unsigned char *xb_get_driver_param(void)
{
	return NULL;
}

unsigned char *xb_init_driver(unsigned char *param, unsigned char *display)
{
    return NULL;	
}

void xb_shutdown_driver()
{

}

struct graphics_device *xb_init_device()
{
	struct graphics_device *dev = (struct graphics_device *)mem_alloc( sizeof( struct graphics_device ) );
	if( !dev )
		return NULL;

	if(g_NewLinksBoksWindow->Initialize(dev))
		return NULL;

	dev->drv = &xbox_driver;

	dev->clip.x1 = 0;
	dev->clip.x2 = g_NewLinksBoksWindow->GetViewPortWidth();
	dev->clip.y1 = 0;
	dev->clip.y2 = g_NewLinksBoksWindow->GetViewPortHeight();

	dev->size.x1 = 0;
	dev->size.x2 = g_NewLinksBoksWindow->GetViewPortWidth();
	dev->size.y1 = 0;
	dev->size.y2 = g_NewLinksBoksWindow->GetViewPortHeight();

	dev->driver_data = g_NewLinksBoksWindow;

	return dev;
}

void xb_shutdown_device(struct graphics_device *dev)
{
	LinksBoksWindow *pLB = (LinksBoksWindow *)dev->driver_data;
	pLB->Terminate();
	mem_free(dev);
}

void xb_set_title(struct graphics_device *dev, unsigned char *title)
{
}

int xb_get_filled_bitmap(struct bitmap *bmp, long color)
{
	LPDIRECT3DSURFACE8 pSurface = NULL;
	g_pLBd3dDevice->CreateImageSurface( bmp->x, bmp->y, D3DFMT_LIN_A8R8G8B8, &pSurface );

    D3DLOCKED_RECT d3dlr;
    pSurface->LockRect( &d3dlr, NULL, 0 );
	DWORD * pDst = (DWORD *)d3dlr.pBits;
	int DPitch = d3dlr.Pitch/4;

	for (int y=0; y<bmp->y; ++y)
		for (int x=0; x<bmp->x; ++x)
			pDst[y*DPitch + x] = color;
			
	bmp->data = pDst;
	bmp->skip = DPitch * 4;
	bmp->flags = pSurface;

	return 0;
}

int xb_get_empty_bitmap(struct bitmap *bmp)
{
	LPDIRECT3DSURFACE8 pSurface = NULL;
	g_pLBd3dDevice->CreateImageSurface( bmp->x, bmp->y, D3DFMT_LIN_A8R8G8B8, &pSurface );

    D3DLOCKED_RECT d3dlr;
    pSurface->LockRect( &d3dlr, NULL, 0 );
	DWORD * pDst = (DWORD *)d3dlr.pBits;
	int DPitch = d3dlr.Pitch/4;

//	for (int y=0; y<bmp->y; ++y)
//		for (int x=0; x<bmp->x; ++x)
//			pDst[y*DPitch + x] = (255 << 24 | 255 << 16);
//			pDst[y*DPitch + x] = (alpha << 24 | (255 - x ) << 16 | x << 8 | y );
//			pDst[y*DPitch + x] = ((x<y ? 0:255) << 24 | (x>320 ? 255:0 ) << 16 | 255 << 8 | 0 );
			
	bmp->data = pDst;
	bmp->skip = DPitch * 4;
	bmp->flags = pSurface;


	return 0;
}

void xb_register_bitmap(struct bitmap *bmp)
{
	LPDIRECT3DSURFACE8 pSurface = (LPDIRECT3DSURFACE8) bmp->flags;

	pSurface->UnlockRect();
}

void *xb_prepare_strip(struct bitmap *bmp, int top, int lines)
{
	LPDIRECT3DSURFACE8 pSurface = (LPDIRECT3DSURFACE8) bmp->flags;

    D3DLOCKED_RECT d3dlr;
    pSurface->LockRect( &d3dlr, 0, 0 );
	DWORD * pDst = (DWORD *)d3dlr.pBits;
	int DPitch = d3dlr.Pitch/4;

	bmp->data = pDst;
	bmp->skip = DPitch;

	return ((unsigned char *) bmp->data + top * bmp->skip);
}

void xb_commit_strip(struct bitmap *bmp, int top, int lines)
{
	LPDIRECT3DSURFACE8 pSurface = (LPDIRECT3DSURFACE8) bmp->flags;
	
	pSurface->UnlockRect();
}

void xb_unregister_bitmap(struct bitmap *bmp)
{
	LPDIRECT3DSURFACE8 pSurface = (LPDIRECT3DSURFACE8) bmp->flags;

	while(pSurface->Release());
}

void xb_draw_bitmap(struct graphics_device *dev, struct bitmap *bmp, int x, int y)
{
	LPDIRECT3DSURFACE8 pSurface = (LPDIRECT3DSURFACE8) bmp->flags;
	LinksBoksWindow *pLB = (LinksBoksWindow *)dev->driver_data;

	pLB->Blit( pSurface, x, y, bmp->x, bmp->y );

	pLB->RegisterFlip( x, y, bmp->x, bmp->y );
}

void xb_draw_bitmaps(struct graphics_device *dev, struct bitmap **bmp, int n, int x, int y)
{
	int x1 = x;
	int h = 0;
	LinksBoksWindow *pLB = (LinksBoksWindow *)dev->driver_data;

	for( int i = 0; i < n; i++ )
	{
		LPDIRECT3DSURFACE8 pSurface = (LPDIRECT3DSURFACE8) (*bmp)->flags;
		pLB->Blit( pSurface, x, y, (*bmp)->x, (*bmp)->y );

		if( h < (*bmp)->y )
			h = (*bmp)->y;

		x += (*bmp)->x;
		bmp++;
	}

	pLB->RegisterFlip( x1, y, x - x1, h );
}

long xb_get_color(int rgb)
{
	return( rgb & 0xffffff );
}

void xb_fill_area(struct graphics_device *dev, int x1, int y1, int x2, int y2, long color)
{
	LinksBoksWindow *pLB = (LinksBoksWindow *)dev->driver_data;
	pLB->FillArea(x1, y1, x2, y2, color);
}

void xb_draw_hline(struct graphics_device *dev, int x1, int y, int x2, long color)
{
	LinksBoksWindow *pLB = (LinksBoksWindow *)dev->driver_data;
	pLB->DrawHLine(x1, y, x2, color);
}

void xb_draw_vline(struct graphics_device *dev, int x, int y1, int y2, long color)
{
	LinksBoksWindow *pLB = (LinksBoksWindow *)dev->driver_data;
	pLB->DrawVLine(x, y1, y2, color);
}

int xb_hscroll(struct graphics_device *dev, struct rect_set **set, int sc)
{
	LinksBoksWindow *pLB = (LinksBoksWindow *)dev->driver_data;
	struct rect r;

	*set = init_rect_set();

	pLB->ScrollBackBuffer( dev->clip.x1, dev->clip.y1, dev->clip.x2, dev->clip.y2, sc, 0 );
	pLB->RegisterFlip( dev->clip.x1, dev->clip.y1, dev->clip.x2 - dev->clip.x1, dev->clip.y2 - dev->clip.y1 );

	r.x1 = dev->clip.x2 - (abs(sc)+1);
	r.x2 = dev->clip.x2;
	r.y1 = dev->clip.y1;
	r.y2 = dev->clip.y2;
	add_to_rect_set(set, &r);
	r.x1 = dev->clip.x1;
	r.x2 = dev->clip.x1 + (abs(sc)+1);
	add_to_rect_set(set, &r);

	return 0;
}

int xb_vscroll(struct graphics_device *dev, struct rect_set **set, int sc)
{
	LinksBoksWindow *pLB = (LinksBoksWindow *)dev->driver_data;
	struct rect r;

	*set = init_rect_set();

	pLB->ScrollBackBuffer( dev->clip.x1, dev->clip.y1, dev->clip.x2, dev->clip.y2, 0, sc );
	pLB->RegisterFlip( dev->clip.x1, dev->clip.y1, dev->clip.x2 - dev->clip.x1, dev->clip.y2 - dev->clip.y1 );

	r.x1 = dev->clip.x1;
	r.x2 = dev->clip.x2;
	r.y1 = dev->clip.y2 - (abs(sc)+1);
	r.y2 = dev->clip.y2;
	add_to_rect_set(set, &r);
	r.y1 = dev->clip.y1;
	r.y2 = dev->clip.y1 + (abs(sc)+1);
	add_to_rect_set(set, &r);

	return 0;
}

void xb_set_clip_area(struct graphics_device *dev, struct rect *r)
{
	LinksBoksWindow *pLB = (LinksBoksWindow *)dev->driver_data;
	dev->clip = *r;
	pLB->SetClipArea(r->x1, r->y1, r->x2, r->y2);
}

void xb_put_to_clipboard(struct graphics_device *gd, char *string, int length)
{
	return;
	mem_free( clipboard );

	clipboard = (char *)mem_alloc( length );
	strcpy( string, clipboard );
}

void xb_request_clipboard(struct graphics_device *gd)
{
}

unsigned char *xb_get_from_clipboard(struct graphics_device *gd)
{
	return NULL;
	return stracpy((unsigned char *)clipboard);
}

extern "C" struct graphics_driver xbox_driver = {
	(unsigned char *)"d3dx",
	xb_init_driver,
	xb_init_device,
	xb_shutdown_device,
	xb_shutdown_driver,
	xb_get_driver_param,
	xb_get_empty_bitmap,
	xb_get_filled_bitmap,
	xb_register_bitmap,
	xb_prepare_strip,
	xb_commit_strip,
	xb_unregister_bitmap,
	xb_draw_bitmap,
	xb_draw_bitmaps,
	xb_get_color,
	xb_fill_area,
	xb_draw_hline,
	xb_draw_vline,
	xb_hscroll,
	xb_vscroll,
	xb_set_clip_area,
	dummy_block,
	dummy_unblock,
	xb_set_title,
    xb_put_to_clipboard,
    xb_request_clipboard,
    xb_get_from_clipboard,
    0xc4,				/* depth: 32 bits per pixel, 24 significant, normal order */
	0, 0,				/* size */
	0,  //GD_DONT_USE_SCROLL,				/* flags */
};

#endif
