/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: psychoacoustics not including preecho
 last mod: $Id: psy.c,v 1.74 2002/07/13 10:18:33 giles Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "vorbis/codec.h"
#include "codec_internal.h"

#include "masking.h"
#include "psy.h"
#include "os.h"
#include "lpc.h"
#include "smallft.h"
#include "scales.h"
#include "misc.h"

#define NEGINF -9999.f
static double stereo_threshholds[]={0.0, .5, 1.0, 1.5, 2.5, 4.5, 8.5, 16.5, 9e10};

vorbis_look_psy_global *_vp_global_look(vorbis_info *vi){
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=&ci->psy_g_param;
  vorbis_look_psy_global *look=_ogg_calloc(1,sizeof(*look));

  look->channels=vi->channels;

  look->ampmax=-9999.;
  look->gi=gi;
  return(look);
}

void _vp_global_free(vorbis_look_psy_global *look){
  if(look){
    memset(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

void _vi_gpsy_free(vorbis_info_psy_global *i){
  if(i){
    memset(i,0,sizeof(*i));
    _ogg_free(i);
  }
}

void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(*i));
    _ogg_free(i);
  }
}

static void min_curve(float *c,
		       float *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]<c[i])c[i]=c2[i];
}
static void max_curve(float *c,
		       float *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]>c[i])c[i]=c2[i];
}

static void attenuate_curve(float *c,float att){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]+=att;
}

static float ***setup_tone_curves(float curveatt_dB[P_BANDS],float binHz,int n,
				  float center_boost, float center_decay_rate){
  int i,j,k,m;
  float ath[EHMER_MAX];
  float workc[P_BANDS][P_LEVELS][EHMER_MAX];
  float athc[P_LEVELS][EHMER_MAX];
  float *brute_buffer=alloca(n*sizeof(*brute_buffer));

  float ***ret=_ogg_malloc(sizeof(*ret)*P_BANDS);

  memset(workc,0,sizeof(workc));

  for(i=0;i<P_BANDS;i++){
    /* we add back in the ATH to avoid low level curves falling off to
       -infinity and unnecessarily cutting off high level curves in the
       curve limiting (last step). */

    /* A half-band's settings must be valid over the whole band, and
       it's better to mask too little than too much */  
    int ath_offset=i*4;
    for(j=0;j<EHMER_MAX;j++){
      float min=999.;
      for(k=0;k<4;k++)
	if(j+k+ath_offset<MAX_ATH){
	  if(min>ATH[j+k+ath_offset])min=ATH[j+k+ath_offset];
	}else{
	  if(min>ATH[MAX_ATH-1])min=ATH[MAX_ATH-1];
	}
      ath[j]=min;
    }

    /* copy curves into working space, replicate the 50dB curve to 30
       and 40, replicate the 100dB curve to 110 */
    for(j=0;j<6;j++)
      memcpy(workc[i][j+2],tonemasks[i][j],EHMER_MAX*sizeof(*tonemasks[i][j]));
    memcpy(workc[i][0],tonemasks[i][0],EHMER_MAX*sizeof(*tonemasks[i][0]));
    memcpy(workc[i][1],tonemasks[i][0],EHMER_MAX*sizeof(*tonemasks[i][0]));
    
    /* apply centered curve boost/decay */
    for(j=0;j<P_LEVELS;j++){
      for(k=0;k<EHMER_MAX;k++){
	float adj=center_boost+abs(EHMER_OFFSET-k)*center_decay_rate;
	if(adj<0. && center_boost>0)adj=0.;
	if(adj>0. && center_boost<0)adj=0.;
	workc[i][j][k]+=adj;
      }
    }

    /* normalize curves so the driving amplitude is 0dB */
    /* make temp curves with the ATH overlayed */
    for(j=0;j<P_LEVELS;j++){
      attenuate_curve(workc[i][j],curveatt_dB[i]+100.-(j<2?2:j)*10.-P_LEVEL_0);
      memcpy(athc[j],ath,EHMER_MAX*sizeof(**athc));
      attenuate_curve(athc[j],+100.-j*10.f-P_LEVEL_0);
      max_curve(athc[j],workc[i][j]);
    }

    /* Now limit the louder curves.
       
       the idea is this: We don't know what the playback attenuation
       will be; 0dB SL moves every time the user twiddles the volume
       knob. So that means we have to use a single 'most pessimal' curve
       for all masking amplitudes, right?  Wrong.  The *loudest* sound
       can be in (we assume) a range of ...+100dB] SL.  However, sounds
       20dB down will be in a range ...+80], 40dB down is from ...+60],
       etc... */
    
    for(j=1;j<P_LEVELS;j++){
      min_curve(athc[j],athc[j-1]);
      min_curve(workc[i][j],athc[j]);
    }
  }

  for(i=0;i<P_BANDS;i++){
    int hi_curve,lo_curve,bin;
    ret[i]=_ogg_malloc(sizeof(**ret)*P_LEVELS);

    /* low frequency curves are measured with greater resolution than
       the MDCT/FFT will actually give us; we want the curve applied
       to the tone data to be pessimistic and thus apply the minimum
       masking possible for a given bin.  That means that a single bin
       could span more than one octave and that the curve will be a
       composite of multiple octaves.  It also may mean that a single
       bin may span > an eighth of an octave and that the eighth
       octave values may also be composited. */
    
    /* which octave curves will we be compositing? */
    bin=floor(fromOC(i*.5)/binHz);
    lo_curve=  ceil(toOC(bin*binHz+1)*2);
    hi_curve=  floor(toOC((bin+1)*binHz)*2);
    if(lo_curve>i)lo_curve=i;
    if(lo_curve<0)lo_curve=0;
    if(hi_curve>=P_BANDS)hi_curve=P_BANDS-1;

    for(m=0;m<P_LEVELS;m++){
      ret[i][m]=_ogg_malloc(sizeof(***ret)*(EHMER_MAX+2));
      
      for(j=0;j<n;j++)brute_buffer[j]=999.;
      
      /* render the curve into bins, then pull values back into curve.
	 The point is that any inherent subsampling aliasing results in
	 a safe minimum */
      for(k=lo_curve;k<=hi_curve;k++){
	int l=0;

	for(j=0;j<EHMER_MAX;j++){
	  int lo_bin= fromOC(j*.125+k*.5-2.0625)/binHz;
	  int hi_bin= fromOC(j*.125+k*.5-1.9375)/binHz+1;
	  
	  if(lo_bin<0)lo_bin=0;
	  if(lo_bin>n)lo_bin=n;
	  if(lo_bin<l)l=lo_bin;
	  if(hi_bin<0)hi_bin=0;
	  if(hi_bin>n)hi_bin=n;

	  for(;l<hi_bin && l<n;l++)
	    if(brute_buffer[l]>workc[k][m][j])
	      brute_buffer[l]=workc[k][m][j];
	}

	for(;l<n;l++)
	  if(brute_buffer[l]>workc[k][m][EHMER_MAX-1])
	    brute_buffer[l]=workc[k][m][EHMER_MAX-1];

      }

      /* be equally paranoid about being valid up to next half ocatve */
      if(i+1<P_BANDS){
	int l=0;
	k=i+1;
	for(j=0;j<EHMER_MAX;j++){
	  int lo_bin= fromOC(j*.125+i*.5-2.0625)/binHz;
	  int hi_bin= fromOC(j*.125+i*.5-1.9375)/binHz+1;
	  
	  if(lo_bin<0)lo_bin=0;
	  if(lo_bin>n)lo_bin=n;
	  if(lo_bin<l)l=lo_bin;
	  if(hi_bin<0)hi_bin=0;
	  if(hi_bin>n)hi_bin=n;

	  for(;l<hi_bin && l<n;l++)
	    if(brute_buffer[l]>workc[k][m][j])
	      brute_buffer[l]=workc[k][m][j];
	}

	for(;l<n;l++)
	  if(brute_buffer[l]>workc[k][m][EHMER_MAX-1])
	    brute_buffer[l]=workc[k][m][EHMER_MAX-1];

      }


      for(j=0;j<EHMER_MAX;j++){
	int bin=fromOC(j*.125+i*.5-2.)/binHz;
	if(bin<0){
	  ret[i][m][j+2]=-999.;
	}else{
	  if(bin>=n){
	    ret[i][m][j+2]=-999.;
	  }else{
	    ret[i][m][j+2]=brute_buffer[bin];
	  }
	}
      }

      /* add fenceposts */
      for(j=0;j<EHMER_OFFSET;j++)
	if(ret[i][m][j+2]>-200.f)break;  
      ret[i][m][0]=j;
      
      for(j=EHMER_MAX-1;j>EHMER_OFFSET+1;j--)
	if(ret[i][m][j+2]>-200.f)
	  break;
      ret[i][m][1]=j;

    }
  }

  return(ret);
}

void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,
		  vorbis_info_psy_global *gi,int n,long rate){
  long i,j,lo=-99,hi=0;
  long maxoc;
  memset(p,0,sizeof(*p));

  p->eighth_octave_lines=gi->eighth_octave_lines;
  p->shiftoc=rint(log(gi->eighth_octave_lines*8.f)/log(2.f))-1;

  p->firstoc=toOC(.25f*rate*.5/n)*(1<<(p->shiftoc+1))-gi->eighth_octave_lines;
  maxoc=toOC((n+.25f)*rate*.5/n)*(1<<(p->shiftoc+1))+.5f;
  p->total_octave_lines=maxoc-p->firstoc+1;
  p->ath=_ogg_malloc(n*sizeof(*p->ath));

  p->octave=_ogg_malloc(n*sizeof(*p->octave));
  p->bark=_ogg_malloc(n*sizeof(*p->bark));
  p->vi=vi;
  p->n=n;
  p->rate=rate;

  /* set up the lookups for a given blocksize and sample rate */

  for(i=0,j=0;i<MAX_ATH-1;i++){
    int endpos=rint(fromOC((i+1)*.125-2.)*2*n/rate);
    float base=ATH[i];
    if(j<endpos){
      float delta=(ATH[i+1]-base)/(endpos-j);
      for(;j<endpos && j<n;j++){
        p->ath[j]=base+100.;
        base+=delta;
      }
    }
  }

  for(i=0;i<n;i++){
    float bark=toBARK(rate/(2*n)*i); 

    for(;lo+vi->noisewindowlomin<i && 
	  toBARK(rate/(2*n)*lo)<(bark-vi->noisewindowlo);lo++);
    
    for(;hi<n && (hi<i+vi->noisewindowhimin ||
	  toBARK(rate/(2*n)*hi)<(bark+vi->noisewindowhi));hi++);
    
    p->bark[i]=(lo<<16)+hi;

  }

  for(i=0;i<n;i++)
    p->octave[i]=toOC((i+.25f)*.5*rate/n)*(1<<(p->shiftoc+1))+.5f;

  p->tonecurves=setup_tone_curves(vi->toneatt,rate*.5/n,n,
				  vi->tone_centerboost,vi->tone_decay);
  
  /* set up rolling noise median */
  p->noiseoffset=_ogg_malloc(P_NOISECURVES*sizeof(*p->noiseoffset));
  for(i=0;i<P_NOISECURVES;i++)
    p->noiseoffset[i]=_ogg_malloc(n*sizeof(**p->noiseoffset));
  
  for(i=0;i<n;i++){
    float halfoc=toOC((i+.5)*rate/(2.*n))*2.;
    int inthalfoc;
    float del;
    
    if(halfoc<0)halfoc=0;
    if(halfoc>=P_BANDS-1)halfoc=P_BANDS-1;
    inthalfoc=(int)halfoc;
    del=halfoc-inthalfoc;
    
    for(j=0;j<P_NOISECURVES;j++)
      p->noiseoffset[j][i]=
	p->vi->noiseoff[j][inthalfoc]*(1.-del) + 
	p->vi->noiseoff[j][inthalfoc+1]*del;
    
  }
#if 0
  {
    static int ls=0;
    _analysis_output_always("noiseoff0",ls,p->noiseoffset[0],n,1,0,0);
    _analysis_output_always("noiseoff1",ls,p->noiseoffset[1],n,1,0,0);
    _analysis_output_always("noiseoff2",ls++,p->noiseoffset[2],n,1,0,0);
  }
#endif
}

void _vp_psy_clear(vorbis_look_psy *p){
  int i,j;
  if(p){
    if(p->ath)_ogg_free(p->ath);
    if(p->octave)_ogg_free(p->octave);
    if(p->bark)_ogg_free(p->bark);
    if(p->tonecurves){
      for(i=0;i<P_BANDS;i++){
	for(j=0;j<P_LEVELS;j++){
	  _ogg_free(p->tonecurves[i][j]);
	}
	_ogg_free(p->tonecurves[i]);
      }
      _ogg_free(p->tonecurves);
    }
    if(p->noiseoffset){
      for(i=0;i<P_NOISECURVES;i++){
        _ogg_free(p->noiseoffset[i]);
      }
      _ogg_free(p->noiseoffset);
    }
    memset(p,0,sizeof(*p));
  }
}

/* octave/(8*eighth_octave_lines) x scale and dB y scale */
static void seed_curve(float *seed,
		       const float **curves,
		       float amp,
		       int oc, int n,
		       int linesper,float dBoffset){
  int i,post1;
  int seedptr;
  const float *posts,*curve;

  int choice=(int)((amp+dBoffset-P_LEVEL_0)*.1f);
  choice=max(choice,0);
  choice=min(choice,P_LEVELS-1);
  posts=curves[choice];
  curve=posts+2;
  post1=(int)posts[1];
  seedptr=oc+(posts[0]-EHMER_OFFSET)*linesper-(linesper>>1);

  for(i=posts[0];i<post1;i++){
    if(seedptr>0){
      float lin=amp+curve[i];
      if(seed[seedptr]<lin)seed[seedptr]=lin;
    }
    seedptr+=linesper;
    if(seedptr>=n)break;
  }
}

static void seed_loop(vorbis_look_psy *p,
		      const float ***curves,
		      const float *f, 
		      const float *flr,
		      float *seed,
		      float specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  float dBoffset=vi->max_curve_dB-specmax;

  /* prime the working vector with peak values */

  for(i=0;i<n;i++){
    float max=f[i];
    long oc=p->octave[i];
    while(i+1<n && p->octave[i+1]==oc){
      i++;
      if(f[i]>max)max=f[i];
    }
    
    if(max+6.f>flr[i]){
      oc=oc>>p->shiftoc;

      if(oc>=P_BANDS)oc=P_BANDS-1;
      if(oc<0)oc=0;

      seed_curve(seed,
		 curves[oc],
		 max,
		 p->octave[i]-p->firstoc,
		 p->total_octave_lines,
		 p->eighth_octave_lines,
		 dBoffset);
    }
  }
}

static void seed_chase(float *seeds, int linesper, long n){
  long  *posstack=alloca(n*sizeof(*posstack));
  float *ampstack=alloca(n*sizeof(*ampstack));
  long   stack=0;
  long   pos=0;
  long   i;

  for(i=0;i<n;i++){
    if(stack<2){
      posstack[stack]=i;
      ampstack[stack++]=seeds[i];
    }else{
      while(1){
	if(seeds[i]<ampstack[stack-1]){
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;
	}else{
	  if(i<posstack[stack-1]+linesper){
	    if(stack>1 && ampstack[stack-1]<=ampstack[stack-2] &&
	       i<posstack[stack-2]+linesper){
	      /* we completely overlap, making stack-1 irrelevant.  pop it */
	      stack--;
	      continue;
	    }
	  }
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;

	}
      }
    }
  }

  /* the stack now contains only the positions that are relevant. Scan
     'em straight through */

  for(i=0;i<stack;i++){
    long endpos;
    if(i<stack-1 && ampstack[i+1]>ampstack[i]){
      endpos=posstack[i+1];
    }else{
      endpos=posstack[i]+linesper+1; /* +1 is important, else bin 0 is
					discarded in short frames */
    }
    if(endpos>n)endpos=n;
    for(;pos<endpos;pos++)
      seeds[pos]=ampstack[i];
  }
  
  /* there.  Linear time.  I now remember this was on a problem set I
     had in Grad Skool... I didn't solve it at the time ;-) */

}

/* bleaugh, this is more complicated than it needs to be */
#include<stdio.h>
static void max_seeds(vorbis_look_psy *p,
		      float *seed,
		      float *flr){
  long   n=p->total_octave_lines;
  int    linesper=p->eighth_octave_lines;
  long   linpos=0;
  long   pos;

  seed_chase(seed,linesper,n); /* for masking */
 
  pos=p->octave[0]-p->firstoc-(linesper>>1);

  while(linpos+1<p->n){
    float minV=seed[pos];
    long end=((p->octave[linpos]+p->octave[linpos+1])>>1)-p->firstoc;
    if(minV>p->vi->tone_abs_limit)minV=p->vi->tone_abs_limit;
    while(pos+1<=end){
      pos++;
      if((seed[pos]>NEGINF && seed[pos]<minV) || minV==NEGINF)
	minV=seed[pos];
    }
    
    end=pos+p->firstoc;
    for(;linpos<p->n && p->octave[linpos]<=end;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }
  
  {
    float minV=seed[p->total_octave_lines-1];
    for(;linpos<p->n;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }
  
}

static void bark_noise_hybridmp(int n,const long *b,
                                const float *f,
                                float *noise,
                                const float offset,
                                const int fixed){
  
  float *N=alloca((n+1)*sizeof(*N));
  float *X=alloca((n+1)*sizeof(*N));
  float *XX=alloca((n+1)*sizeof(*N));
  float *Y=alloca((n+1)*sizeof(*N));
  float *XY=alloca((n+1)*sizeof(*N));

  float tN, tX, tXX, tY, tXY;
  float fi;
  int i;

  int lo, hi;
  float R, A, B, D;
  
  tN = tX = tXX = tY = tXY = 0.f;
  for (i = 0, fi = 0.f; i < n; i++, fi += 1.f) {
    float w, x, y;
    
    x = fi;
    y = f[i] + offset;
    if (y < 1.f) y = 1.f;
    w = y * y;
    N[i] = tN;
    X[i] = tX;
    XX[i] = tXX;
    Y[i] = tY;
    XY[i] = tXY;
    tN += w;
    tX += w * x;
    tXX += w * x * x;
    tY += w * y;
    tXY += w * x * y;
  }
  N[i] = tN;
  X[i] = tX;
  XX[i] = tXX;
  Y[i] = tY;
  XY[i] = tXY;
  
  for (i = 0, fi = 0.f;; i++, fi += 1.f) {
    
    lo = b[i] >> 16;
    if( lo>=0 ) break;
    hi = b[i] & 0xffff;
    
    tN = N[hi] + N[-lo];
    tX = X[hi] - X[-lo];
    tXX = XX[hi] + XX[-lo];
    tY = Y[hi] + Y[-lo];    
    tXY = XY[hi] - XY[-lo];
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + fi * B) / D;
    if (R < 0.f)
      R = 0.f;
    
    noise[i] = R - offset;
  }
  
  for ( ; hi < n; i++, fi += 1.f) {
    
    lo = b[i] >> 16;
    hi = b[i] & 0xffff;
    
    tN = N[hi] - N[lo];
    tX = X[hi] - X[lo];
    tXX = XX[hi] - XX[lo];
    tY = Y[hi] - Y[lo];
    tXY = XY[hi] - XY[lo];
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + fi * B) / D;
    if (R < 0.f) R = 0.f;
    
    noise[i] = R - offset;
  }
  for ( ; i < n; i++, fi += 1.f) {
    
    R = (A + fi * B) / D;
    if (R < 0.f) R = 0.f;
    
    noise[i] = R - offset;
  }
  
  if (fixed <= 0) return;
  
  for (i = 0, fi = 0.f; i < (fixed + 1) / 2; i++, fi += 1.f) {
    hi = i + fixed / 2;
    lo = hi - fixed;
    
    tN = N[hi] + N[-lo];
    tX = X[hi] - X[-lo];
    tXX = XX[hi] + XX[-lo];
    tY = Y[hi] + Y[-lo];
    tXY = XY[hi] - XY[-lo];
    
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + fi * B) / D;

    if (R > 0.f && R - offset < noise[i]) noise[i] = R - offset;
  }
  for ( ; hi < n; i++, fi += 1.f) {
    
    hi = i + fixed / 2;
    lo = hi - fixed;
    
    tN = N[hi] - N[lo];
    tX = X[hi] - X[lo];
    tXX = XX[hi] - XX[lo];
    tY = Y[hi] - Y[lo];
    tXY = XY[hi] - XY[lo];
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + fi * B) / D;
    
    if (R > 0.f && R - offset < noise[i]) noise[i] = R - offset;
  }
  for ( ; i < n; i++, fi += 1.f) {
    R = (A + fi * B) / D;
    if (R > 0.f && R - offset < noise[i]) noise[i] = R - offset;
  }
}

static float FLOOR1_fromdB_INV_LOOKUP[256]={
  0.F, 8.81683e+06F, 8.27882e+06F, 7.77365e+06F, 
  7.29930e+06F, 6.85389e+06F, 6.43567e+06F, 6.04296e+06F, 
  5.67422e+06F, 5.32798e+06F, 5.00286e+06F, 4.69759e+06F, 
  4.41094e+06F, 4.14178e+06F, 3.88905e+06F, 3.65174e+06F, 
  3.42891e+06F, 3.21968e+06F, 3.02321e+06F, 2.83873e+06F, 
  2.66551e+06F, 2.50286e+06F, 2.35014e+06F, 2.20673e+06F, 
  2.07208e+06F, 1.94564e+06F, 1.82692e+06F, 1.71544e+06F, 
  1.61076e+06F, 1.51247e+06F, 1.42018e+06F, 1.33352e+06F, 
  1.25215e+06F, 1.17574e+06F, 1.10400e+06F, 1.03663e+06F, 
  973377.F, 913981.F, 858210.F, 805842.F, 
  756669.F, 710497.F, 667142.F, 626433.F, 
  588208.F, 552316.F, 518613.F, 486967.F, 
  457252.F, 429351.F, 403152.F, 378551.F, 
  355452.F, 333762.F, 313396.F, 294273.F, 
  276316.F, 259455.F, 243623.F, 228757.F, 
  214798.F, 201691.F, 189384.F, 177828.F, 
  166977.F, 156788.F, 147221.F, 138237.F, 
  129802.F, 121881.F, 114444.F, 107461.F, 
  100903.F, 94746.3F, 88964.9F, 83536.2F, 
  78438.8F, 73652.5F, 69158.2F, 64938.1F, 
  60975.6F, 57254.9F, 53761.2F, 50480.6F, 
  47400.3F, 44507.9F, 41792.0F, 39241.9F, 
  36847.3F, 34598.9F, 32487.7F, 30505.3F, 
  28643.8F, 26896.0F, 25254.8F, 23713.7F, 
  22266.7F, 20908.0F, 19632.2F, 18434.2F, 
  17309.4F, 16253.1F, 15261.4F, 14330.1F, 
  13455.7F, 12634.6F, 11863.7F, 11139.7F, 
  10460.0F, 9821.72F, 9222.39F, 8659.64F, 
  8131.23F, 7635.06F, 7169.17F, 6731.70F, 
  6320.93F, 5935.23F, 5573.06F, 5232.99F, 
  4913.67F, 4613.84F, 4332.30F, 4067.94F, 
  3819.72F, 3586.64F, 3367.78F, 3162.28F, 
  2969.31F, 2788.13F, 2617.99F, 2458.24F, 
  2308.24F, 2167.39F, 2035.14F, 1910.95F, 
  1794.35F, 1684.85F, 1582.04F, 1485.51F, 
  1394.86F, 1309.75F, 1229.83F, 1154.78F, 
  1084.32F, 1018.15F, 956.024F, 897.687F, 
  842.910F, 791.475F, 743.179F, 697.830F, 
  655.249F, 615.265F, 577.722F, 542.469F, 
  509.367F, 478.286F, 449.101F, 421.696F, 
  395.964F, 371.803F, 349.115F, 327.812F, 
  307.809F, 289.026F, 271.390F, 254.830F, 
  239.280F, 224.679F, 210.969F, 198.096F, 
  186.008F, 174.658F, 164.000F, 153.993F, 
  144.596F, 135.773F, 127.488F, 119.708F, 
  112.404F, 105.545F, 99.1046F, 93.0572F, 
  87.3788F, 82.0469F, 77.0404F, 72.3394F, 
  67.9252F, 63.7804F, 59.8885F, 56.2341F, 
  52.8027F, 49.5807F, 46.5553F, 43.7144F, 
  41.0470F, 38.5423F, 36.1904F, 33.9821F, 
  31.9085F, 29.9614F, 28.1332F, 26.4165F, 
  24.8045F, 23.2910F, 21.8697F, 20.5352F, 
  19.2822F, 18.1056F, 17.0008F, 15.9634F, 
  14.9893F, 14.0746F, 13.2158F, 12.4094F, 
  11.6522F, 10.9411F, 10.2735F, 9.64662F, 
  9.05798F, 8.50526F, 7.98626F, 7.49894F, 
  7.04135F, 6.61169F, 6.20824F, 5.82941F, 
  5.47370F, 5.13970F, 4.82607F, 4.53158F, 
  4.25507F, 3.99542F, 3.75162F, 3.52269F, 
  3.30774F, 3.10590F, 2.91638F, 2.73842F, 
  2.57132F, 2.41442F, 2.26709F, 2.12875F, 
  1.99885F, 1.87688F, 1.76236F, 1.65482F, 
  1.55384F, 1.45902F, 1.36999F, 1.28640F, 
  1.20790F, 1.13419F, 1.06499F, 1.F
};

void _vp_remove_floor(vorbis_look_psy *p,
		      float *mdct,
		      int *codedflr,
		      float *residue,
		      int sliding_lowpass){ 

  int i,n=p->n;
 
  if(sliding_lowpass>n)sliding_lowpass=n;
  
  for(i=0;i<sliding_lowpass;i++){
    residue[i]=
      mdct[i]*FLOOR1_fromdB_INV_LOOKUP[codedflr[i]];
  }

  for(;i<n;i++)
    residue[i]=0.;
}

void _vp_noisemask(vorbis_look_psy *p,
		   float *logmdct, 
		   float *logmask){

  int i,n=p->n;
  float *work=alloca(n*sizeof(*work));

  bark_noise_hybridmp(n,p->bark,logmdct,logmask,
		      140.,-1);

  for(i=0;i<n;i++)work[i]=logmdct[i]-logmask[i];

  bark_noise_hybridmp(n,p->bark,work,logmask,0.,
		      p->vi->noisewindowfixed);

  for(i=0;i<n;i++)work[i]=logmdct[i]-work[i];
  
#if 0
  {
    static int seq=0;

    float work2[n];
    for(i=0;i<n;i++){
      work2[i]=logmask[i]+work[i];
    }
    
    if(seq&1)
      _analysis_output("medianR",seq/2,work,n,1,0,0);
    else
      _analysis_output("medianL",seq/2,work,n,1,0,0);
    
    if(seq&1)
      _analysis_output("envelopeR",seq/2,work2,n,1,0,0);
    else
      _analysis_output("enveloperL",seq/2,work2,n,1,0,0);
    seq++;
  }
#endif

  for(i=0;i<n;i++){
    int dB=logmask[i]+.5;
    if(dB>=NOISE_COMPAND_LEVELS)dB=NOISE_COMPAND_LEVELS-1;
    logmask[i]= work[i]+p->vi->noisecompand[dB];
  }

}

void _vp_tonemask(vorbis_look_psy *p,
		  float *logfft,
		  float *logmask,
		  float global_specmax,
		  float local_specmax){

  int i,n=p->n;

  float *seed=alloca(sizeof(*seed)*p->total_octave_lines);
  float att=local_specmax+p->vi->ath_adjatt;
  for(i=0;i<p->total_octave_lines;i++)seed[i]=NEGINF;
  
  /* set the ATH (floating below localmax, not global max by a
     specified att) */
  if(att<p->vi->ath_maxatt)att=p->vi->ath_maxatt;
  
  for(i=0;i<n;i++)
    logmask[i]=p->ath[i]+att;

  /* tone masking */
  seed_loop(p,(const float ***)p->tonecurves,logfft,logmask,seed,global_specmax);
  max_seeds(p,seed,logmask);

}

void _vp_offset_and_mix(vorbis_look_psy *p,
			float *noise,
			float *tone,
			int offset_select,
			float *logmask){
  int i,n=p->n;
  float toneatt=p->vi->tone_masteratt[offset_select];
  
  for(i=0;i<n;i++){
    float val= noise[i]+p->noiseoffset[offset_select][i];
    if(val>p->vi->noisemaxsupp)val=p->vi->noisemaxsupp;
    logmask[i]=max(val,tone[i]+toneatt);
  }
}

float _vp_ampmax_decay(float amp,vorbis_dsp_state *vd){
  vorbis_info *vi=vd->vi;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=&ci->psy_g_param;

  int n=ci->blocksizes[vd->W]/2;
  float secs=(float)n/vi->rate;

  amp+=secs*gi->ampmax_att_per_sec;
  if(amp<-9999)amp=-9999;
  return(amp);
}

static void couple_lossless(float A, float B, 
			    float *qA, float *qB){
  int test1=fabs(*qA)>fabs(*qB);
  test1-= fabs(*qA)<fabs(*qB);
  
  if(!test1)test1=((fabs(A)>fabs(B))<<1)-1;
  if(test1==1){
    *qB=(*qA>0.f?*qA-*qB:*qB-*qA);
  }else{
    float temp=*qB;  
    *qB=(*qB>0.f?*qA-*qB:*qB-*qA);
    *qA=temp;
  }

  if(*qB>fabs(*qA)*1.9999f){
    *qB= -fabs(*qA)*2.f;
    *qA= -*qA;
  }
}

static float hypot_lookup[32]={
  -0.009935, -0.011245, -0.012726, -0.014397, 
  -0.016282, -0.018407, -0.020800, -0.023494, 
  -0.026522, -0.029923, -0.033737, -0.038010, 
  -0.042787, -0.048121, -0.054064, -0.060671, 
  -0.068000, -0.076109, -0.085054, -0.094892, 
  -0.105675, -0.117451, -0.130260, -0.144134, 
  -0.159093, -0.175146, -0.192286, -0.210490, 
  -0.229718, -0.249913, -0.271001, -0.292893};

static void precomputed_couple_point(float premag,
				     int floorA,int floorB,
				     float *mag, float *ang){
  
  int test=(floorA>floorB)-1;
  int offset=31-abs(floorA-floorB);
  float floormag=hypot_lookup[((offset<0)-1)&offset]+1.f;

  floormag*=FLOOR1_fromdB_INV_LOOKUP[(floorB&test)|(floorA&(~test))];

  *mag=premag*floormag;
  *ang=0.f;
}

/* just like below, this is currently set up to only do
   single-step-depth coupling.  Otherwise, we'd have to do more
   copying (which will be inevitable later) */

/* doing the real circular magnitude calculation is audibly superior
   to (A+B)/sqrt(2) */
static float dipole_hypot(float a, float b){
  if(a>0.){
    if(b>0.)return sqrt(a*a+b*b);
    if(a>-b)return sqrt(a*a-b*b);
    return -sqrt(b*b-a*a);
  }
  if(b<0.)return -sqrt(a*a+b*b);
  if(-a>b)return -sqrt(a*a-b*b);
  return sqrt(b*b-a*a);
}
static float round_hypot(float a, float b){
  if(a>0.){
    if(b>0.)return sqrt(a*a+b*b);
    if(a>-b)return sqrt(a*a+b*b);
    return -sqrt(b*b+a*a);
  }
  if(b<0.)return -sqrt(a*a+b*b);
  if(-a>b)return -sqrt(a*a+b*b);
  return sqrt(b*b+a*a);
}

/* revert to round hypot for now */
float **_vp_quantize_couple_memo(vorbis_block *vb,
				 vorbis_info_psy_global *g,
				 vorbis_look_psy *p,
				 vorbis_info_mapping0 *vi,
				 float **mdct){
  
  int i,j,n=p->n;
  float **ret=_vorbis_block_alloc(vb,vi->coupling_steps*sizeof(*ret));
  int limit=g->coupling_pointlimit[p->vi->blockflag][PACKETBLOBS/2];
  
  for(i=0;i<vi->coupling_steps;i++){
    float *mdctM=mdct[vi->coupling_mag[i]];
    float *mdctA=mdct[vi->coupling_ang[i]];
    ret[i]=_vorbis_block_alloc(vb,n*sizeof(**ret));
    for(j=0;j<limit;j++)
      ret[i][j]=dipole_hypot(mdctM[j],mdctA[j]);
    for(;j<n;j++)
      ret[i][j]=round_hypot(mdctM[j],mdctA[j]);
  }

  return(ret);
}

/* this is for per-channel noise normalization */
static int apsort(const void *a, const void *b){
  if(fabs(**(float **)a)>fabs(**(float **)b))return -1;
  return 1;
}

int **_vp_quantize_couple_sort(vorbis_block *vb,
			       vorbis_look_psy *p,
			       vorbis_info_mapping0 *vi,
			       float **mags){


  if(p->vi->normal_point_p){
    int i,j,k,n=p->n;
    int **ret=_vorbis_block_alloc(vb,vi->coupling_steps*sizeof(*ret));
    int partition=p->vi->normal_partition;
    float **work=alloca(sizeof(*work)*partition);
    
    for(i=0;i<vi->coupling_steps;i++){
      ret[i]=_vorbis_block_alloc(vb,n*sizeof(**ret));
      
      for(j=0;j<n;j+=partition){
	for(k=0;k<partition;k++)work[k]=mags[i]+k+j;
	qsort(work,partition,sizeof(*work),apsort);
	for(k=0;k<partition;k++)ret[i][k+j]=work[k]-mags[i];
      }
    }
    return(ret);
  }
  return(NULL);
}

void _vp_noise_normalize_sort(vorbis_look_psy *p,
			      float *magnitudes,int *sortedindex){
  int i,j,n=p->n;
  vorbis_info_psy *vi=p->vi;
  int partition=vi->normal_partition;
  float **work=alloca(sizeof(*work)*partition);
  int start=vi->normal_start;

  for(j=start;j<n;j+=partition){
    if(j+partition>n)partition=n-j;
    for(i=0;i<partition;i++)work[i]=magnitudes+i+j;
    qsort(work,partition,sizeof(*work),apsort);
    for(i=0;i<partition;i++){
      sortedindex[i+j-start]=work[i]-magnitudes;
    }
  }
}

void _vp_noise_normalize(vorbis_look_psy *p,
			 float *in,float *out,int *sortedindex){
  int flag=0,i,j=0,n=p->n;
  vorbis_info_psy *vi=p->vi;
  int partition=vi->normal_partition;
  int start=vi->normal_start;

  if(start>n)start=n;

  if(vi->normal_channel_p){
    for(;j<start;j++)
      out[j]=rint(in[j]);
    
    for(;j+partition<=n;j+=partition){
      float acc=0.;
      int k;
      
      for(i=j;i<j+partition;i++)
	acc+=in[i]*in[i];
      
      for(i=0;i<partition;i++){
	k=sortedindex[i+j-start];
	
	if(in[k]*in[k]>=.25f){
	  out[k]=rint(in[k]);
	  acc-=in[k]*in[k];
	  flag=1;
	}else{
	  if(acc<vi->normal_thresh)break;
	  out[k]=unitnorm(in[k]);
	  acc-=1.;
	}
      }
      
      for(;i<partition;i++){
	k=sortedindex[i+j-start];
	out[k]=0.;
      }
    }
  }
  
  for(;j<n;j++)
    out[j]=rint(in[j]);
  
}

void _vp_couple(int blobno,
		vorbis_info_psy_global *g,
		vorbis_look_psy *p,
		vorbis_info_mapping0 *vi,
		float **res,
		float **mag_memo,
		int   **mag_sort,
		int   **ifloor,
		int   *nonzero,
		int  sliding_lowpass){

  int i,j,k,n=p->n;

  /* perform any requested channel coupling */
  /* point stereo can only be used in a first stage (in this encoder)
     because of the dependency on floor lookups */
  for(i=0;i<vi->coupling_steps;i++){

    /* once we're doing multistage coupling in which a channel goes
       through more than one coupling step, the floor vector
       magnitudes will also have to be recalculated an propogated
       along with PCM.  Right now, we're not (that will wait until 5.1
       most likely), so the code isn't here yet. The memory management
       here is all assuming single depth couplings anyway. */

    /* make sure coupling a zero and a nonzero channel results in two
       nonzero channels. */
    if(nonzero[vi->coupling_mag[i]] ||
       nonzero[vi->coupling_ang[i]]){
     

      float *rM=res[vi->coupling_mag[i]];
      float *rA=res[vi->coupling_ang[i]];
      float *qM=rM+n;
      float *qA=rA+n;
      int *floorM=ifloor[vi->coupling_mag[i]];
      int *floorA=ifloor[vi->coupling_ang[i]];
      float prepoint=stereo_threshholds[g->coupling_prepointamp[blobno]];
      float postpoint=stereo_threshholds[g->coupling_postpointamp[blobno]];
      int partition=(p->vi->normal_point_p?p->vi->normal_partition:p->n);
      int limit=g->coupling_pointlimit[p->vi->blockflag][blobno];
      int pointlimit=limit;

      nonzero[vi->coupling_mag[i]]=1; 
      nonzero[vi->coupling_ang[i]]=1; 

      for(j=0;j<p->n;j+=partition){
	float acc=0.f;

	for(k=0;k<partition;k++){
	  int l=k+j;

	  if(l<sliding_lowpass){
	    if((l>=limit && fabs(rM[l])<postpoint && fabs(rA[l])<postpoint) ||
	       (fabs(rM[l])<prepoint && fabs(rA[l])<prepoint)){


	      precomputed_couple_point(mag_memo[i][l],
				       floorM[l],floorA[l],
				       qM+l,qA+l);

	      if(rint(qM[l])==0.f)acc+=qM[l]*qM[l];
	    }else{
	      couple_lossless(rM[l],rA[l],qM+l,qA+l);
	    }
	  }else{
	    qM[l]=0.;
	    qA[l]=0.;
	  }
	}
	
	if(p->vi->normal_point_p){
	  for(k=0;k<partition && acc>=p->vi->normal_thresh;k++){
	    int l=mag_sort[i][j+k];
	    if(l<sliding_lowpass && l>=pointlimit && rint(qM[l])==0.f){
	      qM[l]=unitnorm(qM[l]);
	      acc-=1.f;
	    }
	  } 
	}
      }
    }
  }
}

