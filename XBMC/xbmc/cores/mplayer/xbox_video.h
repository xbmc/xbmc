#ifndef XBOX_VIDEO_RENDERER
#define XBOX_VIDEO_RENDERER
#include <xtl.h>

// enum for the different resolutions
typedef enum _RESOLUTION {	HDTV_1080i		= 0,
							HDTV_720p		= 1,
							HDTV_480p_4_3	= 2,
							HDTV_480p_16_9	= 3,
							NTSC_4_3		= 4,
							NTSC_16_9		= 5,
							PAL_4_3			= 6,
							PAL_16_9		= 7,
							PAL60_4_3		= 8,
							PAL60_16_9		= 9 } RESOLUTION;

// Our resolution structure
typedef struct _RESOLUTION_INFO {
		UINT iWidth;
		UINT iHeight;
		DWORD dwFlags;
		float fPixelRatio;
		char strMode[11];
	} RESOLUTION_INFO;

static RESOLUTION_INFO resInfo[10]={1920,1080, D3DPRESENTFLAG_INTERLACED|D3DPRESENTFLAG_WIDESCREEN,  1.0f, "540p 16:9",
									1280, 720, D3DPRESENTFLAG_PROGRESSIVE|D3DPRESENTFLAG_WIDESCREEN, 1.0f, "720p 16:9",
									 720, 480, D3DPRESENTFLAG_PROGRESSIVE, 72.0f/79.0f, "480p 4:3",
									 720, 480, D3DPRESENTFLAG_PROGRESSIVE|D3DPRESENTFLAG_WIDESCREEN, 72.0f/79.0f*4.0f/3.0f, "480p 16:9",
									 720, 480, 0, 72.0f/79.0f, "NTSC 4:3",
									 720, 480, D3DPRESENTFLAG_WIDESCREEN, 72.0f/79.0f*4.0f/3.0f, "NTSC 16:9",
									 720, 576, 0, 128.0f/117.0f, "PAL 4:3",
									 720, 576, D3DPRESENTFLAG_WIDESCREEN, 128.0f/117.0f*4.0f/3.0f, "PAL 16:9",
									 720, 480, 0, 72.0f/79.0f, "PAL60 4:3",
									 720, 480, D3DPRESENTFLAG_WIDESCREEN, 72.0f/79.0f*4.0f/3.0f, "PAL60 16:9"};

#endif