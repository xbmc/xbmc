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

#include "stdafx.h"
#include "VobSubImage.h"

CVobSubImage::CVobSubImage()
{
  iLang = iIdx = -1;
  fForced = false;
  start = delay = 0;
  rect = Com::SmartRect(0,0,0,0);
  lpPixels = lpTemp1 = lpTemp2 = NULL;
  org = Com::SmartSize(0,0);
}

CVobSubImage::~CVobSubImage()
{
  Free();
}

bool CVobSubImage::Alloc(int w, int h)
{
  // if there is nothing to crop TrimSubImage might even add a 1 pixel
  // wide border around the text, that's why we need a bit more memory
  // to be allocated.

  if(lpTemp1 == NULL || w*h > org.cx*org.cy || (w+2)*(h+2) > (org.cx+2)*(org.cy+2))
  {
    Free();

    lpTemp1 = DNew RGBQUAD[w*h];
    if(!lpTemp1) return(false);

    lpTemp2 = DNew RGBQUAD[(w+2)*(h+2)];
    if(!lpTemp2) {delete [] lpTemp1; lpTemp1 = NULL; return(false);}

    org.cx = w; 
    org.cy = h;
  }

  lpPixels = lpTemp1;

  return(true);
}

void CVobSubImage::Free()
{
  if(lpTemp1) delete [] lpTemp1;
  lpTemp1 = NULL;

  if(lpTemp2) delete [] lpTemp2;
  lpTemp2 = NULL;

  lpPixels = NULL;
}

bool CVobSubImage::Decode(BYTE* lpData, int packetsize, int datasize,
              bool fCustomPal, 
              int tridx, 
              RGBQUAD* orgpal /*[16]*/, RGBQUAD* cuspal /*[4]*/,
              bool fTrim)
{
  GetPacketInfo(lpData, packetsize, datasize);

  if(!Alloc(rect.Width(), rect.Height())) return(false);

  lpPixels = lpTemp1;

  nPlane = 0;
  fAligned = 1;

  this->fCustomPal = fCustomPal;
  this->orgpal = orgpal;
  this->tridx = tridx;
  this->cuspal = cuspal;

  Com::SmartPoint p(rect.left, rect.top);

  int end0 = nOffset[1];
  int end1 = datasize;

  while((nPlane == 0 && nOffset[0] < end0) || (nPlane == 1 && nOffset[1] < end1))
  {
    DWORD code;

    if((code = GetNibble(lpData)) >= 0x4
    || (code = (code << 4) | GetNibble(lpData)) >= 0x10
    || (code = (code << 4) | GetNibble(lpData)) >= 0x40
    || (code = (code << 4) | GetNibble(lpData)) >= 0x100)
    {
      DrawPixels(p, code >> 2, code & 3);
      if((p.x += code >> 2) < rect.right) continue;
    }

    DrawPixels(p, rect.right - p.x, code & 3);

    if(!fAligned) GetNibble(lpData); // align to byte

    p.x = rect.left;
    p.y++;
    nPlane = 1 - nPlane;
  }

  rect.bottom = min(p.y, rect.bottom);

  if(fTrim) TrimSubImage();

  return(true);
}

void CVobSubImage::GetPacketInfo(BYTE* lpData, int packetsize, int datasize)
{
//  delay = 0;

  int i, nextctrlblk = datasize;
  WORD pal = 0, tr = 0;

  do
  {
    i = nextctrlblk;

    int t = (lpData[i] << 8) | lpData[i+1]; i += 2;
    nextctrlblk = (lpData[i] << 8) | lpData[i+1]; i += 2;

    if(nextctrlblk > packetsize || nextctrlblk < datasize)
    {
      ASSERT(0);
      return;
    }

    bool fBreak = false;

    while(!fBreak)
    {
      int len = 0;

      switch(lpData[i])
      {
        case 0x00: len = 0; break;
        case 0x01: len = 0; break;
        case 0x02: len = 0; break;
        case 0x03: len = 2; break;
        case 0x04: len = 2; break;
        case 0x05: len = 6; break;
        case 0x06: len = 4; break;
        default: len = 0; break;
      }

      if(i+len >= packetsize)
      {
        //TRACE(_T("Warning: Wrong subpicture parameter block ending\n"));
        break;
      }

      switch(lpData[i++])
      {
        case 0x00: // forced start displaying
          fForced = true;
          break;
        case 0x01: // start displaying
          fForced = false;
          break;
        case 0x02: // stop displaying
          delay = 1024 * t / 90;
          break;
        case 0x03:
          pal = (lpData[i] << 8) | lpData[i+1]; i += 2;
          break;
        case 0x04:
          tr = (lpData[i] << 8) | lpData[i+1]; i += 2;
//tr &= 0x00f0;
          break;
        case 0x05:
          rect = Com::SmartRect((lpData[i] << 4) + (lpData[i+1] >> 4), 
            (lpData[i+3] << 4) + (lpData[i+4] >> 4), 
            ((lpData[i+1] & 0x0f) << 8) + lpData[i+2] + 1, 
            ((lpData[i+4] & 0x0f) << 8) + lpData[i+5] + 1);
          i += 6;
          break;
        case 0x06:
          nOffset[0] = (lpData[i] << 8) + lpData[i+1]; i += 2;
          nOffset[1] = (lpData[i] << 8) + lpData[i+1]; i += 2;
          break;
        case 0xff: // end of ctrlblk
          fBreak = true;
          continue;
        default: // skip this ctrlblk
          fBreak = true;
          break;
      }
    }
  }
  while(i <= nextctrlblk && i < packetsize);

  for(i = 0; i < 4; i++) 
  {
    this->pal[i].pal = (pal >> (i << 2)) & 0xf;
    this->pal[i].tr = (tr >> (i << 2)) & 0xf;
  }
}

BYTE CVobSubImage::GetNibble(BYTE* lpData)
{
  WORD& off = nOffset[nPlane];
  BYTE ret = (lpData[off] >> (fAligned << 2)) & 0x0f;
  fAligned = !fAligned;
  off += fAligned;
  return(ret);
}

void CVobSubImage::DrawPixels(Com::SmartPoint p, int length, int colorid)
{
  if(length <= 0
  || p.x + length < rect.left
  || p.x >= rect.right
  || p.y < rect.top
  || p.y >= rect.bottom) 
  {
    return;
  }

  if(p.x < rect.left) p.x = rect.left;
  if(p.x + length >= rect.right) length = rect.right - p.x;

  RGBQUAD* ptr = &lpPixels[rect.Width() * (p.y - rect.top) + (p.x - rect.left)];

  RGBQUAD c;

  if(!fCustomPal) 
  {
    c = orgpal[pal[colorid].pal];
    c.rgbReserved = (pal[colorid].tr<<4)|pal[colorid].tr;
  }
  else
  {
    c = cuspal[colorid];
  }

  while(length-- > 0) *ptr++ = c;
}

void CVobSubImage::TrimSubImage()
{
  Com::SmartRect r;
  r.left = rect.Width();
  r.top = rect.Height();
  r.right = 0;
  r.bottom = 0;

  RGBQUAD* ptr = lpTemp1;

  for(int j = 0, y = rect.Height(); j < y; j++)
  {
    for(int i = 0, x = rect.Width(); i < x; i++, ptr++)
    {
      if(ptr->rgbReserved)
      {
        if(r.top > j) r.top = j;
        if(r.bottom < j) r.bottom = j;
        if(r.left > i) r.left = i; 
        if(r.right < i) r.right = i; 
      }
    }
  }

  if(r.left > r.right || r.top > r.bottom) return;

  r += Com::SmartRect(0, 0, 1, 1);

  r &= Com::SmartRect(Com::SmartPoint(0,0), rect.Size());

  int w = r.Width(), h = r.Height();

  DWORD offset = r.top*rect.Width() + r.left;

  r += Com::SmartRect(1, 1, 1, 1);

  DWORD* src = (DWORD*)&lpTemp1[offset];
  DWORD* dst = (DWORD*)&lpTemp2[1 + w + 1];

  memset(lpTemp2, 0, (1 + w + 1)*sizeof(RGBQUAD));

  for(int height = h; height; height--, src += rect.Width())
  {
    *dst++ = 0;
    memcpy(dst, src, w*sizeof(RGBQUAD)); dst += w; 
    *dst++ = 0;
  }

  memset(dst, 0, (1 + w + 1)*sizeof(RGBQUAD));

  lpPixels = lpTemp2;

  rect = r + rect.TopLeft();
}

////////////////////////////////

#include "RTS.h"
#include <math.h>

#define GP(xx, yy) (((xx) < 0 || (yy) < 0 || (xx) >= w || (yy) >= h) ? 0 : p[(yy)*w+(xx)])

std::vector<COutline*>* CVobSubImage::GetOutlineList(Com::SmartPoint& topleft)
{
  int w = rect.Width(), h = rect.Height(), len = w*h;
  if(len <= 0) return NULL;

  BYTE* p = new BYTE[len];
  if(!p) return NULL;

  std::vector<COutline *>* ol = DNew std::vector<COutline *>();
  if (!ol) return NULL;
  
  BYTE* cp = p;
  RGBQUAD* rgbp = (RGBQUAD*)lpPixels;

  for(int i = 0; i < len; i++, cp++, rgbp++)
    *cp = !!rgbp->rgbReserved;

  enum {UP, RIGHT, DOWN, LEFT};

  topleft.x = topleft.y = INT_MAX;

  while(1)
  {
    cp = p;

    int x, y; 

    for(y = 0; y < h; y++)
    {
      for(x = 0; x < w-1; x++, cp++)
      {
        if(cp[0] == 0 && cp[1] == 1) break;
      }

      if(x < w-1) break;

      cp++;
    }

    if(y == h) break;

    int prevdir, dir = UP;

    int ox = x, oy = y, odir = dir;

    COutline *o = DNew COutline;
    if(!o) break;

    do
    {
      Com::SmartPoint pp;
      BYTE fl, fr, br;

      prevdir = dir;

      switch(prevdir)
      {
      case UP:
        pp = Com::SmartPoint(x+1, y);
        fl = GP(x, y-1);
        fr = GP(x+1, y-1);
        br = GP(x+1, y);
        break;
      case RIGHT:
        pp = Com::SmartPoint(x+1, y+1);
        fl = GP(x+1, y);
        fr = GP(x+1, y+1);
        br = GP(x, y+1);
        break;
      case DOWN:
        pp = Com::SmartPoint(x, y+1);
        fl = GP(x, y+1);
        fr = GP(x-1, y+1);
        br = GP(x-1, y);
        break;
      case LEFT:
        pp = Com::SmartPoint(x, y);
        fl = GP(x-1, y);
        fr = GP(x-1, y-1);
        br = GP(x, y-1);
        break;
      }

      // turning left if:
      // o . | o .
      // ^ o | < o
      // turning right if:
      // x x | x >
      // ^ o | x o
      //
      // o set, x empty, . can be anything

      if(fl==1) dir = (dir-1+4)&3;
      else if(fl!=1 && fr!=1 && br==1) dir = (dir+1)&3;
      else if(p[y*w+x]&16) {ASSERT(0); break;} // we are going around in one place (this must not happen if the starting conditions were correct)

      p[y*w+x] = (p[y*w+x]<<1) | 2; // increase turn count (== log2(highordbit(*p)))

      switch(dir)
      {
      case UP:
        if(prevdir == LEFT) {x--; y--;}
        if(prevdir == UP) y--;
        break;
      case RIGHT:
        if(prevdir == UP) {x++; y--;}
        if(prevdir == RIGHT) x++;
        break;
      case DOWN:
        if(prevdir == RIGHT) {x++; y++;}
        if(prevdir == DOWN) y++;
        break;
      case LEFT:
        if(prevdir == DOWN) {x--; y++;}
        if(prevdir == LEFT) x--;
        break;
      }

      int d = dir - prevdir;
      o->Add(pp, d == 3 ? -1 : d == -3 ? 1 : d);

      if(topleft.x > pp.x) topleft.x = pp.x;
      if(topleft.y > pp.y) topleft.y = pp.y;
    }
    while(!(x == ox && y == oy && dir == odir));

    if(o->pa.size() > 0 && (x == ox && y == oy && dir == odir)) 
    {
      ol->push_back(o);
    }
    else
    {
      ASSERT(0);
    }
  }

  if (p)
    delete[] p;
  return(ol);
}

static bool FitLine(COutline& o, int& start, int& end)
{
  int len = int(o.pa.size());
  if(len < 7) return(false); // small segments should be handled with beziers...

  for(start = 0; start < len && !o.da[start]; start++);
  for(end = len-1; end > start && !o.da[end]; end--);

  if(end-start < 8 || end-start < (len-end)+(start-0)) return(false);

  std::vector<UINT> la, ra;

  int i, j, k;

  for(i = start+1, j = end, k = start; i <= j; i++)
  {
    if(!o.da[i]) continue;
    if(o.da[i] == o.da[k]) return(false);
    if(o.da[i] == -1) la.push_back(i-k);
    else ra.push_back(i-k);
    k = i;
  }

  bool fl = true, fr = true;

  // these tests are completly heuristic and might be redundant a bit...

  for(i = 0, j = la.size(); i < j && fl; i++) {if(la[i] != 1) fl = false;} 
  for(i = 0, j = ra.size(); i < j && fr; i++) {if(ra[i] != 1) fr = false;}

  if(!fl && !fr) return(false); // can't be a line if there are bigger steps than one in both directions (lines are usually drawn by stepping one either horizontally or vertically)
  if(fl && fr && 1.0*(end-start)/((len-end)*2+(start-0)*2) > 0.4) return(false); // if this section is relatively too small it may only be a rounded corner
  if(!fl && la.size() > 0 && la.size() <= 4 && (la[0] == 1 && la[la.size()-1] == 1)) return(false); // one step at both ends, doesn't sound good for a line (may be it was skewed, so only eliminate smaller sections where beziers going to look just as good)
  if(!fr && ra.size() > 0 && ra.size() <= 4 && (ra[0] == 1 && ra[ra.size()-1] == 1)) return(false); // -''-

  std::vector<UINT>& a = !fl ? la : ra;

  len = a.size();

  int sum = 0;

  for(i = 0, j = INT_MAX, k = 0; i < len; i++)
  {
    if(j > a[i]) j = a[i];
    if(k < a[i]) k = a[i];
    sum += a[i];
  }

  if(k - j > 2 && 1.0*sum/len < 2) return(false);
  if(k - j > 2 && 1.0*sum/len >= 2 && len < 4) return(false);

  if((la.size()/2+ra.size()/2)/2 <= 2)
  {
    if((k+j)/2 < 2 && k*j!=1) return(false);
  }

  double err = 0;

  Com::SmartPoint sp = o.pa[start], ep = o.pa[end];

  double minerr = 0, maxerr = 0;
  
  double vx = ep.x - sp.x, vy = ep.y - sp.y, l = sqrt(vx*vx+vy*vy);
  vx /= l; vy /= l;

  for(i = start+1, j = end-1; i <= j; i++)
  {
    Com::SmartPoint p = o.pa[i], dp = p - sp;
    double t = vx*dp.x+vy*dp.y, dx = vx*t + sp.x - p.x, dy = vy*t + sp.y - p.y;
    t = dx*dx+dy*dy;
    err += t;
    t = sqrt(t);
    if(vy*dx-dy*vx < 0) {if(minerr > -t) minerr = -t;}
    else {if(maxerr < t) maxerr = t;}
  }

  return((maxerr-minerr)/l < 0.1  || err/l < 1.5 || (fabs(maxerr) < 8 && fabs(minerr) < 8));
}

static int CalcPossibleCurveDegree(COutline& o)
{
  int len2 = int(o.da.size());

  std::vector<UINT> la;

  for(int i = 0, j = 0; j < len2; j++)
  {
    if(j == len2-1 || o.da[j])
    {
      la.push_back(j-i);
      i = j;
    }
  }

  int len = la.size();

  int ret = 0;

  // check if we can find a reason to add a penalty degree, or two :P
  // it is mainly about looking for distant corners
  {
    int penalty = 0;

    int ma[2] = {0, 0};
    for(int i = 0; i < len; i++) ma[i&1] += la[i];

    int ca[2] = {ma[0], ma[1]};
    for(int i = 0; i < len; i++) 
    {
      ca[i&1] -= la[i];

      double c1 = 1.0*ca[0]/ma[0], c2 = 1.0*ca[1]/ma[1], c3 = 1.0*la[i]/ma[i&1];

      if(len2 > 16 && (fabs(c1-c2) > 0.7 || (c3 > 0.6 && la[i] > 5)))
        {penalty = 2; break;}

      if(fabs(c1-c2) > 0.6 || (c3 > 0.4 && la[i] > 5))
        {penalty = 1;}
    }

    ret += penalty;
  }

  la[0] <<= 1;
  la[len-1] <<= 1;

  for(int i = 0; i < len; i+=2)
  {
    if(la[i] > 1) {ret++; i--;} // prependicular to the last chosen section and bigger then 1 -> add a degree and continue with the other dir
  }

  return(ret);
}

inline double vectlen(Com::SmartPoint p)
{
  return(sqrt((double)(p.x*p.x+p.y*p.y)));
}

inline double vectlen(Com::SmartPoint p1, Com::SmartPoint p2)
{
  return(vectlen(p2 - p1));
}

static bool MinMaxCosfi(COutline& o, double& mincf, double& maxcf) // not really cosfi, it is weighted by the distance from the segment endpoints, and since it would be always between -1 and 0, the applied sign marks side 
{
  std::vector<Com::SmartPoint>& pa = o.pa;

  int len = (int)pa.size();
  if(len < 6) return(false);

  mincf = 1;
  maxcf = -1;

  Com::SmartPoint p = pa[len-1] - pa[0];
  double l = vectlen(p);

  for(int i = 2; i < len-2; i++) // skip the endpoints, they aren't accurate
  {
    Com::SmartPoint p1 = pa[0] - pa[i], p2 = pa[len-1] - pa[i];
    double l1 = vectlen(p1), l2 = vectlen(p2);
    int sign = p1.x*p.y-p1.y*p.x >= 0 ? 1 : -1;

    double c = (1.0*len/2 - fabs(i - 1.0*len/2)) / len * 2; // c: 0 -> 1 -> 0

    double cosfi = (1+(p1.x*p2.x+p1.y*p2.y)/(l1*l2)) * sign * c;
    if(mincf > cosfi) mincf = cosfi;
    if(maxcf < cosfi) maxcf = cosfi;
  }

  return(true);
}

static bool FitBezierVH(COutline& o, Com::SmartPoint& p1, Com::SmartPoint& p2)
{
  int i;

  std::vector<Com::SmartPoint>& pa = o.pa;

  int len = (int)pa.size();

  if(len <= 1)
  {
    return(false);
  }
  else if(len == 2)
  {
    Com::SmartPoint mid = pa[0]+pa[1];
    mid.x >>= 1;
    mid.y >>= 1;
    p1 = p2 = mid;
    return(true);
  }

  Com::SmartPoint dir1 = pa[1] - pa[0], dir2 = pa[len-2] - pa[len-1];
  if((dir1.x&&dir1.y)||(dir2.x&&dir2.y)) 
    return(false); // we are only fitting beziers with hor./ver. endings

  if(CalcPossibleCurveDegree(o) > 3) 
    return(false);
  
  double mincf, maxcf;
  if(MinMaxCosfi(o, mincf, maxcf))
  {
    if(maxcf-mincf > 0.8 
    || maxcf-mincf > 0.6 && (maxcf >= 0.4 || mincf <= -0.4)) 
      return(false);
  }

  Com::SmartPoint p0 = p1 = pa[0];
  Com::SmartPoint p3 = p2 = pa[len-1];

  std::vector<double> pl;
  pl.resize(len);

  double c10 = 0, c11 = 0, c12 = 0, c13 = 0, c1x = 0, c1y = 0;
  double c20 = 0, c21 = 0, c22 = 0, c23 = 0, c2x = 0, c2y = 0;
  double length = 0;

  for(pl[0] = 0, i = 1; i < len; i++)
  {
    Com::SmartPoint diff = (pa[i] - pa[i-1]);
    pl[i] = (length += sqrt((double)(diff.x*diff.x+diff.y*diff.y)));
  }

  for(i = 0; i < len; i++) 
  {
    double t1 = pl[i] / length;
    double t2 = t1*t1;
    double t3 = t2*t1;
    double it1 = 1 - t1;
    double it2 = it1*it1;
    double it3 = it2*it1;

    double dc1 = 3.0*it2*t1;
    double dc2 = 3.0*it1*t2;

    c10 += it3*dc1;
    c11 += dc1*dc1;
    c12 += dc2*dc1;
    c13 += t3*dc1;
    c1x += pa[i].x*dc1;
    c1y += pa[i].y*dc1;

    c20 += it3*dc2;
    c21 += dc1*dc2;
    c22 += dc2*dc2;
    c23 += t3*dc2;
    c2x += pa[i].x*dc2;
    c2y += pa[i].y*dc2;
  }

  if(dir1.y == 0 && dir2.x == 0)
  {
    p1.x = (int)((c1x - c10*p0.x - c12*p3.x - c13*p3.x) / c11 + 0.5);
    p2.y = (int)((c2y - c20*p0.y - c21*p0.y - c23*p3.y) / c22 + 0.5);
  }
  else if(dir1.x == 0 && dir2.y == 0)
  {
    p2.x = (int)((c2x - c20*p0.x - c21*p0.x - c23*p3.x) / c22 + 0.5);
    p1.y = (int)((c1y - c10*p0.y - c12*p3.y - c13*p3.y) / c11 + 0.5);
  }
  else if(dir1.y == 0 && dir2.y == 0)
  {
    // cramer's rule
    double D = c11*c22 - c12*c21;
    p1.x = (int)(((c1x-c10*p0.x-c13*p3.x)*c22 - c12*(c2x-c20*p0.x-c23*p3.x)) / D + 0.5);
    p2.x = (int)((c11*(c2x-c20*p0.x-c23*p3.x) - (c1x-c10*p0.x-c13*p3.x)*c21) / D + 0.5);
  }
  else if(dir1.x == 0 && dir2.x == 0)
  {
    // cramer's rule
    double D = c11*c22 - c12*c21;
    p1.y = (int)(((c1y-c10*p0.y-c13*p3.y)*c22 - c12*(c2y-c20*p0.y-c23*p3.y)) / D + 0.5);
    p2.y = (int)((c11*(c2y-c20*p0.y-c23*p3.y) - (c1y-c10*p0.y-c13*p3.y)*c21) / D + 0.5);
  }
  else // must not happen
  {
    ASSERT(0); 
    return(false);
  }

  // check for "inside-out" beziers
  Com::SmartPoint dir3 = p1 - p0, dir4 = p2 - p3;
  if((dir1.x*dir3.x+dir1.y*dir3.y) <= 0 || (dir2.x*dir4.x+dir2.y*dir4.y) <= 0)
    return(false);

  return(true);
}

int CVobSubImage::GrabSegment(int start, COutline& o, COutline& ret)
{
  ret.RemoveAll();

  int len = int(o.pa.size());
  
  int cur = (start)%len, first = -1, last = -1;
  int curDir = 0, lastDir = 0;

  for(int i = 0; i < len; i++)
  {
    cur = (cur+1)%len;

    if(o.da[cur] == 0) continue;

    if(first == -1) first = cur;

    if(lastDir == o.da[cur])
    {
      Com::SmartPoint startp = o.pa[first]+o.pa[start]; startp.x >>= 1; startp.y >>= 1;
      Com::SmartPoint endp = o.pa[last]+o.pa[cur]; endp.x >>= 1; endp.y >>= 1;

      if(first < start) first += len;
      start = ((start+first)>>1)+1;
      if(start >= len) start -= len;
      if(cur < last) cur += len;
      cur = ((last+cur+1)>>1);
      if(cur >= len) cur -= len;

      ret.Add(startp, 0);

      while(start != cur)
      {
        ret.Add(o.pa[start], o.da[start]);

        start++;
        if(start >= len) start -= len;
      }

      ret.Add(endp, 0);

      return(last);
    }

    lastDir = o.da[cur];
    last = cur;
  }

  ASSERT(0);

  return(start);
}

void CVobSubImage::SplitOutline(COutline& o, COutline& o1, COutline& o2)
{
  int len = int(o.pa.size());
  if(len < 4) return;

  std::vector<UINT> la, sa, ea;

  int i, j, k;

  for(i = 0, j = 0; j < len; j++)
  {
    if(j == len-1 || o.da[j])
    {
      la.push_back(j-i);
      sa.push_back(i);
      ea.push_back(j);
      i = j;
    }
  }

  int maxlen = 0, maxidx = -1;
  int maxlen2 = 0, maxidx2 = -1;

  for(i = 0; i < la.size(); i++)
  {
    if(maxlen < la[i])
    {
      maxlen = la[i];
      maxidx = i;
    }

    if(maxlen2 < la[i] && i > 0 && i < la.size()-1)
    {
      maxlen2 = la[i];
      maxidx2 = i;
    }
  }

  if(maxlen == maxlen2) maxidx = maxidx2; // if equal choose the inner section

  j = (sa[maxidx] + ea[maxidx]) >> 1, k = (sa[maxidx] + ea[maxidx] + 1) >> 1;

  o1.RemoveAll();
  o2.RemoveAll();

  for(i = 0; i <= j; i++)
    o1.Add(o.pa[i], o.da[i]);

  if(j != k)
  {
    Com::SmartPoint mid = o.pa[j]+o.pa[k]; mid.x >>= 1; mid.y >>= 1;
    o1.Add(mid, 0);
    o2.Add(mid, 0);
  }

  for(i = k; i < len; i++)
    o2.Add(o.pa[i], o.da[i]);
}

void CVobSubImage::AddSegment(COutline& o, std::vector<BYTE>& pathTypes, std::vector<Com::SmartPoint>& pathPoints)
{
  int i, len = int(o.pa.size());
  if(len < 3) return;

  int nLeftTurns = 0, nRightTurns = 0;

  for(i = 0; i < len; i++)
  {
    if(o.da[i] == -1) nLeftTurns++;
    else if(o.da[i] == 1) nRightTurns++;
  }

  if(nLeftTurns == 0 && nRightTurns == 0) // line
  {
    pathTypes.push_back(PT_LINETO);
    pathPoints.push_back(o.pa[len-1]);

    return;
  }

  if(nLeftTurns == 0 || nRightTurns == 0) // b-spline
  {
    pathTypes.push_back(PT_MOVETONC);
    pathPoints.push_back(o.pa[0]+(o.pa[0]-o.pa[1]));

    for(i = 0; i < 3; i++)
    {
      pathTypes.push_back(PT_BSPLINETO);
      pathPoints.push_back(o.pa[i]);
    }

    for(; i < len; i++)
    {
      pathTypes.push_back(PT_BSPLINEPATCHTO);
      pathPoints.push_back(o.pa[i]);
    }

    pathTypes.push_back(PT_BSPLINEPATCHTO);
    pathPoints.push_back(o.pa[len-1]+(o.pa[len-1]-o.pa[len-2]));

    pathTypes.push_back(PT_MOVETONC);
    pathPoints.push_back(o.pa[len-1]);

    return;
  }

  int start, end;
  if(FitLine(o, start, end)) // b-spline, line, b-spline
  {
    pathTypes.push_back(PT_MOVETONC);
    pathPoints.push_back(o.pa[0]+(o.pa[0]-o.pa[1]));

    pathTypes.push_back(PT_BSPLINETO);
    pathPoints.push_back(o.pa[0]);

    pathTypes.push_back(PT_BSPLINETO);
    pathPoints.push_back(o.pa[1]);

    Com::SmartPoint p[4], pp, d = o.pa[end] - o.pa[start];
    double l = sqrt((double)(d.x*d.x+d.y*d.y)), dx = 1.0 * d.x / l, dy = 1.0 * d.y / l;

    pp = o.pa[start]-o.pa[start-1];
    double l1 = abs(pp.x)+abs(pp.y);
    pp = o.pa[end]-o.pa[end+1];
    double l2 = abs(pp.x)+abs(pp.y);
    p[0] = Com::SmartPoint((int)(1.0 * o.pa[start].x + dx*l1 + 0.5), (int)(1.0 * o.pa[start].y + dy*l1 + 0.5));
    p[1] = Com::SmartPoint((int)(1.0 * o.pa[start].x + dx*l1*2 + 0.5), (int)(1.0 * o.pa[start].y + dy*l1*2 + 0.5));
    p[2] = Com::SmartPoint((int)(1.0 * o.pa[end].x - dx*l2*2 + 0.5), (int)(1.0 * o.pa[end].y - dy*l2*2 + 0.5));
    p[3] = Com::SmartPoint((int)(1.0 * o.pa[end].x - dx*l2 + 0.5), (int)(1.0 * o.pa[end].y - dy*l2 + 0.5));

    if(start == 1)
    {
      pathTypes.push_back(PT_BSPLINETO);
      pathPoints.push_back(p[0]);
    }
    else
    {
      pathTypes.push_back(PT_BSPLINETO);
      pathPoints.push_back(o.pa[2]);

      for(int i = 3; i <= start; i++)
      {
        pathTypes.push_back(PT_BSPLINEPATCHTO);
        pathPoints.push_back(o.pa[i]);
      }

      pathTypes.push_back(PT_BSPLINEPATCHTO);
      pathPoints.push_back(p[0]);
    }

    pathTypes.push_back(PT_BSPLINEPATCHTO);
    pathPoints.push_back(p[1]);

    pathTypes.push_back(PT_MOVETONC);
    pathPoints.push_back(p[0]);

    pathTypes.push_back(PT_LINETO);
    pathPoints.push_back(p[3]);

    pathTypes.push_back(PT_MOVETONC);
    pathPoints.push_back(p[2]);

    pathTypes.push_back(PT_BSPLINEPATCHTO);
    pathPoints.push_back(p[3]);

    for(i = end; i < len; i++)
    {
      pathTypes.push_back(PT_BSPLINEPATCHTO);
      pathPoints.push_back(o.pa[i]);
    }

    pathTypes.push_back(PT_BSPLINEPATCHTO);
    pathPoints.push_back(o.pa[len-1]+(o.pa[len-1]-o.pa[len-2]));

    pathTypes.push_back(PT_MOVETONC);
    pathPoints.push_back(o.pa[len-1]);

    return;
  }

  Com::SmartPoint p1, p2;
  if(FitBezierVH(o, p1, p2)) // bezier
  {
    pathTypes.push_back(PT_BEZIERTO);
    pathPoints.push_back(p1);
    pathTypes.push_back(PT_BEZIERTO);
    pathPoints.push_back(p2);
    pathTypes.push_back(PT_BEZIERTO);
    pathPoints.push_back(o.pa[o.pa.size()-1]);

    return;
  }

  COutline o1, o2;
  SplitOutline(o, o1, o2);
  AddSegment(o1, pathTypes, pathPoints);
  AddSegment(o2, pathTypes, pathPoints);
}

bool CVobSubImage::Polygonize(std::vector<BYTE>& pathTypes, std::vector<Com::SmartPoint>& pathPoints, bool fSmooth, int scale)
{
  Com::SmartPoint topleft;
  std::vector<COutline *>* ol(GetOutlineList(topleft));
  if(!ol) return(false);

  std::vector<COutline *>::iterator pos;

  pos = ol->begin();
  for (; pos != ol->end(); ++pos)
    {
    std::vector<Com::SmartPoint>& pa = (*pos)->pa;
    for(int i = 0; i < pa.size(); i++)
    {
      pa[i].x = (pa[i].x-topleft.x)<<scale;
      pa[i].y = (pa[i].y-topleft.y)<<scale;
    }
  }

  pos = ol->begin();
  for (; pos != ol->end(); ++pos)
  {
    COutline& o = **pos, o2;

    if(fSmooth)
    {
      int i = 0, iFirst = -1;

      while(1)
      {
        i = GrabSegment(i, o, o2);

        if(i == iFirst) break;

        if(iFirst < 0) 
        {
          iFirst = i;
          pathTypes.push_back(PT_MOVETO);
          pathPoints.push_back(o2.pa[0]);
        }

        AddSegment(o2, pathTypes, pathPoints);
      }
    }
    else
    {
/*
      for(int i = 1, len = o.pa.size(); i < len; i++)
      {
                if(int dir = o.da[i-1])
        {
          Com::SmartPoint dir2 = o.pa[i] - o.pa[i-1];
          dir2.x /= 2; dir2.y /= 2;
          Com::SmartPoint dir1 = dir > 0 ? Com::SmartPoint(dir2.y, -dir2.x) : Com::SmartPoint(-dir2.y, dir2.x);
          i = i;
          o.pa[i-1] -= dir1;
          o.pa.InsertAt(i, o.pa[i-1] + dir2);
          o.da.InsertAt(i, -dir);
          o.pa.InsertAt(i+1, o.pa[i] + dir1);
          o.da.InsertAt(i+1, dir);
          i += 2;
          len += 2;
        }
      }
*/
      pathTypes.push_back(PT_MOVETO);
      pathPoints.push_back(o.pa[0]);

      for(int i = 1, len = int(o.pa.size()); i < len; i++)
      {
        pathTypes.push_back(PT_LINETO);
        pathPoints.push_back(o.pa[i]);
      }
    }
  }
  /* Clean ol */
  while (! ol->empty())
  {
    delete ol->back();
    ol->pop_back();
  }

  return !pathTypes.empty();
}

bool CVobSubImage::Polygonize(CStdStringW& assstr, bool fSmooth, int scale)
{
  std::vector<BYTE> pathTypes;
  std::vector<Com::SmartPoint> pathPoints;

  if(!Polygonize(pathTypes, pathPoints, fSmooth, scale))
    return(false);

  assstr.Format(L"{\\an7\\pos(%d,%d)\\p%d}", rect.left, rect.top, 1+scale);
//  assstr.Format(L"{\\p%d}", 1+scale);

  BYTE lastType = 0;

  int nPoints = int(pathTypes.size());

  for(int i = 0; i < nPoints; i++)
  {
    CStdStringW s;

    switch(pathTypes[i])
    {
    case PT_MOVETO: 
      if(lastType != PT_MOVETO) assstr += L"m ";
      s.Format(L"%d %d ", pathPoints[i].x, pathPoints[i].y); 
      break;
    case PT_MOVETONC: 
      if(lastType != PT_MOVETONC) assstr += L"n ";
      s.Format(L"%d %d ", pathPoints[i].x, pathPoints[i].y); 
      break;
    case PT_LINETO: 
      if(lastType != PT_LINETO) assstr += L"l ";
      s.Format(L"%d %d ", pathPoints[i].x, pathPoints[i].y); 
      break;
    case PT_BEZIERTO: 
      if(i < nPoints-2)
      {
        if(lastType != PT_BEZIERTO) assstr += L"b ";
        s.Format(L"%d %d %d %d %d %d ", pathPoints[i].x, pathPoints[i].y, pathPoints[i+1].x, pathPoints[i+1].y, pathPoints[i+2].x, pathPoints[i+2].y); 
        i+=2;
      }
      break;
    case PT_BSPLINETO: 
      if(i < nPoints-2)
      {
        if(lastType != PT_BSPLINETO) assstr += L"s ";
        s.Format(L"%d %d %d %d %d %d ", pathPoints[i].x, pathPoints[i].y, pathPoints[i+1].x, pathPoints[i+1].y, pathPoints[i+2].x, pathPoints[i+2].y); 
        i+=2;
      }
      break;
    case PT_BSPLINEPATCHTO: 
      if(lastType != PT_BSPLINEPATCHTO) assstr += L"p ";
      s.Format(L"%d %d ", pathPoints[i].x, pathPoints[i].y); 
      break;
    }

    lastType = pathTypes[i];

    assstr += s;
  }

  assstr += L"{\\p0}";

  return nPoints > 0;
}

void CVobSubImage::Scale2x()
{
  int w = rect.Width(), h = rect.Height();

  DWORD* src = (DWORD*)lpPixels;
  DWORD* dst = DNew DWORD[w*h];

  for(int y = 0; y < h; y++)
  {
    for(int x = 0; x < w; x++, src++, dst++)
    {
      DWORD E = *src;

      DWORD A = x > 0 && y > 0 ? src[-w-1] : E;
      DWORD B = y > 0 ? src[-w] : E;
      DWORD C = x < w-1 && y > 0 ? src[-w+1] : E;

      DWORD D = x > 0 ? src[-1] : E;;
      DWORD F = x < w-1 ? src[+1] : E;;

      DWORD G = x > 0 && y < h-1 ? src[+w-1] : E;
      DWORD H = y < h-1 ? src[+w] : E;
      DWORD I = x < w-1 && y < h-1 ? src[+w+1] : E;

      DWORD E0 = D == B && B != F && D != H ? D : E;
      DWORD E1 = B == F && B != D && F != H ? F : E;
      DWORD E2 = D == H && D != B && H != F ? D : E;
      DWORD E3 = H == F && D != H && B != F ? F : E;

      *dst = ((((E0&0x00ff00ff)+(E1&0x00ff00ff)+(E2&0x00ff00ff)+(E3&0x00ff00ff)+2)>>2)&0x00ff00ff)
        | (((((E0>>8)&0x00ff00ff)+((E1>>8)&0x00ff00ff)+((E2>>8)&0x00ff00ff)+((E3>>8)&0x00ff00ff)+2)<<6)&0xff00ff00);
    }
  }

  src -= w*h;
  dst -= w*h;

  memcpy(src, dst, w*h*4);

  delete [] dst;
}
