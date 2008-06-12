#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct vo_info_s
  {
    /* driver name ("Matrox Millennium G200/G400" */
    const char *name;
    /* short name (for config strings) ("mga") */
    const char *short_name;
    /* author ("Aaron Holtzman <aholtzma@ess.engr.uvic.ca>") */
    const char *author;
    /* any additional comments */
    const char *comment;
  }
  vo_info_t;

#define IMGFMT_RGB_MASK 0xFFFFFF00
#define IMGFMT_RGB (('R'<<24)|('G'<<16)|('B'<<8))
#define IMGFMT_RGB1  (IMGFMT_RGB|1)
#define IMGFMT_RGB4  (IMGFMT_RGB|4)
#define IMGFMT_RG4B  (IMGFMT_RGB|4|128) // RGB4 with 1 pixel per byte
#define IMGFMT_RGB8  (IMGFMT_RGB|8)
#define IMGFMT_RGB15 (IMGFMT_RGB|15)
#define IMGFMT_RGB16 (IMGFMT_RGB|16)
#define IMGFMT_RGB24 (IMGFMT_RGB|24)
#define IMGFMT_RGB32 (IMGFMT_RGB|32)

#define IMGFMT_BGR_MASK 0xFFFFFF00
#define IMGFMT_BGR (('B'<<24)|('G'<<16)|('R'<<8))
#define IMGFMT_BGR1 (IMGFMT_BGR|1)
#define IMGFMT_BGR4 (IMGFMT_BGR|4)
#define IMGFMT_BG4B (IMGFMT_BGR|4|128) // BGR4 with 1 pixel per byte
#define IMGFMT_BGR8 (IMGFMT_BGR|8)
#define IMGFMT_BGR15 (IMGFMT_BGR|15)
#define IMGFMT_BGR16 (IMGFMT_BGR|16)
#define IMGFMT_BGR24 (IMGFMT_BGR|24)
#define IMGFMT_BGR32 (IMGFMT_BGR|32)

#define IMGFMT_IS_RGB(fmt) (((fmt)&IMGFMT_RGB_MASK)==IMGFMT_RGB)
#define IMGFMT_IS_BGR(fmt) (((fmt)&IMGFMT_BGR_MASK)==IMGFMT_BGR)

#define IMGFMT_RGB_DEPTH(fmt) ((fmt)&0x3F)
#define IMGFMT_BGR_DEPTH(fmt) ((fmt)&0x3F)


  /* Planar YUV Formats */

#define IMGFMT_YVU9 0x39555659
#define IMGFMT_IF09 0x39304649
#define IMGFMT_YV12 0x32315659
#define IMGFMT_I420 0x30323449
#define IMGFMT_IYUV 0x56555949
#define IMGFMT_CLPL 0x4C504C43
#define IMGFMT_Y800 0x30303859
#define IMGFMT_Y8   0x20203859
#define IMGFMT_NV12 0x3231564E

  /* unofficial Planar Formats, FIXME if official 4CC exists */
#define IMGFMT_444P 0x50343434
#define IMGFMT_422P 0x50323234
#define IMGFMT_411P 0x50313134
#define IMGFMT_HM12 0x32314D48

  /* Packed YUV Formats */

#define IMGFMT_IUYV 0x56595549
#define IMGFMT_IY41 0x31435949
#define IMGFMT_IYU1 0x31555949
#define IMGFMT_IYU2 0x32555949
#define IMGFMT_UYVY 0x59565955
#define IMGFMT_UYNV 0x564E5955
#define IMGFMT_cyuv 0x76757963
#define IMGFMT_Y422 0x32323459
#define IMGFMT_YUY2 0x32595559
#define IMGFMT_YUNV 0x564E5559
#define IMGFMT_YVYU 0x55595659
#define IMGFMT_Y41P 0x50313459
#define IMGFMT_Y211 0x31313259
#define IMGFMT_Y41T 0x54313459
#define IMGFMT_Y42T 0x54323459
#define IMGFMT_V422 0x32323456
#define IMGFMT_V655 0x35353656
#define IMGFMT_CLJR 0x524A4C43
#define IMGFMT_YUVP 0x50565559
#define IMGFMT_UYVP 0x50565955
#define VO_EVENT_EXPOSE 1
#define VO_EVENT_RESIZE 2
#define VO_EVENT_KEYPRESS 4

  /* Obsolete: VOCTRL_QUERY_VAA 1 */
  /* does the device support the required format */
#define VOCTRL_QUERY_FORMAT 2 
  /* signal a device reset seek */
#define VOCTRL_RESET 3 
  /* true if vo driver can use GUI created windows */
#define VOCTRL_GUISUPPORT 4
#define VOCTRL_GUI_NOWINDOW 19 
  /* used to switch to fullscreen */
#define VOCTRL_FULLSCREEN 5
#define VOCTRL_SCREENSHOT 6 
  /* signal a device pause */
#define VOCTRL_PAUSE 7 
  /* start/resume playback */
#define VOCTRL_RESUME 8 
  /* libmpcodecs direct rendering: */
#define VOCTRL_GET_IMAGE 9
#define VOCTRL_DRAW_IMAGE 13
#define VOCTRL_SET_SPU_PALETTE 14 
  /* decoding ahead: */
#define VOCTRL_GET_NUM_FRAMES 10
#define VOCTRL_GET_FRAME_NUM  11
#define VOCTRL_SET_FRAME_NUM  12
#define VOCTRL_GET_PANSCAN 15
#define VOCTRL_SET_PANSCAN 16 
  /* equalizer controls */
#define VOCTRL_SET_EQUALIZER 17
#define VOCTRL_GET_EQUALIZER 18 
  //#define VOCTRL_GUI_NOWINDOW 19
  /* Frame duplication */
#define VOCTRL_DUPLICATE_FRAME 20 
  // ... 21
#define VOCTRL_START_SLICE 21
#define MP_MAX_PLANES 4

#define MP_IMGFIELD_ORDERED 0x01
#define MP_IMGFIELD_TOP_FIRST 0x02
#define MP_IMGFIELD_REPEAT_FIRST 0x04
#define MP_IMGFIELD_TOP 0x08
#define MP_IMGFIELD_BOTTOM 0x10
#define MP_IMGFIELD_INTERLACED 0x20

  typedef struct mp_image_s
  {
    unsigned short flags;
    unsigned char type;
    unsigned char bpp;  // bits/pixel. NOT depth! for RGB it will be n*8
    unsigned int imgfmt;
    int width, height;  // stored dimensions
    int x, y, w, h;  // visible dimensions
    unsigned char* planes[MP_MAX_PLANES];
    unsigned int stride[MP_MAX_PLANES];
    char * qscale;
    int qstride;
    int pict_type; // 0->unknown, 1->I, 2->P, 3->B
    int fields;
    int qscale_type; // 0->mpeg1/4/h263, 1->mpeg2
    int num_planes;
    /* these are only used by planar formats Y,U(Cb),V(Cr) */
    int chroma_width;
    int chroma_height;
    int chroma_x_shift; // horizontal
    int chroma_y_shift; // vertical
    /* for private use by filter or vo driver (to store buffer id or dmpi) */
    void* priv;
  }
  mp_image_t;


  typedef struct vo_functions_s
  {
    vo_info_t *info;
    unsigned int (*preinit)(const char *arg);
    unsigned int (*config)(unsigned int width, unsigned int height, unsigned int d_width,
                           unsigned int d_height, unsigned int fullscreen, char *title,
                           unsigned int format);
    unsigned int (*control)(unsigned int request, void *data, ...);
    unsigned int (*draw_frame)(unsigned char *src[]);
    unsigned int (*draw_slice)(unsigned char *src[], int stride[], int w, int h, int x, int y);
    void (*draw_osd)(void);
    void (*flip_page)(void);
    void (*check_events)(void);
    void (*uninit)(void);
  }
  vo_functions_t;


#define A_ZOOM 1
#define A_NOZOOM 0


  //--- buffer content restrictions:
  // set if buffer content shouldn't be modified:
#define MP_IMGFLAG_PRESERVE 0x01 
  // set if buffer content will be READ for next frame's MC: (I/P mpeg frames)
#define MP_IMGFLAG_READABLE 0x02

  //--- buffer width/stride/plane restrictions: (used for direct rendering)
  // stride _have_to_ be aligned to MB boundary:  [for DR restrictions]
#define MP_IMGFLAG_ACCEPT_ALIGNED_STRIDE 0x4 
  // stride should be aligned to MB boundary:     [for buffer allocation]
#define MP_IMGFLAG_PREFER_ALIGNED_STRIDE 0x8 
  // codec accept any stride (>=width):
#define MP_IMGFLAG_ACCEPT_STRIDE 0x10 
  // codec accept any width (width*bpp=stride -> stride%bpp==0) (>=width):
#define MP_IMGFLAG_ACCEPT_WIDTH 0x20 
  //--- for planar formats only:
  // uses only stride[0], and stride[1]=stride[2]=stride[0]>>mpi->chroma_x_shift
#define MP_IMGFLAG_COMMON_STRIDE 0x40 
  // uses only planes[0], and calculates planes[1,2] from width,height,imgfmt
#define MP_IMGFLAG_COMMON_PLANE 0x80

#define MP_IMGFLAGMASK_RESTRICTIONS 0xFF

  //--------- color info (filled by mp_image_setfmt() ) -----------
  // set if number of planes > 1
#define MP_IMGFLAG_PLANAR 0x100 
  // set if it's YUV colorspace
#define MP_IMGFLAG_YUV 0x200 
  // set if it's swapped (BGR or YVU) plane/byteorder
#define MP_IMGFLAG_SWAPPED 0x400 
  // using palette for RGB data
#define MP_IMGFLAG_RGB_PALETTE 0x800

#define MP_IMGFLAGMASK_COLORS 0xF00

  // codec uses drawing/rendering callbacks (draw_slice()-like thing, DR method 2)
  // [the codec will set this flag if it supports callbacks, and the vo _may_
  //  clear it in get_image() if draw_slice() not implemented]
#define MP_IMGFLAG_DRAW_CALLBACK 0x1000 
  // set if it's in video buffer/memory: [set by vo/vf's get_image() !!!]
#define MP_IMGFLAG_DIRECT 0x2000 
  // set if buffer is allocated (used in destination images):
#define MP_IMGFLAG_ALLOCATED 0x4000

  // buffer type was printed (do NOT set this flag - it's for INTERNAL USE!!!)
#define MP_IMGFLAG_TYPE_DISPLAYED 0x8000

  // codec doesn't support any form of direct rendering - it has own buffer
  // allocation. so we just export its buffer pointers:
#define MP_IMGTYPE_EXPORT 0 
  // codec requires a static WO buffer, but it does only partial updates later:
#define MP_IMGTYPE_STATIC 1 
  // codec just needs some WO memory, where it writes/copies the whole frame to:
#define MP_IMGTYPE_TEMP 2 
  // I+P type, requires 2+ independent static R/W buffers
#define MP_IMGTYPE_IP 3 
  // I+P+B type, requires 2+ independent static R/W and 1+ temp WO buffers
#define MP_IMGTYPE_IPB 4


  // set, if the given colorspace is supported (with or without conversion)
#define VFCAP_CSP_SUPPORTED 0x1 
  // set, if the given colorspace is supported _without_ conversion
#define VFCAP_CSP_SUPPORTED_BY_HW 0x2 
  // set if the driver/filter can draw OSD
#define VFCAP_OSD 0x4 
  // set if the driver/filter can handle compressed SPU stream
#define VFCAP_SPU 0x8 
  // scaling up/down by hardware, or software:
#define VFCAP_HWSCALE_UP 0x10
#define VFCAP_HWSCALE_DOWN 0x20
#define VFCAP_SWSCALE 0x40 
  // driver/filter can do vertical flip (upside-down)
#define VFCAP_FLIP 0x80

  // driver/hardware handles timing (blocking)
#define VFCAP_TIMER 0x100 
  // driver _always_ flip image upside-down (for ve_vfw)
#define VFCAP_FLIPPED 0x200 
  // vf filter: accepts stride (put_image)
  // vo driver: has draw_slice() support for the given csp
#define VFCAP_ACCEPT_STRIDE 0x400 
  // filter does postprocessing (so you shouldn't scale/filter image before it)
#define VFCAP_POSTPROC 0x800

#define VO_TRUE  1
#define VO_FALSE 0
#define VO_ERROR -1
#define VO_NOTAVAIL -2
#define VO_NOTIMPL -3

#define VOFLAG_FULLSCREEN 0x01
#define VOFLAG_MODESWITCHING 0x02
#define VOFLAG_SWSCALE  0x04
#define VOFLAG_FLIPPING  0x08
#define VOFLAG_XOVERLAY_SUB_VO  0x10000

  extern void vo_draw_alpha_yv12(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase, int dststride);
  extern void vo_draw_alpha_yuy2(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase, int dststride);
  extern void vo_draw_alpha_rgb24(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase, int dststride);
  extern void vo_draw_alpha_rgb32(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase, int dststride);
  extern void vo_draw_alpha_rgb15(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase, int dststride);
  extern void vo_draw_alpha_rgb16(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase, int dststride);
  extern void aspect_save_orig(int orgw, int orgh);
  extern void aspect(unsigned int *srcw, unsigned int *srch, int zoom);
  extern void aspect_save_prescale(int prew, int preh);
  extern void aspect_save_screenres(int scrw, int scrh);

  extern void draw_alpha(int x0, int y0, int w, int h, unsigned char* src, unsigned char *srca, int stride);
  extern void vo_draw_text(int dxs, int dys, void (*draw_alpha)(int x0, int y0, int w, int h, unsigned char* src, unsigned char *srca, int stride));

  extern vo_functions_t video_functions;

  typedef struct
  {
    unsigned char *y;
    unsigned char *u;
    unsigned char *v;
  }
  IMAGE;
  extern int image_output(IMAGE * image, unsigned int width, int height, unsigned int edged_width, unsigned char * dst[4], unsigned int dst_stride[4], int csp, int interlaced);


#define XVID_CSP_USER     (1<< 0) /* 4:2:0 planar */
#define XVID_CSP_I420     (1<< 1) /* 4:2:0 packed(planar win32) */
#define XVID_CSP_YV12     (1<< 2) /* 4:2:0 packed(planar win32) */
#define XVID_CSP_YUY2     (1<< 3) /* 4:2:2 packed */
#define XVID_CSP_UYVY     (1<< 4) /* 4:2:2 packed */
#define XVID_CSP_YVYU     (1<< 5) /* 4:2:2 packed */
#define XVID_CSP_BGRA     (1<< 6) /* 32-bit bgra packed */
#define XVID_CSP_ABGR     (1<< 7) /* 32-bit abgr packed */
#define XVID_CSP_RGBA     (1<< 8) /* 32-bit rgba packed */
#define XVID_CSP_BGR      (1<< 9) /* 24-bit bgr packed */
#define XVID_CSP_RGB555   (1<<10) /* 16-bit rgb555 packed */
#define XVID_CSP_RGB565   (1<<11) /* 16-bit rgb565 packed */
#define XVID_CSP_SLICE    (1<<12) /* decoder only: 4:2:0 planar, per slice rendering */
#define XVID_CSP_INTERNAL (1<<13) /* decoder only: 4:2:0 planar, returns ptrs to internal buffers */
#define XVID_CSP_NULL     (1<<14) /* decoder only: dont output anything */
#define XVID_CSP_VFLIP    (1<<31) /* vertical flip mask */






#ifdef __cplusplus
};
#endif
