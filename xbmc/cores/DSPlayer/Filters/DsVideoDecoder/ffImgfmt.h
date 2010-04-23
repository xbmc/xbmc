#ifndef _FFIMGFMT_H_
#define _FFIMGFMT_H_

//================================ ffdshow ==================================
// the RGB related values in this enum refers to the "memory byte order" (byte order as stored in memory).
// under x86 architecture (little endians), the byte order is stored reversed (comparing to the write order),
// this means RGB will be stored in the memory as BGR.
// When working with DirectShow in the Red-Green-Blue colorspace, DirectShow always
// expects RGB at the "memory byte order", so RGB should be selected.
// When working with files, we are interested in the "write order", which is the opposite
// of the "memory byte order" (under x86), so you should select the opposite from the
// destination byte order for the file.
// (e.g. PNG images uses RGB order, so select BGR order
// BMP image uses BGR order, so select RGB order)
enum
{
 FF_CSP_NULL       =     0,

 FF_CSP_420P       =1U<< 0,
 FF_CSP_422P       =1U<< 1,
 FF_CSP_444P       =1U<< 2,
 FF_CSP_411P       =1U<< 3,
 FF_CSP_410P       =1U<< 4,

 FF_CSP_YUY2       =1U<< 5,
 FF_CSP_UYVY       =1U<< 6,
 FF_CSP_YVYU       =1U<< 7,
 FF_CSP_VYUY       =1U<< 8,

 FF_CSP_ABGR       =1U<< 9,  // [a|b|g|r]
 FF_CSP_RGBA       =1U<<10,  // [r|g|b|a]
 FF_CSP_BGR32      =1U<<11,
 FF_CSP_BGR24      =1U<<12,
 FF_CSP_BGR15      =1U<<13,
 FF_CSP_BGR16      =1U<<14,
 FF_CSP_RGB32      =1U<<15,
 FF_CSP_RGB24      =1U<<16,
 FF_CSP_RGB15      =1U<<17,
 FF_CSP_RGB16      =1U<<18,

 FF_CSP_PAL8       =1U<<19,
 FF_CSP_CLJR       =1U<<20,
 FF_CSP_Y800       =1U<<21,
 FF_CSP_NV12       =1U<<22
};

#define FF_CSPS_NUM 24

#define FF_CSPS_MASK            ((2<<FF_CSPS_NUM)-1)
#define FF_CSPS_MASK_YUV_PLANAR (FF_CSP_420P|FF_CSP_422P|FF_CSP_444P|FF_CSP_411P|FF_CSP_410P)
#define FF_CSPS_MASK_YUV_PACKED (FF_CSP_YUY2|FF_CSP_UYVY|FF_CSP_YVYU|FF_CSP_VYUY)
#define FF_CSPS_MASK_RGB        (FF_CSP_RGBA|FF_CSP_RGB32|FF_CSP_RGB24|FF_CSP_RGB15|FF_CSP_RGB16)
#define FF_CSPS_MASK_BGR        (FF_CSP_ABGR|FF_CSP_BGR32|FF_CSP_BGR24|FF_CSP_BGR15|FF_CSP_BGR16)

#define FF_CSP_FLAGS_VFLIP      (1U<<31)   // flip mask
#define FF_CSP_FLAGS_INTERLACED (1U<<30)
#define FF_CSP_FLAGS_YUV_ADJ    (1U<<29)   // YUV planes are stored consecutively in one memory block
#define FF_CSP_FLAGS_YUV_ORDER  (1U<<28)   // UV ordered chroma planes (not VU as default)
#define FF_CSP_FLAGS_YUV_JPEG   (1U<<27)

#include <stddef.h>
typedef ptrdiff_t stride_t;

//==================================== xvid4 =====================================

#define XVID4_CSP_PLANAR   (1<< 0) /* 4:2:0 planar */
#define XVID4_CSP_I420     (1<< 1) /* 4:2:0 packed(planar win32) */
#define XVID4_CSP_YV12     (1<< 2) /* 4:2:0 packed(planar win32) */
#define XVID4_CSP_YUY2     (1<< 3) /* 4:2:2 packed */
#define XVID4_CSP_UYVY     (1<< 4) /* 4:2:2 packed */
#define XVID4_CSP_YVYU     (1<< 5) /* 4:2:2 packed */
#define XVID4_CSP_BGRA     (1<< 6) /* 32-bit bgra packed */
#define XVID4_CSP_ABGR     (1<< 7) /* 32-bit abgr packed */
#define XVID4_CSP_RGBA     (1<< 8) /* 32-bit rgba packed */
#define XVID4_CSP_BGR      (1<< 9) /* 24-bit bgr packed */
#define XVID4_CSP_RGB555   (1<<10) /* 16-bit rgb555 packed */
#define XVID4_CSP_RGB565   (1<<11) /* 16-bit rgb565 packed */
#define XVID4_CSP_SLICE    (1<<12) /* decoder only: 4:2:0 planar, per slice rendering */
#define XVID4_CSP_INTERNAL (1<<13) /* decoder only: 4:2:0 planar, returns ptrs to internal buffers */
#define XVID4_CSP_NULL     (1<<14) /* decoder only: dont output anything */
#define XVID4_CSP_VFLIP    (1<<31) /* vertical flip mask */

static __inline int csp_xvid4_2ffdshow(int csp)
{
 switch (csp)
  {
   case XVID4_CSP_BGR   :return FF_CSP_RGB24;
   case XVID4_CSP_YV12  :return FF_CSP_420P|FF_CSP_FLAGS_YUV_ADJ;
   case XVID4_CSP_YUY2  :return FF_CSP_YUY2;
   case XVID4_CSP_UYVY  :return FF_CSP_UYVY;
   case XVID4_CSP_I420  :return FF_CSP_420P|FF_CSP_FLAGS_YUV_ADJ|FF_CSP_FLAGS_YUV_ORDER;
   case XVID4_CSP_RGB555:return FF_CSP_RGB15;
   case XVID4_CSP_RGB565:return FF_CSP_RGB16;
   case XVID4_CSP_PLANAR:return FF_CSP_420P;
   case XVID4_CSP_YVYU  :return FF_CSP_YVYU;
   case XVID4_CSP_BGRA  :return FF_CSP_RGB32;
   case XVID4_CSP_ABGR  :return FF_CSP_ABGR;
   case XVID4_CSP_RGBA  :return FF_CSP_RGBA;
   default              :return FF_CSP_NULL;
  }
}

//================================= libavcodec ===================================

/**
 * Pixel format. Notes:
 *
 * PIX_FMT_RGB32 is handled in an endian-specific manner. An RGBA
 * color is put together as:
 *  (A << 24) | (R << 16) | (G << 8) | B
 * This is stored as BGRA on little-endian CPU architectures and ARGB on
 * big-endian CPUs.
 *
 * When the pixel format is palettized RGB (PIX_FMT_PAL8), the palettized
 * image data is stored in AVFrame.data[0]. The palette is transported in
 * AVFrame.data[1], is 1024 bytes long (256 4-byte entries) and is
 * formatted the same as in PIX_FMT_RGB32 described above (i.e., it is
 * also endian-specific). Note also that the individual RGB palette
 * components stored in AVFrame.data[1] should be in the range 0..255.
 * This is important as many custom PAL8 video codecs that were designed
 * to run on the IBM VGA graphics adapter use 6-bit palette components.
 *
 * For all the 8bit per pixel formats, an RGB32 palette is in data[1] like
 * for pal8. This palette is filled in automatically by the function
 * allocating the picture.
 *
 * Note, make sure that all newly added big endian formats have pix_fmt&1==1
 *       and that all newly added little endian formats have pix_fmt&1==0
 *       this allows simpler detection of big vs little endian.
 */
extern "C"
{
#include "libavutil/pixfmt.h"
}
#define PIX_FMT_RGBA PIX_FMT_BGR32
#define PIX_FMT_BGRA PIX_FMT_RGB32
#define PIX_FMT_ARGB PIX_FMT_BGR32_1
#define PIX_FMT_ABGR PIX_FMT_RGB32_1

static __inline int csp_lavc2ffdshow(enum PixelFormat pix_fmt)
{
 switch (pix_fmt)
  {
   case PIX_FMT_YUV420P :return FF_CSP_420P;
   case PIX_FMT_YUVJ420P:return FF_CSP_420P|FF_CSP_FLAGS_YUV_JPEG;
   case PIX_FMT_YUV422P :return FF_CSP_422P;
   case PIX_FMT_YUVJ422P:return FF_CSP_422P|FF_CSP_FLAGS_YUV_JPEG;
   case PIX_FMT_YUV444P :return FF_CSP_444P;
   case PIX_FMT_YUVJ444P:return FF_CSP_444P|FF_CSP_FLAGS_YUV_JPEG;
   case PIX_FMT_YUV411P :return FF_CSP_411P;
   case PIX_FMT_YUV410P :return FF_CSP_410P;
   case PIX_FMT_YUYV422 :return FF_CSP_YUY2;
   case PIX_FMT_UYVY422 :return FF_CSP_UYVY;
   case PIX_FMT_RGB24   :return FF_CSP_RGB24;
   case PIX_FMT_BGR24   :return FF_CSP_BGR24;
   //case PIX_FMT_RGB32   :return FF_CSP_RGB32;
   case PIX_FMT_RGB555  :return FF_CSP_RGB15;
   case PIX_FMT_RGB565  :return FF_CSP_RGB16;
   case PIX_FMT_PAL8    :return FF_CSP_PAL8;
   case PIX_FMT_GRAY8   :return FF_CSP_Y800;
   default              :return FF_CSP_NULL;
  }
}
static __inline enum PixelFormat csp_ffdshow2lavc(int pix_fmt)
{
 switch (pix_fmt&FF_CSPS_MASK)
  {
   case FF_CSP_420P:return pix_fmt&FF_CSP_FLAGS_YUV_JPEG?PIX_FMT_YUVJ420P:PIX_FMT_YUV420P;
   case FF_CSP_422P:return pix_fmt&FF_CSP_FLAGS_YUV_JPEG?PIX_FMT_YUVJ422P:PIX_FMT_YUV422P;
   case FF_CSP_444P:return pix_fmt&FF_CSP_FLAGS_YUV_JPEG?PIX_FMT_YUVJ444P:PIX_FMT_YUV444P;
   case FF_CSP_411P:return PIX_FMT_YUV411P;
   case FF_CSP_410P:return PIX_FMT_YUV410P;
   case FF_CSP_YUY2:return PIX_FMT_YUYV422;
   case FF_CSP_UYVY:return PIX_FMT_UYVY422;
   case FF_CSP_RGB24:return PIX_FMT_RGB24;
   case FF_CSP_BGR24:return PIX_FMT_BGR24;
   //case FF_CSP_RGB32:return PIX_FMT_RGB32;
   case FF_CSP_RGB15:return PIX_FMT_RGB555;
   case FF_CSP_RGB16:return PIX_FMT_RGB565;
   case FF_CSP_PAL8:return PIX_FMT_PAL8;
   case FF_CSP_Y800:return PIX_FMT_GRAY8;
   default         :return PIX_FMT_NB;
  }
}

//=================================== mplayer ===================================

// RGB/BGR Formats

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

// Planar YUV Formats

#define IMGFMT_YVU9 0x39555659
#define IMGFMT_IF09 0x39304649
#define IMGFMT_YV12 0x32315659
#define IMGFMT_I420 0x30323449
#define IMGFMT_IYUV 0x56555949
#define IMGFMT_CLPL 0x4C504C43
#define IMGFMT_Y800 0x30303859
#define IMGFMT_Y8   0x20203859
#define IMGFMT_NV12 0x3231564E
#define IMGFMT_NV21 0x3132564E

// unofficial Planar Formats, FIXME if official 4CC exists
#define IMGFMT_444P 0x50343434
#define IMGFMT_422P 0x50323234
#define IMGFMT_411P 0x50313134

// Packed YUV Formats

#define IMGFMT_IUYV 0x56595549
#define IMGFMT_IY41 0x31435949
#define IMGFMT_IYU1 0x31555949
#define IMGFMT_IYU2 0x32555949
#define IMGFMT_UYVY 0x59565955
#define IMGFMT_VYUY 0x59555956
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

static __inline int csp_mplayercsp2Bpp(int mplayercsp)
{
 // return byte per pixel for the first plane.
 // IMGFMT_NV21,IMGFMT_NV12 are troublesome, because bpp is different in first plane and second plane.
 switch (mplayercsp)
  {
   case IMGFMT_YV12 :return 1;
   case IMGFMT_422P :return 1;
   case IMGFMT_444P :return 1;
   case IMGFMT_411P :return 1;
   case IMGFMT_YVU9 :return 1;

   case IMGFMT_YUY2 :return 2;
   case IMGFMT_UYVY :return 2;
   case IMGFMT_YVYU :return 2;
   case IMGFMT_VYUY :return 2;

   case IMGFMT_RGB32 :return 4;

   case IMGFMT_BGR15:return 2;
   case IMGFMT_BGR16:return 2;
   case IMGFMT_BGR24:return 3;
   case IMGFMT_BGR32:return 4;

   case IMGFMT_RGB15:return 2;
   case IMGFMT_RGB16:return 2;
   case IMGFMT_RGB24:return 3;

   case IMGFMT_Y800 :return 1;
   case IMGFMT_NV12 :return 1;
   case IMGFMT_NV21 :return 1;
   default          :return 1;
  }
}

// libmplayer refers to the write order, FF_CSP_ enum refers to the "memory byte order", 
// which under x86 is reversed, see the comment above the FF_CSP_ enum definition.
static __inline int csp_ffdshow2mplayer(int csp)
{
 switch (csp&FF_CSPS_MASK)
  {
   case FF_CSP_420P :return IMGFMT_YV12;
   case FF_CSP_422P :return IMGFMT_422P;
   case FF_CSP_444P :return IMGFMT_444P;
   case FF_CSP_411P :return IMGFMT_411P;
   case FF_CSP_410P :return IMGFMT_YVU9;
/*
   case FF_CSP_YUY2 :return IMGFMT_YVYU;
   case FF_CSP_UYVY :return IMGFMT_VYUY;
   case FF_CSP_YVYU :return IMGFMT_YUY2;
   case FF_CSP_VYUY :return IMGFMT_UYVY;
*/
   case FF_CSP_YUY2 :return IMGFMT_YUY2;
   case FF_CSP_UYVY :return IMGFMT_UYVY;
   case FF_CSP_YVYU :return IMGFMT_YVYU;
   case FF_CSP_VYUY :return IMGFMT_VYUY;

   case FF_CSP_ABGR :return IMGFMT_RGB32;
   case FF_CSP_RGBA :return IMGFMT_RGB32;

   case FF_CSP_RGB15:return IMGFMT_BGR15; // see the comment above
   case FF_CSP_RGB16:return IMGFMT_BGR16; // see the comment above
   case FF_CSP_RGB24:return IMGFMT_BGR24; // see the comment above
   case FF_CSP_RGB32:return IMGFMT_BGR32; // see the comment above

   case FF_CSP_BGR15:return IMGFMT_RGB15; // see the comment above
   case FF_CSP_BGR16:return IMGFMT_RGB16; // see the comment above
   case FF_CSP_BGR24:return IMGFMT_RGB24; // see the comment above
   case FF_CSP_BGR32:return IMGFMT_RGB32; // see the comment above

   case FF_CSP_Y800 :return IMGFMT_Y800;
   case FF_CSP_NV12 :return csp&FF_CSP_FLAGS_YUV_ORDER?IMGFMT_NV12:IMGFMT_NV21;
   default          :return 0;
  }
}

#define SWS_IN_CSPS \
 (                  \
  FF_CSP_420P|      \
  FF_CSP_444P|      \
  FF_CSP_422P|      \
  FF_CSP_411P|      \
  FF_CSP_410P|      \
  FF_CSP_YUY2|      \
  FF_CSP_UYVY|      \
  FF_CSP_YVYU|      \
  FF_CSP_VYUY|      \
  FF_CSP_BGR32|     \
  FF_CSP_BGR24|     \
  FF_CSP_BGR16|     \
  FF_CSP_BGR15|     \
  FF_CSP_RGB32|     \
  FF_CSP_RGB24|     \
  FF_CSP_RGB16|     \
  FF_CSP_RGB15|     \
  FF_CSP_NV12|      \
  FF_CSP_Y800       \
 )
#define SWS_OUT_CSPS \
 (                   \
  FF_CSP_420P|       \
  FF_CSP_444P|       \
  FF_CSP_422P|       \
  FF_CSP_411P|       \
  FF_CSP_410P|       \
  FF_CSP_YUY2|       \
  FF_CSP_UYVY|       \
  FF_CSP_YVYU|       \
  FF_CSP_VYUY|       \
  FF_CSP_RGB32|      \
  FF_CSP_RGB24|      \
  FF_CSP_RGB16|      \
  FF_CSP_RGB15|      \
  FF_CSP_BGR32|      \
  FF_CSP_BGR24|      \
  FF_CSP_BGR16|      \
  FF_CSP_BGR15|      \
  FF_CSP_NV12|       \
  FF_CSP_Y800        \
 )

static __inline int csp_supSWSin(int x)
{
 return (x&FF_CSPS_MASK)&SWS_IN_CSPS;
}
static __inline int csp_supSWSout(int x)
{
 return (x&FF_CSPS_MASK)&SWS_OUT_CSPS;
}

#endif


