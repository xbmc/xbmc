/*
 * video_out_dx.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 *
 * Contributed by Gildas Bazin <gbazin@netcourrier.com>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef LIBVO_DX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "mpeg2.h"
#include "video_out.h"
#include "mpeg2convert.h"

#include <ddraw.h>
#include <initguid.h>

#define USE_OVERLAY_TRIPLE_BUFFERING 0

/*
 * DirectDraw GUIDs.
 * Defining them here allows us to get rid of the dxguid library during link.
 */
DEFINE_GUID (IID_IDirectDraw2, 0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_GUID (IID_IDirectDrawSurface2, 0x57805885,0x6eec,0x11cf,0x94,0x41,0xa8,0x23,0x03,0xc1,0x0e,0x27);

#define FOURCC_YV12 0x32315659

typedef struct {
    vo_instance_t vo;
    int width;
    int height;

    HWND window;
    RECT window_coords;
    HINSTANCE hddraw_dll;
    LPDIRECTDRAW2 ddraw;
    LPDIRECTDRAWSURFACE2 display;
    LPDIRECTDRAWCLIPPER clipper;
    LPDIRECTDRAWSURFACE2 frame[3];
    int index;

    LPDIRECTDRAWSURFACE2 overlay;
    uint8_t * yuv[3];
    int stride;
} dx_instance_t;

static void update_overlay (dx_instance_t * instance)
{
    DDOVERLAYFX ddofx;
    DWORD dwFlags;

    memset (&ddofx, 0, sizeof (DDOVERLAYFX));
    ddofx.dwSize = sizeof (DDOVERLAYFX);
    dwFlags = DDOVER_SHOW | DDOVER_KEYDESTOVERRIDE;
    IDirectDrawSurface2_UpdateOverlay (instance->overlay, NULL,
				       instance->display,
				       &instance->window_coords,
				       dwFlags, &ddofx);
}

static long FAR PASCAL event_procedure (HWND hwnd, UINT message,
					WPARAM wParam, LPARAM lParam)
{
    RECT rect_window;
    POINT point_window;
    dx_instance_t * instance;

    switch (message) {

    case WM_WINDOWPOSCHANGED:
	instance = (dx_instance_t *) GetWindowLong (hwnd, GWL_USERDATA);

	/* update the window position and size */
	point_window.x = 0;
	point_window.y = 0;
	ClientToScreen (hwnd, &point_window);
	instance->window_coords.left = point_window.x;
	instance->window_coords.top = point_window.y;
	GetClientRect (hwnd, &rect_window);
	instance->window_coords.right = rect_window.right + point_window.x;
	instance->window_coords.bottom = rect_window.bottom + point_window.y;

	/* update the overlay */
	if (instance->overlay && instance->display)
	    update_overlay (instance);

	return 0;

    case WM_CLOSE:	/* forbid the user to close the window */
	return 0;

    case WM_DESTROY:	/* just destroy the window */
	PostQuitMessage (0);
	return 0;
    }

    return DefWindowProc (hwnd, message, wParam, lParam);
}

static void check_events (dx_instance_t * instance)
{
    MSG msg;

    while (PeekMessage (&msg, instance->window, 0, 0, PM_REMOVE)) {
	TranslateMessage (&msg);
	DispatchMessage (&msg);
    }
}

static int create_window (dx_instance_t * instance)
{
    RECT rect_window;
    WNDCLASSEX wc;

    wc.cbSize        = sizeof (WNDCLASSEX);
    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = (WNDPROC) event_procedure;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = GetModuleHandle (NULL);
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush (RGB (0, 0, 0));
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "libvo_dx";
    wc.hIconSm       = NULL;
    if (!RegisterClassEx (&wc)) {
	fprintf (stderr, "Can not register window class\n");
	return 1;
    }

    rect_window.top    = 10;
    rect_window.left   = 10;
    rect_window.right  = rect_window.left + instance->width;
    rect_window.bottom = rect_window.top + instance->height;
    AdjustWindowRect (&rect_window, WS_OVERLAPPEDWINDOW|WS_SIZEBOX, 0);

    instance->window = CreateWindow ("libvo_dx", "mpeg2dec",
				     WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
				     CW_USEDEFAULT, 0,
				     rect_window.right - rect_window.left,
				     rect_window.bottom - rect_window.top,
				     NULL, NULL, GetModuleHandle (NULL), NULL);
    if (instance->window == NULL) {
	fprintf (stderr, "Can not create window\n");
	return 1;
    }

    /* store a directx_instance pointer into the window local storage
     * (for later use in event_handler).
     * We need to use SetWindowLongPtr when it is available in mingw */
    SetWindowLong (instance->window, GWL_USERDATA, (LONG) instance);

    ShowWindow (instance->window, SW_SHOW);

    return 0;
}

static LPDIRECTDRAWSURFACE2 alloc_surface (dx_instance_t * instance,
					   DDSURFACEDESC * ddsd)
{
    LPDIRECTDRAWSURFACE surface;
    LPDIRECTDRAWSURFACE2 surface2;

    if (DD_OK != IDirectDraw2_CreateSurface (instance->ddraw, ddsd,
					     &surface, NULL) ||
	DD_OK != IDirectDrawSurface_QueryInterface (surface,
						    &IID_IDirectDrawSurface2,
						    (LPVOID *) &surface2)) {
	fprintf (stderr, "Can not create directDraw frame surface\n");
	return NULL;
    }
    IDirectDrawSurface_Release (surface);

    return surface2;
}

static int dx_init (dx_instance_t *instance)
{
    HRESULT (WINAPI * OurDirectDrawCreate) (GUID *, LPDIRECTDRAW *,
					    IUnknown *);
    LPDIRECTDRAW ddraw;
    DDSURFACEDESC ddsd;

    /* load direct draw DLL */
    instance->hddraw_dll = LoadLibrary ("DDRAW.DLL");
    if (instance->hddraw_dll == NULL) {
	fprintf (stderr, "Can not load DDRAW.DLL\n");
	return 1;
    }

    ddraw = NULL;
    OurDirectDrawCreate = (void *) GetProcAddress (instance->hddraw_dll,
						   "DirectDrawCreate");
    if (OurDirectDrawCreate == NULL ||
	DD_OK != OurDirectDrawCreate (NULL, &ddraw, NULL) ||
	DD_OK != IDirectDraw_QueryInterface (ddraw, &IID_IDirectDraw2,
					     (LPVOID *) &instance->ddraw) ||
	DD_OK != IDirectDraw_SetCooperativeLevel (instance->ddraw,
						  instance->window,
						  DDSCL_NORMAL)) {
	fprintf (stderr, "Can not initialize directDraw interface\n");
	return 1;
    }
    IDirectDraw_Release (ddraw);

    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    instance->display = alloc_surface (instance, &ddsd);
    if (instance->display == NULL) {
	fprintf (stderr, "Can not create directDraw display surface\n");
	return 1;
    }

    if (DD_OK != IDirectDraw2_CreateClipper (instance->ddraw, 0,
					     &instance->clipper, NULL) ||
	DD_OK != IDirectDrawClipper_SetHWnd (instance->clipper, 0,
					     instance->window) ||
	DD_OK != IDirectDrawSurface_SetClipper (instance->display,
						instance->clipper)) {
	fprintf (stderr, "Can not initialize directDraw clipper\n");
	return 1;
    }

    return 0;
}

static int common_setup (dx_instance_t * instance, int width, int height)
{
    instance->width = width;
    instance->height = height;
    instance->index = 0;

    if (create_window (instance) || dx_init (instance))
	return 1;
    return 0;
}

static int dxrgb_setup (vo_instance_t * _instance, unsigned int width,
			unsigned int height, unsigned int chroma_width,
			unsigned int chroma_height, vo_setup_result_t * result)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    HDC hdc;
    int bpp;

    if (common_setup (instance, width, height))
	return 1;

    hdc = GetDC (NULL);
    bpp = GetDeviceCaps (hdc, BITSPIXEL);
    ReleaseDC (NULL, hdc);

    result->convert = mpeg2convert_rgb (MPEG2CONVERT_RGB, bpp);
    return 0;
}

static LPDIRECTDRAWSURFACE2 alloc_frame (dx_instance_t * instance)
{
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE2 surface;

    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    ddsd.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_CAPS;
    ddsd.dwHeight = instance->height;
    ddsd.dwWidth = instance->width;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    surface = alloc_surface (instance, &ddsd);
    if (surface == NULL)
	fprintf (stderr, "Can not create directDraw frame surface\n");
    return surface;
}

static void * surface_addr (LPDIRECTDRAWSURFACE2 surface, int * stride)
{
    DDSURFACEDESC ddsd;

    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    IDirectDrawSurface2_Lock (surface, NULL, &ddsd,
			      DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL);
    IDirectDrawSurface2_Unlock (surface, NULL);
    *stride = ddsd.lPitch;
    return ddsd.lpSurface;
}

static void dx_setup_fbuf (vo_instance_t * _instance,
			   uint8_t ** buf, void ** id)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    int stride;

    *id = instance->frame[instance->index++] = alloc_frame (instance);
    buf[0] = surface_addr (*id, &stride);
    buf[1] = NULL;	buf[2] = NULL;
}

static void dxrgb_draw_frame (vo_instance_t * _instance,
			      uint8_t * const * buf, void * id)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    LPDIRECTDRAWSURFACE2 surface = (LPDIRECTDRAWSURFACE2) id;
    DDBLTFX ddbltfx;

    check_events (instance);

    memset (&ddbltfx, 0, sizeof (DDBLTFX));
    ddbltfx.dwSize = sizeof (DDBLTFX);
    ddbltfx.dwDDFX = DDBLTFX_NOTEARING;
    if (DDERR_SURFACELOST ==
	IDirectDrawSurface2_Blt (instance->display, &instance->window_coords,
				 surface, NULL, DDBLT_WAIT, &ddbltfx)) {
	/* restore surface and try again */
	IDirectDrawSurface2_Restore (instance->display);
	IDirectDrawSurface2_Blt (instance->display, &instance->window_coords,
				 surface, NULL, DDBLT_WAIT, &ddbltfx);
    }
}

static vo_instance_t * common_open (int setup (vo_instance_t *,
					       unsigned int, unsigned int,
					       unsigned int, unsigned int,
					       vo_setup_result_t *),
				    void setup_fbuf (vo_instance_t *,
						     uint8_t **, void **),
				    void draw (vo_instance_t *,
					       uint8_t * const *, void * id))
{
    dx_instance_t * instance;

    instance = malloc (sizeof (dx_instance_t));
    if (instance == NULL)
	return NULL;

    memset (instance, 0, sizeof (dx_instance_t));
    instance->vo.setup = setup;
    instance->vo.setup_fbuf = setup_fbuf;
    instance->vo.set_fbuf = NULL;
    instance->vo.start_fbuf = NULL;
    instance->vo.draw = draw;
    instance->vo.discard = NULL;
    instance->vo.close = NULL; //dx_close;

    return (vo_instance_t *) instance;
}

vo_instance_t * vo_dxrgb_open (void)
{
    return common_open (dxrgb_setup, dx_setup_fbuf, dxrgb_draw_frame);
}

static LPDIRECTDRAWSURFACE2 alloc_overlay (dx_instance_t * instance)
{
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE2 surface;

    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    ddsd.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_CAPS;
    ddsd.dwHeight = instance->height;
    ddsd.dwWidth = instance->width;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    ddsd.ddpfPixelFormat.dwFourCC = FOURCC_YV12;
    ddsd.dwFlags |= DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
#if USE_OVERLAY_TRIPLE_BUFFERING
    ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
#endif
    ddsd.dwBackBufferCount = 2;

    surface = alloc_surface (instance, &ddsd);
    if (surface == NULL)
	fprintf (stderr, "Can not create directDraw frame surface\n");
    return surface;
}

static int dx_setup (vo_instance_t * _instance, unsigned int width,
		     unsigned int height, unsigned int chroma_width,
		     unsigned int chroma_height, vo_setup_result_t * result)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    LPDIRECTDRAWSURFACE2 surface;
    DDSURFACEDESC ddsd;

    if (common_setup (instance, width, height))
	return 1;

    instance->overlay = alloc_overlay (instance);
    if (!instance->overlay)
	return 1;
    update_overlay (instance);

    surface = instance->overlay;

    /* Get the back buffer */
    memset (&ddsd.ddsCaps, 0, sizeof (DDSCAPS));
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    if (DD_OK != IDirectDrawSurface2_GetAttachedSurface (instance->overlay,
							 &ddsd.ddsCaps,
							 &surface))
	surface = instance->overlay;

    instance->yuv[0] = surface_addr (surface, &instance->stride);
    instance->yuv[2] = instance->yuv[0] + instance->stride * instance->height;
    instance->yuv[1] =
	instance->yuv[2] + (instance->stride * instance->height >> 2);

    result->convert = NULL;
    return 0;
}

static void copy_yuv_picture (dx_instance_t * instance,
			      uint8_t * const * buf, void * id)
{
    uint8_t * dest[3];
    int width, i;

    dest[0] = instance->yuv[0];
    dest[1] = instance->yuv[1];
    dest[2] = instance->yuv[2];

    width = instance->width;
    for (i = 0; i < instance->height >> 1; i++) {
	memcpy (dest[0], buf[0] + 2 * i * width, width);
	dest[0] += instance->stride;
	memcpy (dest[0], buf[0] + (2 * i + 1) * width, width);
	dest[0] += instance->stride;
	memcpy (dest[1], buf[1] + i * (width >> 1), width >> 1);
	dest[1] += instance->stride >> 1;
	memcpy (dest[2], buf[2] + i * (width >> 1), width >> 1);
	dest[2] += instance->stride >> 1;
    }
}

static void dx_draw_frame (vo_instance_t * _instance,
			   uint8_t * const * buf, void * id)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;

    check_events (instance);

    copy_yuv_picture (instance, buf, id);

    if (DDERR_SURFACELOST ==
	IDirectDrawSurface2_Flip (instance->overlay, NULL, DDFLIP_WAIT)) {
	/* restore surfaces and try again */
	IDirectDrawSurface2_Restore (instance->display);
	IDirectDrawSurface2_Restore (instance->overlay);
	IDirectDrawSurface2_Flip (instance->overlay, NULL, DDFLIP_WAIT);
    }
}

vo_instance_t * vo_dx_open (void)
{
    return common_open (dx_setup, NULL, dx_draw_frame);
}

#if 0
static void dx_close (vo_instance_t * _instance)
{
    dx_instance_t * instance;
    int i;

    instance = (dx_instance_t *) _instance;

    if (instance->using_overlay && instance->overlay) {
	IDirectDrawSurface2_Release (instance->overlay);
	instance->overlay = NULL;
    } else
	for (i = 0; i < 3; i++) {
	    if (instance->frame[i].p_surface != NULL)
		IDirectDrawSurface2_Release (instance->frame[i].p_surface);
	    instance->frame[i].p_surface = NULL;
	}

    if (instance->clipper != NULL)
	IDirectDrawClipper_Release (instance->clipper);

    if (instance->display != NULL)
	IDirectDrawSurface2_Release (instance->display);

    if (instance->ddraw != NULL)
	IDirectDraw2_Release (instance->ddraw);

    if (instance->hddraw_dll != NULL)
	FreeLibrary (instance->hddraw_dll);

    if (instance->window != NULL)
	DestroyWindow (instance->window);
}

#endif
#endif
