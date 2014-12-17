/* stbi-1.03 - public domain JPEG/PNG reader - http://nothings.org/stb_image.c
                      when you control the images you're loading

   QUICK NOTES:
      Primarily of interest to game developers and other people who can
          avoid problematic images and only need the trivial interface

      JPEG baseline (no JPEG progressive, no oddball channel decimations)
      PNG non-interlaced
      BMP non-1bpp, non-RLE
      TGA (not sure what subset, if a subset)
      HDR (radiance rgbE format)
      writes BMP,TGA (define STBI_NO_WRITE to remove code)
      decoded from memory or through stdio FILE (define STBI_NO_STDIO to remove code)

   TODO:
      stbi_info_*
      PSD loader

   history:
      1.03   bugfixes to STBI_NO_STDIO, STBI_NO_HDR
      1.02   support for (subset of) HDR files, float interface for preferred access to them
      1.01   fix bug: possible bug in handling right-side up bmps... not sure
             fix bug: the stbi_bmp_load() and stbi_tga_load() functions didn't work at all
      1.00   interface to zlib that skips zlib header
      0.99   correct handling of alpha in palette
      0.98   TGA loader by lonesock; dynamically add loaders (untested)
      0.97   jpeg errors on too large a file; also catch another malloc failure
      0.96   fix detection of invalid v value - particleman@mollyrocket forum
      0.95   during header scan, seek to markers in case of padding
      0.94   STBI_NO_STDIO to disable stdio usage; rename all #defines the same
      0.93   handle jpegtran output; verbose errors
      0.92   read 4,8,16,24,32-bit BMP files of several formats
      0.91   output 24-bit Windows 3.0 BMP files
      0.90   fix a few more warnings; bump version number to approach 1.0
      0.61   bugfixes due to Marc LeBlanc, Christopher Lloyd
      0.60   fix compiling as c++
      0.59   fix warnings: merge Dave Moore's -Wall fixes
      0.58   fix bug: zlib uncompressed mode len/nlen was wrong endian
      0.57   fix bug: jpg last huffman symbol before marker was >9 bits but less
                      than 16 available
      0.56   fix bug: zlib uncompressed mode len vs. nlen
      0.55   fix bug: restart_interval not initialized to 0
      0.54   allow NULL for 'int *comp'
      0.53   fix bug in png 3->4; speedup png decoding
      0.52   png handles req_comp=3,4 directly; minor cleanup; jpeg comments
      0.51   obey req_comp requests, 1-component jpegs return as 1-component,
             on 'test' only check type, not whether we support this variant
*/

#include "stb_image_aug.h"

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <stdarg.h>

#ifndef _MSC_VER
#define __forceinline
#endif

// implementation:
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef   signed short  int16;
typedef unsigned int   uint32;
typedef   signed int    int32;
typedef unsigned int   uint;

// should produce compiler error if size is wrong
typedef unsigned char validate_uint32[sizeof(uint32)==4];

#if defined(STBI_NO_STDIO) && !defined(STBI_NO_WRITE)
#define STBI_NO_WRITE
#endif

#ifndef STBI_NO_DDS
#include "stbi_DDS_aug.h"
#endif

//	I (JLD) want full messages for SOIL
#define STBI_FAILURE_USERMSG 1

//////////////////////////////////////////////////////////////////////////////
//
// Generic API that works on all image types
//

static char *failure_reason;

char *stbi_failure_reason(void)
{
   return failure_reason;
}

static int e(char *str)
{
   failure_reason = str;
   return 0;
}

#ifdef STBI_NO_FAILURE_STRINGS
   #define e(x,y)  0
#elif defined(STBI_FAILURE_USERMSG)
   #define e(x,y)  e(y)
#else
   #define e(x,y)  e(x)
#endif

#define ep(x,y)   (e(x,y)?NULL:NULL)

void stbi_image_free(unsigned char *retval_from_stbi_load)
{
   free(retval_from_stbi_load);
}

#define MAX_LOADERS  32
stbi_loader *loaders[MAX_LOADERS];
static int max_loaders = 0;

int stbi_register_loader(stbi_loader *loader)
{
   int i;
   for (i=0; i < MAX_LOADERS; ++i) {
      // already present?
      if (loaders[i] == loader)
         return 1;
      // end of the list?
      if (loaders[i] == NULL) {
         loaders[i] = loader;
         max_loaders = i+1;
         return 1;
      }
   }
   // no room for it
   return 0;
}

#ifndef STBI_NO_HDR
static float   *ldr_to_hdr(stbi_uc *data, int x, int y, int comp);
static stbi_uc *hdr_to_ldr(float   *data, int x, int y, int comp);
#endif

#ifndef STBI_NO_STDIO
unsigned char *stbi_load(char *filename, int *x, int *y, int *comp, int req_comp)
{
   FILE *f = fopen(filename, "rb");
   unsigned char *result;
   if (!f) return ep("can't fopen", "Unable to open file");
   result = stbi_load_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return result;
}

unsigned char *stbi_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   int i;
   if (stbi_jpeg_test_file(f))
      return stbi_jpeg_load_from_file(f,x,y,comp,req_comp);
   if (stbi_png_test_file(f))
      return stbi_png_load_from_file(f,x,y,comp,req_comp);
   if (stbi_bmp_test_file(f))
      return stbi_bmp_load_from_file(f,x,y,comp,req_comp);
   #ifndef STBI_NO_DDS
   if (stbi_dds_test_file(f))
      return stbi_dds_load_from_file(f,x,y,comp,req_comp);
   #endif
   #ifndef STBI_NO_HDR
   if (stbi_hdr_test_file(f)) {
      float *hdr = stbi_hdr_load_from_file(f, x,y,comp,req_comp);
      return hdr_to_ldr(hdr, *x, *y, req_comp ? req_comp : *comp);
   }
   #endif
   for (i=0; i < max_loaders; ++i)
      if (loaders[i]->test_file(f))
         return loaders[i]->load_from_file(f,x,y,comp,req_comp);
   // test tga last because it's a crappy test!
   if (stbi_tga_test_file(f))
      return stbi_tga_load_from_file(f,x,y,comp,req_comp);
   return ep("unknown image type", "Image not of any known type, or corrupt");
}
#endif

unsigned char *stbi_load_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   int i;
   if (stbi_jpeg_test_memory(buffer,len))
      return stbi_jpeg_load_from_memory(buffer,len,x,y,comp,req_comp);
   if (stbi_png_test_memory(buffer,len))
      return stbi_png_load_from_memory(buffer,len,x,y,comp,req_comp);
   if (stbi_bmp_test_memory(buffer,len))
      return stbi_bmp_load_from_memory(buffer,len,x,y,comp,req_comp);
   #ifndef STBI_NO_DDS
   if (stbi_dds_test_memory(buffer,len))
      return stbi_dds_load_from_memory(buffer,len,x,y,comp,req_comp);
   #endif
   #ifndef STBI_NO_HDR
   if (stbi_hdr_test_memory(buffer, len)) {
      float *hdr = stbi_hdr_load_from_memory(buffer, len,x,y,comp,req_comp);
      return hdr_to_ldr(hdr, *x, *y, req_comp ? req_comp : *comp);
   }
   #endif
   for (i=0; i < max_loaders; ++i)
      if (loaders[i]->test_memory(buffer,len))
         return loaders[i]->load_from_memory(buffer,len,x,y,comp,req_comp);
   // test tga last because it's a crappy test!
   if (stbi_tga_test_memory(buffer,len))
      return stbi_tga_load_from_memory(buffer,len,x,y,comp,req_comp);
   return ep("unknown image type", "Image not of any known type, or corrupt");
}

#ifndef STBI_NO_HDR

#ifndef STBI_NO_STDIO
float *stbi_loadf(char *filename, int *x, int *y, int *comp, int req_comp)
{
   FILE *f = fopen(filename, "rb");
   float *result;
   if (!f) return ep("can't fopen", "Unable to open file");
   result = stbi_loadf_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return result;
}

float *stbi_loadf_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   unsigned char *data;
   #ifndef STBI_NO_HDR
   if (stbi_hdr_test_file(f))
      return stbi_hdr_load_from_file(f,x,y,comp,req_comp);
   #endif
   data = stbi_load_from_file(f, x, y, comp, req_comp);
   if (data)
      return ldr_to_hdr(data, *x, *y, req_comp ? req_comp : *comp);
   return ep("unknown image type", "Image not of any known type, or corrupt");
}
#endif

float *stbi_loadf_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   stbi_uc *data;
   #ifndef STBI_NO_HDR
   if (stbi_hdr_test_memory(buffer, len))
      return stbi_hdr_load_from_memory(buffer, len,x,y,comp,req_comp);
   #endif
   data = stbi_load_from_memory(buffer, len, x, y, comp, req_comp);
   if (data)
      return ldr_to_hdr(data, *x, *y, req_comp ? req_comp : *comp);
   return ep("unknown image type", "Image not of any known type, or corrupt");
}
#endif

// these is-hdr-or-not is defined independent of whether STBI_NO_HDR is
// defined, for API simplicity; if STBI_NO_HDR is defined, it always
// reports false!

extern int      stbi_is_hdr_from_memory(stbi_uc *buffer, int len)
{
   #ifndef STBI_NO_HDR
   return stbi_hdr_test_memory(buffer, len);
   #else
   return 0;
   #endif
}

#ifndef STBI_NO_STDIO
extern int      stbi_is_hdr          (char *filename)
{
   FILE *f = fopen(filename, "rb");
   int result=0;
   if (f) {
      result = stbi_is_hdr_from_file(f);
      fclose(f);
   }
   return result;
}

extern int      stbi_is_hdr_from_file(FILE *f)
{
   #ifndef STBI_NO_HDR
   return stbi_hdr_test_file(f);
   #else
   return 0;
   #endif
}

#endif

// @TODO: get image dimensions & components without fully decoding
#ifndef STBI_NO_STDIO
extern int      stbi_info            (char *filename,           int *x, int *y, int *comp);
extern int      stbi_info_from_file  (FILE *f,                  int *x, int *y, int *comp);
#endif
extern int      stbi_info_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp);

#ifndef STBI_NO_HDR
static float h2l_gamma_i=1.0f/2.2f, h2l_scale_i=1.0f;
static float l2h_gamma=2.2f, l2h_scale=1.0f;

void   stbi_hdr_to_ldr_gamma(float gamma) { h2l_gamma_i = 1/gamma; }
void   stbi_hdr_to_ldr_scale(float scale) { h2l_scale_i = 1/scale; }

void   stbi_ldr_to_hdr_gamma(float gamma) { l2h_gamma = gamma; }
void   stbi_ldr_to_hdr_scale(float scale) { l2h_scale = scale; }
#endif


//////////////////////////////////////////////////////////////////////////////
//
// Common code used by all image loaders
//

// image width, height, # components
static uint32 img_x, img_y;
static int img_n, img_out_n;

enum
{
   SCAN_load=0,
   SCAN_type,
   SCAN_header,
};

// An API for reading either from memory or file.
#ifndef STBI_NO_STDIO
static FILE  *img_file;
#endif
static uint8 *img_buffer, *img_buffer_end;

#ifndef STBI_NO_STDIO
static void start_file(FILE *f)
{
   img_file = f;
}
#endif

static void start_mem(uint8 *buffer, int len)
{
#ifndef STBI_NO_STDIO
   img_file = NULL;
#endif
   img_buffer = buffer;
   img_buffer_end = buffer+len;
}

static int get8(void)
{
#ifndef STBI_NO_STDIO
   if (img_file) {
      int c = fgetc(img_file);
      return c == EOF ? 0 : c;
   }
#endif
   if (img_buffer < img_buffer_end)
      return *img_buffer++;
   return 0;
}

static int at_eof(void)
{
#ifndef STBI_NO_STDIO
   if (img_file)
      return feof(img_file);
#endif
   return img_buffer >= img_buffer_end;
}

static uint8 get8u(void)
{
   return (uint8) get8();
}

static void skip(int n)
{
#ifndef STBI_NO_STDIO
   if (img_file)
      fseek(img_file, n, SEEK_CUR);
   else
#endif
      img_buffer += n;
}

static int get16(void)
{
   int z = get8();
   return (z << 8) + get8();
}

static uint32 get32(void)
{
   uint32 z = get16();
   return (z << 16) + get16();
}

static int get16le(void)
{
   int z = get8();
   return z + (get8() << 8);
}

static uint32 get32le(void)
{
   uint32 z = get16le();
   return z + (get16le() << 16);
}

static void getn(stbi_uc *buffer, int n)
{
#ifndef STBI_NO_STDIO
   if (img_file) {
      fread(buffer, 1, n, img_file);
      return;
   }
#endif
   memcpy(buffer, img_buffer, n);
   img_buffer += n;
}

//////////////////////////////////////////////////////////////////////////////
//
//  generic converter from built-in img_n to req_comp
//    individual types do this automatically as much as possible (e.g. jpeg
//    does all cases internally since it needs to colorspace convert anyway,
//    and it never has alpha, so very few cases ). png can automatically
//    interleave an alpha=255 channel, but falls back to this for other cases
//
//  assume data buffer is malloced, so malloc a new one and free that one
//  only failure mode is malloc failing

static uint8 compute_y(int r, int g, int b)
{
   return (uint8) (((r*77) + (g*150) +  (29*b)) >> 8);
}

static unsigned char *convert_format(unsigned char *data, int img_n, int req_comp)
{
   uint i,j;
   unsigned char *good;

   if (req_comp == img_n) return data;
   assert(req_comp >= 1 && req_comp <= 4);

   good = (unsigned char *) malloc(req_comp * img_x * img_y);
   if (good == NULL) {
      free(data);
      return ep("outofmem", "Out of memory");
   }

   for (j=0; j < img_y; ++j) {
      unsigned char *src  = data + j * img_x * img_n   ;
      unsigned char *dest = good + j * img_x * req_comp;

      #define COMBO(a,b)  ((a)*8+(b))
      #define CASE(a,b)   case COMBO(a,b): for(i=0; i < img_x; ++i, src += a, dest += b)

      // convert source image with img_n components to one with req_comp components;
      // avoid switch per pixel, so use switch per scanline and massive macros
      switch(COMBO(img_n, req_comp)) {
         CASE(1,2) dest[0]=src[0], dest[1]=255; break;
         CASE(1,3) dest[0]=dest[1]=dest[2]=src[0]; break;
         CASE(1,4) dest[0]=dest[1]=dest[2]=src[0], dest[3]=255; break;
         CASE(2,1) dest[0]=src[0]; break;
         CASE(2,3) dest[0]=dest[1]=dest[2]=src[0]; break;
         CASE(2,4) dest[0]=dest[1]=dest[2]=src[0], dest[3]=src[1]; break;
         CASE(3,4) dest[0]=src[0],dest[1]=src[1],dest[2]=src[2],dest[3]=255; break;
         CASE(3,1) dest[0]=compute_y(src[0],src[1],src[2]); break;
         CASE(3,2) dest[0]=compute_y(src[0],src[1],src[2]), dest[1] = 255; break;
         CASE(4,1) dest[0]=compute_y(src[0],src[1],src[2]); break;
         CASE(4,2) dest[0]=compute_y(src[0],src[1],src[2]), dest[1] = src[3]; break;
         CASE(4,3) dest[0]=src[0],dest[1]=src[1],dest[2]=src[2]; break;
         default: assert(0);
      }
      #undef CASE
   }

   free(data);
   img_out_n = req_comp;
   return good;
}

#ifndef STBI_NO_HDR
static float   *ldr_to_hdr(stbi_uc *data, int x, int y, int comp)
{
   int i,k,n;
   float *output = (float *) malloc(x * y * comp * sizeof(float));
   if (output == NULL) { free(data); return ep("outofmem", "Out of memory"); }
   // compute number of non-alpha components
   if (comp & 1) n = comp; else n = comp-1;
   for (i=0; i < x*y; ++i) {
      for (k=0; k < n; ++k) {
         output[i*comp + k] = (float) pow(data[i*comp+k]/255.0, l2h_gamma) * l2h_scale;
      }
      if (k < comp) output[i*comp + k] = data[i*comp+k]/255.0f;
   }
   free(data);
   return output;
}

#define float2int(x)   ((int) (x))
static stbi_uc *hdr_to_ldr(float   *data, int x, int y, int comp)
{
   int i,k,n;
   stbi_uc *output = (stbi_uc *) malloc(x * y * comp);
   if (output == NULL) { free(data); return ep("outofmem", "Out of memory"); }
   // compute number of non-alpha components
   if (comp & 1) n = comp; else n = comp-1;
   for (i=0; i < x*y; ++i) {
      for (k=0; k < n; ++k) {
         float z = (float) pow(data[i*comp+k]*h2l_scale_i, h2l_gamma_i) * 255 + 0.5f;
         if (z < 0) z = 0;
         if (z > 255) z = 255;
         output[i*comp + k] = float2int(z);
      }
      if (k < comp) {
         float z = data[i*comp+k] * 255 + 0.5f;
         if (z < 0) z = 0;
         if (z > 255) z = 255;
         output[i*comp + k] = float2int(z);
      }
   }
   free(data);
   return output;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  "baseline" JPEG/JFIF decoder (not actually fully baseline implementation)
//
//    simple implementation
//      - channel subsampling of at most 2 in each dimension
//      - doesn't support delayed output of y-dimension
//      - simple interface (only one output format: 8-bit interleaved RGB)
//      - doesn't try to recover corrupt jpegs
//      - doesn't allow partial loading, loading multiple at once
//      - still fast on x86 (copying globals into locals doesn't help x86)
//      - allocates lots of intermediate memory (full size of all components)
//        - non-interleaved case requires this anyway
//        - allows good upsampling (see next)
//    high-quality
//      - upsampled channels are bilinearly interpolated, even across blocks
//      - quality integer IDCT derived from IJG's 'slow'
//    performance
//      - fast huffman; reasonable integer IDCT
//      - uses a lot of intermediate memory, could cache poorly
//      - load http://nothings.org/remote/anemones.jpg 3 times on 2.8Ghz P4
//          stb_jpeg:   1.34 seconds (MSVC6, default release build)
//          stb_jpeg:   1.06 seconds (MSVC6, processor = Pentium Pro)
//          IJL11.dll:  1.08 seconds (compiled by intel)
//          IJG 1998:   0.98 seconds (MSVC6, makefile provided by IJG)
//          IJG 1998:   0.95 seconds (MSVC6, makefile + proc=PPro)

int stbi_jpeg_dc_only;

// huffman decoding acceleration
#define FAST_BITS   9  // larger handles more cases; smaller stomps less cache

typedef struct
{
   uint8  fast[1 << FAST_BITS];
   // weirdly, repacking this into AoS is a 10% speed loss, instead of a win
   uint16 code[256];
   uint8  values[256];
   uint8  size[257];
   unsigned int maxcode[18];
   int    delta[17];   // old 'firstsymbol' - old 'firstcode'
} huffman;

static huffman huff_dc[4];  // baseline is 2 tables, extended is 4
static huffman huff_ac[4];
static uint8 dequant[4][64];

static int build_huffman(huffman *h, int *count)
{
   int i,j,k=0,code;
   // build size list for each symbol (from JPEG spec)
   for (i=0; i < 16; ++i)
      for (j=0; j < count[i]; ++j)
         h->size[k++] = (uint8) (i+1);
   h->size[k] = 0;

   // compute actual symbols (from jpeg spec)
   code = 0;
   k = 0;
   for(j=1; j <= 16; ++j) {
      // compute delta to add to code to compute symbol id
      h->delta[j] = k - code;
      if (h->size[k] == j) {
         while (h->size[k] == j)
            h->code[k++] = (uint16) (code++);
         if (code-1 >= (1 << j)) return e("bad code lengths","Corrupt JPEG");
      }
      // compute largest code + 1 for this size, preshifted as needed later
      h->maxcode[j] = code << (16-j);
      code <<= 1;
   }
   h->maxcode[j] = 0xffffffff;

   // build non-spec acceleration table; 255 is flag for not-accelerated
   memset(h->fast, 255, 1 << FAST_BITS);
   for (i=0; i < k; ++i) {
      int s = h->size[i];
      if (s <= FAST_BITS) {
         int c = h->code[i] << (FAST_BITS-s);
         int m = 1 << (FAST_BITS-s);
         for (j=0; j < m; ++j) {
            h->fast[c+j] = (uint8) i;
         }
      }
   }
   return 1;
}

// sizes for components, interleaved MCUs
static int img_h_max, img_v_max;
static int img_mcu_x, img_mcu_y;
static int img_mcu_w, img_mcu_h;

// definition of jpeg image component
static struct
{
   int id;
   int h,v;
   int tq;
   int hd,ha;
   int dc_pred;

   int x,y,w2,h2;
   uint8 *data;
} img_comp[4];

static unsigned long  code_buffer; // jpeg entropy-coded buffer
static int            code_bits;   // number of valid bits
static unsigned char  marker;      // marker seen while filling entropy buffer
static int            nomore;      // flag if we saw a marker so must stop

static void grow_buffer_unsafe(void)
{
   do {
      int b = nomore ? 0 : get8();
      if (b == 0xff) {
         int c = get8();
         if (c != 0) {
            marker = (unsigned char) c;
            nomore = 1;
            return;
         }
      }
      code_buffer = (code_buffer << 8) | b;
      code_bits += 8;
   } while (code_bits <= 24);
}

// (1 << n) - 1
static unsigned long bmask[17]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535};

// decode a jpeg huffman value from the bitstream
__forceinline static int decode(huffman *h)
{
   unsigned int temp;
   int c,k;

   if (code_bits < 16) grow_buffer_unsafe();

   // look at the top FAST_BITS and determine what symbol ID it is,
   // if the code is <= FAST_BITS
   c = (code_buffer >> (code_bits - FAST_BITS)) & ((1 << FAST_BITS)-1);
   k = h->fast[c];
   if (k < 255) {
      if (h->size[k] > code_bits)
         return -1;
      code_bits -= h->size[k];
      return h->values[k];
   }

   // naive test is to shift the code_buffer down so k bits are
   // valid, then test against maxcode. To speed this up, we've
   // preshifted maxcode left so that it has (16-k) 0s at the
   // end; in other words, regardless of the number of bits, it
   // wants to be compared against something shifted to have 16;
   // that way we don't need to shift inside the loop.
   if (code_bits < 16)
      temp = (code_buffer << (16 - code_bits)) & 0xffff;
   else
      temp = (code_buffer >> (code_bits - 16)) & 0xffff;
   for (k=FAST_BITS+1 ; ; ++k)
      if (temp < h->maxcode[k])
         break;
   if (k == 17) {
      // error! code not found
      code_bits -= 16;
      return -1;
   }

   if (k > code_bits)
      return -1;

   // convert the huffman code to the symbol id
   c = ((code_buffer >> (code_bits - k)) & bmask[k]) + h->delta[k];
   assert((((code_buffer) >> (code_bits - h->size[c])) & bmask[h->size[c]]) == h->code[c]);

   // convert the id to a symbol
   code_bits -= k;
   return h->values[c];
}

// combined JPEG 'receive' and JPEG 'extend', since baseline
// always extends everything it receives.
__forceinline static int extend_receive(int n)
{
   unsigned int m = 1 << (n-1);
   unsigned int k;
   if (code_bits < n) grow_buffer_unsafe();
   k = (code_buffer >> (code_bits - n)) & bmask[n];
   code_bits -= n;
   // the following test is probably a random branch that won't
   // predict well. I tried to table accelerate it but failed.
   // maybe it's compiling as a conditional move?
   if (k < m)
      return (-1 << n) + k + 1;
   else
      return k;
}

// given a value that's at position X in the zigzag stream,
// where does it appear in the 8x8 matrix coded as row-major?
static uint8 dezigzag[64+15] =
{
    0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63,
   // let corrupt input sample past end
   63, 63, 63, 63, 63, 63, 63, 63,
   63, 63, 63, 63, 63, 63, 63
};

// decode one 64-entry block--
static int decode_block(short data[64], huffman *hdc, huffman *hac, int b)
{
   int diff,dc,k;
   int t = decode(hdc);
   if (t < 0) return e("bad huffman code","Corrupt JPEG");

   // 0 all the ac values now so we can do it 32-bits at a time
   memset(data,0,64*sizeof(data[0]));

   diff = t ? extend_receive(t) : 0;
   dc = img_comp[b].dc_pred + diff;
   img_comp[b].dc_pred = dc;
   data[0] = (short) dc;

   // decode AC components, see JPEG spec
   k = 1;
   do {
      int r,s;
      int rs = decode(hac);
      if (rs < 0) return e("bad huffman code","Corrupt JPEG");
      s = rs & 15;
      r = rs >> 4;
      if (s == 0) {
         if (rs != 0xf0) break; // end block
         k += 16;
      } else {
         k += r;
         // decode into unzigzag'd location
         data[dezigzag[k++]] = (short) extend_receive(s);
      }
   } while (k < 64);
   return 1;
}

// take a -128..127 value and clamp it and convert to 0..255
__forceinline static uint8 clamp(int x)
{
   x += 128;
   // trick to use a single test to catch both cases
   if ((unsigned int) x > 255) {
      if (x < 0) return 0;
      if (x > 255) return 255;
   }
   return (uint8) x;
}

#define f2f(x)  (int) (((x) * 4096 + 0.5))
#define fsh(x)  ((x) << 12)

// derived from jidctint -- DCT_ISLOW
#define IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7)       \
   int t0,t1,t2,t3,p1,p2,p3,p4,p5,x0,x1,x2,x3; \
   p2 = s2;                                    \
   p3 = s6;                                    \
   p1 = (p2+p3) * f2f(0.5411961f);             \
   t2 = p1 + p3*f2f(-1.847759065f);            \
   t3 = p1 + p2*f2f( 0.765366865f);            \
   p2 = s0;                                    \
   p3 = s4;                                    \
   t0 = fsh(p2+p3);                            \
   t1 = fsh(p2-p3);                            \
   x0 = t0+t3;                                 \
   x3 = t0-t3;                                 \
   x1 = t1+t2;                                 \
   x2 = t1-t2;                                 \
   t0 = s7;                                    \
   t1 = s5;                                    \
   t2 = s3;                                    \
   t3 = s1;                                    \
   p3 = t0+t2;                                 \
   p4 = t1+t3;                                 \
   p1 = t0+t3;                                 \
   p2 = t1+t2;                                 \
   p5 = (p3+p4)*f2f( 1.175875602f);            \
   t0 = t0*f2f( 0.298631336f);                 \
   t1 = t1*f2f( 2.053119869f);                 \
   t2 = t2*f2f( 3.072711026f);                 \
   t3 = t3*f2f( 1.501321110f);                 \
   p1 = p5 + p1*f2f(-0.899976223f);            \
   p2 = p5 + p2*f2f(-2.562915447f);            \
   p3 = p3*f2f(-1.961570560f);                 \
   p4 = p4*f2f(-0.390180644f);                 \
   t3 += p1+p4;                                \
   t2 += p2+p3;                                \
   t1 += p2+p4;                                \
   t0 += p1+p3;

// .344 seconds on 3*anemones.jpg
static void idct_block(uint8 *out, int out_stride, short data[64], uint8 *dequantize)
{
   int i,val[64],*v=val;
   uint8 *o,*dq = dequantize;
   short *d = data;

   if (stbi_jpeg_dc_only) {
      // ok, I don't really know why this is right, but it seems to be:
      int z = 128 + ((d[0] * dq[0]) >> 3);
      for (i=0; i < 8; ++i) {
         out[0] = out[1] = out[2] = out[3] = out[4] = out[5] = out[6] = out[7] = z;
         out += out_stride;
      }
      return;
   }

   // columns
   for (i=0; i < 8; ++i,++d,++dq, ++v) {
      // if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing
      if (d[ 8]==0 && d[16]==0 && d[24]==0 && d[32]==0
           && d[40]==0 && d[48]==0 && d[56]==0) {
         //    no shortcut                 0     seconds
         //    (1|2|3|4|5|6|7)==0          0     seconds
         //    all separate               -0.047 seconds
         //    1 && 2|3 && 4|5 && 6|7:    -0.047 seconds
         int dcterm = d[0] * dq[0] << 2;
         v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = dcterm;
      } else {
         IDCT_1D(d[ 0]*dq[ 0],d[ 8]*dq[ 8],d[16]*dq[16],d[24]*dq[24],
                 d[32]*dq[32],d[40]*dq[40],d[48]*dq[48],d[56]*dq[56])
         // constants scaled things up by 1<<12; let's bring them back
         // down, but keep 2 extra bits of precision
         x0 += 512; x1 += 512; x2 += 512; x3 += 512;
         v[ 0] = (x0+t3) >> 10;
         v[56] = (x0-t3) >> 10;
         v[ 8] = (x1+t2) >> 10;
         v[48] = (x1-t2) >> 10;
         v[16] = (x2+t1) >> 10;
         v[40] = (x2-t1) >> 10;
         v[24] = (x3+t0) >> 10;
         v[32] = (x3-t0) >> 10;
      }
   }

   for (i=0, v=val, o=out; i < 8; ++i,v+=8,o+=out_stride) {
      // no fast case since the first 1D IDCT spread components out
      IDCT_1D(v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7])
      // constants scaled things up by 1<<12, plus we had 1<<2 from first
      // loop, plus horizontal and vertical each scale by sqrt(8) so together
      // we've got an extra 1<<3, so 1<<17 total we need to remove.
      x0 += 65536; x1 += 65536; x2 += 65536; x3 += 65536;
      o[0] = clamp((x0+t3) >> 17);
      o[7] = clamp((x0-t3) >> 17);
      o[1] = clamp((x1+t2) >> 17);
      o[6] = clamp((x1-t2) >> 17);
      o[2] = clamp((x2+t1) >> 17);
      o[5] = clamp((x2-t1) >> 17);
      o[3] = clamp((x3+t0) >> 17);
      o[4] = clamp((x3-t0) >> 17);
   }
}

#define MARKER_none  0xff
// if there's a pending marker from the entropy stream, return that
// otherwise, fetch from the stream and get a marker. if there's no
// marker, return 0xff, which is never a valid marker value
static uint8 get_marker(void)
{
   uint8 x;
   if (marker != MARKER_none) { x = marker; marker = MARKER_none; return x; }
   x = get8u();
   if (x != 0xff) return MARKER_none;
   while (x == 0xff)
      x = get8u();
   return x;
}

// in each scan, we'll have scan_n components, and the order
// of the components is specified by order[]
static int scan_n, order[4];
static int restart_interval, todo;
#define RESTART(x)     ((x) >= 0xd0 && (x) <= 0xd7)

// after a restart interval, reset the entropy decoder and
// the dc prediction
static void reset(void)
{
   code_bits = 0;
   code_buffer = 0;
   nomore = 0;
   img_comp[0].dc_pred = img_comp[1].dc_pred = img_comp[2].dc_pred = 0;
   marker = MARKER_none;
   todo = restart_interval ? restart_interval : 0x7fffffff;
   // no more than 1<<31 MCUs if no restart_interal? that's plenty safe,
   // since we don't even allow 1<<30 pixels
}

static int parse_entropy_coded_data(void)
{
   reset();
   if (scan_n == 1) {
      int i,j;
      short data[64];
      int n = order[0];
      // non-interleaved data, we just need to process one block at a time,
      // in trivial scanline order
      // number of blocks to do just depends on how many actual "pixels" this
      // component has, independent of interleaved MCU blocking and such
      int w = (img_comp[n].x+7) >> 3;
      int h = (img_comp[n].y+7) >> 3;
      for (j=0; j < h; ++j) {
         for (i=0; i < w; ++i) {
            if (!decode_block(data, huff_dc+img_comp[n].hd, huff_ac+img_comp[n].ha, n)) return 0;
            idct_block(img_comp[n].data+img_comp[n].w2*j*8+i*8, img_comp[n].w2, data, dequant[img_comp[n].tq]);
            // every data block is an MCU, so countdown the restart interval
            if (--todo <= 0) {
               if (code_bits < 24) grow_buffer_unsafe();
               // if it's NOT a restart, then just bail, so we get corrupt data
               // rather than no data
               if (!RESTART(marker)) return 1;
               reset();
            }
         }
      }
   } else { // interleaved!
      int i,j,k,x,y;
      short data[64];
      for (j=0; j < img_mcu_y; ++j) {
         for (i=0; i < img_mcu_x; ++i) {
            // scan an interleaved mcu... process scan_n components in order
            for (k=0; k < scan_n; ++k) {
               int n = order[k];
               // scan out an mcu's worth of this component; that's just determined
               // by the basic H and V specified for the component
               for (y=0; y < img_comp[n].v; ++y) {
                  for (x=0; x < img_comp[n].h; ++x) {
                     int x2 = (i*img_comp[n].h + x)*8;
                     int y2 = (j*img_comp[n].v + y)*8;
                     if (!decode_block(data, huff_dc+img_comp[n].hd, huff_ac+img_comp[n].ha, n)) return 0;
                     idct_block(img_comp[n].data+img_comp[n].w2*y2+x2, img_comp[n].w2, data, dequant[img_comp[n].tq]);
                  }
               }
            }
            // after all interleaved components, that's an interleaved MCU,
            // so now count down the restart interval
            if (--todo <= 0) {
               if (code_bits < 24) grow_buffer_unsafe();
               // if it's NOT a restart, then just bail, so we get corrupt data
               // rather than no data
               if (!RESTART(marker)) return 1;
               reset();
            }
         }
      }
   }
   return 1;
}

static int process_marker(int m)
{
   int L;
   switch (m) {
      case MARKER_none: // no marker found
         return e("expected marker","Corrupt JPEG");

      case 0xC2: // SOF - progressive
         return e("progressive jpeg","JPEG format not supported (progressive)");

      case 0xDD: // DRI - specify restart interval
         if (get16() != 4) return e("bad DRI len","Corrupt JPEG");
         restart_interval = get16();
         return 1;

      case 0xDB: // DQT - define quantization table
         L = get16()-2;
         while (L > 0) {
            int z = get8();
            int p = z >> 4;
            int t = z & 15,i;
            if (p != 0) return e("bad DQT type","Corrupt JPEG");
            if (t > 3) return e("bad DQT table","Corrupt JPEG");
            for (i=0; i < 64; ++i)
               dequant[t][dezigzag[i]] = get8u();
            L -= 65;
         }
         return L==0;

      case 0xC4: // DHT - define huffman table
         L = get16()-2;
         while (L > 0) {
            uint8 *v;
            int sizes[16],i,m=0;
            int z = get8();
            int tc = z >> 4;
            int th = z & 15;
            if (tc > 1 || th > 3) return e("bad DHT header","Corrupt JPEG");
            for (i=0; i < 16; ++i) {
               sizes[i] = get8();
               m += sizes[i];
            }
            L -= 17;
            if (tc == 0) {
               if (!build_huffman(huff_dc+th, sizes)) return 0;
               v = huff_dc[th].values;
            } else {
               if (!build_huffman(huff_ac+th, sizes)) return 0;
               v = huff_ac[th].values;
            }
            for (i=0; i < m; ++i)
               v[i] = get8u();
            L -= m;
         }
         return L==0;
   }
   // check for comment block or APP blocks
   if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
      skip(get16()-2);
      return 1;
   }
   return 0;
}

// after we see SOS
static int process_scan_header(void)
{
   int i;
   int Ls = get16();
   scan_n = get8();
   if (scan_n < 1 || scan_n > 4 || scan_n > (int) img_n) return e("bad SOS component count","Corrupt JPEG");
   if (Ls != 6+2*scan_n) return e("bad SOS len","Corrupt JPEG");
   for (i=0; i < scan_n; ++i) {
      int id = get8(), which;
      int z = get8();
      for (which = 0; which < img_n; ++which)
         if (img_comp[which].id == id)
            break;
      if (which == img_n) return 0;
      img_comp[which].hd = z >> 4;   if (img_comp[which].hd > 3) return e("bad DC huff","Corrupt JPEG");
      img_comp[which].ha = z & 15;   if (img_comp[which].ha > 3) return e("bad AC huff","Corrupt JPEG");
      order[i] = which;
   }
   if (get8() != 0) return e("bad SOS","Corrupt JPEG");
   get8(); // should be 63, but might be 0
   if (get8() != 0) return e("bad SOS","Corrupt JPEG");

   return 1;
}

static int process_frame_header(int scan)
{
   int Lf,p,i,z, h_max=1,v_max=1;
   Lf = get16();         if (Lf < 11) return e("bad SOF len","Corrupt JPEG"); // JPEG
   p  = get8();          if (p != 8) return e("only 8-bit","JPEG format not supported: 8-bit only"); // JPEG baseline
   img_y = get16();      if (img_y == 0) return e("no header height", "JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
   img_x = get16();      if (img_x == 0) return e("0 width","Corrupt JPEG"); // JPEG requires
   img_n = get8();
   if (img_n != 3 && img_n != 1) return e("bad component count","Corrupt JPEG");    // JFIF requires

   if (Lf != 8+3*img_n) return e("bad SOF len","Corrupt JPEG");

   for (i=0; i < img_n; ++i) {
      img_comp[i].id = get8();
      if (img_comp[i].id != i+1)   // JFIF requires
         if (img_comp[i].id != i)  // jpegtran outputs non-JFIF-compliant files!
            return e("bad component ID","Corrupt JPEG");
      z = get8();
      img_comp[i].h = (z >> 4);  if (!img_comp[i].h || img_comp[i].h > 4) return e("bad H","Corrupt JPEG");
      img_comp[i].v = z & 15;    if (!img_comp[i].v || img_comp[i].v > 4) return e("bad V","Corrupt JPEG");
      img_comp[i].tq = get8();   if (img_comp[i].tq > 3) return e("bad TQ","Corrupt JPEG");
   }

   if (scan != SCAN_load) return 1;

   if ((1 << 30) / img_x / img_n < img_y) return e("too large", "Image too large to decode");

   for (i=0; i < img_n; ++i) {
      if (img_comp[i].h > h_max) h_max = img_comp[i].h;
      if (img_comp[i].v > v_max) v_max = img_comp[i].v;
   }

   // compute interleaved mcu info
   img_h_max = h_max;
   img_v_max = v_max;
   img_mcu_w = h_max * 8;
   img_mcu_h = v_max * 8;
   img_mcu_x = (img_x + img_mcu_w-1) / img_mcu_w;
   img_mcu_y = (img_y + img_mcu_h-1) / img_mcu_h;

   for (i=0; i < img_n; ++i) {
      // number of effective pixels (e.g. for non-interleaved MCU)
      img_comp[i].x = (img_x * img_comp[i].h + h_max-1) / h_max;
      img_comp[i].y = (img_y * img_comp[i].v + v_max-1) / v_max;
      // to simplify generation, we'll allocate enough memory to decode
      // the bogus oversized data from using interleaved MCUs and their
      // big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
      // discard the extra data until colorspace conversion
      img_comp[i].w2 = img_mcu_x * img_comp[i].h * 8;
      img_comp[i].h2 = img_mcu_y * img_comp[i].v * 8;
      img_comp[i].data = (uint8 *) malloc(img_comp[i].w2 * img_comp[i].h2);
      if (img_comp[i].data == NULL) {
         for(--i; i >= 0; --i)
            free(img_comp[i].data);
         return e("outofmem", "Out of memory");
      }
   }

   return 1;
}

// use comparisons since in some cases we handle more than one case (e.g. SOF)
#define DNL(x)         ((x) == 0xdc)
#define SOI(x)         ((x) == 0xd8)
#define EOI(x)         ((x) == 0xd9)
#define SOF(x)         ((x) == 0xc0 || (x) == 0xc1)
#define SOS(x)         ((x) == 0xda)

static int decode_jpeg_header(int scan)
{
   int m;
   marker = MARKER_none; // initialize cached marker to empty
   m = get_marker();
   if (!SOI(m)) return e("no SOI","Corrupt JPEG");
   if (scan == SCAN_type) return 1;
   m = get_marker();
   while (!SOF(m)) {
      if (!process_marker(m)) return 0;
      m = get_marker();
      while (m == MARKER_none) {
         // some files have extra padding after their blocks, so ok, we'll scan
         if (at_eof()) return e("no SOF", "Corrupt JPEG");
         m = get_marker();
      }
   }
   if (!process_frame_header(scan)) return 0;
   return 1;
}

static int decode_jpeg_image(void)
{
   int m;
   restart_interval = 0;
   if (!decode_jpeg_header(SCAN_load)) return 0;
   m = get_marker();
   while (!EOI(m)) {
      if (SOS(m)) {
         if (!process_scan_header()) return 0;
         if (!parse_entropy_coded_data()) return 0;
      } else {
         if (!process_marker(m)) return 0;
      }
      m = get_marker();
   }
   return 1;
}

// static jfif-centered resampling with cross-block smoothing
// here by cross-block smoothing what I mean is that the resampling
// is bilerp and crosses blocks; I dunno what IJG means

#define div4(x) ((uint8) ((x) >> 2))

static void resample_v_2(uint8 *out1, uint8 *input, int w, int h, int s)
{
   // need to generate two samples vertically for every one in input
   uint8 *above;
   uint8 *below;
   uint8 *source;
   uint8 *out2;
   int i,j;
   source = input;
   out2 = out1+w;
   for (j=0; j < h; ++j) {
      above = source;
      source = input + j*s;
      below = source + s; if (j == h-1) below = source;
      for (i=0; i < w; ++i) {
         int n = source[i]*3;
         out1[i] = div4(above[i] + n);
         out2[i] = div4(below[i] + n);
      }
      out1 += w*2;
      out2 += w*2;
   }
}

static void resample_h_2(uint8 *out, uint8 *input, int w, int h, int s)
{
   // need to generate two samples horizontally for every one in input
   int i,j;
   if (w == 1) {
      for (j=0; j < h; ++j)
         out[j*2+0] = out[j*2+1] = input[j*s];
      return;
   }
   for (j=0; j < h; ++j) {
      out[0] = input[0];
      out[1] = div4(input[0]*3 + input[1]);
      for (i=1; i < w-1; ++i) {
         int n = input[i]*3;
         out[i*2-2] = div4(input[i-1] + n);
         out[i*2-1] = div4(input[i+1] + n);
      }
      out[w*2-2] = div4(input[w-2]*3 + input[w-1]);
      out[w*2-1] = input[w-1];
      out += w*2;
      input += s;
   }
}

// .172 seconds on 3*anemones.jpg
static void resample_hv_2(uint8 *out, uint8 *input, int w, int h, int s)
{
   // need to generate 2x2 samples for every one in input
   int i,j;
   int os = w*2;
   // generate edge samples... @TODO lerp them!
   for (i=0; i < w; ++i) {
      out[i*2+0] = out[i*2+1] = input[i];
      out[i*2+(2*h-1)*os+0] = out[i*2+(2*h-1)*os+1] = input[i+(h-1)*w];
   }
   for (j=0; j < h; ++j) {
      out[j*os*2+0] = out[j*os*2+os+0] = input[j*w];
      out[j*os*2+os-1] = out[j*os*2+os+os-1] = input[j*w+i-1];
   }
   // now generate interior samples; i & j point to top left of input
   for (j=0; j < h-1; ++j) {
      uint8 *in1 = input+j*s;
      uint8 *in2 = in1 + s;
      uint8 *out1 = out + (j*2+1)*os + 1;
      uint8 *out2 = out1 + os;
      for (i=0; i < w-1; ++i) {
         int p00 = in1[0], p01=in1[1], p10=in2[0], p11=in2[1];
         int p00_3 = p00*3, p01_3 = p01*3, p10_3 = p10*3, p11_3 = p11*3;

         #define div16(x)  ((uint8) ((x) >> 4))

         out1[0] = div16(p00*9 + p01_3 + p10_3 + p11);
         out1[1] = div16(p01*9 + p00_3 + p01_3 + p10);
         out2[0] = div16(p10*9 + p11_3 + p00_3 + p01);
         out2[1] = div16(p11*9 + p10_3 + p01_3 + p00);
         out1 += 2;
         out2 += 2;
         ++in1;
         ++in2;
      }
   }
}

#define float2fixed(x)  ((int) ((x) * 65536 + 0.5))

// 0.38 seconds on 3*anemones.jpg   (0.25 with processor = Pro)
// VC6 without processor=Pro is generating multiple LEAs per multiply!
static void YCbCr_to_RGB_row(uint8 *out, uint8 *y, uint8 *pcb, uint8 *pcr, int count, int step)
{
   int i;
   for (i=0; i < count; ++i) {
      int y_fixed = (y[i] << 16) + 32768; // rounding
      int r,g,b;
      int cr = pcr[i] - 128;
      int cb = pcb[i] - 128;
      r = y_fixed + cr*float2fixed(1.40200f);
      g = y_fixed - cr*float2fixed(0.71414f) - cb*float2fixed(0.34414f);
      b = y_fixed                            + cb*float2fixed(1.77200f);
      r >>= 16;
      g >>= 16;
      b >>= 16;
      if ((unsigned) r > 255) { if (r < 0) r = 0; else r = 255; }
      if ((unsigned) g > 255) { if (g < 0) g = 0; else g = 255; }
      if ((unsigned) b > 255) { if (b < 0) b = 0; else b = 255; }
      out[0] = (uint8)r;
      out[1] = (uint8)g;
      out[2] = (uint8)b;
      if (step == 4) out[3] = 255;
      out += step;
   }
}

// clean up the temporary component buffers
static void cleanup_jpeg(void)
{
   int i;
   for (i=0; i < img_n; ++i) {
      if (img_comp[i].data) {
         free(img_comp[i].data);
         img_comp[i].data = NULL;
      }
   }
}

static uint8 *load_jpeg_image(int *out_x, int *out_y, int *comp, int req_comp)
{
   int i, n;
   // validate req_comp
   if (req_comp < 0 || req_comp > 4) return ep("bad req_comp", "Internal error");

   // load a jpeg image from whichever source
   if (!decode_jpeg_image()) { cleanup_jpeg(); return NULL; }

   // determine actual number of components to generate
   n = req_comp ? req_comp : img_n;

   // resample components to full size... memory wasteful, but this
   // lets us bilerp across blocks while upsampling
   for (i=0; i < img_n; ++i) {
      // if we're outputting fewer than 3 components, we're grey not RGB;
      // in that case, don't bother upsampling Cb or Cr
      if (n < 3 && i) continue;

      // check if the component scale is less than max; if so it needs upsampling
      if (img_comp[i].h != img_h_max || img_comp[i].v != img_v_max) {
         int stride = img_x;
         // allocate final size; make sure it's big enough for upsampling off
         // the edges with upsample up to 4x4 (although we only support 2x2
         // currently)
         uint8 *new_data = (uint8 *) malloc((img_x+3)*(img_y+3));
         if (new_data == NULL) {
            cleanup_jpeg();
            return ep("outofmem", "Out of memory (image too large?)");
         }
         if (img_comp[i].h*2 == img_h_max && img_comp[i].v*2 == img_v_max) {
            int tx = (img_x+1)>>1;
            resample_hv_2(new_data, img_comp[i].data, tx,(img_y+1)>>1, img_comp[i].w2);
            stride = tx*2;
         } else if (img_comp[i].h == img_h_max && img_comp[i].v*2 == img_v_max) {
            resample_v_2(new_data, img_comp[i].data, img_x,(img_y+1)>>1, img_comp[i].w2);
         } else if (img_comp[i].h*2 == img_h_max && img_comp[i].v == img_v_max) {
            int tx = (img_x+1)>>1;
            resample_h_2(new_data, img_comp[i].data, tx,img_y, img_comp[i].w2);
            stride = tx*2;
         } else {
            // @TODO resample uncommon sampling pattern with nearest neighbor
            free(new_data);
            cleanup_jpeg();
            return ep("uncommon H or V", "JPEG not supported: atypical downsampling mode");
         }
         img_comp[i].w2 = stride;
         free(img_comp[i].data);
         img_comp[i].data = new_data;
      }
   }

   // now convert components to output image
   {
      uint32 i,j;
      uint8 *output = (uint8 *) malloc(n * img_x * img_y + 1);
      if (n >= 3) { // output STBI_rgb_*
         for (j=0; j < img_y; ++j) {
            uint8 *y  = img_comp[0].data + j*img_comp[0].w2;
            uint8 *out = output + n * img_x * j;
            if (img_n == 3) {
               uint8 *cb = img_comp[1].data + j*img_comp[1].w2;
               uint8 *cr = img_comp[2].data + j*img_comp[2].w2;
               YCbCr_to_RGB_row(out, y, cb, cr, img_x, n);
            } else {
               for (i=0; i < img_x; ++i) {
                  out[0] = out[1] = out[2] = y[i];
                  out[3] = 255; // not used if n == 3
                  out += n;
               }
            }
         }
      } else {      // output STBI_grey_*
         for (j=0; j < img_y; ++j) {
            uint8 *y  = img_comp[0].data + j*img_comp[0].w2;
            uint8 *out = output + n * img_x * j;
            if (n == 1)
               for (i=0; i < img_x; ++i) *out++ = *y++;
            else
               for (i=0; i < img_x; ++i) *out++ = *y++, *out++ = 255;
         }
      }
      cleanup_jpeg();
      *out_x = img_x;
      *out_y = img_y;
      if (comp) *comp  = n; // Changed JLD: report output components
      //if (comp) *comp  = img_n; // report original components, not output
      return output;
   }
}

#ifndef STBI_NO_STDIO
unsigned char *stbi_jpeg_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   start_file(f);
   return load_jpeg_image(x,y,comp,req_comp);
}

unsigned char *stbi_jpeg_load(char *filename, int *x, int *y, int *comp, int req_comp)
{
   unsigned char *data;
   FILE *f = fopen(filename, "rb");
   if (!f) return NULL;
   data = stbi_jpeg_load_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return data;
}
#endif

unsigned char *stbi_jpeg_load_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   start_mem(buffer,len);
   return load_jpeg_image(x,y,comp,req_comp);
}

#ifndef STBI_NO_STDIO
int stbi_jpeg_test_file(FILE *f)
{
   int n,r;
   n = ftell(f);
   start_file(f);
   r = decode_jpeg_header(SCAN_type);
   fseek(f,n,SEEK_SET);
   return r;
}
#endif

int stbi_jpeg_test_memory(unsigned char *buffer, int len)
{
   start_mem(buffer,len);
   return decode_jpeg_header(SCAN_type);
}

// @TODO:
#ifndef STBI_NO_STDIO
extern int      stbi_jpeg_info            (char *filename,           int *x, int *y, int *comp);
extern int      stbi_jpeg_info_from_file  (FILE *f,                  int *x, int *y, int *comp);
#endif
extern int      stbi_jpeg_info_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp);

// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
//    simple implementation
//      - all input must be provided in an upfront buffer
//      - all output is written to a single output buffer (can malloc/realloc)
//    performance
//      - fast huffman

// fast-way is faster to check than jpeg huffman, but slow way is slower
#define ZFAST_BITS  9 // accelerate all cases in default tables
#define ZFAST_MASK  ((1 << ZFAST_BITS) - 1)

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
typedef struct
{
   uint16 fast[1 << ZFAST_BITS];
   uint16 firstcode[16];
   int maxcode[17];
   uint16 firstsymbol[16];
   uint8  size[288];
   uint16 value[288];
} zhuffman;

__forceinline static int bitreverse16(int n)
{
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

__forceinline static int bit_reverse(int v, int bits)
{
   assert(bits <= 16);
   // to bit reverse n bits, reverse 16 and shift
   // e.g. 11 bits, bit reverse and shift away 5
   return bitreverse16(v) >> (16-bits);
}

static int zbuild_huffman(zhuffman *z, uint8 *sizelist, int num)
{
   int i,k=0;
   int code, next_code[16], sizes[17];

   // DEFLATE spec for generating codes
   memset(sizes, 0, sizeof(sizes));
   memset(z->fast, 255, sizeof(z->fast));
   for (i=0; i < num; ++i)
      ++sizes[sizelist[i]];
   sizes[0] = 0;
   for (i=1; i < 16; ++i)
      assert(sizes[i] <= (1 << i));
   code = 0;
   for (i=1; i < 16; ++i) {
      next_code[i] = code;
      z->firstcode[i] = (uint16) code;
      z->firstsymbol[i] = (uint16) k;
      code = (code + sizes[i]);
      if (sizes[i])
         if (code-1 >= (1 << i)) return e("bad codelengths","Corrupt JPEG");
      z->maxcode[i] = code << (16-i); // preshift for inner loop
      code <<= 1;
      k += sizes[i];
   }
   z->maxcode[16] = 0x10000; // sentinel
   for (i=0; i < num; ++i) {
      int s = sizelist[i];
      if (s) {
         int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
         z->size[c] = (uint8)s;
         z->value[c] = (uint16)i;
         if (s <= ZFAST_BITS) {
            int k = bit_reverse(next_code[s],s);
            while (k < (1 << ZFAST_BITS)) {
               z->fast[k] = (uint16) c;
               k += (1 << s);
            }
         }
         ++next_code[s];
      }
   }
   return 1;
}

// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer

static uint8 *zbuffer, *zbuffer_end;

__forceinline static int zget8(void)
{
   if (zbuffer >= zbuffer_end) return 0;
   return *zbuffer++;
}

//static unsigned long code_buffer;
static int           num_bits;

static void fill_bits(void)
{
   do {
      assert(code_buffer < (1U << num_bits));
      code_buffer |= zget8() << num_bits;
      num_bits += 8;
   } while (num_bits <= 24);
}

__forceinline static unsigned int zreceive(int n)
{
   unsigned int k;
   if (num_bits < n) fill_bits();
   k = code_buffer & ((1 << n) - 1);
   code_buffer >>= n;
   num_bits -= n;
   return k;
}

__forceinline static int zhuffman_decode(zhuffman *z)
{
   int b,s,k;
   if (num_bits < 16) fill_bits();
   b = z->fast[code_buffer & ZFAST_MASK];
   if (b < 0xffff) {
      s = z->size[b];
      code_buffer >>= s;
      num_bits -= s;
      return z->value[b];
   }

   // not resolved by fast table, so compute it the slow way
   // use jpeg approach, which requires MSbits at top
   k = bit_reverse(code_buffer, 16);
   for (s=ZFAST_BITS+1; ; ++s)
      if (k < z->maxcode[s])
         break;
   if (s == 16) return -1; // invalid code!
   // code size is s, so:
   b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
   assert(z->size[b] == s);
   code_buffer >>= s;
   num_bits -= s;
   return z->value[b];
}

static char *zout;
static char *zout_start;
static char *zout_end;
static int   z_expandable;

static int expand(int n)  // need to make room for n bytes
{
   char *q;
   int cur, limit;
   if (!z_expandable) return e("output buffer limit","Corrupt PNG");
   cur   = (int) (zout     - zout_start);
   limit = (int) (zout_end - zout_start);
   while (cur + n > limit)
      limit *= 2;
   q = (char *) realloc(zout_start, limit);
   if (q == NULL) return e("outofmem", "Out of memory");
   zout_start = q;
   zout       = q + cur;
   zout_end   = q + limit;
   return 1;
}

static zhuffman z_length, z_distance;

static int length_base[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static int length_extra[31]=
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static int dist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static int dist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int parse_huffman_block(void)
{
   for(;;) {
      int z = zhuffman_decode(&z_length);
      if (z < 256) {
         if (z < 0) return e("bad huffman code","Corrupt PNG"); // error in huffman codes
         if (zout >= zout_end) if (!expand(1)) return 0;
         *zout++ = (char) z;
      } else {
         uint8 *p;
         int len,dist;
         if (z == 256) return 1;
         z -= 257;
         len = length_base[z];
         if (length_extra[z]) len += zreceive(length_extra[z]);
         z = zhuffman_decode(&z_distance);
         if (z < 0) return e("bad huffman code","Corrupt PNG");
         dist = dist_base[z];
         if (dist_extra[z]) dist += zreceive(dist_extra[z]);
         if (zout - zout_start < dist) return e("bad dist","Corrupt PNG");
         if (zout + len > zout_end) if (!expand(len)) return 0;
         p = (uint8 *) (zout - dist);
         while (len--)
            *zout++ = *p++;
      }
   }
}

static int compute_huffman_codes(void)
{
   static uint8 length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
   static zhuffman z_codelength; // static just to save stack space
   uint8 lencodes[286+32+137];//padding for maximum single op
   uint8 codelength_sizes[19];
   int i,n;

   int hlit  = zreceive(5) + 257;
   int hdist = zreceive(5) + 1;
   int hclen = zreceive(4) + 4;

   memset(codelength_sizes, 0, sizeof(codelength_sizes));
   for (i=0; i < hclen; ++i) {
      int s = zreceive(3);
      codelength_sizes[length_dezigzag[i]] = (uint8) s;
   }
   if (!zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

   n = 0;
   while (n < hlit + hdist) {
      int c = zhuffman_decode(&z_codelength);
      assert(c >= 0 && c < 19);
      if (c < 16)
         lencodes[n++] = (uint8) c;
      else if (c == 16) {
         c = zreceive(2)+3;
         memset(lencodes+n, lencodes[n-1], c);
         n += c;
      } else if (c == 17) {
         c = zreceive(3)+3;
         memset(lencodes+n, 0, c);
         n += c;
      } else {
         assert(c == 18);
         c = zreceive(7)+11;
         memset(lencodes+n, 0, c);
         n += c;
      }
   }
   if (n != hlit+hdist) return e("bad codelengths","Corrupt PNG");
   if (!zbuild_huffman(&z_length, lencodes, hlit)) return 0;
   if (!zbuild_huffman(&z_distance, lencodes+hlit, hdist)) return 0;
   return 1;
}

static int parse_uncompressed_block(void)
{
   uint8 header[4];
   int len,nlen,k;
   if (num_bits & 7)
      zreceive(num_bits & 7); // discard
   // drain the bit-packed data into header
   k = 0;
   while (num_bits > 0) {
      header[k++] = (uint8) (code_buffer & 255); // wtf this warns?
      code_buffer >>= 8;
      num_bits -= 8;
   }
   assert(num_bits == 0);
   // now fill header the normal way
   while (k < 4)
      header[k++] = (uint8) zget8();
   len  = header[1] * 256 + header[0];
   nlen = header[3] * 256 + header[2];
   if (nlen != (len ^ 0xffff)) return e("zlib corrupt","Corrupt PNG");
   if (zbuffer + len > zbuffer_end) return e("read past buffer","Corrupt PNG");
   if (zout + len > zout_end)
      if (!expand(len)) return 0;
   memcpy(zout, zbuffer, len);
   zbuffer += len;
   zout += len;
   return 1;
}

static int parse_zlib_header(void)
{
   int cmf   = zget8();
   int cm    = cmf & 15;
   /* int cinfo = cmf >> 4; */
   int flg   = zget8();
   if ((cmf*256+flg) % 31 != 0) return e("bad zlib header","Corrupt PNG"); // zlib spec
   if (flg & 32) return e("no preset dict","Corrupt PNG"); // preset dictionary not allowed in png
   if (cm != 8) return e("bad compression","Corrupt PNG"); // DEFLATE required for png
   // window = 1 << (8 + cinfo)... but who cares, we fully buffer output
   return 1;
}

static uint8 default_length[288], default_distance[32];
static void init_defaults(void)
{
   int i;   // use <= to match clearly with spec
   for (i=0; i <= 143; ++i)     default_length[i]   = 8;
   for (   ; i <= 255; ++i)     default_length[i]   = 9;
   for (   ; i <= 279; ++i)     default_length[i]   = 7;
   for (   ; i <= 287; ++i)     default_length[i]   = 8;

   for (i=0; i <=  31; ++i)     default_distance[i] = 5;
}

static int parse_zlib(int parse_header)
{
   int final, type;
   if (parse_header)
      if (!parse_zlib_header()) return 0;
   num_bits = 0;
   code_buffer = 0;
   do {
      final = zreceive(1);
      type = zreceive(2);
      if (type == 0) {
         if (!parse_uncompressed_block()) return 0;
      } else if (type == 3) {
         return 0;
      } else {
         if (type == 1) {
            // use fixed code lengths
            if (!default_length[0]) init_defaults();
            if (!zbuild_huffman(&z_length  , default_length  , 288)) return 0;
            if (!zbuild_huffman(&z_distance, default_distance,  32)) return 0;
         } else {
            if (!compute_huffman_codes()) return 0;
         }
         if (!parse_huffman_block()) return 0;
      }
   } while (!final);
   return 1;
}

static int do_zlib(char *obuf, int olen, int exp, int parse_header)
{
   zout_start = obuf;
   zout       = obuf;
   zout_end   = obuf + olen;
   z_expandable = exp;

   return parse_zlib(parse_header);
}

char *stbi_zlib_decode_malloc_guesssize(int initial_size, int *outlen)
{
   char *p = (char *) malloc(initial_size);
   if (p == NULL) return NULL;
   if (do_zlib(p, initial_size, 1, 1)) {
      *outlen = (int) (zout - zout_start);
      return zout_start;
   } else {
      free(zout_start);
      return NULL;
   }
}

char *stbi_zlib_decode_malloc(char *buffer, int len, int *outlen)
{
   zbuffer = (uint8 *) buffer;
   zbuffer_end = (uint8 *) buffer+len;
   return stbi_zlib_decode_malloc_guesssize(16384, outlen);
}

int stbi_zlib_decode_buffer(char *obuffer, int olen, char *ibuffer, int ilen)
{
   zbuffer = (uint8 *) ibuffer;
   zbuffer_end = (uint8 *) ibuffer + ilen;
   if (do_zlib(obuffer, olen, 0, 1))
      return (int) (zout - zout_start);
   else
      return -1;
}

char *stbi_zlib_decode_noheader_malloc(char *buffer, int len, int *outlen)
{
   char *p = (char *) malloc(16384);
   if (p == NULL) return NULL;
   zbuffer = (uint8 *) buffer;
   zbuffer_end = (uint8 *) buffer+len;
   if (do_zlib(p, 16384, 1, 0)) {
      *outlen = (int) (zout - zout_start);
      return zout_start;
   } else {
      free(zout_start);
      return NULL;
   }
}

int stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, char *ibuffer, int ilen)
{
   zbuffer = (uint8 *) ibuffer;
   zbuffer_end = (uint8 *) ibuffer + ilen;
   if (do_zlib(obuffer, olen, 0, 0))
      return (int) (zout - zout_start);
   else
      return -1;
}

// public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
//    simple implementation
//      - only 8-bit samples
//      - no CRC checking
//      - allocates lots of intermediate memory
//        - avoids problem of streaming data between subsystems
//        - avoids explicit window management
//    performance
//      - uses stb_zlib, a PD zlib implementation with fast huffman decoding


typedef struct
{
   unsigned long length;
   unsigned long type;
} chunk;

#define PNG_TYPE(a,b,c,d)  (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

static chunk get_chunk_header(void)
{
   chunk c;
   c.length = get32();
   c.type   = get32();
   return c;
}

static int check_png_header(void)
{
   static uint8 png_sig[8] = { 137,80,78,71,13,10,26,10 };
   int i;
   for (i=0; i < 8; ++i)
      if (get8() != png_sig[i]) return e("bad png sig","Not a PNG");
   return 1;
}

static uint8 *idata, *expanded, *out;

enum {
   F_none=0, F_sub=1, F_up=2, F_avg=3, F_paeth=4,
   F_avg_first, F_paeth_first,
};

static uint8 first_row_filter[5] =
{
   F_none, F_sub, F_none, F_avg_first, F_paeth_first
};

static int paeth(int a, int b, int c)
{
   int p = a + b - c;
   int pa = abs(p-a);
   int pb = abs(p-b);
   int pc = abs(p-c);
   if (pa <= pb && pa <= pc) return a;
   if (pb <= pc) return b;
   return c;
}

// create the png data from post-deflated data
static int create_png_image(uint8 *raw, uint32 raw_len, int out_n)
{
   uint32 i,j,stride = img_x*out_n;
   int k;
   assert(out_n == img_n || out_n == img_n+1);
   out = (uint8 *) malloc(img_x * img_y * out_n);
   if (!out) return e("outofmem", "Out of memory");
   if (raw_len != (img_n * img_x + 1) * img_y) return e("not enough pixels","Corrupt PNG");
   for (j=0; j < img_y; ++j) {
      uint8 *cur = out + stride*j;
      uint8 *prior = cur - stride;
      int filter = *raw++;
      if (filter > 4) return e("invalid filter","Corrupt PNG");
      // if first row, use special filter that doesn't sample previous row
      if (j == 0) filter = first_row_filter[filter];
      // handle first pixel explicitly
      for (k=0; k < img_n; ++k) {
         switch(filter) {
            case F_none       : cur[k] = raw[k]; break;
            case F_sub        : cur[k] = raw[k]; break;
            case F_up         : cur[k] = raw[k] + prior[k]; break;
            case F_avg        : cur[k] = raw[k] + (prior[k]>>1); break;
            case F_paeth      : cur[k] = (uint8) (raw[k] + paeth(0,prior[k],0)); break;
            case F_avg_first  : cur[k] = raw[k]; break;
            case F_paeth_first: cur[k] = raw[k]; break;
         }
      }
      if (img_n != out_n) cur[img_n] = 255;
      raw += img_n;
      cur += out_n;
      prior += out_n;
      // this is a little gross, so that we don't switch per-pixel or per-component
      if (img_n == out_n) {
         #define CASE(f) \
             case f:     \
                for (i=1; i < img_x; ++i, raw+=img_n,cur+=img_n,prior+=img_n) \
                   for (k=0; k < img_n; ++k)
         switch(filter) {
            CASE(F_none)  cur[k] = raw[k]; break;
            CASE(F_sub)   cur[k] = raw[k] + cur[k-img_n]; break;
            CASE(F_up)    cur[k] = raw[k] + prior[k]; break;
            CASE(F_avg)   cur[k] = raw[k] + ((prior[k] + cur[k-img_n])>>1); break;
            CASE(F_paeth)  cur[k] = (uint8) (raw[k] + paeth(cur[k-img_n],prior[k],prior[k-img_n])); break;
            CASE(F_avg_first)    cur[k] = raw[k] + (cur[k-img_n] >> 1); break;
            CASE(F_paeth_first)  cur[k] = (uint8) (raw[k] + paeth(cur[k-img_n],0,0)); break;
         }
         #undef CASE
      } else {
         assert(img_n+1 == out_n);
         #define CASE(f) \
             case f:     \
                for (i=1; i < img_x; ++i, cur[img_n]=255,raw+=img_n,cur+=out_n,prior+=out_n) \
                   for (k=0; k < img_n; ++k)
         switch(filter) {
            CASE(F_none)  cur[k] = raw[k]; break;
            CASE(F_sub)   cur[k] = raw[k] + cur[k-out_n]; break;
            CASE(F_up)    cur[k] = raw[k] + prior[k]; break;
            CASE(F_avg)   cur[k] = raw[k] + ((prior[k] + cur[k-out_n])>>1); break;
            CASE(F_paeth)  cur[k] = (uint8) (raw[k] + paeth(cur[k-out_n],prior[k],prior[k-out_n])); break;
            CASE(F_avg_first)    cur[k] = raw[k] + (cur[k-out_n] >> 1); break;
            CASE(F_paeth_first)  cur[k] = (uint8) (raw[k] + paeth(cur[k-out_n],0,0)); break;
         }
         #undef CASE
      }
   }
   return 1;
}

static int compute_transparency(uint8 tc[3], int out_n)
{
   uint32 i, pixel_count = img_x * img_y;
   uint8 *p = out;

   // compute color-based transparency, assuming we've
   // already got 255 as the alpha value in the output
   assert(out_n == 2 || out_n == 4);

   p = out;
   if (out_n == 2) {
      for (i=0; i < pixel_count; ++i) {
         p[1] = (p[0] == tc[0] ? 0 : 255);
         p += 2;
      }
   } else {
      for (i=0; i < pixel_count; ++i) {
         if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
            p[3] = 0;
         p += 4;
      }
   }
   return 1;
}

static int expand_palette(uint8 *palette, int len, int pal_img_n)
{
   uint32 i, pixel_count = img_x * img_y;
   uint8 *p, *temp_out, *orig = out;

   p = (uint8 *) malloc(pixel_count * pal_img_n);
   if (p == NULL) return e("outofmem", "Out of memory");

   // between here and free(out) below, exitting would leak
   temp_out = p;

   if (pal_img_n == 3) {
      for (i=0; i < pixel_count; ++i) {
         int n = orig[i]*4;
         p[0] = palette[n  ];
         p[1] = palette[n+1];
         p[2] = palette[n+2];
         p += 3;
      }
   } else {
      for (i=0; i < pixel_count; ++i) {
         int n = orig[i]*4;
         p[0] = palette[n  ];
         p[1] = palette[n+1];
         p[2] = palette[n+2];
         p[3] = palette[n+3];
         p += 4;
      }
   }
   free(out);
   out = temp_out;
   return 1;
}

static int parse_png_file(int scan, int req_comp)
{
   uint8 palette[1024], pal_img_n=0;
   uint8 has_trans=0, tc[3];
   uint32 ioff=0, idata_limit=0, i, pal_len=0;
   int first=1,k;

   if (!check_png_header()) return 0;

   if (scan == SCAN_type) return 1;

   for(;;first=0) {
      chunk c = get_chunk_header();
      if (first && c.type != PNG_TYPE('I','H','D','R'))
         return e("first not IHDR","Corrupt PNG");
      switch (c.type) {
         case PNG_TYPE('I','H','D','R'): {
            int depth,color,interlace,comp,filter;
            if (!first) return e("multiple IHDR","Corrupt PNG");
            if (c.length != 13) return e("bad IHDR len","Corrupt PNG");
            img_x = get32(); if (img_x > (1 << 24)) return e("too large","Very large image (corrupt?)");
            img_y = get32(); if (img_y > (1 << 24)) return e("too large","Very large image (corrupt?)");
            depth = get8();  if (depth != 8)        return e("8bit only","PNG not supported: 8-bit only");
            color = get8();  if (color > 6)         return e("bad ctype","Corrupt PNG");
            if (color == 3) pal_img_n = 3; else if (color & 1) return e("bad ctype","Corrupt PNG");
            comp  = get8();  if (comp) return e("bad comp method","Corrupt PNG");
            filter= get8();  if (filter) return e("bad filter method","Corrupt PNG");
            interlace = get8(); if (interlace) return e("interlaced","PNG not supported: interlaced mode");
            if (!img_x || !img_y) return e("0-pixel image","Corrupt PNG");
            if (!pal_img_n) {
               img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
               if ((1 << 30) / img_x / img_n < img_y) return e("too large", "Image too large to decode");
               if (scan == SCAN_header) return 1;
            } else {
               // if paletted, then pal_n is our final components, and
               // img_n is # components to decompress/filter.
               img_n = 1;
               if ((1 << 30) / img_x / 4 < img_y) return e("too large","Corrupt PNG");
               // if SCAN_header, have to scan to see if we have a tRNS
            }
            break;
         }

         case PNG_TYPE('P','L','T','E'):  {
            if (c.length > 256*3) return e("invalid PLTE","Corrupt PNG");
            pal_len = c.length / 3;
            if (pal_len * 3 != c.length) return e("invalid PLTE","Corrupt PNG");
            for (i=0; i < pal_len; ++i) {
               palette[i*4+0] = get8u();
               palette[i*4+1] = get8u();
               palette[i*4+2] = get8u();
               palette[i*4+3] = 255;
            }
            break;
         }

         case PNG_TYPE('t','R','N','S'): {
            if (idata) return e("tRNS after IDAT","Corrupt PNG");
            if (pal_img_n) {
               if (scan == SCAN_header) { img_n = 4; return 1; }
               if (pal_len == 0) return e("tRNS before PLTE","Corrupt PNG");
               if (c.length > pal_len) return e("bad tRNS len","Corrupt PNG");
               pal_img_n = 4;
               for (i=0; i < c.length; ++i)
                  palette[i*4+3] = get8u();
            } else {
               if (!(img_n & 1)) return e("tRNS with alpha","Corrupt PNG");
               if (c.length != (uint32) img_n*2) return e("bad tRNS len","Corrupt PNG");
               has_trans = 1;
               for (k=0; k < img_n; ++k)
                  tc[k] = (uint8) get16(); // non 8-bit images will be larger
            }
            break;
         }

         case PNG_TYPE('I','D','A','T'): {
            if (pal_img_n && !pal_len) return e("no PLTE","Corrupt PNG");
            if (scan == SCAN_header) { img_n = pal_img_n; return 1; }
            if (ioff + c.length > idata_limit) {
               uint8 *p;
               if (idata_limit == 0) idata_limit = c.length > 4096 ? c.length : 4096;
               while (ioff + c.length > idata_limit)
                  idata_limit *= 2;
               p = (uint8 *) realloc(idata, idata_limit); if (p == NULL) return e("outofmem", "Out of memory");
               idata = p;
            }
            #ifndef STBI_NO_STDIO
            if (img_file)
            {
               if (fread(idata+ioff,1,c.length,img_file) != c.length) return e("outofdata","Corrupt PNG");
            }
            else
            #endif
            {
               memcpy(idata+ioff, img_buffer, c.length);
               img_buffer += c.length;
            }
            ioff += c.length;
            break;
         }

         case PNG_TYPE('I','E','N','D'): {
            uint32 raw_len;
            if (scan != SCAN_load) return 1;
            if (idata == NULL) return e("no IDAT","Corrupt PNG");
            expanded = (uint8 *) stbi_zlib_decode_malloc((char *) idata, ioff, (int *) &raw_len);
            if (expanded == NULL) return 0; // zlib should set error
            free(idata); idata = NULL;
            if ((req_comp == img_n+1 && req_comp != 3 && !pal_img_n) || has_trans)
               img_out_n = img_n+1;
            else
               img_out_n = img_n;
            if (!create_png_image(expanded, raw_len, img_out_n)) return 0;
            if (has_trans)
               if (!compute_transparency(tc, img_out_n)) return 0;
            if (pal_img_n) {
               // pal_img_n == 3 or 4
               img_n = pal_img_n; // record the actual colors we had
               img_out_n = pal_img_n;
               if (req_comp >= 3) img_out_n = req_comp;
               if (!expand_palette(palette, pal_len, img_out_n))
                  return 0;
            }
            free(expanded); expanded = NULL;
            return 1;
         }

         default:
            // if critical, fail
            if ((c.type & (1 << 29)) == 0) {
               #ifndef STBI_NO_FAILURE_STRINGS
               static char invalid_chunk[] = "XXXX chunk not known";
               invalid_chunk[0] = (uint8) (c.type >> 24);
               invalid_chunk[1] = (uint8) (c.type >> 16);
               invalid_chunk[2] = (uint8) (c.type >>  8);
               invalid_chunk[3] = (uint8) (c.type >>  0);
               #endif
               return e(invalid_chunk, "PNG not supported: unknown chunk type");
            }
            skip(c.length);
            break;
      }
      // end of chunk, read and skip CRC
      get8(); get8(); get8(); get8();
   }
}

static unsigned char *do_png(int *x, int *y, int *n, int req_comp)
{
   unsigned char *result=NULL;
   if (req_comp < 0 || req_comp > 4) return ep("bad req_comp", "Internal error");
   if (parse_png_file(SCAN_load, req_comp)) {
      result = out;
      out = NULL;
      if (req_comp && req_comp != img_out_n) {
         result = convert_format(result, img_out_n, req_comp);
         if (result == NULL) return result;
      }
      *x = img_x;
      *y = img_y;
      if (n) *n = img_n;
   }
   free(out);      out      = NULL;
   free(expanded); expanded = NULL;
   free(idata);    idata    = NULL;

   return result;
}

#ifndef STBI_NO_STDIO
unsigned char *stbi_png_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   start_file(f);
   return do_png(x,y,comp,req_comp);
}

unsigned char *stbi_png_load(char *filename, int *x, int *y, int *comp, int req_comp)
{
   unsigned char *data;
   FILE *f = fopen(filename, "rb");
   if (!f) return NULL;
   data = stbi_png_load_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return data;
}
#endif

unsigned char *stbi_png_load_from_memory(unsigned char *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   start_mem(buffer,len);
   return do_png(x,y,comp,req_comp);
}

#ifndef STBI_NO_STDIO
int stbi_png_test_file(FILE *f)
{
   int n,r;
   n = ftell(f);
   start_file(f);
   r = parse_png_file(SCAN_type,STBI_default);
   fseek(f,n,SEEK_SET);
   return r;
}
#endif

int stbi_png_test_memory(unsigned char *buffer, int len)
{
   start_mem(buffer, len);
   return parse_png_file(SCAN_type,STBI_default);
}

// TODO: load header from png
#ifndef STBI_NO_STDIO
extern int      stbi_png_info             (char *filename,           int *x, int *y, int *comp);
extern int      stbi_png_info_from_file   (FILE *f,                  int *x, int *y, int *comp);
#endif
extern int      stbi_png_info_from_memory (stbi_uc *buffer, int len, int *x, int *y, int *comp);

// Microsoft/Windows BMP image

static int bmp_test(void)
{
   int sz;
   if (get8() != 'B') return 0;
   if (get8() != 'M') return 0;
   get32le(); // discard filesize
   get16le(); // discard reserved
   get16le(); // discard reserved
   get32le(); // discard data offset
   sz = get32le();
   if (sz == 12 || sz == 40 || sz == 56 || sz == 108) return 1;
   return 0;
}

#ifndef STBI_NO_STDIO
int      stbi_bmp_test_file        (FILE *f)
{
   int r,n = ftell(f);
   start_file(f);
   r = bmp_test();
   fseek(f,n,SEEK_SET);
   return r;
}
#endif

int      stbi_bmp_test_memory      (stbi_uc *buffer, int len)
{
   start_mem(buffer, len);
   return bmp_test();
}

// returns 0..31 for the highest set bit
static int high_bit(unsigned int z)
{
   int n=0;
   if (z == 0) return -1;
   if (z >= 0x10000) n += 16, z >>= 16;
   if (z >= 0x00100) n +=  8, z >>=  8;
   if (z >= 0x00010) n +=  4, z >>=  4;
   if (z >= 0x00004) n +=  2, z >>=  2;
   if (z >= 0x00002) n +=  1, z >>=  1;
   return n;
}

static int bitcount(unsigned int a)
{
   a = (a & 0x55555555) + ((a >>  1) & 0x55555555); // max 2
   a = (a & 0x33333333) + ((a >>  2) & 0x33333333); // max 4
   a = (a + (a >> 4)) & 0x0f0f0f0f; // max 8 per 4, now 8 bits
   a = (a + (a >> 8)); // max 16 per 8 bits
   a = (a + (a >> 16)); // max 32 per 8 bits
   return a & 0xff;
}

static int shiftsigned(int v, int shift, int bits)
{
   int result;
   int z=0;

   if (shift < 0) v <<= -shift;
   else v >>= shift;
   result = v;

   z = bits;
   while (z < 8) {
      result += v >> z;
      z += bits;
   }
   return result;
}

static stbi_uc *bmp_load(int *x, int *y, int *comp, int req_comp)
{
   unsigned int mr=0,mg=0,mb=0,ma=0;
   stbi_uc pal[256][4];
   int psize=0,i,j,compress=0,width;
   int bpp, flip_vertically, pad, target, offset, hsz;
   if (get8() != 'B' || get8() != 'M') return ep("not BMP", "Corrupt BMP");
   get32le(); // discard filesize
   get16le(); // discard reserved
   get16le(); // discard reserved
   offset = get32le();
   hsz = get32le();
   if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108) return ep("unknown BMP", "BMP type not supported: unknown");
   failure_reason = "bad BMP";
   if (hsz == 12) {
      img_x = get16le();
      img_y = get16le();
   } else {
      img_x = get32le();
      img_y = get32le();
   }
   if (get16le() != 1) return 0;
   bpp = get16le();
   if (bpp == 1) return ep("monochrome", "BMP type not supported: 1-bit");
   flip_vertically = ((int) img_y) > 0;
   img_y = abs((int) img_y);
   if (hsz == 12) {
      if (bpp < 24)
         psize = (offset - 14 - 24) / 3;
   } else {
      compress = get32le();
      if (compress == 1 || compress == 2) return ep("BMP RLE", "BMP type not supported: RLE");
      get32le(); // discard sizeof
      get32le(); // discard hres
      get32le(); // discard vres
      get32le(); // discard colorsused
      get32le(); // discard max important
      if (hsz == 40 || hsz == 56) {
         if (hsz == 56) {
            get32le();
            get32le();
            get32le();
            get32le();
         }
         if (bpp == 16 || bpp == 32) {
            mr = mg = mb = 0;
            if (compress == 0) {
               if (bpp == 32) {
                  mr = 0xff << 16;
                  mg = 0xff <<  8;
                  mb = 0xff <<  0;
               } else {
                  mr = 31 << 10;
                  mg = 31 <<  5;
                  mb = 31 <<  0;
               }
            } else if (compress == 3) {
               mr = get32le();
               mg = get32le();
               mb = get32le();
               // not documented, but generated by photoshop and handled by mspaint
               if (mr == mg && mg == mb) {
                  // ?!?!?
                  return NULL;
               }
            } else
               return NULL;
         }
      } else {
         assert(hsz == 108);
         mr = get32le();
         mg = get32le();
         mb = get32le();
         ma = get32le();
         get32le(); // discard color space
         for (i=0; i < 12; ++i)
            get32le(); // discard color space parameters
      }
      if (bpp < 16)
         psize = (offset - 14 - hsz) >> 2;
   }
   img_n = ma ? 4 : 3;
   if (req_comp && req_comp >= 3) // we can directly decode 3 or 4
      target = req_comp;
   else
      target = img_n; // if they want monochrome, we'll post-convert
   out = (stbi_uc *) malloc(target * img_x * img_y);
   if (!out) return ep("outofmem", "Out of memory");
   if (bpp < 16) {
      int z=0;
      if (psize == 0 || psize > 256) return ep("invalid", "Corrupt BMP");
      for (i=0; i < psize; ++i) {
         pal[i][2] = get8();
         pal[i][1] = get8();
         pal[i][0] = get8();
         if (hsz != 12) get8();
         pal[i][3] = 255;
      }
      skip(offset - 14 - hsz - psize * (hsz == 12 ? 3 : 4));
      if (bpp == 4) width = (img_x + 1) >> 1;
      else if (bpp == 8) width = img_x;
      else return ep("bad bpp", "Corrupt BMP");
      pad = (-width)&3;
      for (j=0; j < (int) img_y; ++j) {
         for (i=0; i < (int) img_x; i += 2) {
            int v=get8(),v2=0;
            if (bpp == 4) {
               v2 = v & 15;
               v >>= 4;
            }
            out[z++] = pal[v][0];
            out[z++] = pal[v][1];
            out[z++] = pal[v][2];
            if (target == 4) out[z++] = 255;
            if (i+1 == (int) img_x) break;
            v = (bpp == 8) ? get8() : v2;
            out[z++] = pal[v][0];
            out[z++] = pal[v][1];
            out[z++] = pal[v][2];
            if (target == 4) out[z++] = 255;
         }
         skip(pad);
      }
   } else {
      int rshift=0,gshift=0,bshift=0,ashift=0,rcount=0,gcount=0,bcount=0,acount=0;
      int z = 0;
      int easy=0;
      skip(offset - 14 - hsz);
      if (bpp == 24) width = 3 * img_x;
      else if (bpp == 16) width = 2*img_x;
      else /* bpp = 32 and pad = 0 */ width=0;
      pad = (-width) & 3;
      if (bpp == 24) {
         easy = 1;
      } else if (bpp == 32) {
         if (mb == 0xff && mg == 0xff00 && mr == 0xff000000 && ma == 0xff000000)
            easy = 2;
      }
      if (!easy) {
         if (!mr || !mg || !mb) return ep("bad masks", "Corrupt BMP");
         // right shift amt to put high bit in position #7
         rshift = high_bit(mr)-7; rcount = bitcount(mr);
         gshift = high_bit(mg)-7; gcount = bitcount(mr);
         bshift = high_bit(mb)-7; bcount = bitcount(mr);
         ashift = high_bit(ma)-7; acount = bitcount(mr);
      }
      for (j=0; j < (int) img_y; ++j) {
         if (easy) {
            for (i=0; i < (int) img_x; ++i) {
               int a;
               out[z+2] = get8();
               out[z+1] = get8();
               out[z+0] = get8();
               z += 3;
               a = (easy == 2 ? get8() : 255);
               if (target == 4) out[z++] = a;
            }
         } else {
            for (i=0; i < (int) img_x; ++i) {
               unsigned long v = (bpp == 16 ? get16le() : get32le());
               int a;
               out[z++] = shiftsigned(v & mr, rshift, rcount);
               out[z++] = shiftsigned(v & mg, gshift, gcount);
               out[z++] = shiftsigned(v & mb, bshift, bcount);
               a = (ma ? shiftsigned(v & ma, ashift, acount) : 255);
               if (target == 4) out[z++] = a;
            }
         }
         skip(pad);
      }
   }
   if (flip_vertically) {
      stbi_uc t;
      for (j=0; j < (int) img_y>>1; ++j) {
         stbi_uc *p1 = out +      j     *img_x*target;
         stbi_uc *p2 = out + (img_y-1-j)*img_x*target;
         for (i=0; i < (int) img_x*target; ++i) {
            t = p1[i], p1[i] = p2[i], p2[i] = t;
         }
      }
   }

   if (req_comp && req_comp != target) {
      out = convert_format(out, target, req_comp);
      if (out == NULL) return out; // convert_format frees input on failure
   }

   *x = img_x;
   *y = img_y;
   if (comp) *comp = target;
   return out;
}

#ifndef STBI_NO_STDIO
stbi_uc *stbi_bmp_load             (char *filename,           int *x, int *y, int *comp, int req_comp)
{
   stbi_uc *data;
   FILE *f = fopen(filename, "rb");
   if (!f) return NULL;
   data = stbi_bmp_load_from_file(f, x,y,comp,req_comp);
   fclose(f);
   return data;
}

stbi_uc *stbi_bmp_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp)
{
   start_file(f);
   return bmp_load(x,y,comp,req_comp);
}
#endif

stbi_uc *stbi_bmp_load_from_memory (stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   start_mem(buffer, len);
   return bmp_load(x,y,comp,req_comp);
}

// Targa Truevision - TGA
// by Jonathan Dummer

static int tga_test(void)
{
	int sz;
	get8u();		//	discard Offset
	sz = get8u();	//	color type
	if( sz > 1 ) return 0;	//	only RGB or indexed allowed
	sz = get8u();	//	image type
	if( (sz != 1) && (sz != 2) && (sz != 3) && (sz != 9) && (sz != 10) && (sz != 11) ) return 0;	//	only RGB or grey allowed, +/- RLE
	get16();		//	discard palette start
	get16();		//	discard palette length
	get8();			//	discard bits per palette color entry
	get16();		//	discard x origin
	get16();		//	discard y origin
	if( get16() < 1 ) return 0;		//	test width
	if( get16() < 1 ) return 0;		//	test height
	sz = get8();	//	bits per pixel
	if( (sz != 8) && (sz != 16) && (sz != 24) && (sz != 32) ) return 0;	//	only RGB or RGBA or grey allowed
	return 1;		//	seems to have passed everything
}

#ifndef STBI_NO_STDIO
int      stbi_tga_test_file        (FILE *f)
{
   int r,n = ftell(f);
   start_file(f);
   r = tga_test();
   fseek(f,n,SEEK_SET);
   return r;
}
#endif

int      stbi_tga_test_memory      (stbi_uc *buffer, int len)
{
   start_mem(buffer, len);
   return tga_test();
}

static stbi_uc *tga_load(int *x, int *y, int *comp, int req_comp)
{
	//	read in the TGA header stuff
	int tga_offset = get8u();
	int tga_indexed = get8u();
	int tga_image_type = get8u();
	int tga_is_RLE = 0;
	int tga_palette_start = get16le();
	int tga_palette_len = get16le();
	int tga_palette_bits = get8u();
	int tga_x_origin = get16le();
	int tga_y_origin = get16le();
	int tga_width = get16le();
	int tga_height = get16le();
	int tga_bits_per_pixel = get8u();
	int tga_inverted = get8u();
	//	image data
	unsigned char *tga_data;
	unsigned char *tga_palette = NULL;
	int i, j;
	unsigned char raw_data[4];
	unsigned char trans_data[] = { 0, 0, 0, 0 };
	int RLE_count = 0;
	int RLE_repeating = 0;
	int read_next_pixel = 1;
	//	do a tiny bit of precessing
	if( tga_image_type >= 8 )
	{
		tga_image_type -= 8;
		tga_is_RLE = 1;
	}
	/* int tga_alpha_bits = tga_inverted & 15; */
	tga_inverted = 1 - ((tga_inverted >> 5) & 1);

	//	error check
	if( //(tga_indexed) ||
		(tga_width < 1) || (tga_height < 1) ||
		(tga_image_type < 1) || (tga_image_type > 3) ||
		((tga_bits_per_pixel != 8) && (tga_bits_per_pixel != 16) &&
		(tga_bits_per_pixel != 24) && (tga_bits_per_pixel != 32))
		)
	{
		return NULL;
	}

	//	If I'm paletted, then I'll use the number of bits from the palette
	if( tga_indexed )
	{
		tga_bits_per_pixel = tga_palette_bits;
	}

	//	tga info
	*x = tga_width;
	*y = tga_height;
	if( (req_comp < 1) || (req_comp > 4) )
	{
		//	just use whatever the file was
		req_comp = tga_bits_per_pixel / 8;
		*comp = req_comp;
	} else
	{
		//	force a new number of components
		*comp = req_comp;
	}
	tga_data = (unsigned char*)malloc( tga_width * tga_height * req_comp );

	//	skip to the data's starting position (offset usually = 0)
	skip( tga_offset );
	//	do I need to load a palette?
	if( tga_indexed )
	{
		//	any data to skip? (offset usually = 0)
		skip( tga_palette_start );
		//	load the palette
		tga_palette = (unsigned char*)malloc( tga_palette_len * tga_palette_bits / 8 );
		getn( tga_palette, tga_palette_len * tga_palette_bits / 8 );
	}
	//	load the data
	for( i = 0; i < tga_width * tga_height; ++i )
	{
		//	if I'm in RLE mode, do I need to get a RLE chunk?
		if( tga_is_RLE )
		{
			if( RLE_count == 0 )
			{
				//	yep, get the next byte as a RLE command
				int RLE_cmd = get8u();
				RLE_count = 1 + (RLE_cmd & 127);
				RLE_repeating = RLE_cmd >> 7;
				read_next_pixel = 1;
			} else if( !RLE_repeating )
			{
				read_next_pixel = 1;
			}
		} else
		{
			read_next_pixel = 1;
		}
		//	OK, if I need to read a pixel, do it now
		if( read_next_pixel )
		{
			//	load however much data we did have
			if( tga_indexed )
			{
				//	read in 1 byte, then perform the lookup
				int pal_idx = get8u();
				if( pal_idx >= tga_palette_len )
				{
					//	invalid index
					pal_idx = 0;
				}
				pal_idx *= tga_bits_per_pixel / 8;
				for( j = 0; j*8 < tga_bits_per_pixel; ++j )
				{
					raw_data[j] = tga_palette[pal_idx+j];
				}
			} else
			{
				//	read in the data raw
				for( j = 0; j*8 < tga_bits_per_pixel; ++j )
				{
					raw_data[j] = get8u();
				}
			}
			//	convert raw to the intermediate format
			switch( tga_bits_per_pixel )
			{
			case 8:
				//	Luminous => RGBA
				trans_data[0] = raw_data[0];
				trans_data[1] = raw_data[0];
				trans_data[2] = raw_data[0];
				trans_data[3] = 255;
				break;
			case 16:
				//	Luminous,Alpha => RGBA
				trans_data[0] = raw_data[0];
				trans_data[1] = raw_data[0];
				trans_data[2] = raw_data[0];
				trans_data[3] = raw_data[1];
				break;
			case 24:
				//	BGR => RGBA
				trans_data[0] = raw_data[2];
				trans_data[1] = raw_data[1];
				trans_data[2] = raw_data[0];
				trans_data[3] = 255;
				break;
			case 32:
				//	BGRA => RGBA
				trans_data[0] = raw_data[2];
				trans_data[1] = raw_data[1];
				trans_data[2] = raw_data[0];
				trans_data[3] = raw_data[3];
				break;
			}
			//	clear the reading flag for the next pixel
			read_next_pixel = 0;
		} // end of reading a pixel
		//	convert to final format
		switch( req_comp )
		{
		case 1:
			//	RGBA => Luminous
			tga_data[i*req_comp+0] = (trans_data[0] + trans_data[1] + trans_data[2]) / 3;
			break;
		case 2:
			//	RGBA => Luminous,Alpha
			tga_data[i*req_comp+0] = (trans_data[0] + trans_data[1] + trans_data[2]) / 3;
			tga_data[i*req_comp+1] = trans_data[3];
			break;
		case 3:
			//	RGBA => RGB
			tga_data[i*req_comp+0] = trans_data[0];
			tga_data[i*req_comp+1] = trans_data[1];
			tga_data[i*req_comp+2] = trans_data[2];
			break;
		case 4:
			//	RGBA => RGBA
			tga_data[i*req_comp+0] = trans_data[0];
			tga_data[i*req_comp+1] = trans_data[1];
			tga_data[i*req_comp+2] = trans_data[2];
			tga_data[i*req_comp+3] = trans_data[3];
			break;
		}
		//	in case we're in RLE mode, keep counting down
		--RLE_count;
	}
	//	do I need to invert the image?
	if( tga_inverted )
	{
		for( j = 0; j*2 < tga_height; ++j )
		{
			int index1 = j * tga_width * req_comp;
			int index2 = (tga_height - 1 - j) * tga_width * req_comp;
			for( i = tga_width * req_comp; i > 0; --i )
			{
				unsigned char temp = tga_data[index1];
				tga_data[index1] = tga_data[index2];
				tga_data[index2] = temp;
				++index1;
				++index2;
			}
		}
	}
	//	clear my palette, if I had one
	if( tga_palette != NULL )
	{
		free( tga_palette );
	}
	//	the things I do to get rid of an error message, and yet keep
	//	Microsoft's C compilers happy... [8^(
	tga_palette_start = tga_palette_len = tga_palette_bits =
			tga_x_origin = tga_y_origin = 0;
	//	OK, done
	return tga_data;
}

#ifndef STBI_NO_STDIO
stbi_uc *stbi_tga_load             (char *filename,           int *x, int *y, int *comp, int req_comp)
{
   stbi_uc *data;
   FILE *f = fopen(filename, "rb");
   if (!f) return NULL;
   data = stbi_tga_load_from_file(f, x,y,comp,req_comp);
   fclose(f);
   return data;
}

stbi_uc *stbi_tga_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp)
{
   start_file(f);
   return tga_load(x,y,comp,req_comp);
}
#endif

stbi_uc *stbi_tga_load_from_memory (stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   start_mem(buffer, len);
   return tga_load(x,y,comp,req_comp);
}

// *************************************************************************************************
// Radiance RGBE HDR loader
// originally by Nicolas Schulz
#ifndef STBI_NO_HDR
static int hdr_test(void)
{
   char *signature = "#?RADIANCE\n";
   int i;
   for (i=0; signature[i]; ++i)
      if (get8() != signature[i])
         return 0;
	return 1;
}

int stbi_hdr_test_memory(stbi_uc *buffer, int len)
{
	start_mem(buffer, len);
	return hdr_test();
}

#ifndef STBI_NO_STDIO
int stbi_hdr_test_file(FILE *f)
{
   int r,n = ftell(f);
   start_file(f);
   r = hdr_test();
   fseek(f,n,SEEK_SET);
   return r;
}
#endif

#define HDR_BUFLEN  1024
static char *hdr_gettoken(char *buffer)
{
   int len=0;
   /*char *s = buffer, */
   char c = '\0';

   c = get8();

	while (!at_eof() && c != '\n') {
		buffer[len++] = c;
      if (len == HDR_BUFLEN-1) {
         // flush to end of line
         while (!at_eof() && get8() != '\n')
            ;
         break;
      }
      c = get8();
	}

   buffer[len] = 0;
	return buffer;
}

static void hdr_convert(float *output, stbi_uc *input, int req_comp)
{
	if( input[3] != 0 ) {
      float f1;
		// Exponent
		f1 = (float) ldexp(1.0f, input[3] - (int)(128 + 8));
      if (req_comp <= 2)
         output[0] = (input[0] + input[1] + input[2]) * f1 / 3;
      else {
         output[0] = input[0] * f1;
         output[1] = input[1] * f1;
         output[2] = input[2] * f1;
      }
      if (req_comp == 2) output[1] = 1;
      if (req_comp == 4) output[3] = 1;
	} else {
      switch (req_comp) {
         case 4: output[3] = 255; /* fallthrough */
         case 3: output[0] = output[1] = output[2] = 0;
                 break;
         case 2: output[1] = 255; /* fallthrough */
         case 1: output[0] = 0;
                 break;
      }
	}
}


static float *hdr_load(int *x, int *y, int *comp, int req_comp)
{
   char buffer[HDR_BUFLEN];
	char *token;
	int valid = 0;
	int width, height;
   stbi_uc *scanline;
	float *hdr_data;
	int len;
	unsigned char count, value;
	int i, j, k, c1,c2, z;


	// Check identifier
	if (strcmp(hdr_gettoken(buffer), "#?RADIANCE") != 0)
		return ep("not HDR", "Corrupt HDR image");

	// Parse header
	while(1) {
		token = hdr_gettoken(buffer);
      if (token[0] == 0) break;
		if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
   }

	if (!valid)    return ep("unsupported format", "Unsupported HDR format");

   // Parse width and height
   // can't use sscanf() if we're not using stdio!
   token = hdr_gettoken(buffer);
   if (strncmp(token, "-Y ", 3))  return ep("unsupported data layout", "Unsupported HDR format");
   token += 3;
   height = strtol(token, &token, 10);
   while (*token == ' ') ++token;
   if (strncmp(token, "+X ", 3))  return ep("unsupported data layout", "Unsupported HDR format");
   token += 3;
   width = strtol(token, NULL, 10);

	*x = width;
	*y = height;

   *comp = 3;
	if (req_comp == 0) req_comp = 3;

	// Read data
	hdr_data = (float *) malloc(height * width * req_comp * sizeof(float));

	// Load image data
   // image data is stored as some number of sca
	if( width < 8 || width >= 32768) {
		// Read flat data
      for (j=0; j < height; ++j) {
         for (i=0; i < width; ++i) {
            stbi_uc rgbe[4];
           main_decode_loop:
            getn(rgbe, 4);
            hdr_convert(hdr_data + j * width * req_comp + i * req_comp, rgbe, req_comp);
         }
      }
	} else {
		// Read RLE-encoded data
		scanline = NULL;

		for (j = 0; j < height; ++j) {
         c1 = get8();
         c2 = get8();
         len = get8();
         if (c1 != 2 || c2 != 2 || (len & 0x80)) {
            // not run-length encoded, so we have to actually use THIS data as a decoded
            // pixel (note this can't be a valid pixel--one of RGB must be >= 128)
            stbi_uc rgbe[4] = { c1,c2,len, get8() };
            hdr_convert(hdr_data, rgbe, req_comp);
            i = 1;
            j = 0;
            free(scanline);
            goto main_decode_loop; // yes, this is fucking insane; blame the fucking insane format
         }
         len <<= 8;
         len |= get8();
         if (len != width) { free(hdr_data); free(scanline); return ep("invalid decoded scanline length", "corrupt HDR"); }
         if (scanline == NULL) scanline = (stbi_uc *) malloc(width * 4);

			for (k = 0; k < 4; ++k) {
				i = 0;
				while (i < width) {
					count = get8();
					if (count > 128) {
						// Run
						value = get8();
                  count -= 128;
						for (z = 0; z < count; ++z)
							scanline[i++ * 4 + k] = value;
					} else {
						// Dump
						for (z = 0; z < count; ++z)
							scanline[i++ * 4 + k] = get8();
					}
				}
			}
         for (i=0; i < width; ++i)
            hdr_convert(hdr_data+(j*width + i)*req_comp, scanline + i*4, req_comp);
		}
      free(scanline);
	}

   return hdr_data;
}

#ifndef STBI_NO_STDIO
float *stbi_hdr_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   start_file(f);
   return hdr_load(x,y,comp,req_comp);
}
#endif

float *stbi_hdr_load_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   start_mem(buffer, len);
   return hdr_load(x,y,comp,req_comp);
}

#endif // STBI_NO_HDR

/////////////////////// write image ///////////////////////

#ifndef STBI_NO_WRITE

static void write8(FILE *f, int x) { uint8 z = (uint8) x; fwrite(&z,1,1,f); }

static void writefv(FILE *f, char *fmt, va_list v)
{
   while (*fmt) {
      switch (*fmt++) {
         case ' ': break;
         case '1': { uint8 x = va_arg(v, int); write8(f,x); break; }
         case '2': { int16 x = va_arg(v, int); write8(f,x); write8(f,x>>8); break; }
         case '4': { int32 x = va_arg(v, int); write8(f,x); write8(f,x>>8); write8(f,x>>16); write8(f,x>>24); break; }
         default:
            assert(0);
            va_end(v);
            return;
      }
   }
}

static void writef(FILE *f, char *fmt, ...)
{
   va_list v;
   va_start(v, fmt);
   writefv(f,fmt,v);
   va_end(v);
}

static void write_pixels(FILE *f, int rgb_dir, int vdir, int x, int y, int comp, void *data, int write_alpha, int scanline_pad)
{
   uint8 bg[3] = { 255, 0, 255}, px[3];
   uint32 zero = 0;
   int i,j,k, j_end;

   if (vdir < 0)
      j_end = -1, j = y-1;
   else
      j_end =  y, j = 0;

   for (; j != j_end; j += vdir) {
      for (i=0; i < x; ++i) {
         uint8 *d = (uint8 *) data + (j*x+i)*comp;
         if (write_alpha < 0)
            fwrite(&d[comp-1], 1, 1, f);
         switch (comp) {
            case 1:
            case 2: writef(f, "111", d[0],d[0],d[0]);
                    break;
            case 4:
               if (!write_alpha) {
                  for (k=0; k < 3; ++k)
                     px[k] = bg[k] + ((d[k] - bg[k]) * d[3])/255;
                  writef(f, "111", px[1-rgb_dir],px[1],px[1+rgb_dir]);
                  break;
               }
               /* FALLTHROUGH */
            case 3:
               writef(f, "111", d[1-rgb_dir],d[1],d[1+rgb_dir]);
               break;
         }
         if (write_alpha > 0)
            fwrite(&d[comp-1], 1, 1, f);
      }
      fwrite(&zero,scanline_pad,1,f);
   }
}

static int outfile(char *filename, int rgb_dir, int vdir, int x, int y, int comp, void *data, int alpha, int pad, char *fmt, ...)
{
   FILE *f = fopen(filename, "wb");
   if (f) {
      va_list v;
      va_start(v, fmt);
      writefv(f, fmt, v);
      va_end(v);
      write_pixels(f,rgb_dir,vdir,x,y,comp,data,alpha,pad);
      fclose(f);
   }
   return f != NULL;
}

int stbi_write_bmp(char *filename, int x, int y, int comp, void *data)
{
   int pad = (-x*3) & 3;
   return outfile(filename,-1,-1,x,y,comp,data,0,pad,
           "11 4 22 4" "4 44 22 444444",
           'B', 'M', 14+40+(x*3+pad)*y, 0,0, 14+40,  // file header
            40, x,y, 1,24, 0,0,0,0,0,0);             // bitmap header
}

int stbi_write_tga(char *filename, int x, int y, int comp, void *data)
{
   int has_alpha = !(comp & 1);
   return outfile(filename, -1,-1, x, y, comp, data, has_alpha, 0,
                  "111 221 2222 11", 0,0,2, 0,0,0, 0,0,x,y, 24+8*has_alpha, 8*has_alpha);
}

// any other image formats that do interleaved rgb data?
//    PNG: requires adler32,crc32 -- significant amount of code
//    PSD: no, channels output separately
//    TIFF: no, stripwise-interleaved... i think

#endif /* STBI_NO_WRITE */

//	add in my DDS loading support
#ifndef STBI_NO_DDS
#include "stbi_DDS_aug_c.h"
#endif
