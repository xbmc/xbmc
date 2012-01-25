/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <vector>
#include "../SubPic/ISubPic.h"
#ifdef _VSMOD // patch m004. gradient colors
#include "STS.h" 
#endif

#ifdef _VSMOD // patch m006. moveable vector clip
class MOD_MOVEVC
{
public:
  // movevc
  bool enable;
  Com::SmartSize size;
  Com::SmartPoint pos;
	
  //CSize canvas;	// canvas size
  Com::SmartSize spd;		// output canvas size
  Com::SmartPoint curpos;  // output origin point
  int hfull;		// full height
  byte* alphamask;

  MOD_MOVEVC();

  byte GetAlphaValue(int wx,int wy);
  void clear();
};
#endif

#define PT_MOVETONC 0xfe
#define PT_BSPLINETO 0xfc
#define PT_BSPLINEPATCHTO 0xfa

class RasterizerNfo
{
public:
  int w;
  int h;
  int spdw;
  int overlayp;
  int pitch;
  DWORD color;

  int xo;
  int yo;

  const DWORD* sw;
  byte* s;
  byte* src;
  DWORD* dst;

#ifdef _VSMOD
  int typ;
  MOD_GRADIENT mod_grad;
  MOD_MOVEVC mod_vc;
#else
  byte* am;
#endif

  RasterizerNfo();
};

class Rasterizer
{
  bool fFirstSet;
  Com::SmartPoint firstp, lastp;

protected:
  BYTE* mpPathTypes;
  POINT* mpPathPoints;
  size_t mPathPoints;

private:
  int mWidth, mHeight;

  typedef std::pair<unsigned __int64, unsigned __int64> tSpan;
  typedef std::vector<tSpan> tSpanBuffer;

  tSpanBuffer mOutline;
  tSpanBuffer mWideOutline;
  int mWideBorder;

  struct Edge {
    int next;
    int posandflag;
  } *mpEdgeBuffer;
  size_t mEdgeHeapSize;
  size_t mEdgeNext;

  size_t* mpScanBuffer;

  typedef unsigned char byte;

protected:
  int mPathOffsetX, mPathOffsetY;
  int mOffsetX, mOffsetY;
  int mOverlayWidth, mOverlayHeight;
  byte *mpOverlayBuffer;

private:
  void _TrashPath();
  void _TrashOverlay();
  void _ReallocEdgeBuffer(int edges);
  void _EvaluateBezier(int ptbase, bool fBSpline);
  void _EvaluateLine(int pt1idx, int pt2idx);
  void _EvaluateLine(int x0, int y0, int x1, int y1);
  static void _OverlapRegion(tSpanBuffer& dst, tSpanBuffer& src, int dx, int dy);
  // helpers
  void Draw_noAlpha_spFF_Body_0(RasterizerNfo& rnfo);
  void Draw_noAlpha_spFF_noBody_0(RasterizerNfo& rnfo);
  void Draw_noAlpha_sp_Body_0(RasterizerNfo& rnfo);
  void Draw_noAlpha_sp_noBody_0(RasterizerNfo& rnfo);
  void Draw_noAlpha_spFF_Body_sse2(RasterizerNfo& rnfo);
  void Draw_noAlpha_spFF_noBody_sse2(RasterizerNfo& rnfo);
  void Draw_noAlpha_sp_Body_sse2(RasterizerNfo& rnfo);
  void Draw_noAlpha_sp_noBody_sse2(RasterizerNfo& rnfo);
  void Draw_Alpha_spFF_Body_0(RasterizerNfo& rnfo);
  void Draw_Alpha_spFF_noBody_0(RasterizerNfo& rnfo);
  void Draw_Alpha_sp_Body_0(RasterizerNfo& rnfo);
  void Draw_Alpha_sp_noBody_0(RasterizerNfo& rnfo);
  void Draw_Alpha_spFF_Body_sse2(RasterizerNfo& rnfo);
  void Draw_Alpha_spFF_noBody_sse2(RasterizerNfo& rnfo);
  void Draw_Alpha_sp_Body_sse2(RasterizerNfo& rnfo);
  void Draw_Alpha_sp_noBody_sse2(RasterizerNfo& rnfo);

#ifdef _VSMOD // patch m004. gradient colors
  void Draw_Grad_noAlpha_spFF_Body_0(RasterizerNfo& rnfo);
  void Draw_Grad_noAlpha_spFF_noBody_0(RasterizerNfo& rnfo);
  void Draw_Grad_noAlpha_sp_Body_0(RasterizerNfo& rnfo);
  void Draw_Grad_noAlpha_sp_noBody_0(RasterizerNfo& rnfo);
  void Draw_Grad_Alpha_spFF_Body_0(RasterizerNfo& rnfo);
  void Draw_Grad_Alpha_spFF_noBody_0(RasterizerNfo& rnfo);
  void Draw_Grad_Alpha_sp_Body_0(RasterizerNfo& rnfo); 
  void Draw_Grad_Alpha_sp_noBody_0(RasterizerNfo& rnfo);
  void Draw_Grad_noAlpha_spFF_Body_sse2(RasterizerNfo& rnfo);
  void Draw_Grad_noAlpha_spFF_noBody_sse2(RasterizerNfo& rnfo);
  void Draw_Grad_noAlpha_sp_Body_sse2(RasterizerNfo& rnfo);
  void Draw_Grad_noAlpha_sp_noBody_sse2(RasterizerNfo& rnfo);
  void Draw_Grad_Alpha_spFF_Body_sse2(RasterizerNfo& rnfo);
  void Draw_Grad_Alpha_spFF_noBody_sse2(RasterizerNfo& rnfo);
  void Draw_Grad_Alpha_sp_Body_sse2(RasterizerNfo& rnfo);
  void Draw_Grad_Alpha_sp_noBody_sse2(RasterizerNfo& rnfo);
#endif
public:
  Rasterizer();
  virtual ~Rasterizer();

  bool BeginPath(HDC hdc);
  bool EndPath(HDC hdc);
  bool PartialBeginPath(HDC hdc, bool bClearPath);
  bool PartialEndPath(HDC hdc, long dx, long dy);
  bool ScanConvert();
  bool CreateWidenedRegion(int borderX, int borderY);
  void DeleteOutlines();
  bool Rasterize(int xsub, int ysub, int fBlur, double fGaussianBlur);
  int getOverlayWidth();
#ifdef _VSMOD // patch m004. gradient colors
  Com::SmartRect Draw(SubPicDesc& spd, Com::SmartRect& clipRect, byte* pAlphaMask, int xsub, int ysub, const DWORD* switchpts, bool fBody, bool fBorder, int typ, MOD_GRADIENT& mod_grad, MOD_MOVEVC& mod_vc);
#else
  Com::SmartRect Draw(SubPicDesc& spd, Com::SmartRect& clipRect, byte* pAlphaMask, int xsub, int ysub, const DWORD* switchpts, bool fBody, bool fBorder);
#endif
  void FillSolidRect(SubPicDesc& spd, int x, int y, int nWidth, int nHeight, DWORD lColor);
};

