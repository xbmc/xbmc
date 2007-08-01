// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
/*
 * File:	ximaj2k.cpp
 * Purpose:	Platform Independent J2K Image Class Loader and Writer
 * 12/Jul/2002 <ing.davide.pizzolato@libero.it>
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximaj2k.h"

#if CXIMAGE_SUPPORT_J2K

#define CEILDIV(a,b) ((a+b-1)/b)

////////////////////////////////////////////////////////////////////////////////
bool CxImageJ2K::Decode(CxFile *hFile)
{
	if (hFile == NULL) return false;

  try
  {
	BYTE* src;
	long len;
	j2k_image_t *img=NULL;
	j2k_cp_t *cp=NULL;
	long i,x,y,w,h,max;

	len=hFile->Size();
	src=(BYTE*)malloc(len);
	hFile->Read(src, len, 1);

	if (!j2k_decode(src, len, &img, &cp)) {
		free(src);
		throw "failed to decode J2K image!";
	}

	free(src);

    if (img->numcomps==3 &&
		img->comps[0].dx==img->comps[1].dx &&
		img->comps[1].dx==img->comps[2].dx &&
		img->comps[0].dy==img->comps[1].dy &&
		img->comps[1].dy==img->comps[2].dy &&
		img->comps[0].prec==img->comps[1].prec &&
		img->comps[1].prec==img->comps[2].prec)
	{
        w=CEILDIV(img->x1-img->x0, img->comps[0].dx);
        h=CEILDIV(img->y1-img->y0, img->comps[0].dy);
        max=(1<<img->comps[0].prec)-1;

		Create(w,h,24,CXIMAGE_FORMAT_J2K);

		RGBQUAD c;
        for (i=0,y=0; y<h; y++) {
			for (x=0; x<w; x++,i++){
				c.rgbRed   = img->comps[0].data[i];
				c.rgbGreen = img->comps[1].data[i];
				c.rgbBlue  = img->comps[2].data[i];
				SetPixelColor(x,h-1-y,c);
			}
		}
	} else {
		int compno;
		info.nNumFrames = img->numcomps;
		if ((info.nFrame<0)||(info.nFrame>=info.nNumFrames)){
			j2k_destroy(&img,&cp);
			throw "wrong frame!";
		}
		for (compno=0; compno<=info.nFrame; compno++) {
			w=CEILDIV(img->x1-img->x0, img->comps[compno].dx);
			h=CEILDIV(img->y1-img->y0, img->comps[compno].dy);
			max=(1<<img->comps[compno].prec)-1;
			Create(w,h,8,CXIMAGE_FORMAT_J2K);
			SetGrayPalette();
			for (i=0,y=0; y<h; y++) {
				for (x=0; x<w; x++,i++){
					SetPixelIndex(x,h-1-y,img->comps[compno].data[i]);
				}
			}
		}
	}

	j2k_destroy(&img,&cp);

  } catch (char *message) {
	strncpy(info.szLastError,message,255);
	return FALSE;
  }
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImageJ2K::Encode(CxFile * hFile)
{
	if (EncodeSafeCheck(hFile)) return false;

	if (head.biClrUsed!=0 && !IsGrayScale()){
		strcpy(info.szLastError,"J2K can save only RGB or GrayScale images");
		return false;
	}

    int i,x,y;
    j2k_image_t *img;
    j2k_cp_t *cp;
    j2k_tcp_t *tcp;
    j2k_tccp_t *tccp;

	img = (j2k_image_t *)calloc(sizeof(j2k_image_t),1);
	cp = (j2k_cp_t *)calloc(sizeof(j2k_cp_t),1);

    cp->tx0=0; cp->ty0=0;
    cp->tw=1; cp->th=1;
    cp->tcps=(j2k_tcp_t*)calloc(sizeof(j2k_tcp_t),1);
    tcp=&cp->tcps[0];

	long w=head.biWidth;
	long h=head.biHeight;
 
	tcp->numlayers=1;
	for (i=0;i<tcp->numlayers;i++) tcp->rates[i]=(w*h*GetJpegQuality())/600;


    if (IsGrayScale()) {
        img->x0=0;
		img->y0=0;
		img->x1=w;
		img->y1=h;
        img->numcomps=1;
        img->comps=(j2k_comp_t*)calloc(sizeof(j2k_comp_t),1);
        img->comps[0].data=(int*)calloc(w*h*sizeof(int),1);
        img->comps[0].prec=8;
        img->comps[0].sgnd=0;
        img->comps[0].dx=1;
        img->comps[0].dy=1;
		for (i=0,y=0; y<h; y++) {
			for (x=0; x<w; x++,i++){
				img->comps[0].data[i]=GetPixelIndex(x,h-1-y);
			}
		}
    } else if (!IsIndexed()) {
        img->x0=0;
		img->y0=0;
		img->x1=w;
		img->y1=h;
        img->numcomps=3;
        img->comps=(j2k_comp_t*)calloc(img->numcomps*sizeof(j2k_comp_t),1);
        for (i=0; i<img->numcomps; i++) {
            img->comps[i].data=(int*)calloc(w*h*sizeof(int),1);
            img->comps[i].prec=8;
            img->comps[i].sgnd=0;
            img->comps[i].dx=1;
            img->comps[i].dy=1;
        }
		RGBQUAD c;
        for (i=0,y=0; y<h; y++) {
			for (x=0; x<w; x++,i++){
				c=GetPixelColor(x,h-1-y);
				img->comps[0].data[i]=c.rgbRed;
				img->comps[1].data[i]=c.rgbGreen;
				img->comps[2].data[i]=c.rgbBlue;
			}
		}
    } else {
        return 0;
    }
	
    cp->tdx=img->x1-img->x0;
	cp->tdy=img->y1-img->y0;

    tcp->csty=0;
    tcp->prg=0;
    tcp->mct=img->numcomps==3?1:0;
    tcp->tccps=(j2k_tccp_t*)calloc(img->numcomps*sizeof(j2k_tccp_t),1);

    int ir=0; /* or 1 ???*/

    for (i=0; i<img->numcomps; i++) {
        tccp=&tcp->tccps[i];
        tccp->csty=0;
        tccp->numresolutions=6;
        tccp->cblkw=6;
        tccp->cblkh=6;
        tccp->cblksty=0;
        tccp->qmfbid=ir?0:1;
        tccp->qntsty=ir?J2K_CCP_QNTSTY_SEQNT:J2K_CCP_QNTSTY_NOQNT;
        tccp->numgbits=2;
        tccp->roishift=0;
        j2k_calc_explicit_stepsizes(tccp, img->comps[i].prec);
    }

    BYTE* dest=(BYTE*)calloc(tcp->rates[tcp->numlayers-1]+2,1);
    long len = j2k_encode(img, cp, dest, tcp->rates[tcp->numlayers-1]+2);

    if (len==0) {
		strcpy(info.szLastError,"J2K failed to encode image");
    } else {
		hFile->Write(dest, len, 1);
	}
	
	free(dest);
	j2k_destroy(&img,&cp);

	return (len!=0);
}
////////////////////////////////////////////////////////////////////////////////

static const double dwt_norms_97[4][10]={
    {1.000, 1.965, 4.177, 8.403, 16.90, 33.84, 67.69, 135.3, 270.6, 540.9},
    {2.022, 3.989, 8.355, 17.04, 34.27, 68.63, 137.3, 274.6, 549.0},
    {2.022, 3.989, 8.355, 17.04, 34.27, 68.63, 137.3, 274.6, 549.0},
    {2.080, 3.865, 8.307, 17.18, 34.71, 69.59, 139.3, 278.6, 557.2}
};

////////////////////////////////////////////////////////////////////////////////
void CxImageJ2K::j2k_calc_explicit_stepsizes(j2k_tccp_t *tccp, int prec) {
    int numbands, bandno;
    numbands=3*tccp->numresolutions-2;
    for (bandno=0; bandno<numbands; bandno++) {
        double stepsize;

        int resno, level, orient, gain;
        resno=bandno==0?0:(bandno-1)/3+1;
        orient=bandno==0?0:(bandno-1)%3+1;
        level=tccp->numresolutions-1-resno;
        gain=tccp->qmfbid==0?0:(orient==0?0:(orient==1||orient==2?1:2));
        if (tccp->qntsty==J2K_CCP_QNTSTY_NOQNT) {
            stepsize=1.0;
        } else {
            double norm=dwt_norms_97[orient][level];
            stepsize=(1<<(gain+1))/norm;
        }
        j2k_encode_stepsize((int)floor(stepsize*8192.0), prec+gain, &tccp->stepsizes[bandno].expn, &tccp->stepsizes[bandno].mant);
    }
}
////////////////////////////////////////////////////////////////////////////////
void CxImageJ2K::j2k_encode_stepsize(int stepsize, int numbps, int *expn, int *mant) {
    int p, n;
    p=j2k_floorlog2(stepsize)-13;
    n=11-j2k_floorlog2(stepsize);
    *mant=(n<0?stepsize>>-n:stepsize<<n)&0x7ff;
    *expn=numbps-p;
}
////////////////////////////////////////////////////////////////////////////////
int CxImageJ2K::j2k_floorlog2(int a) {
    int l;
    for (l=0; a>1; l++) {
        a>>=1;
    }
    return l;
}
////////////////////////////////////////////////////////////////////////////////
#endif 	// CXIMAGE_SUPPORT_J2K

