#include "goom_fx.h"
#include "goom_plugin_info.h"
#include "goomsl.h"
#include "goom_config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define CONV_MOTIF_W 32
//#define CONV_MOTIF_WMASK 0x1f

#define CONV_MOTIF_W 128
#define CONV_MOTIF_WMASK 0x7f

typedef char Motif[CONV_MOTIF_W][CONV_MOTIF_W];

#include "motif_goom1.h"
#include "motif_goom2.h"

#define NB_THETA 512

#define MAX 2.0f

typedef struct _CONV_DATA{
  PluginParam light;
  PluginParam factor_adj_p;
  PluginParam factor_p;
  PluginParameters params;

  GoomSL *script;

  /* rotozoom */
  int   theta;
  float ftheta;
  int   h_sin[NB_THETA];
  int   h_cos[NB_THETA];
  int   h_height;
  float visibility;
  Motif conv_motif;
  int   inverse_motif;
  
} ConvData;

/* init rotozoom tables */
static void compute_tables(VisualFX *_this, PluginInfo *info)
{
  ConvData *data = (ConvData*)_this->fx_data;
  double screen_coef;
  int i;
  double h;
  double radian;

  if (data->h_height == info->screen.height) return;

  screen_coef = 2.0 * 300.0 / (double)info->screen.height;
  data->h_height = info->screen.height;

  for ( i=0 ; i<NB_THETA ; i++ ) {
    radian = 2*i*M_PI/NB_THETA;
    h = (0.2 + cos (radian) / 15.0 * sin(radian * 2.0 + 12.123)) * screen_coef;
    data->h_cos[i] = 0x10000 * (-h * cos (radian) * cos(radian));
    data->h_sin[i] = 0x10000 * (h * sin (radian + 1.57) * sin(radian));
  }
}

static void set_motif(ConvData *data, Motif motif)
{
  int i,j;
  for (i=0;i<CONV_MOTIF_W;++i) for (j=0;j<CONV_MOTIF_W;++j)
    data->conv_motif[i][j] = motif[CONV_MOTIF_W-i-1][CONV_MOTIF_W-j-1];
}

static void convolve_init(VisualFX *_this, PluginInfo *info) {
  ConvData *data;
  data = (ConvData*)malloc(sizeof(ConvData));
  _this->fx_data = (void*)data;

  data->light = secure_f_param("Screen Brightness");
  data->light.param.fval.max = 300.0f;
  data->light.param.fval.step = 1.0f;
  data->light.param.fval.value = 100.0f;

  data->factor_adj_p = secure_f_param("Flash Intensity");
  data->factor_adj_p.param.fval.max = 200.0f;
  data->factor_adj_p.param.fval.step = 1.0f;
  data->factor_adj_p.param.fval.value = 70.0f;

  data->factor_p = secure_f_feedback("Factor");

  data->params = plugin_parameters ("Bright Flash", 5);
  data->params.params[0] = &data->light;
  data->params.params[1] = &data->factor_adj_p;
  data->params.params[2] = 0;
  data->params.params[3] = &data->factor_p;
  data->params.params[4] = 0;

  /* init rotozoom tables */
  compute_tables(_this, info);
  data->theta = 0;
  data->ftheta = 0.0;
  data->visibility = 1.0;
  set_motif(data, CONV_MOTIF2);
  data->inverse_motif = 0;

  _this->params = &data->params;
}

static void convolve_free(VisualFX *_this) {
  free (_this->fx_data);
}

static void create_output_with_brightness(VisualFX *_this, Pixel *src, Pixel *dest,
                                         PluginInfo *info, int iff)
{
  ConvData *data = (ConvData*)_this->fx_data;
  
  int x,y;
  int i = 0;//info->screen.height * info->screen.width - 1;

  const int c = data->h_cos [data->theta];
  const int s = data->h_sin [data->theta];

  const int xi = -(info->screen.width/2) * c;
  const int yi =  (info->screen.width/2) * s;

  const int xj = -(info->screen.height/2) * s;
  const int yj = -(info->screen.height/2) * c;

  int xprime = xj;
  int yprime = yj;

  int ifftab[16];
  if (data->inverse_motif) {
    int i;
    for (i=0;i<16;++i)
      ifftab[i] = (double)iff * (1.0 + data->visibility * (15.0 - i) / 15.0);
  }
  else {
    int i;
    for (i=0;i<16;++i)
      ifftab[i] = (double)iff / (1.0 + data->visibility * (15.0 - i) / 15.0);
  }

  for (y=info->screen.height;y--;) {
    int xtex,ytex;

    xtex = xprime + xi + CONV_MOTIF_W * 0x10000 / 2;
    xprime += s;

    ytex = yprime + yi + CONV_MOTIF_W * 0x10000 / 2;
    yprime += c;

#ifdef HAVE_MMX
    __asm__ __volatile__
      ("\n\t pxor  %%mm7,  %%mm7"  /* mm7 = 0   */
       "\n\t movd %[xtex],  %%mm2"
       "\n\t movd %[ytex],  %%mm3"
       "\n\t punpckldq %%mm3, %%mm2" /* mm2 = [ ytex | xtex ] */
       "\n\t movd %[c],     %%mm4"
       "\n\t movd %[s],     %%mm6"
       "\n\t pxor  %%mm5,   %%mm5"
       "\n\t psubd %%mm6,   %%mm5"
       "\n\t punpckldq %%mm5, %%mm4" /* mm4 = [ -s | c ]      */
       "\n\t movd %[motif], %%mm6"   /* mm6 = motif           */

       ::[xtex]"g"(xtex) ,[ytex]"g"(ytex)
        , [c]"g"(c), [s]"g"(s)
        , [motif] "g"(&data->conv_motif[0][0]));
    
    for (x=info->screen.width;x--;)
    {
      __asm__ __volatile__
        (
         "\n\t movd  %[src], %%mm0"  /* mm0 = src */
         "\n\t paddd %%mm4, %%mm2"   /* [ ytex | xtex ] += [ -s | s ] */
         "\n\t movd  %%esi, %%mm5"   /* save esi into mm5 */
         "\n\t movq  %%mm2, %%mm3"
         "\n\t psrld  $16,  %%mm3"   /* mm3 = [ (ytex>>16) | (xtex>>16) ] */
         "\n\t movd  %%mm3, %%eax"   /* eax = xtex' */

         "\n\t psrlq $25,   %%mm3"
         "\n\t movd  %%mm3, %%ecx"   /* ecx = ytex' << 7 */

         "\n\t andl  $127, %%eax"
         "\n\t andl  $16256, %%ecx"
         
         "\n\t addl  %%ecx, %%eax"
         "\n\t movd  %%mm6, %%esi"   /* esi = motif */
         "\n\t xorl  %%ecx, %%ecx"
         "\n\t movb  (%%eax,%%esi), %%cl"

         "\n\t movl  %[ifftab], %%eax"
         "\n\t movd  %%mm5, %%esi"    /* restore esi from mm5 */
         "\n\t movd  (%%eax,%%ecx,4), %%mm1" /* mm1 = [0|0|0|iff2] */

         "\n\t punpcklwd %%mm1, %%mm1"
         "\n\t punpcklbw %%mm7, %%mm0"
         "\n\t punpckldq %%mm1, %%mm1"
         "\n\t psrlw     $1,    %%mm0"
         "\n\t psrlw     $2,    %%mm1"
         "\n\t pmullw    %%mm1, %%mm0"
         "\n\t psrlw     $5,    %%mm0"
         "\n\t packuswb  %%mm7, %%mm0"
         "\n\t movd      %%mm0, %[dest]"
         : [dest] "=g" (dest[i].val)
         : [src]   "g"  (src[i].val)
         , [ifftab]"g"(&ifftab[0])
         : "eax","ecx");

      i++;
    }
#else
    for (x=info->screen.width;x--;) {

      int iff2;
      unsigned int f0,f1,f2,f3;
      
      xtex += c;
      ytex -= s;
      
      iff2 = ifftab[data->conv_motif[(ytex >>16) & CONV_MOTIF_WMASK][(xtex >> 16) & CONV_MOTIF_WMASK]];

#define sat(a) ((a)>0xFF?0xFF:(a))
      f0 = src[i].val;
      f1 = ((f0 >> R_OFFSET) & 0xFF) * iff2 >> 8;
      f2 = ((f0 >> G_OFFSET) & 0xFF) * iff2 >> 8;
      f3 = ((f0 >> B_OFFSET) & 0xFF) * iff2 >> 8;
      dest[i].val = (sat(f1) << R_OFFSET) | (sat(f2) << G_OFFSET) | (sat(f3) << B_OFFSET);
/*
      f0 = (src[i].cop[0] * iff2) >> 8;
      f1 = (src[i].cop[1] * iff2) >> 8;
      f2 = (src[i].cop[2] * iff2) >> 8;
      f3 = (src[i].cop[3] * iff2) >> 8;

      dest[i].cop[0] = (f0 & 0xffffff00) ? 0xff : (unsigned char)f0;
      dest[i].cop[1] = (f1 & 0xffffff00) ? 0xff : (unsigned char)f1;
      dest[i].cop[2] = (f2 & 0xffffff00) ? 0xff : (unsigned char)f2;
      dest[i].cop[3] = (f3 & 0xffffff00) ? 0xff : (unsigned char)f3;
*/
      i++;
    }
#endif 
  }
#ifdef HAVE_MMX
  __asm__ __volatile__ ("\n\t emms");
#endif
    
  compute_tables(_this, info);
}


/*#include <stdint.h>

static uint64_t GetTick()
{
  uint64_t x;
  asm volatile ("RDTSC" : "=A" (x));
  return x;
}*/


static void convolve_apply(VisualFX *_this, Pixel *src, Pixel *dest, PluginInfo *info) {

  ConvData *data = (ConvData*)_this->fx_data;
  float ff;
  int iff;
  
  ff = (FVAL(data->factor_p) * FVAL(data->factor_adj_p) + FVAL(data->light) ) / 100.0f;
  iff = (unsigned int)(ff * 256);

  {
    double fcycle = (double)info->cycle;
    double rotate_param, rotate_coef;
    float INCREASE_RATE = 1.5;
    float DECAY_RATE = 0.955;
    if (FVAL(info->sound.last_goom_p) > 0.8)
      FVAL(data->factor_p) += FVAL(info->sound.goom_power_p) * INCREASE_RATE;
    FVAL(data->factor_p) *= DECAY_RATE;

    rotate_param = FVAL(info->sound.last_goom_p);
    if (rotate_param < 0.0)
        rotate_param = 0.0;
    rotate_param += FVAL(info->sound.goom_power_p);
    
    rotate_coef  = 4.0 + FVAL(info->sound.goom_power_p) * 6.0;
    data->ftheta = (data->ftheta + rotate_coef * sin(rotate_param * 6.3));
    data->theta  = ((unsigned int)data->ftheta) % NB_THETA;
    data->visibility   = (cos(fcycle * 0.001 + 1.5) * sin(fcycle * 0.008) + cos(fcycle * 0.011 + 5.0) - 0.8 + info->sound.speedvar) * 1.5;
    if (data->visibility < 0.0) data->visibility = 0.0;
    data->factor_p.change_listener(&data->factor_p);
  }

  if (data->visibility < 0.01) {
    switch (goom_irand(info->gRandom, 300))
    {
      case 1:
        set_motif(data, CONV_MOTIF1); data->inverse_motif = 1; break;
      case 2:
        set_motif(data, CONV_MOTIF2); data->inverse_motif = 0; break;
    }
  }

  if ((ff > 0.98f) && (ff < 1.02f))
    memcpy(dest, src, info->screen.size * sizeof(Pixel));
  else
    create_output_with_brightness(_this,src,dest,info,iff);
/*
//   Benching suite...
   {
    uint64_t before, after;
    double   timed;
    static double stimed = 10000.0;
    before = GetTick();
    data->visibility = 1.0;
    create_output_with_brightness(_this,src,dest,info,iff);
    after  = GetTick();
    timed = (double)((after-before) / info->screen.size);
    if (timed < stimed) {
      stimed = timed;
      printf ("CLK = %3.0f CPP\n", stimed);
    }
  }
*/
}

VisualFX convolve_create(void) {
  VisualFX vfx = {
      init: convolve_init,
      free: convolve_free,
      apply: convolve_apply,
      fx_data: 0
  };
  return vfx;
}
