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
 * File:	ximajas.cpp
 * Purpose:	Platform Independent JasPer Image Class Loader and Writer
 * 12/Apr/2003 <ing.davide.pizzolato@libero.it>
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximajas.h"

#if CXIMAGE_SUPPORT_JASPER

////////////////////////////////////////////////////////////////////////////////
bool CxImageJAS::Decode(CxFile *hFile, DWORD imagetype)
{
	if (hFile == NULL) return false;

	jas_image_t *image=0;
	jas_stream_t *in=0;
	jas_matrix_t **bufs=0;
	long i,error=0;
	//jas_setdbglevel(0);

  try
  {
	if (jas_init())
		throw "cannot initialize jasper";

	if (!(in = jas_stream_fdopen(0, "rb")))
		throw "error: cannot open standard input";

	CxFileJas src(hFile,in);

	if (!(image = jas_image_decode(in, -1, 0))) 
		throw "error: cannot load image data";

	long x,y,w,h,depth,cmptno;

	w = jas_image_cmptwidth(image,0);
	h = jas_image_cmptheight(image,0);
	depth = jas_image_cmptprec(image,0);

	if (image->numcmpts_ > 64 || image->numcmpts_ < 0)
		throw "error: too much components";

	if (depth!=1 && depth!=4 && depth!=8){
		jas_image_t *newimage;
		jas_cmprof_t *outprof;
		//jas_eprintf("forcing conversion to sRGB\n");
		if (!(outprof = jas_cmprof_createfromclrspc(JAS_CLRSPC_SRGB))) {
			throw "cannot create sRGB profile";
		}
		if (!(newimage = jas_image_chclrspc(image, outprof, JAS_CMXFORM_INTENT_PER))) {
			throw "cannot convert to sRGB";
		}
		jas_image_destroy(image);
		jas_cmprof_destroy(outprof);
		image = newimage;
	}

	bufs = (jas_matrix_t **)calloc(image->numcmpts_, sizeof(jas_matrix_t**));
	for (i = 0; i < image->numcmpts_; ++i) {
		if (!(bufs[i] = jas_matrix_create(1, w))) {
			throw "error: cannot allocate memory";
		}
	}

	if (image->numcmpts_==3 &&
		image->cmpts_[0]->width_ == image->cmpts_[1]->width_ &&
		image->cmpts_[1]->width_ == image->cmpts_[2]->width_ &&
		image->cmpts_[0]->height_ == image->cmpts_[1]->height_ &&
		image->cmpts_[1]->height_ == image->cmpts_[2]->height_ &&
		image->cmpts_[0]->prec_  == image->cmpts_[1]->prec_ &&
		image->cmpts_[1]->prec_ == image->cmpts_[2]->prec_ )
	{

		if(!Create(w,h,24,imagetype))
			throw "Can't allocate memory";

		RGBQUAD c;
        for (y=0; y<h; y++) {
			for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno) {
				jas_image_readcmpt(image, cmptno, 0, y, w, 1, bufs[cmptno]);
			}

			for (x=0; x<w; x++){
				c.rgbRed   = (jas_matrix_getv(bufs[0], x));
				c.rgbGreen = (jas_matrix_getv(bufs[1], x));
				c.rgbBlue  = (jas_matrix_getv(bufs[2], x));
				SetPixelColor(x,h-1-y,c);
			}
		}
	} else {
		info.nNumFrames = image->numcmpts_;
		if ((info.nFrame<0)||(info.nFrame>=info.nNumFrames)){
			throw "wrong frame!";
		}
		for (cmptno=0; cmptno<=info.nFrame; cmptno++) {
			w = jas_image_cmptwidth(image,cmptno);
			h = jas_image_cmptheight(image,cmptno);
			depth = jas_image_cmptprec(image,cmptno);
			if (depth>8) depth=8;
			if(!Create(w,h,depth,imagetype))
				throw "Can't allocate memory";
			SetGrayPalette();
			for (y=0; y<h; y++) {
				jas_image_readcmpt(image, cmptno, 0, y, w, 1, bufs[0]);
				for (x=0; x<w; x++){
					SetPixelIndex(x,h-1-y,(jas_matrix_getv(bufs[0], x)));
				}
			}
		}
	}


  } catch (char *message) {
	strncpy(info.szLastError,message,255);
	error = 1;
  }

	if (bufs) {
		for (i = 0; i < image->numcmpts_; ++i){	if (bufs[i]) jas_matrix_destroy(bufs[i]);}
		free(bufs);
	}
	jas_cleanup();
	if (image) jas_image_destroy(image);
	if (in) jas_stream_close(in);
	return (error==0);
}
////////////////////////////////////////////////////////////////////////////////
bool CxImageJAS::Encode(CxFile * hFile, DWORD imagetype)
{
	if (EncodeSafeCheck(hFile)) return false;

	if (head.biClrUsed!=0 && !IsGrayScale()){
		strcpy(info.szLastError,"JasPer can save only RGB or GrayScale images");
		return false;
	}

	jas_image_t *image=0;
	jas_stream_t *out=0;
	jas_matrix_t *cmpts[3];
	long x,y,yflip,error=0;
	uint_fast16_t cmptno, numcmpts;
	jas_image_cmptparm_t cmptparms[3], *cmptparm;

  try
  {

	if (jas_init())
		throw "cannot initialize jasper";

	if (!(out = jas_stream_fdopen(0, "wb")))
		throw "error: cannot open standard output";

	CxFileJas src(hFile,out);

	numcmpts = head.biClrUsed==0 ? 3 : 1;

	for (cmptno = 0, cmptparm = cmptparms; cmptno < numcmpts; ++cmptno, ++cmptparm) {
		cmptparm->tlx = 0;
		cmptparm->tly = 0;
		cmptparm->hstep = 1;
		cmptparm->vstep = 1;
		cmptparm->width = head.biWidth;
		cmptparm->height = head.biHeight;
		cmptparm->prec = 8;
		cmptparm->sgnd = false;
	}

	/* Create image object. */
	if (!(image = jas_image_create(numcmpts, cmptparms, JAS_CLRSPC_UNKNOWN)))
		throw "error : jas_image_create";

	if (numcmpts == 3) {
		jas_image_setclrspc(image, JAS_CLRSPC_SRGB);
		jas_image_setcmpttype(image, 0,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R));
		jas_image_setcmpttype(image, 1,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G));
		jas_image_setcmpttype(image, 2,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B));
	} else {
		jas_image_setclrspc(image, JAS_CLRSPC_SGRAY);
		jas_image_setcmpttype(image, 0,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y));
	}


	for (x = 0; x < numcmpts; ++x) { cmpts[x] = 0; }
	/* Create temporary matrices to hold component data. */
	for (x = 0; x < numcmpts; ++x) {
		if (!(cmpts[x] = jas_matrix_create(1, head.biWidth))) {
			throw "error : can't allocate memory";
		}
	}

	RGBQUAD c;
	for (y = 0; y < head.biHeight; ++y) {
		for (x = 0; x < head.biWidth; ++x) {
			if (head.biClrUsed==0){
				c = GetPixelColor(x,y);
				jas_matrix_setv(cmpts[0], x, c.rgbRed);
				jas_matrix_setv(cmpts[1], x, c.rgbGreen);
				jas_matrix_setv(cmpts[2], x, c.rgbBlue);
			} else {
				jas_matrix_setv(cmpts[0], x, GetPixelIndex(x,y));
			}
		}
		yflip = head.biHeight - 1 - y;
		for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
			if (jas_image_writecmpt(image, cmptno, 0, yflip, head.biWidth, 1, cmpts[cmptno])) {
				throw "error : jas_image_writecmpt";
			}
		}
	}

	 char szfmt[4];
	*szfmt = '\0';
#if CXIMAGE_SUPPORT_JP2
	if (imagetype == CXIMAGE_FORMAT_JP2) strcpy(szfmt,"jp2");
#endif
#if CXIMAGE_SUPPORT_JPC
	if (imagetype == CXIMAGE_FORMAT_JPC) strcpy(szfmt,"jpc");
#endif
#if CXIMAGE_SUPPORT_RAS
	if (imagetype == CXIMAGE_FORMAT_RAS) strcpy(szfmt,"ras");
#endif
#if CXIMAGE_SUPPORT_PNM
	if (imagetype == CXIMAGE_FORMAT_PNM) strcpy(szfmt,"pnm");
#endif
#if CXIMAGE_SUPPORT_PGX
	if (imagetype == CXIMAGE_FORMAT_PGX){
		strcpy(szfmt,"pgx");
		if (head.biClrUsed==0) throw "PGX can save only GrayScale images";
	}
#endif
	int outfmt = jas_image_strtofmt(szfmt);

	char szoutopts[16];
	sprintf(szoutopts,"rate=%.3f", info.nQuality/100.0f);

	if (jas_image_encode(image, out, outfmt, szoutopts)) {
		throw "error: cannot encode image\n";
	}
	jas_stream_flush(out);

  } catch (char *message) {
	strncpy(info.szLastError,message,255);
	error = 1;
  }

	for (x = 0; x < numcmpts; ++x) { if (cmpts[x]) { jas_matrix_destroy(cmpts[x]); } }
	jas_cleanup();
	if (image) jas_image_destroy(image);
	if (out) jas_stream_close(out);

	return (error==0);
}
////////////////////////////////////////////////////////////////////////////////
#endif 	// CXIMAGE_SUPPORT_JASPER

