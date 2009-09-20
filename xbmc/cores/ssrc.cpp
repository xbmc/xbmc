//------------------------------------------------------------------------------------
// Sample frequency change class, based on original SSRC by Naoki Shibata
// Released under LGPL
// Updated to C++ class by Spoon (www.dbpoweramp.com) March 2002 dbpoweramp@dbpoweramp.com
//
// Updated to work with XBMC March 2004 by JMarshall
//
//
// Example (NB wfxInformat should be already filled in with the input data type):
//
// Cssrc SampleRateChange;
// SampleRateChange.InitConverter(&wfxInformat, 48000); // convert to 48KHz
// while (ReadChunksOfInputFile)
// {
//    int DataSize = InputJustRead;
//    IsEOF = false;
//    char *pConvData = SampleRateChange.ConvertSomeData(InData, DataSize, IsEOF);
//    if (pConvData)
//    {
//        // Data is in pConvData and is DataSize bytes long
//    delete pConvData;
//    }
// }
//-------------------------------------------------------------------------------------

#include <math.h>
#include <string.h>

#include "ssrc.h" 
#include "system.h"
//#include "SRand.h"

//--------------------------------------------------------------------------------------
void Cssrc::cdft(int n, int isgn, REAL *a, int *ip, REAL *w)
{
  int nw;

  nw = ip[0];
  if (n > (nw << 2))
  {
    nw = n >> 2;
    makewt(nw, ip, w);
  }
  if (isgn >= 0)
  {
    cftfsub(n, a, ip + 2, nw, w);
  }
  else
  {
    cftbsub(n, a, ip + 2, nw, w);
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::rdft(int n, int isgn, REAL *a, int *ip, REAL *w)
{
  int nw, nc;
  REAL xi;

  nw = ip[0];
  if (n > (nw << 2))
  {
    nw = n >> 2;
    makewt(nw, ip, w);
  }
  nc = ip[1];
  if (n > (nc << 2))
  {
    nc = n >> 2;
    makect(nc, ip, w + nw);
  }
  if (isgn >= 0)
  {
    if (n > 4)
    {
      cftfsub(n, a, ip + 2, nw, w);
      rftfsub(n, a, nc, w + nw);
    }
    else if (n == 4)
    {
      cftfsub(n, a, ip + 2, nw, w);
    }
    xi = a[0] - a[1];
    a[0] += a[1];
    a[1] = xi;
  }
  else
  {
    a[1] = REAL(0.5 * (a[0] - a[1]));
    a[0] -= a[1];
    if (n > 4)
    {
      rftbsub(n, a, nc, w + nw);
      cftbsub(n, a, ip + 2, nw, w);
    }
    else if (n == 4)
    {
      cftbsub(n, a, ip + 2, nw, w);
    }
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::ddct(int n, int isgn, REAL *a, int *ip, REAL *w)
{
  int j, nw, nc;
  REAL xr;

  nw = ip[0];
  if (n > (nw << 2))
  {
    nw = n >> 2;
    makewt(nw, ip, w);
  }
  nc = ip[1];
  if (n > nc)
  {
    nc = n;
    makect(nc, ip, w + nw);
  }
  if (isgn < 0)
  {
    xr = a[n - 1];
    for (j = n - 2; j >= 2; j -= 2)
    {
      a[j + 1] = a[j] - a[j - 1];
      a[j] += a[j - 1];
    }
    a[1] = a[0] - xr;
    a[0] += xr;
    if (n > 4)
    {
      rftbsub(n, a, nc, w + nw);
      cftbsub(n, a, ip + 2, nw, w);
    }
    else if (n == 4)
    {
      cftbsub(n, a, ip + 2, nw, w);
    }
  }
  dctsub(n, a, nc, w + nw);
  if (isgn >= 0)
  {
    if (n > 4)
    {
      cftfsub(n, a, ip + 2, nw, w);
      rftfsub(n, a, nc, w + nw);
    }
    else if (n == 4)
    {
      cftfsub(n, a, ip + 2, nw, w);
    }
    xr = a[0] - a[1];
    a[0] += a[1];
    for (j = 2; j < n; j += 2)
    {
      a[j - 1] = a[j] - a[j + 1];
      a[j] += a[j + 1];
    }
    a[n - 1] = xr;
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::ddst(int n, int isgn, REAL *a, int *ip, REAL *w)
{
  int j, nw, nc;
  REAL xr;

  nw = ip[0];
  if (n > (nw << 2))
  {
    nw = n >> 2;
    makewt(nw, ip, w);
  }
  nc = ip[1];
  if (n > nc)
  {
    nc = n;
    makect(nc, ip, w + nw);
  }
  if (isgn < 0)
  {
    xr = a[n - 1];
    for (j = n - 2; j >= 2; j -= 2)
    {
      a[j + 1] = -a[j] - a[j - 1];
      a[j] -= a[j - 1];
    }
    a[1] = a[0] + xr;
    a[0] -= xr;
    if (n > 4)
    {
      rftbsub(n, a, nc, w + nw);
      cftbsub(n, a, ip + 2, nw, w);
    }
    else if (n == 4)
    {
      cftbsub(n, a, ip + 2, nw, w);
    }
  }
  dstsub(n, a, nc, w + nw);
  if (isgn >= 0)
  {
    if (n > 4)
    {
      cftfsub(n, a, ip + 2, nw, w);
      rftfsub(n, a, nc, w + nw);
    }
    else if (n == 4)
    {
      cftfsub(n, a, ip + 2, nw, w);
    }
    xr = a[0] - a[1];
    a[0] += a[1];
    for (j = 2; j < n; j += 2)
    {
      a[j - 1] = -a[j] - a[j + 1];
      a[j] -= a[j + 1];
    }
    a[n - 1] = -xr;
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::dfct(int n, REAL *a, REAL *t, int *ip, REAL *w)
{
  int j, k, l, m, mh, nw, nc;
  REAL xr, xi, yr, yi;

  nw = ip[0];
  if (n > (nw << 3))
  {
    nw = n >> 3;
    makewt(nw, ip, w);
  }
  nc = ip[1];
  if (n > (nc << 1))
  {
    nc = n >> 1;
    makect(nc, ip, w + nw);
  }
  m = n >> 1;
  yi = a[m];
  xi = a[0] + a[n];
  a[0] -= a[n];
  t[0] = xi - yi;
  t[m] = xi + yi;
  if (n > 2)
  {
    mh = m >> 1;
    for (j = 1; j < mh; j++)
    {
      k = m - j;
      xr = a[j] - a[n - j];
      xi = a[j] + a[n - j];
      yr = a[k] - a[n - k];
      yi = a[k] + a[n - k];
      a[j] = xr;
      a[k] = yr;
      t[j] = xi - yi;
      t[k] = xi + yi;
    }
    t[mh] = a[mh] + a[n - mh];
    a[mh] -= a[n - mh];
    dctsub(m, a, nc, w + nw);
    if (m > 4)
    {
      cftfsub(m, a, ip + 2, nw, w);
      rftfsub(m, a, nc, w + nw);
    }
    else if (m == 4)
    {
      cftfsub(m, a, ip + 2, nw, w);
    }
    a[n - 1] = a[0] - a[1];
    a[1] = a[0] + a[1];
    for (j = m - 2; j >= 2; j -= 2)
    {
      a[2 * j + 1] = a[j] + a[j + 1];
      a[2 * j - 1] = a[j] - a[j + 1];
    }
    l = 2;
    m = mh;
    while (m >= 2)
    {
      dctsub(m, t, nc, w + nw);
      if (m > 4)
      {
        cftfsub(m, t, ip + 2, nw, w);
        rftfsub(m, t, nc, w + nw);
      }
      else if (m == 4)
      {
        cftfsub(m, t, ip + 2, nw, w);
      }
      a[n - l] = t[0] - t[1];
      a[l] = t[0] + t[1];
      k = 0;
      for (j = 2; j < m; j += 2)
      {
        k += l << 2;
        a[k - l] = t[j] - t[j + 1];
        a[k + l] = t[j] + t[j + 1];
      }
      l <<= 1;
      mh = m >> 1;
      for (j = 0; j < mh; j++)
      {
        k = m - j;
        t[j] = t[m + k] - t[m + j];
        t[k] = t[m + k] + t[m + j];
      }
      t[mh] = t[m + mh];
      m = mh;
    }
    a[l] = t[0];
    a[n] = t[2] - t[1];
    a[0] = t[2] + t[1];
  }
  else
  {
    a[1] = a[0];
    a[2] = t[0];
    a[0] = t[1];
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::dfst(int n, REAL *a, REAL *t, int *ip, REAL *w)
{
  int j, k, l, m, mh, nw, nc;
  REAL xr, xi, yr, yi;

  nw = ip[0];
  if (n > (nw << 3))
  {
    nw = n >> 3;
    makewt(nw, ip, w);
  }
  nc = ip[1];
  if (n > (nc << 1))
  {
    nc = n >> 1;
    makect(nc, ip, w + nw);
  }
  if (n > 2)
  {
    m = n >> 1;
    mh = m >> 1;
    for (j = 1; j < mh; j++)
    {
      k = m - j;
      xr = a[j] + a[n - j];
      xi = a[j] - a[n - j];
      yr = a[k] + a[n - k];
      yi = a[k] - a[n - k];
      a[j] = xr;
      a[k] = yr;
      t[j] = xi + yi;
      t[k] = xi - yi;
    }
    t[0] = a[mh] - a[n - mh];
    a[mh] += a[n - mh];
    a[0] = a[m];
    dstsub(m, a, nc, w + nw);
    if (m > 4)
    {
      cftfsub(m, a, ip + 2, nw, w);
      rftfsub(m, a, nc, w + nw);
    }
    else if (m == 4)
    {
      cftfsub(m, a, ip + 2, nw, w);
    }
    a[n - 1] = a[1] - a[0];
    a[1] = a[0] + a[1];
    for (j = m - 2; j >= 2; j -= 2)
    {
      a[2 * j + 1] = a[j] - a[j + 1];
      a[2 * j - 1] = -a[j] - a[j + 1];
    }
    l = 2;
    m = mh;
    while (m >= 2)
    {
      dstsub(m, t, nc, w + nw);
      if (m > 4)
      {
        cftfsub(m, t, ip + 2, nw, w);
        rftfsub(m, t, nc, w + nw);
      }
      else if (m == 4)
      {
        cftfsub(m, t, ip + 2, nw, w);
      }
      a[n - l] = t[1] - t[0];
      a[l] = t[0] + t[1];
      k = 0;
      for (j = 2; j < m; j += 2)
      {
        k += l << 2;
        a[k - l] = -t[j] - t[j + 1];
        a[k + l] = t[j] - t[j + 1];
      }
      l <<= 1;
      mh = m >> 1;
      for (j = 1; j < mh; j++)
      {
        k = m - j;
        t[j] = t[m + k] + t[m + j];
        t[k] = t[m + k] - t[m + j];
      }
      t[0] = t[m + mh];
      m = mh;
    }
    a[l] = t[0];
  }
  a[0] = 0;
}


/* -------- initializing routines -------- */



//--------------------------------------------------------------------------------------
void Cssrc::makewt(int nw, int *ip, REAL *w)
{
  int j, nwh, nw0, nw1;
  REAL delta, wn4r, wk1r, wk1i, wk3r, wk3i;

  ip[0] = nw;
  ip[1] = 1;
  if (nw > 2)
  {
    nwh = nw >> 1;
    delta = REAL(atan(1.0) / nwh);
    wn4r = cos(delta * nwh);
    w[0] = 1;
    w[1] = wn4r;
    if (nwh >= 4)
    {
      w[2] = REAL(0.5 / cos(delta * 2));
      w[3] = REAL(0.5 / cos(delta * 6));
    }
    for (j = 4; j < nwh; j += 4)
    {
      w[j] = cos(delta * j);
      w[j + 1] = sin(delta * j);
      w[j + 2] = cos(3 * delta * j);
      w[j + 3] = sin(3 * delta * j);
    }
    nw0 = 0;
    while (nwh > 2)
    {
      nw1 = nw0 + nwh;
      nwh >>= 1;
      w[nw1] = 1;
      w[nw1 + 1] = wn4r;
      if (nwh >= 4)
      {
        wk1r = w[nw0 + 4];
        wk3r = w[nw0 + 6];
        w[nw1 + 2] = REAL(0.5 / wk1r);
        w[nw1 + 3] = REAL(0.5 / wk3r);
      }
      for (j = 4; j < nwh; j += 4)
      {
        wk1r = w[nw0 + 2 * j];
        wk1i = w[nw0 + 2 * j + 1];
        wk3r = w[nw0 + 2 * j + 2];
        wk3i = w[nw0 + 2 * j + 3];
        w[nw1 + j] = wk1r;
        w[nw1 + j + 1] = wk1i;
        w[nw1 + j + 2] = wk3r;
        w[nw1 + j + 3] = wk3i;
      }
      nw0 = nw1;
    }
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::makect(int nc, int *ip, REAL *c)
{
  int j, nch;
  REAL delta;

  ip[1] = nc;
  if (nc > 1)
  {
    nch = nc >> 1;
    delta = REAL(atan(1.0) / nch);
    c[0] = cos(delta * nch);
    c[nch] = REAL(0.5 * c[0]);
    for (j = 1; j < nch; j++)
    {
      c[j] = REAL(0.5 * cos(delta * j));
      c[nc - j] = REAL(0.5 * sin(delta * j));
    }
  }
}

#ifndef CDFT_RECURSIVE_N  /* length of the recursive FFT mode */
#define CDFT_RECURSIVE_N 512  /* <= (L1 cache size) / 16 */
#endif


//--------------------------------------------------------------------------------------
void Cssrc::cftfsub(int n, REAL *a, int *ip, int nw, REAL *w)
{
  int m;

  if (n > 32)
  {
    m = n >> 2;
    cftf1st(n, a, &w[nw - m]);
    if (n > CDFT_RECURSIVE_N)
    {
      cftrec1(m, a, nw, w);
      cftrec2(m, &a[m], nw, w);
      cftrec1(m, &a[2 * m], nw, w);
      cftrec1(m, &a[3 * m], nw, w);
    }
    else if (m > 32)
    {
      cftexp1(n, a, nw, w);
    }
    else
    {
      cftfx41(n, a, nw, w);
    }
    bitrv2(n, ip, a);
  }
  else if (n > 8)
  {
    if (n == 32)
    {
      cftf161(a, &w[nw - 8]);
      bitrv216(a);
    }
    else
    {
      cftf081(a, w);
      bitrv208(a);
    }
  }
  else if (n == 8)
  {
    cftf040(a);
  }
  else if (n == 4)
  {
    cftx020(a);
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::cftbsub(int n, REAL *a, int *ip, int nw, REAL *w)
{
  int m;

  if (n > 32)
  {
    m = n >> 2;
    cftb1st(n, a, &w[nw - m]);
    if (n > CDFT_RECURSIVE_N)
    {
      cftrec1(m, a, nw, w);
      cftrec2(m, &a[m], nw, w);
      cftrec1(m, &a[2 * m], nw, w);
      cftrec1(m, &a[3 * m], nw, w);
    }
    else if (m > 32)
    {
      cftexp1(n, a, nw, w);
    }
    else
    {
      cftfx41(n, a, nw, w);
    }
    bitrv2conj(n, ip, a);
  }
  else if (n > 8)
  {
    if (n == 32)
    {
      cftf161(a, &w[nw - 8]);
      bitrv216neg(a);
    }
    else
    {
      cftf081(a, w);
      bitrv208neg(a);
    }
  }
  else if (n == 8)
  {
    cftb040(a);
  }
  else if (n == 4)
  {
    cftx020(a);
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::bitrv2(int n, int *ip, REAL *a)
{
  int j, j1, k, k1, l, m, m2;
  REAL xr, xi, yr, yi;

  ip[0] = 0;
  l = n;
  m = 1;
  while ((m << 3) < l)
  {
    l >>= 1;
    for (j = 0; j < m; j++)
    {
      ip[m + j] = ip[j] + l;
    }
    m <<= 1;
  }
  m2 = 2 * m;
  if ((m << 3) == l)
  {
    for (k = 0; k < m; k++)
    {
      for (j = 0; j < k; j++)
      {
        j1 = 2 * j + ip[k];
        k1 = 2 * k + ip[j];
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += 2 * m2;
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 -= m2;
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += 2 * m2;
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
      }
      j1 = 2 * k + m2 + ip[k];
      k1 = j1 + m2;
      xr = a[j1];
      xi = a[j1 + 1];
      yr = a[k1];
      yi = a[k1 + 1];
      a[j1] = yr;
      a[j1 + 1] = yi;
      a[k1] = xr;
      a[k1 + 1] = xi;
    }
  }
  else
  {
    for (k = 1; k < m; k++)
    {
      for (j = 0; j < k; j++)
      {
        j1 = 2 * j + ip[k];
        k1 = 2 * k + ip[j];
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += m2;
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
      }
    }
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::bitrv2conj(int n, int *ip, REAL *a)
{
  int j, j1, k, k1, l, m, m2;
  REAL xr, xi, yr, yi;

  ip[0] = 0;
  l = n;
  m = 1;
  while ((m << 3) < l)
  {
    l >>= 1;
    for (j = 0; j < m; j++)
    {
      ip[m + j] = ip[j] + l;
    }
    m <<= 1;
  }
  m2 = 2 * m;
  if ((m << 3) == l)
  {
    for (k = 0; k < m; k++)
    {
      for (j = 0; j < k; j++)
      {
        j1 = 2 * j + ip[k];
        k1 = 2 * k + ip[j];
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += 2 * m2;
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 -= m2;
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += 2 * m2;
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
      }
      k1 = 2 * k + ip[k];
      a[k1 + 1] = -a[k1 + 1];
      j1 = k1 + m2;
      k1 = j1 + m2;
      xr = a[j1];
      xi = -a[j1 + 1];
      yr = a[k1];
      yi = -a[k1 + 1];
      a[j1] = yr;
      a[j1 + 1] = yi;
      a[k1] = xr;
      a[k1 + 1] = xi;
      k1 += m2;
      a[k1 + 1] = -a[k1 + 1];
    }
  }
  else
  {
    a[1] = -a[1];
    a[m2 + 1] = -a[m2 + 1];
    for (k = 1; k < m; k++)
    {
      for (j = 0; j < k; j++)
      {
        j1 = 2 * j + ip[k];
        k1 = 2 * k + ip[j];
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += m2;
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
      }
      k1 = 2 * k + ip[k];
      a[k1 + 1] = -a[k1 + 1];
      a[k1 + m2 + 1] = -a[k1 + m2 + 1];
    }
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::bitrv216(REAL *a)
{
  REAL x1r, x1i, x2r, x2i, x3r, x3i, x4r, x4i,
  x5r, x5i, x7r, x7i, x8r, x8i, x10r, x10i,
  x11r, x11i, x12r, x12i, x13r, x13i, x14r, x14i;

  x1r = a[2];
  x1i = a[3];
  x2r = a[4];
  x2i = a[5];
  x3r = a[6];
  x3i = a[7];
  x4r = a[8];
  x4i = a[9];
  x5r = a[10];
  x5i = a[11];
  x7r = a[14];
  x7i = a[15];
  x8r = a[16];
  x8i = a[17];
  x10r = a[20];
  x10i = a[21];
  x11r = a[22];
  x11i = a[23];
  x12r = a[24];
  x12i = a[25];
  x13r = a[26];
  x13i = a[27];
  x14r = a[28];
  x14i = a[29];
  a[2] = x8r;
  a[3] = x8i;
  a[4] = x4r;
  a[5] = x4i;
  a[6] = x12r;
  a[7] = x12i;
  a[8] = x2r;
  a[9] = x2i;
  a[10] = x10r;
  a[11] = x10i;
  a[14] = x14r;
  a[15] = x14i;
  a[16] = x1r;
  a[17] = x1i;
  a[20] = x5r;
  a[21] = x5i;
  a[22] = x13r;
  a[23] = x13i;
  a[24] = x3r;
  a[25] = x3i;
  a[26] = x11r;
  a[27] = x11i;
  a[28] = x7r;
  a[29] = x7i;
}


//--------------------------------------------------------------------------------------
void Cssrc::bitrv216neg(REAL *a)
{
  REAL x1r, x1i, x2r, x2i, x3r, x3i, x4r, x4i,
  x5r, x5i, x6r, x6i, x7r, x7i, x8r, x8i,
  x9r, x9i, x10r, x10i, x11r, x11i, x12r, x12i,
  x13r, x13i, x14r, x14i, x15r, x15i;

  x1r = a[2];
  x1i = a[3];
  x2r = a[4];
  x2i = a[5];
  x3r = a[6];
  x3i = a[7];
  x4r = a[8];
  x4i = a[9];
  x5r = a[10];
  x5i = a[11];
  x6r = a[12];
  x6i = a[13];
  x7r = a[14];
  x7i = a[15];
  x8r = a[16];
  x8i = a[17];
  x9r = a[18];
  x9i = a[19];
  x10r = a[20];
  x10i = a[21];
  x11r = a[22];
  x11i = a[23];
  x12r = a[24];
  x12i = a[25];
  x13r = a[26];
  x13i = a[27];
  x14r = a[28];
  x14i = a[29];
  x15r = a[30];
  x15i = a[31];
  a[2] = x15r;
  a[3] = x15i;
  a[4] = x7r;
  a[5] = x7i;
  a[6] = x11r;
  a[7] = x11i;
  a[8] = x3r;
  a[9] = x3i;
  a[10] = x13r;
  a[11] = x13i;
  a[12] = x5r;
  a[13] = x5i;
  a[14] = x9r;
  a[15] = x9i;
  a[16] = x1r;
  a[17] = x1i;
  a[18] = x14r;
  a[19] = x14i;
  a[20] = x6r;
  a[21] = x6i;
  a[22] = x10r;
  a[23] = x10i;
  a[24] = x2r;
  a[25] = x2i;
  a[26] = x12r;
  a[27] = x12i;
  a[28] = x4r;
  a[29] = x4i;
  a[30] = x8r;
  a[31] = x8i;
}


//--------------------------------------------------------------------------------------
void Cssrc::bitrv208(REAL *a)
{
  REAL x1r, x1i, x3r, x3i, x4r, x4i, x6r, x6i;

  x1r = a[2];
  x1i = a[3];
  x3r = a[6];
  x3i = a[7];
  x4r = a[8];
  x4i = a[9];
  x6r = a[12];
  x6i = a[13];
  a[2] = x4r;
  a[3] = x4i;
  a[6] = x6r;
  a[7] = x6i;
  a[8] = x1r;
  a[9] = x1i;
  a[12] = x3r;
  a[13] = x3i;
}


//--------------------------------------------------------------------------------------
void Cssrc::bitrv208neg(REAL *a)
{
  REAL x1r, x1i, x2r, x2i, x3r, x3i, x4r, x4i,
  x5r, x5i, x6r, x6i, x7r, x7i;

  x1r = a[2];
  x1i = a[3];
  x2r = a[4];
  x2i = a[5];
  x3r = a[6];
  x3i = a[7];
  x4r = a[8];
  x4i = a[9];
  x5r = a[10];
  x5i = a[11];
  x6r = a[12];
  x6i = a[13];
  x7r = a[14];
  x7i = a[15];
  a[2] = x7r;
  a[3] = x7i;
  a[4] = x3r;
  a[5] = x3i;
  a[6] = x5r;
  a[7] = x5i;
  a[8] = x1r;
  a[9] = x1i;
  a[10] = x6r;
  a[11] = x6i;
  a[12] = x2r;
  a[13] = x2i;
  a[14] = x4r;
  a[15] = x4i;
}


//--------------------------------------------------------------------------------------
void Cssrc::cftf1st(int n, REAL *a, REAL *w)
{
  int j, j0, j1, j2, j3, k, m, mh;
  REAL wn4r, csc1, csc3, wk1r, wk1i, wk3r, wk3i,
  wd1r, wd1i, wd3r, wd3i;
  REAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
  y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i;

  mh = n >> 3;
  m = 2 * mh;
  j1 = m;
  j2 = j1 + m;
  j3 = j2 + m;
  x0r = a[0] + a[j2];
  x0i = a[1] + a[j2 + 1];
  x1r = a[0] - a[j2];
  x1i = a[1] - a[j2 + 1];
  x2r = a[j1] + a[j3];
  x2i = a[j1 + 1] + a[j3 + 1];
  x3r = a[j1] - a[j3];
  x3i = a[j1 + 1] - a[j3 + 1];
  a[0] = x0r + x2r;
  a[1] = x0i + x2i;
  a[j1] = x0r - x2r;
  a[j1 + 1] = x0i - x2i;
  a[j2] = x1r - x3i;
  a[j2 + 1] = x1i + x3r;
  a[j3] = x1r + x3i;
  a[j3 + 1] = x1i - x3r;
  wn4r = w[1];
  csc1 = w[2];
  csc3 = w[3];
  wd1r = 1;
  wd1i = 0;
  wd3r = 1;
  wd3i = 0;
  k = 0;
  for (j = 2; j < mh - 2; j += 4)
  {
    k += 4;
    wk1r = csc1 * (wd1r + w[k]);
    wk1i = csc1 * (wd1i + w[k + 1]);
    wk3r = csc3 * (wd3r + w[k + 2]);
    wk3i = csc3 * (wd3i - w[k + 3]);
    wd1r = w[k];
    wd1i = w[k + 1];
    wd3r = w[k + 2];
    wd3i = -w[k + 3];
    j1 = j + m;
    j2 = j1 + m;
    j3 = j2 + m;
    x0r = a[j] + a[j2];
    x0i = a[j + 1] + a[j2 + 1];
    x1r = a[j] - a[j2];
    x1i = a[j + 1] - a[j2 + 1];
    y0r = a[j + 2] + a[j2 + 2];
    y0i = a[j + 3] + a[j2 + 3];
    y1r = a[j + 2] - a[j2 + 2];
    y1i = a[j + 3] - a[j2 + 3];
    x2r = a[j1] + a[j3];
    x2i = a[j1 + 1] + a[j3 + 1];
    x3r = a[j1] - a[j3];
    x3i = a[j1 + 1] - a[j3 + 1];
    y2r = a[j1 + 2] + a[j3 + 2];
    y2i = a[j1 + 3] + a[j3 + 3];
    y3r = a[j1 + 2] - a[j3 + 2];
    y3i = a[j1 + 3] - a[j3 + 3];
    a[j] = x0r + x2r;
    a[j + 1] = x0i + x2i;
    a[j + 2] = y0r + y2r;
    a[j + 3] = y0i + y2i;
    a[j1] = x0r - x2r;
    a[j1 + 1] = x0i - x2i;
    a[j1 + 2] = y0r - y2r;
    a[j1 + 3] = y0i - y2i;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    a[j2] = wk1r * x0r - wk1i * x0i;
    a[j2 + 1] = wk1r * x0i + wk1i * x0r;
    x0r = y1r - y3i;
    x0i = y1i + y3r;
    a[j2 + 2] = wd1r * x0r - wd1i * x0i;
    a[j2 + 3] = wd1r * x0i + wd1i * x0r;
    x0r = x1r + x3i;
    x0i = x1i - x3r;
    a[j3] = wk3r * x0r + wk3i * x0i;
    a[j3 + 1] = wk3r * x0i - wk3i * x0r;
    x0r = y1r + y3i;
    x0i = y1i - y3r;
    a[j3 + 2] = wd3r * x0r + wd3i * x0i;
    a[j3 + 3] = wd3r * x0i - wd3i * x0r;
    j0 = m - j;
    j1 = j0 + m;
    j2 = j1 + m;
    j3 = j2 + m;
    x0r = a[j0] + a[j2];
    x0i = a[j0 + 1] + a[j2 + 1];
    x1r = a[j0] - a[j2];
    x1i = a[j0 + 1] - a[j2 + 1];
    y0r = a[j0 - 2] + a[j2 - 2];
    y0i = a[j0 - 1] + a[j2 - 1];
    y1r = a[j0 - 2] - a[j2 - 2];
    y1i = a[j0 - 1] - a[j2 - 1];
    x2r = a[j1] + a[j3];
    x2i = a[j1 + 1] + a[j3 + 1];
    x3r = a[j1] - a[j3];
    x3i = a[j1 + 1] - a[j3 + 1];
    y2r = a[j1 - 2] + a[j3 - 2];
    y2i = a[j1 - 1] + a[j3 - 1];
    y3r = a[j1 - 2] - a[j3 - 2];
    y3i = a[j1 - 1] - a[j3 - 1];
    a[j0] = x0r + x2r;
    a[j0 + 1] = x0i + x2i;
    a[j0 - 2] = y0r + y2r;
    a[j0 - 1] = y0i + y2i;
    a[j1] = x0r - x2r;
    a[j1 + 1] = x0i - x2i;
    a[j1 - 2] = y0r - y2r;
    a[j1 - 1] = y0i - y2i;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    a[j2] = wk1i * x0r - wk1r * x0i;
    a[j2 + 1] = wk1i * x0i + wk1r * x0r;
    x0r = y1r - y3i;
    x0i = y1i + y3r;
    a[j2 - 2] = wd1i * x0r - wd1r * x0i;
    a[j2 - 1] = wd1i * x0i + wd1r * x0r;
    x0r = x1r + x3i;
    x0i = x1i - x3r;
    a[j3] = wk3i * x0r + wk3r * x0i;
    a[j3 + 1] = wk3i * x0i - wk3r * x0r;
    x0r = y1r + y3i;
    x0i = y1i - y3r;
    a[j3 - 2] = wd3i * x0r + wd3r * x0i;
    a[j3 - 1] = wd3i * x0i - wd3r * x0r;
  }
  wk1r = csc1 * (wd1r + wn4r);
  wk1i = csc1 * (wd1i + wn4r);
  wk3r = csc3 * (wd3r - wn4r);
  wk3i = csc3 * (wd3i - wn4r);
  j0 = mh;
  j1 = j0 + m;
  j2 = j1 + m;
  j3 = j2 + m;
  x0r = a[j0 - 2] + a[j2 - 2];
  x0i = a[j0 - 1] + a[j2 - 1];
  x1r = a[j0 - 2] - a[j2 - 2];
  x1i = a[j0 - 1] - a[j2 - 1];
  x2r = a[j1 - 2] + a[j3 - 2];
  x2i = a[j1 - 1] + a[j3 - 1];
  x3r = a[j1 - 2] - a[j3 - 2];
  x3i = a[j1 - 1] - a[j3 - 1];
  a[j0 - 2] = x0r + x2r;
  a[j0 - 1] = x0i + x2i;
  a[j1 - 2] = x0r - x2r;
  a[j1 - 1] = x0i - x2i;
  x0r = x1r - x3i;
  x0i = x1i + x3r;
  a[j2 - 2] = wk1r * x0r - wk1i * x0i;
  a[j2 - 1] = wk1r * x0i + wk1i * x0r;
  x0r = x1r + x3i;
  x0i = x1i - x3r;
  a[j3 - 2] = wk3r * x0r + wk3i * x0i;
  a[j3 - 1] = wk3r * x0i - wk3i * x0r;
  x0r = a[j0] + a[j2];
  x0i = a[j0 + 1] + a[j2 + 1];
  x1r = a[j0] - a[j2];
  x1i = a[j0 + 1] - a[j2 + 1];
  x2r = a[j1] + a[j3];
  x2i = a[j1 + 1] + a[j3 + 1];
  x3r = a[j1] - a[j3];
  x3i = a[j1 + 1] - a[j3 + 1];
  a[j0] = x0r + x2r;
  a[j0 + 1] = x0i + x2i;
  a[j1] = x0r - x2r;
  a[j1 + 1] = x0i - x2i;
  x0r = x1r - x3i;
  x0i = x1i + x3r;
  a[j2] = wn4r * (x0r - x0i);
  a[j2 + 1] = wn4r * (x0i + x0r);
  x0r = x1r + x3i;
  x0i = x1i - x3r;
  a[j3] = -wn4r * (x0r + x0i);
  a[j3 + 1] = -wn4r * (x0i - x0r);
  x0r = a[j0 + 2] + a[j2 + 2];
  x0i = a[j0 + 3] + a[j2 + 3];
  x1r = a[j0 + 2] - a[j2 + 2];
  x1i = a[j0 + 3] - a[j2 + 3];
  x2r = a[j1 + 2] + a[j3 + 2];
  x2i = a[j1 + 3] + a[j3 + 3];
  x3r = a[j1 + 2] - a[j3 + 2];
  x3i = a[j1 + 3] - a[j3 + 3];
  a[j0 + 2] = x0r + x2r;
  a[j0 + 3] = x0i + x2i;
  a[j1 + 2] = x0r - x2r;
  a[j1 + 3] = x0i - x2i;
  x0r = x1r - x3i;
  x0i = x1i + x3r;
  a[j2 + 2] = wk1i * x0r - wk1r * x0i;
  a[j2 + 3] = wk1i * x0i + wk1r * x0r;
  x0r = x1r + x3i;
  x0i = x1i - x3r;
  a[j3 + 2] = wk3i * x0r + wk3r * x0i;
  a[j3 + 3] = wk3i * x0i - wk3r * x0r;
}


//--------------------------------------------------------------------------------------
void Cssrc::cftb1st(int n, REAL *a, REAL *w)
{
  int j, j0, j1, j2, j3, k, m, mh;
  REAL wn4r, csc1, csc3, wk1r, wk1i, wk3r, wk3i,
  wd1r, wd1i, wd3r, wd3i;
  REAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
  y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i;

  mh = n >> 3;
  m = 2 * mh;
  j1 = m;
  j2 = j1 + m;
  j3 = j2 + m;
  x0r = a[0] + a[j2];
  x0i = -a[1] - a[j2 + 1];
  x1r = a[0] - a[j2];
  x1i = -a[1] + a[j2 + 1];
  x2r = a[j1] + a[j3];
  x2i = a[j1 + 1] + a[j3 + 1];
  x3r = a[j1] - a[j3];
  x3i = a[j1 + 1] - a[j3 + 1];
  a[0] = x0r + x2r;
  a[1] = x0i - x2i;
  a[j1] = x0r - x2r;
  a[j1 + 1] = x0i + x2i;
  a[j2] = x1r + x3i;
  a[j2 + 1] = x1i + x3r;
  a[j3] = x1r - x3i;
  a[j3 + 1] = x1i - x3r;
  wn4r = w[1];
  csc1 = w[2];
  csc3 = w[3];
  wd1r = 1;
  wd1i = 0;
  wd3r = 1;
  wd3i = 0;
  k = 0;
  for (j = 2; j < mh - 2; j += 4)
  {
    k += 4;
    wk1r = csc1 * (wd1r + w[k]);
    wk1i = csc1 * (wd1i + w[k + 1]);
    wk3r = csc3 * (wd3r + w[k + 2]);
    wk3i = csc3 * (wd3i - w[k + 3]);
    wd1r = w[k];
    wd1i = w[k + 1];
    wd3r = w[k + 2];
    wd3i = -w[k + 3];
    j1 = j + m;
    j2 = j1 + m;
    j3 = j2 + m;
    x0r = a[j] + a[j2];
    x0i = -a[j + 1] - a[j2 + 1];
    x1r = a[j] - a[j2];
    x1i = -a[j + 1] + a[j2 + 1];
    y0r = a[j + 2] + a[j2 + 2];
    y0i = -a[j + 3] - a[j2 + 3];
    y1r = a[j + 2] - a[j2 + 2];
    y1i = -a[j + 3] + a[j2 + 3];
    x2r = a[j1] + a[j3];
    x2i = a[j1 + 1] + a[j3 + 1];
    x3r = a[j1] - a[j3];
    x3i = a[j1 + 1] - a[j3 + 1];
    y2r = a[j1 + 2] + a[j3 + 2];
    y2i = a[j1 + 3] + a[j3 + 3];
    y3r = a[j1 + 2] - a[j3 + 2];
    y3i = a[j1 + 3] - a[j3 + 3];
    a[j] = x0r + x2r;
    a[j + 1] = x0i - x2i;
    a[j + 2] = y0r + y2r;
    a[j + 3] = y0i - y2i;
    a[j1] = x0r - x2r;
    a[j1 + 1] = x0i + x2i;
    a[j1 + 2] = y0r - y2r;
    a[j1 + 3] = y0i + y2i;
    x0r = x1r + x3i;
    x0i = x1i + x3r;
    a[j2] = wk1r * x0r - wk1i * x0i;
    a[j2 + 1] = wk1r * x0i + wk1i * x0r;
    x0r = y1r + y3i;
    x0i = y1i + y3r;
    a[j2 + 2] = wd1r * x0r - wd1i * x0i;
    a[j2 + 3] = wd1r * x0i + wd1i * x0r;
    x0r = x1r - x3i;
    x0i = x1i - x3r;
    a[j3] = wk3r * x0r + wk3i * x0i;
    a[j3 + 1] = wk3r * x0i - wk3i * x0r;
    x0r = y1r - y3i;
    x0i = y1i - y3r;
    a[j3 + 2] = wd3r * x0r + wd3i * x0i;
    a[j3 + 3] = wd3r * x0i - wd3i * x0r;
    j0 = m - j;
    j1 = j0 + m;
    j2 = j1 + m;
    j3 = j2 + m;
    x0r = a[j0] + a[j2];
    x0i = -a[j0 + 1] - a[j2 + 1];
    x1r = a[j0] - a[j2];
    x1i = -a[j0 + 1] + a[j2 + 1];
    y0r = a[j0 - 2] + a[j2 - 2];
    y0i = -a[j0 - 1] - a[j2 - 1];
    y1r = a[j0 - 2] - a[j2 - 2];
    y1i = -a[j0 - 1] + a[j2 - 1];
    x2r = a[j1] + a[j3];
    x2i = a[j1 + 1] + a[j3 + 1];
    x3r = a[j1] - a[j3];
    x3i = a[j1 + 1] - a[j3 + 1];
    y2r = a[j1 - 2] + a[j3 - 2];
    y2i = a[j1 - 1] + a[j3 - 1];
    y3r = a[j1 - 2] - a[j3 - 2];
    y3i = a[j1 - 1] - a[j3 - 1];
    a[j0] = x0r + x2r;
    a[j0 + 1] = x0i - x2i;
    a[j0 - 2] = y0r + y2r;
    a[j0 - 1] = y0i - y2i;
    a[j1] = x0r - x2r;
    a[j1 + 1] = x0i + x2i;
    a[j1 - 2] = y0r - y2r;
    a[j1 - 1] = y0i + y2i;
    x0r = x1r + x3i;
    x0i = x1i + x3r;
    a[j2] = wk1i * x0r - wk1r * x0i;
    a[j2 + 1] = wk1i * x0i + wk1r * x0r;
    x0r = y1r + y3i;
    x0i = y1i + y3r;
    a[j2 - 2] = wd1i * x0r - wd1r * x0i;
    a[j2 - 1] = wd1i * x0i + wd1r * x0r;
    x0r = x1r - x3i;
    x0i = x1i - x3r;
    a[j3] = wk3i * x0r + wk3r * x0i;
    a[j3 + 1] = wk3i * x0i - wk3r * x0r;
    x0r = y1r - y3i;
    x0i = y1i - y3r;
    a[j3 - 2] = wd3i * x0r + wd3r * x0i;
    a[j3 - 1] = wd3i * x0i - wd3r * x0r;
  }
  wk1r = csc1 * (wd1r + wn4r);
  wk1i = csc1 * (wd1i + wn4r);
  wk3r = csc3 * (wd3r - wn4r);
  wk3i = csc3 * (wd3i - wn4r);
  j0 = mh;
  j1 = j0 + m;
  j2 = j1 + m;
  j3 = j2 + m;
  x0r = a[j0 - 2] + a[j2 - 2];
  x0i = -a[j0 - 1] - a[j2 - 1];
  x1r = a[j0 - 2] - a[j2 - 2];
  x1i = -a[j0 - 1] + a[j2 - 1];
  x2r = a[j1 - 2] + a[j3 - 2];
  x2i = a[j1 - 1] + a[j3 - 1];
  x3r = a[j1 - 2] - a[j3 - 2];
  x3i = a[j1 - 1] - a[j3 - 1];
  a[j0 - 2] = x0r + x2r;
  a[j0 - 1] = x0i - x2i;
  a[j1 - 2] = x0r - x2r;
  a[j1 - 1] = x0i + x2i;
  x0r = x1r + x3i;
  x0i = x1i + x3r;
  a[j2 - 2] = wk1r * x0r - wk1i * x0i;
  a[j2 - 1] = wk1r * x0i + wk1i * x0r;
  x0r = x1r - x3i;
  x0i = x1i - x3r;
  a[j3 - 2] = wk3r * x0r + wk3i * x0i;
  a[j3 - 1] = wk3r * x0i - wk3i * x0r;
  x0r = a[j0] + a[j2];
  x0i = -a[j0 + 1] - a[j2 + 1];
  x1r = a[j0] - a[j2];
  x1i = -a[j0 + 1] + a[j2 + 1];
  x2r = a[j1] + a[j3];
  x2i = a[j1 + 1] + a[j3 + 1];
  x3r = a[j1] - a[j3];
  x3i = a[j1 + 1] - a[j3 + 1];
  a[j0] = x0r + x2r;
  a[j0 + 1] = x0i - x2i;
  a[j1] = x0r - x2r;
  a[j1 + 1] = x0i + x2i;
  x0r = x1r + x3i;
  x0i = x1i + x3r;
  a[j2] = wn4r * (x0r - x0i);
  a[j2 + 1] = wn4r * (x0i + x0r);
  x0r = x1r - x3i;
  x0i = x1i - x3r;
  a[j3] = -wn4r * (x0r + x0i);
  a[j3 + 1] = -wn4r * (x0i - x0r);
  x0r = a[j0 + 2] + a[j2 + 2];
  x0i = -a[j0 + 3] - a[j2 + 3];
  x1r = a[j0 + 2] - a[j2 + 2];
  x1i = -a[j0 + 3] + a[j2 + 3];
  x2r = a[j1 + 2] + a[j3 + 2];
  x2i = a[j1 + 3] + a[j3 + 3];
  x3r = a[j1 + 2] - a[j3 + 2];
  x3i = a[j1 + 3] - a[j3 + 3];
  a[j0 + 2] = x0r + x2r;
  a[j0 + 3] = x0i - x2i;
  a[j1 + 2] = x0r - x2r;
  a[j1 + 3] = x0i + x2i;
  x0r = x1r + x3i;
  x0i = x1i + x3r;
  a[j2 + 2] = wk1i * x0r - wk1r * x0i;
  a[j2 + 3] = wk1i * x0i + wk1r * x0r;
  x0r = x1r - x3i;
  x0i = x1i - x3r;
  a[j3 + 2] = wk3i * x0r + wk3r * x0i;
  a[j3 + 3] = wk3i * x0i - wk3r * x0r;
}


//--------------------------------------------------------------------------------------
void Cssrc::cftrec1(int n, REAL *a, int nw, REAL *w)
{
  int m;

  m = n >> 2;
  cftmdl1(n, a, &w[nw - 2 * m]);
  if (n > CDFT_RECURSIVE_N)
  {
    cftrec1(m, a, nw, w);
    cftrec2(m, &a[m], nw, w);
    cftrec1(m, &a[2 * m], nw, w);
    cftrec1(m, &a[3 * m], nw, w);
  }
  else
  {
    cftexp1(n, a, nw, w);
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::cftrec2(int n, REAL *a, int nw, REAL *w)
{
  int m;

  m = n >> 2;
  cftmdl2(n, a, &w[nw - n]);
  if (n > CDFT_RECURSIVE_N)
  {
    cftrec1(m, a, nw, w);
    cftrec2(m, &a[m], nw, w);
    cftrec1(m, &a[2 * m], nw, w);
    cftrec2(m, &a[3 * m], nw, w);
  }
  else
  {
    cftexp2(n, a, nw, w);
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::cftexp1(int n, REAL *a, int nw, REAL *w)
{
  int j, k, l;

  l = n >> 2;
  while (l > 128)
  {
    for (k = l; k < n; k <<= 2)
    {
      for (j = k - l; j < n; j += 4 * k)
      {
        cftmdl1(l, &a[j], &w[nw - (l >> 1)]);
        cftmdl2(l, &a[k + j], &w[nw - l]);
        cftmdl1(l, &a[2 * k + j], &w[nw - (l >> 1)]);
      }
    }
    cftmdl1(l, &a[n - l], &w[nw - (l >> 1)]);
    l >>= 2;
  }
  for (k = l; k < n; k <<= 2)
  {
    for (j = k - l; j < n; j += 4 * k)
    {
      cftmdl1(l, &a[j], &w[nw - (l >> 1)]);
      cftfx41(l, &a[j], nw, w);
      cftmdl2(l, &a[k + j], &w[nw - l]);
      cftfx42(l, &a[k + j], nw, w);
      cftmdl1(l, &a[2 * k + j], &w[nw - (l >> 1)]);
      cftfx41(l, &a[2 * k + j], nw, w);
    }
  }
  cftmdl1(l, &a[n - l], &w[nw - (l >> 1)]);
  cftfx41(l, &a[n - l], nw, w);
}


//--------------------------------------------------------------------------------------
void Cssrc::cftexp2(int n, REAL *a, int nw, REAL *w)
{
  int j, k, l, m;

  m = n >> 1;
  l = n >> 2;
  while (l > 128)
  {
    for (k = l; k < m; k <<= 2)
    {
      for (j = k - l; j < m; j += 2 * k)
      {
        cftmdl1(l, &a[j], &w[nw - (l >> 1)]);
        cftmdl1(l, &a[m + j], &w[nw - (l >> 1)]);
      }
      for (j = 2 * k - l; j < m; j += 4 * k)
      {
        cftmdl2(l, &a[j], &w[nw - l]);
        cftmdl2(l, &a[m + j], &w[nw - l]);
      }
    }
    l >>= 2;
  }
  for (k = l; k < m; k <<= 2)
  {
    for (j = k - l; j < m; j += 2 * k)
    {
      cftmdl1(l, &a[j], &w[nw - (l >> 1)]);
      cftfx41(l, &a[j], nw, w);
      cftmdl1(l, &a[m + j], &w[nw - (l >> 1)]);
      cftfx41(l, &a[m + j], nw, w);
    }
    for (j = 2 * k - l; j < m; j += 4 * k)
    {
      cftmdl2(l, &a[j], &w[nw - l]);
      cftfx42(l, &a[j], nw, w);
      cftmdl2(l, &a[m + j], &w[nw - l]);
      cftfx42(l, &a[m + j], nw, w);
    }
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::cftmdl1(int n, REAL *a, REAL *w)
{
  int j, j0, j1, j2, j3, k, m, mh;
  REAL wn4r, wk1r, wk1i, wk3r, wk3i;
  REAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

  mh = n >> 3;
  m = 2 * mh;
  j1 = m;
  j2 = j1 + m;
  j3 = j2 + m;
  x0r = a[0] + a[j2];
  x0i = a[1] + a[j2 + 1];
  x1r = a[0] - a[j2];
  x1i = a[1] - a[j2 + 1];
  x2r = a[j1] + a[j3];
  x2i = a[j1 + 1] + a[j3 + 1];
  x3r = a[j1] - a[j3];
  x3i = a[j1 + 1] - a[j3 + 1];
  a[0] = x0r + x2r;
  a[1] = x0i + x2i;
  a[j1] = x0r - x2r;
  a[j1 + 1] = x0i - x2i;
  a[j2] = x1r - x3i;
  a[j2 + 1] = x1i + x3r;
  a[j3] = x1r + x3i;
  a[j3 + 1] = x1i - x3r;
  wn4r = w[1];
  k = 0;
  for (j = 2; j < mh; j += 2)
  {
    k += 4;
    wk1r = w[k];
    wk1i = w[k + 1];
    wk3r = w[k + 2];
    wk3i = -w[k + 3];
    j1 = j + m;
    j2 = j1 + m;
    j3 = j2 + m;
    x0r = a[j] + a[j2];
    x0i = a[j + 1] + a[j2 + 1];
    x1r = a[j] - a[j2];
    x1i = a[j + 1] - a[j2 + 1];
    x2r = a[j1] + a[j3];
    x2i = a[j1 + 1] + a[j3 + 1];
    x3r = a[j1] - a[j3];
    x3i = a[j1 + 1] - a[j3 + 1];
    a[j] = x0r + x2r;
    a[j + 1] = x0i + x2i;
    a[j1] = x0r - x2r;
    a[j1 + 1] = x0i - x2i;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    a[j2] = wk1r * x0r - wk1i * x0i;
    a[j2 + 1] = wk1r * x0i + wk1i * x0r;
    x0r = x1r + x3i;
    x0i = x1i - x3r;
    a[j3] = wk3r * x0r + wk3i * x0i;
    a[j3 + 1] = wk3r * x0i - wk3i * x0r;
    j0 = m - j;
    j1 = j0 + m;
    j2 = j1 + m;
    j3 = j2 + m;
    x0r = a[j0] + a[j2];
    x0i = a[j0 + 1] + a[j2 + 1];
    x1r = a[j0] - a[j2];
    x1i = a[j0 + 1] - a[j2 + 1];
    x2r = a[j1] + a[j3];
    x2i = a[j1 + 1] + a[j3 + 1];
    x3r = a[j1] - a[j3];
    x3i = a[j1 + 1] - a[j3 + 1];
    a[j0] = x0r + x2r;
    a[j0 + 1] = x0i + x2i;
    a[j1] = x0r - x2r;
    a[j1 + 1] = x0i - x2i;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    a[j2] = wk1i * x0r - wk1r * x0i;
    a[j2 + 1] = wk1i * x0i + wk1r * x0r;
    x0r = x1r + x3i;
    x0i = x1i - x3r;
    a[j3] = wk3i * x0r + wk3r * x0i;
    a[j3 + 1] = wk3i * x0i - wk3r * x0r;
  }
  j0 = mh;
  j1 = j0 + m;
  j2 = j1 + m;
  j3 = j2 + m;
  x0r = a[j0] + a[j2];
  x0i = a[j0 + 1] + a[j2 + 1];
  x1r = a[j0] - a[j2];
  x1i = a[j0 + 1] - a[j2 + 1];
  x2r = a[j1] + a[j3];
  x2i = a[j1 + 1] + a[j3 + 1];
  x3r = a[j1] - a[j3];
  x3i = a[j1 + 1] - a[j3 + 1];
  a[j0] = x0r + x2r;
  a[j0 + 1] = x0i + x2i;
  a[j1] = x0r - x2r;
  a[j1 + 1] = x0i - x2i;
  x0r = x1r - x3i;
  x0i = x1i + x3r;
  a[j2] = wn4r * (x0r - x0i);
  a[j2 + 1] = wn4r * (x0i + x0r);
  x0r = x1r + x3i;
  x0i = x1i - x3r;
  a[j3] = -wn4r * (x0r + x0i);
  a[j3 + 1] = -wn4r * (x0i - x0r);
}


//--------------------------------------------------------------------------------------
void Cssrc::cftmdl2(int n, REAL *a, REAL *w)
{
  int j, j0, j1, j2, j3, k, kr, m, mh;
  REAL wn4r, wk1r, wk1i, wk3r, wk3i, wd1r, wd1i, wd3r, wd3i;
  REAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i, y0r, y0i, y2r, y2i;

  mh = n >> 3;
  m = 2 * mh;
  wn4r = w[1];
  j1 = m;
  j2 = j1 + m;
  j3 = j2 + m;
  x0r = a[0] - a[j2 + 1];
  x0i = a[1] + a[j2];
  x1r = a[0] + a[j2 + 1];
  x1i = a[1] - a[j2];
  x2r = a[j1] - a[j3 + 1];
  x2i = a[j1 + 1] + a[j3];
  x3r = a[j1] + a[j3 + 1];
  x3i = a[j1 + 1] - a[j3];
  y0r = wn4r * (x2r - x2i);
  y0i = wn4r * (x2i + x2r);
  a[0] = x0r + y0r;
  a[1] = x0i + y0i;
  a[j1] = x0r - y0r;
  a[j1 + 1] = x0i - y0i;
  y0r = wn4r * (x3r - x3i);
  y0i = wn4r * (x3i + x3r);
  a[j2] = x1r - y0i;
  a[j2 + 1] = x1i + y0r;
  a[j3] = x1r + y0i;
  a[j3 + 1] = x1i - y0r;
  k = 0;
  kr = 2 * m;
  for (j = 2; j < mh; j += 2)
  {
    k += 4;
    wk1r = w[k];
    wk1i = w[k + 1];
    wk3r = w[k + 2];
    wk3i = -w[k + 3];
    kr -= 4;
    wd1i = w[kr];
    wd1r = w[kr + 1];
    wd3i = w[kr + 2];
    wd3r = -w[kr + 3];
    j1 = j + m;
    j2 = j1 + m;
    j3 = j2 + m;
    x0r = a[j] - a[j2 + 1];
    x0i = a[j + 1] + a[j2];
    x1r = a[j] + a[j2 + 1];
    x1i = a[j + 1] - a[j2];
    x2r = a[j1] - a[j3 + 1];
    x2i = a[j1 + 1] + a[j3];
    x3r = a[j1] + a[j3 + 1];
    x3i = a[j1 + 1] - a[j3];
    y0r = wk1r * x0r - wk1i * x0i;
    y0i = wk1r * x0i + wk1i * x0r;
    y2r = wd1r * x2r - wd1i * x2i;
    y2i = wd1r * x2i + wd1i * x2r;
    a[j] = y0r + y2r;
    a[j + 1] = y0i + y2i;
    a[j1] = y0r - y2r;
    a[j1 + 1] = y0i - y2i;
    y0r = wk3r * x1r + wk3i * x1i;
    y0i = wk3r * x1i - wk3i * x1r;
    y2r = wd3r * x3r + wd3i * x3i;
    y2i = wd3r * x3i - wd3i * x3r;
    a[j2] = y0r + y2r;
    a[j2 + 1] = y0i + y2i;
    a[j3] = y0r - y2r;
    a[j3 + 1] = y0i - y2i;
    j0 = m - j;
    j1 = j0 + m;
    j2 = j1 + m;
    j3 = j2 + m;
    x0r = a[j0] - a[j2 + 1];
    x0i = a[j0 + 1] + a[j2];
    x1r = a[j0] + a[j2 + 1];
    x1i = a[j0 + 1] - a[j2];
    x2r = a[j1] - a[j3 + 1];
    x2i = a[j1 + 1] + a[j3];
    x3r = a[j1] + a[j3 + 1];
    x3i = a[j1 + 1] - a[j3];
    y0r = wd1i * x0r - wd1r * x0i;
    y0i = wd1i * x0i + wd1r * x0r;
    y2r = wk1i * x2r - wk1r * x2i;
    y2i = wk1i * x2i + wk1r * x2r;
    a[j0] = y0r + y2r;
    a[j0 + 1] = y0i + y2i;
    a[j1] = y0r - y2r;
    a[j1 + 1] = y0i - y2i;
    y0r = wd3i * x1r + wd3r * x1i;
    y0i = wd3i * x1i - wd3r * x1r;
    y2r = wk3i * x3r + wk3r * x3i;
    y2i = wk3i * x3i - wk3r * x3r;
    a[j2] = y0r + y2r;
    a[j2 + 1] = y0i + y2i;
    a[j3] = y0r - y2r;
    a[j3 + 1] = y0i - y2i;
  }
  wk1r = w[m];
  wk1i = w[m + 1];
  j0 = mh;
  j1 = j0 + m;
  j2 = j1 + m;
  j3 = j2 + m;
  x0r = a[j0] - a[j2 + 1];
  x0i = a[j0 + 1] + a[j2];
  x1r = a[j0] + a[j2 + 1];
  x1i = a[j0 + 1] - a[j2];
  x2r = a[j1] - a[j3 + 1];
  x2i = a[j1 + 1] + a[j3];
  x3r = a[j1] + a[j3 + 1];
  x3i = a[j1 + 1] - a[j3];
  y0r = wk1r * x0r - wk1i * x0i;
  y0i = wk1r * x0i + wk1i * x0r;
  y2r = wk1i * x2r - wk1r * x2i;
  y2i = wk1i * x2i + wk1r * x2r;
  a[j0] = y0r + y2r;
  a[j0 + 1] = y0i + y2i;
  a[j1] = y0r - y2r;
  a[j1 + 1] = y0i - y2i;
  y0r = wk1i * x1r - wk1r * x1i;
  y0i = wk1i * x1i + wk1r * x1r;
  y2r = wk1r * x3r - wk1i * x3i;
  y2i = wk1r * x3i + wk1i * x3r;
  a[j2] = y0r - y2r;
  a[j2 + 1] = y0i - y2i;
  a[j3] = y0r + y2r;
  a[j3 + 1] = y0i + y2i;
}


//--------------------------------------------------------------------------------------
void Cssrc::cftfx41(int n, REAL *a, int nw, REAL *w)
{
  if (n == 128)
  {
    cftf161(a, &w[nw - 8]);
    cftf162(&a[32], &w[nw - 32]);
    cftf161(&a[64], &w[nw - 8]);
    cftf161(&a[96], &w[nw - 8]);
  }
  else
  {
    cftf081(a, &w[nw - 16]);
    cftf082(&a[16], &w[nw - 16]);
    cftf081(&a[32], &w[nw - 16]);
    cftf081(&a[48], &w[nw - 16]);
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::cftfx42(int n, REAL *a, int nw, REAL *w)
{
  if (n == 128)
  {
    cftf161(a, &w[nw - 8]);
    cftf162(&a[32], &w[nw - 32]);
    cftf161(&a[64], &w[nw - 8]);
    cftf162(&a[96], &w[nw - 32]);
  }
  else
  {
    cftf081(a, &w[nw - 16]);
    cftf082(&a[16], &w[nw - 16]);
    cftf081(&a[32], &w[nw - 16]);
    cftf082(&a[48], &w[nw - 16]);
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::cftf161(REAL *a, REAL *w)
{
  REAL wn4r, wk1r, wk1i,
  x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
  y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
  y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i,
  y8r, y8i, y9r, y9i, y10r, y10i, y11r, y11i,
  y12r, y12i, y13r, y13i, y14r, y14i, y15r, y15i;

  wn4r = w[1];
  wk1i = wn4r * w[2];
  wk1r = wk1i + w[2];
  x0r = a[0] + a[16];
  x0i = a[1] + a[17];
  x1r = a[0] - a[16];
  x1i = a[1] - a[17];
  x2r = a[8] + a[24];
  x2i = a[9] + a[25];
  x3r = a[8] - a[24];
  x3i = a[9] - a[25];
  y0r = x0r + x2r;
  y0i = x0i + x2i;
  y4r = x0r - x2r;
  y4i = x0i - x2i;
  y8r = x1r - x3i;
  y8i = x1i + x3r;
  y12r = x1r + x3i;
  y12i = x1i - x3r;
  x0r = a[2] + a[18];
  x0i = a[3] + a[19];
  x1r = a[2] - a[18];
  x1i = a[3] - a[19];
  x2r = a[10] + a[26];
  x2i = a[11] + a[27];
  x3r = a[10] - a[26];
  x3i = a[11] - a[27];
  y1r = x0r + x2r;
  y1i = x0i + x2i;
  y5r = x0r - x2r;
  y5i = x0i - x2i;
  x0r = x1r - x3i;
  x0i = x1i + x3r;
  y9r = wk1r * x0r - wk1i * x0i;
  y9i = wk1r * x0i + wk1i * x0r;
  x0r = x1r + x3i;
  x0i = x1i - x3r;
  y13r = wk1i * x0r - wk1r * x0i;
  y13i = wk1i * x0i + wk1r * x0r;
  x0r = a[4] + a[20];
  x0i = a[5] + a[21];
  x1r = a[4] - a[20];
  x1i = a[5] - a[21];
  x2r = a[12] + a[28];
  x2i = a[13] + a[29];
  x3r = a[12] - a[28];
  x3i = a[13] - a[29];
  y2r = x0r + x2r;
  y2i = x0i + x2i;
  y6r = x0r - x2r;
  y6i = x0i - x2i;
  x0r = x1r - x3i;
  x0i = x1i + x3r;
  y10r = wn4r * (x0r - x0i);
  y10i = wn4r * (x0i + x0r);
  x0r = x1r + x3i;
  x0i = x1i - x3r;
  y14r = wn4r * (x0r + x0i);
  y14i = wn4r * (x0i - x0r);
  x0r = a[6] + a[22];
  x0i = a[7] + a[23];
  x1r = a[6] - a[22];
  x1i = a[7] - a[23];
  x2r = a[14] + a[30];
  x2i = a[15] + a[31];
  x3r = a[14] - a[30];
  x3i = a[15] - a[31];
  y3r = x0r + x2r;
  y3i = x0i + x2i;
  y7r = x0r - x2r;
  y7i = x0i - x2i;
  x0r = x1r - x3i;
  x0i = x1i + x3r;
  y11r = wk1i * x0r - wk1r * x0i;
  y11i = wk1i * x0i + wk1r * x0r;
  x0r = x1r + x3i;
  x0i = x1i - x3r;
  y15r = wk1r * x0r - wk1i * x0i;
  y15i = wk1r * x0i + wk1i * x0r;
  x0r = y12r - y14r;
  x0i = y12i - y14i;
  x1r = y12r + y14r;
  x1i = y12i + y14i;
  x2r = y13r - y15r;
  x2i = y13i - y15i;
  x3r = y13r + y15r;
  x3i = y13i + y15i;
  a[24] = x0r + x2r;
  a[25] = x0i + x2i;
  a[26] = x0r - x2r;
  a[27] = x0i - x2i;
  a[28] = x1r - x3i;
  a[29] = x1i + x3r;
  a[30] = x1r + x3i;
  a[31] = x1i - x3r;
  x0r = y8r + y10r;
  x0i = y8i + y10i;
  x1r = y8r - y10r;
  x1i = y8i - y10i;
  x2r = y9r + y11r;
  x2i = y9i + y11i;
  x3r = y9r - y11r;
  x3i = y9i - y11i;
  a[16] = x0r + x2r;
  a[17] = x0i + x2i;
  a[18] = x0r - x2r;
  a[19] = x0i - x2i;
  a[20] = x1r - x3i;
  a[21] = x1i + x3r;
  a[22] = x1r + x3i;
  a[23] = x1i - x3r;
  x0r = y5r - y7i;
  x0i = y5i + y7r;
  x2r = wn4r * (x0r - x0i);
  x2i = wn4r * (x0i + x0r);
  x0r = y5r + y7i;
  x0i = y5i - y7r;
  x3r = wn4r * (x0r - x0i);
  x3i = wn4r * (x0i + x0r);
  x0r = y4r - y6i;
  x0i = y4i + y6r;
  x1r = y4r + y6i;
  x1i = y4i - y6r;
  a[8] = x0r + x2r;
  a[9] = x0i + x2i;
  a[10] = x0r - x2r;
  a[11] = x0i - x2i;
  a[12] = x1r - x3i;
  a[13] = x1i + x3r;
  a[14] = x1r + x3i;
  a[15] = x1i - x3r;
  x0r = y0r + y2r;
  x0i = y0i + y2i;
  x1r = y0r - y2r;
  x1i = y0i - y2i;
  x2r = y1r + y3r;
  x2i = y1i + y3i;
  x3r = y1r - y3r;
  x3i = y1i - y3i;
  a[0] = x0r + x2r;
  a[1] = x0i + x2i;
  a[2] = x0r - x2r;
  a[3] = x0i - x2i;
  a[4] = x1r - x3i;
  a[5] = x1i + x3r;
  a[6] = x1r + x3i;
  a[7] = x1i - x3r;
}


//--------------------------------------------------------------------------------------
void Cssrc::cftf162(REAL *a, REAL *w)
{
  REAL wn4r, wk1r, wk1i, wk2r, wk2i, wk3r, wk3i,
  x0r, x0i, x1r, x1i, x2r, x2i,
  y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
  y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i,
  y8r, y8i, y9r, y9i, y10r, y10i, y11r, y11i,
  y12r, y12i, y13r, y13i, y14r, y14i, y15r, y15i;

  wn4r = w[1];
  wk1r = w[4];
  wk1i = w[5];
  wk3r = w[6];
  wk3i = w[7];
  wk2r = w[8];
  wk2i = w[9];
  x1r = a[0] - a[17];
  x1i = a[1] + a[16];
  x0r = a[8] - a[25];
  x0i = a[9] + a[24];
  x2r = wn4r * (x0r - x0i);
  x2i = wn4r * (x0i + x0r);
  y0r = x1r + x2r;
  y0i = x1i + x2i;
  y4r = x1r - x2r;
  y4i = x1i - x2i;
  x1r = a[0] + a[17];
  x1i = a[1] - a[16];
  x0r = a[8] + a[25];
  x0i = a[9] - a[24];
  x2r = wn4r * (x0r - x0i);
  x2i = wn4r * (x0i + x0r);
  y8r = x1r - x2i;
  y8i = x1i + x2r;
  y12r = x1r + x2i;
  y12i = x1i - x2r;
  x0r = a[2] - a[19];
  x0i = a[3] + a[18];
  x1r = wk1r * x0r - wk1i * x0i;
  x1i = wk1r * x0i + wk1i * x0r;
  x0r = a[10] - a[27];
  x0i = a[11] + a[26];
  x2r = wk3i * x0r - wk3r * x0i;
  x2i = wk3i * x0i + wk3r * x0r;
  y1r = x1r + x2r;
  y1i = x1i + x2i;
  y5r = x1r - x2r;
  y5i = x1i - x2i;
  x0r = a[2] + a[19];
  x0i = a[3] - a[18];
  x1r = wk3r * x0r - wk3i * x0i;
  x1i = wk3r * x0i + wk3i * x0r;
  x0r = a[10] + a[27];
  x0i = a[11] - a[26];
  x2r = wk1r * x0r + wk1i * x0i;
  x2i = wk1r * x0i - wk1i * x0r;
  y9r = x1r - x2r;
  y9i = x1i - x2i;
  y13r = x1r + x2r;
  y13i = x1i + x2i;
  x0r = a[4] - a[21];
  x0i = a[5] + a[20];
  x1r = wk2r * x0r - wk2i * x0i;
  x1i = wk2r * x0i + wk2i * x0r;
  x0r = a[12] - a[29];
  x0i = a[13] + a[28];
  x2r = wk2i * x0r - wk2r * x0i;
  x2i = wk2i * x0i + wk2r * x0r;
  y2r = x1r + x2r;
  y2i = x1i + x2i;
  y6r = x1r - x2r;
  y6i = x1i - x2i;
  x0r = a[4] + a[21];
  x0i = a[5] - a[20];
  x1r = wk2i * x0r - wk2r * x0i;
  x1i = wk2i * x0i + wk2r * x0r;
  x0r = a[12] + a[29];
  x0i = a[13] - a[28];
  x2r = wk2r * x0r - wk2i * x0i;
  x2i = wk2r * x0i + wk2i * x0r;
  y10r = x1r - x2r;
  y10i = x1i - x2i;
  y14r = x1r + x2r;
  y14i = x1i + x2i;
  x0r = a[6] - a[23];
  x0i = a[7] + a[22];
  x1r = wk3r * x0r - wk3i * x0i;
  x1i = wk3r * x0i + wk3i * x0r;
  x0r = a[14] - a[31];
  x0i = a[15] + a[30];
  x2r = wk1i * x0r - wk1r * x0i;
  x2i = wk1i * x0i + wk1r * x0r;
  y3r = x1r + x2r;
  y3i = x1i + x2i;
  y7r = x1r - x2r;
  y7i = x1i - x2i;
  x0r = a[6] + a[23];
  x0i = a[7] - a[22];
  x1r = wk1i * x0r + wk1r * x0i;
  x1i = wk1i * x0i - wk1r * x0r;
  x0r = a[14] + a[31];
  x0i = a[15] - a[30];
  x2r = wk3i * x0r - wk3r * x0i;
  x2i = wk3i * x0i + wk3r * x0r;
  y11r = x1r + x2r;
  y11i = x1i + x2i;
  y15r = x1r - x2r;
  y15i = x1i - x2i;
  x1r = y0r + y2r;
  x1i = y0i + y2i;
  x2r = y1r + y3r;
  x2i = y1i + y3i;
  a[0] = x1r + x2r;
  a[1] = x1i + x2i;
  a[2] = x1r - x2r;
  a[3] = x1i - x2i;
  x1r = y0r - y2r;
  x1i = y0i - y2i;
  x2r = y1r - y3r;
  x2i = y1i - y3i;
  a[4] = x1r - x2i;
  a[5] = x1i + x2r;
  a[6] = x1r + x2i;
  a[7] = x1i - x2r;
  x1r = y4r - y6i;
  x1i = y4i + y6r;
  x0r = y5r - y7i;
  x0i = y5i + y7r;
  x2r = wn4r * (x0r - x0i);
  x2i = wn4r * (x0i + x0r);
  a[8] = x1r + x2r;
  a[9] = x1i + x2i;
  a[10] = x1r - x2r;
  a[11] = x1i - x2i;
  x1r = y4r + y6i;
  x1i = y4i - y6r;
  x0r = y5r + y7i;
  x0i = y5i - y7r;
  x2r = wn4r * (x0r - x0i);
  x2i = wn4r * (x0i + x0r);
  a[12] = x1r - x2i;
  a[13] = x1i + x2r;
  a[14] = x1r + x2i;
  a[15] = x1i - x2r;
  x1r = y8r + y10r;
  x1i = y8i + y10i;
  x2r = y9r - y11r;
  x2i = y9i - y11i;
  a[16] = x1r + x2r;
  a[17] = x1i + x2i;
  a[18] = x1r - x2r;
  a[19] = x1i - x2i;
  x1r = y8r - y10r;
  x1i = y8i - y10i;
  x2r = y9r + y11r;
  x2i = y9i + y11i;
  a[20] = x1r - x2i;
  a[21] = x1i + x2r;
  a[22] = x1r + x2i;
  a[23] = x1i - x2r;
  x1r = y12r - y14i;
  x1i = y12i + y14r;
  x0r = y13r + y15i;
  x0i = y13i - y15r;
  x2r = wn4r * (x0r - x0i);
  x2i = wn4r * (x0i + x0r);
  a[24] = x1r + x2r;
  a[25] = x1i + x2i;
  a[26] = x1r - x2r;
  a[27] = x1i - x2i;
  x1r = y12r + y14i;
  x1i = y12i - y14r;
  x0r = y13r - y15i;
  x0i = y13i + y15r;
  x2r = wn4r * (x0r - x0i);
  x2i = wn4r * (x0i + x0r);
  a[28] = x1r - x2i;
  a[29] = x1i + x2r;
  a[30] = x1r + x2i;
  a[31] = x1i - x2r;
}


//--------------------------------------------------------------------------------------
void Cssrc::cftf081(REAL *a, REAL *w)
{
  REAL wn4r, x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
  y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
  y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i;

  wn4r = w[1];
  x0r = a[0] + a[8];
  x0i = a[1] + a[9];
  x1r = a[0] - a[8];
  x1i = a[1] - a[9];
  x2r = a[4] + a[12];
  x2i = a[5] + a[13];
  x3r = a[4] - a[12];
  x3i = a[5] - a[13];
  y0r = x0r + x2r;
  y0i = x0i + x2i;
  y2r = x0r - x2r;
  y2i = x0i - x2i;
  y1r = x1r - x3i;
  y1i = x1i + x3r;
  y3r = x1r + x3i;
  y3i = x1i - x3r;
  x0r = a[2] + a[10];
  x0i = a[3] + a[11];
  x1r = a[2] - a[10];
  x1i = a[3] - a[11];
  x2r = a[6] + a[14];
  x2i = a[7] + a[15];
  x3r = a[6] - a[14];
  x3i = a[7] - a[15];
  y4r = x0r + x2r;
  y4i = x0i + x2i;
  y6r = x0r - x2r;
  y6i = x0i - x2i;
  x0r = x1r - x3i;
  x0i = x1i + x3r;
  x2r = x1r + x3i;
  x2i = x1i - x3r;
  y5r = wn4r * (x0r - x0i);
  y5i = wn4r * (x0r + x0i);
  y7r = wn4r * (x2r - x2i);
  y7i = wn4r * (x2r + x2i);
  a[8] = y1r + y5r;
  a[9] = y1i + y5i;
  a[10] = y1r - y5r;
  a[11] = y1i - y5i;
  a[12] = y3r - y7i;
  a[13] = y3i + y7r;
  a[14] = y3r + y7i;
  a[15] = y3i - y7r;
  a[0] = y0r + y4r;
  a[1] = y0i + y4i;
  a[2] = y0r - y4r;
  a[3] = y0i - y4i;
  a[4] = y2r - y6i;
  a[5] = y2i + y6r;
  a[6] = y2r + y6i;
  a[7] = y2i - y6r;
}


//--------------------------------------------------------------------------------------
void Cssrc::cftf082(REAL *a, REAL *w)
{
  REAL wn4r, wk1r, wk1i, x0r, x0i, x1r, x1i,
  y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
  y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i;

  wn4r = w[1];
  wk1r = w[4];
  wk1i = w[5];
  y0r = a[0] - a[9];
  y0i = a[1] + a[8];
  y1r = a[0] + a[9];
  y1i = a[1] - a[8];
  x0r = a[4] - a[13];
  x0i = a[5] + a[12];
  y2r = wn4r * (x0r - x0i);
  y2i = wn4r * (x0i + x0r);
  x0r = a[4] + a[13];
  x0i = a[5] - a[12];
  y3r = wn4r * (x0r - x0i);
  y3i = wn4r * (x0i + x0r);
  x0r = a[2] - a[11];
  x0i = a[3] + a[10];
  y4r = wk1r * x0r - wk1i * x0i;
  y4i = wk1r * x0i + wk1i * x0r;
  x0r = a[2] + a[11];
  x0i = a[3] - a[10];
  y5r = wk1i * x0r - wk1r * x0i;
  y5i = wk1i * x0i + wk1r * x0r;
  x0r = a[6] - a[15];
  x0i = a[7] + a[14];
  y6r = wk1i * x0r - wk1r * x0i;
  y6i = wk1i * x0i + wk1r * x0r;
  x0r = a[6] + a[15];
  x0i = a[7] - a[14];
  y7r = wk1r * x0r - wk1i * x0i;
  y7i = wk1r * x0i + wk1i * x0r;
  x0r = y0r + y2r;
  x0i = y0i + y2i;
  x1r = y4r + y6r;
  x1i = y4i + y6i;
  a[0] = x0r + x1r;
  a[1] = x0i + x1i;
  a[2] = x0r - x1r;
  a[3] = x0i - x1i;
  x0r = y0r - y2r;
  x0i = y0i - y2i;
  x1r = y4r - y6r;
  x1i = y4i - y6i;
  a[4] = x0r - x1i;
  a[5] = x0i + x1r;
  a[6] = x0r + x1i;
  a[7] = x0i - x1r;
  x0r = y1r - y3i;
  x0i = y1i + y3r;
  x1r = y5r - y7r;
  x1i = y5i - y7i;
  a[8] = x0r + x1r;
  a[9] = x0i + x1i;
  a[10] = x0r - x1r;
  a[11] = x0i - x1i;
  x0r = y1r + y3i;
  x0i = y1i - y3r;
  x1r = y5r + y7r;
  x1i = y5i + y7i;
  a[12] = x0r - x1i;
  a[13] = x0i + x1r;
  a[14] = x0r + x1i;
  a[15] = x0i - x1r;
}


//--------------------------------------------------------------------------------------
void Cssrc::cftf040(REAL *a)
{
  REAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

  x0r = a[0] + a[4];
  x0i = a[1] + a[5];
  x1r = a[0] - a[4];
  x1i = a[1] - a[5];
  x2r = a[2] + a[6];
  x2i = a[3] + a[7];
  x3r = a[2] - a[6];
  x3i = a[3] - a[7];
  a[0] = x0r + x2r;
  a[1] = x0i + x2i;
  a[4] = x0r - x2r;
  a[5] = x0i - x2i;
  a[2] = x1r - x3i;
  a[3] = x1i + x3r;
  a[6] = x1r + x3i;
  a[7] = x1i - x3r;
}


//--------------------------------------------------------------------------------------
void Cssrc::cftb040(REAL *a)
{
  REAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

  x0r = a[0] + a[4];
  x0i = a[1] + a[5];
  x1r = a[0] - a[4];
  x1i = a[1] - a[5];
  x2r = a[2] + a[6];
  x2i = a[3] + a[7];
  x3r = a[2] - a[6];
  x3i = a[3] - a[7];
  a[0] = x0r + x2r;
  a[1] = x0i + x2i;
  a[4] = x0r - x2r;
  a[5] = x0i - x2i;
  a[2] = x1r + x3i;
  a[3] = x1i - x3r;
  a[6] = x1r - x3i;
  a[7] = x1i + x3r;
}


//--------------------------------------------------------------------------------------
void Cssrc::cftx020(REAL *a)
{
  REAL x0r, x0i;

  x0r = a[0] - a[2];
  x0i = a[1] - a[3];
  a[0] += a[2];
  a[1] += a[3];
  a[2] = x0r;
  a[3] = x0i;
}


//--------------------------------------------------------------------------------------
void Cssrc::rftfsub(int n, REAL *a, int nc, REAL *c)
{
  int j, k, kk, ks, m;
  REAL wkr, wki, xr, xi, yr, yi;

  m = n >> 1;
  ks = 2 * nc / m;
  kk = 0;
  for (j = 2; j < m; j += 2)
  {
    k = n - j;
    kk += ks;
    wkr = REAL(0.5 - c[nc - kk]);
    wki = c[kk];
    xr = a[j] - a[k];
    xi = a[j + 1] + a[k + 1];
    yr = wkr * xr - wki * xi;
    yi = wkr * xi + wki * xr;
    a[j] -= yr;
    a[j + 1] -= yi;
    a[k] += yr;
    a[k + 1] -= yi;
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::rftbsub(int n, REAL *a, int nc, REAL *c)
{
  int j, k, kk, ks, m;
  REAL wkr, wki, xr, xi, yr, yi;

  m = n >> 1;
  ks = 2 * nc / m;
  kk = 0;
  for (j = 2; j < m; j += 2)
  {
    k = n - j;
    kk += ks;
    wkr = REAL(0.5 - c[nc - kk]);
    wki = c[kk];
    xr = a[j] - a[k];
    xi = a[j + 1] + a[k + 1];
    yr = wkr * xr + wki * xi;
    yi = wkr * xi - wki * xr;
    a[j] -= yr;
    a[j + 1] -= yi;
    a[k] += yr;
    a[k + 1] -= yi;
  }
}


//--------------------------------------------------------------------------------------
void Cssrc::dctsub(int n, REAL *a, int nc, REAL *c)
{
  int j, k, kk, ks, m;
  REAL wkr, wki, xr;

  m = n >> 1;
  ks = nc / n;
  kk = 0;
  for (j = 1; j < m; j++)
  {
    k = n - j;
    kk += ks;
    wkr = c[kk] - c[nc - kk];
    wki = c[kk] + c[nc - kk];
    xr = wki * a[j] - wkr * a[k];
    a[j] = wkr * a[j] + wki * a[k];
    a[k] = xr;
  }
  a[m] *= c[0];
}


//--------------------------------------------------------------------------------------
void Cssrc::dstsub(int n, REAL *a, int nc, REAL *c)
{
  int j, k, kk, ks, m;
  REAL wkr, wki, xr;

  m = n >> 1;
  ks = nc / n;
  kk = 0;
  for (j = 1; j < m; j++)
  {
    k = n - j;
    kk += ks;
    wkr = c[kk] - c[nc - kk];
    wki = c[kk] + c[nc - kk];
    xr = wki * a[k] - wkr * a[j];
    a[k] = wkr * a[k] + wki * a[j];
    a[j] = xr;
  }
  a[m] *= c[0];
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
Cssrc::Cssrc(void)
{
  filter2len = FFTFIRLEN; /* stage 2 filter length */
  filter1len = FFTFIRLEN; /* stage 1 filter length */
  spcount = 0;
  peak = 0;
  fft_ip = NULL;
  fft_w = NULL;
  f1order = NULL;
  f1inc = NULL;
  f2order = NULL;
  f2inc = NULL;
  rawinbuf = NULL;
  rawoutbuf = NULL;
  m_pResampleBuffer = NULL;
  inbuf = NULL;
  outbuf = NULL;
  buf1 = NULL;
  buf2 = NULL;
  stage1US = NULL;
  stage2US = NULL;
  stage1DS = NULL;
  stage2DS = NULL;
  UpSampling = false;
  DownSampling = false;
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
Cssrc::~Cssrc()
{
  DeInitialize();
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void Cssrc::DeInitialize()
{
  if (f1order) delete [] f1order; f1order = NULL;
  if (f1inc) delete [] f1inc; f1inc = NULL;
  if (stage1US)
  {
    if (stage1US[0])
    {
      delete [] stage1US[0];
      stage1US[0] = NULL;
    }
    delete [] stage1US;
    stage1US = NULL;
  }
  if (stage2US) delete [] stage2US; stage2US = NULL;
  if (fft_ip) delete [] fft_ip; fft_ip = NULL;
  if (fft_w) delete [] fft_w; fft_w = NULL;
  if (buf1)
  {
    for (int i = 0;i < nch;i++)
    {
      if (buf1[i])
      {
        delete [] buf1[i]; buf1[i] = NULL;
      }
    }
    delete [] buf1; buf1 = NULL;
  }
  if (buf2)
  {
    for (int i = 0;i < nch;i++)
    {
      if (buf2[i])
      {
        delete [] buf2[i]; buf2[i] = NULL;
      }
    }
    delete [] buf2; buf2 = NULL;
  }
  if (inbuf) delete [] inbuf; inbuf = NULL;
  if (outbuf) delete [] outbuf; outbuf = NULL;
  // if (rawinbuf) delete [] rawinbuf; rawinbuf = NULL;
  if (rawoutbuf) delete [] rawoutbuf; rawoutbuf = NULL;
  if (m_pResampleBuffer) delete [] m_pResampleBuffer; m_pResampleBuffer = NULL;

  if (stage1DS) delete [] stage1DS; stage1DS = NULL;
  if (stage2DS)
  {
    if (stage2DS[0])
    {
      delete [] stage2DS[0];
      stage2DS[0] = NULL;
    }
    delete [] stage2DS;
    stage2DS = NULL;
  }
  if (f2order) delete [] f2order; f2order = NULL;
  if (f2inc) delete [] f2inc; f2inc = NULL;
  UpSampling = false;
  DownSampling = false;
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
double Cssrc::alpha(double a)
{
  if (a <= 21) return 0;
  if (a <= 50) return 0.5842*pow(a - 21, 0.4) + 0.07886*(a - 21);
  return 0.1102*(a - 8.7);
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
double Cssrc::dbesi0(double x)
{
  int k;
  double w, t, y;
  static double a[65] = {
                          8.5246820682016865877e-11, 2.5966600546497407288e-9,
                          7.9689994568640180274e-8, 1.9906710409667748239e-6,
                          4.0312469446528002532e-5, 6.4499871606224265421e-4,
                          0.0079012345761930579108, 0.071111111109207045212,
                          0.444444444444724909, 1.7777777777777532045,
                          4.0000000000000011182, 3.99999999999999998,
                          1.0000000000000000001,
                          1.1520919130377195927e-10, 2.2287613013610985225e-9,
                          8.1903951930694585113e-8, 1.9821560631611544984e-6,
                          4.0335461940910133184e-5, 6.4495330974432203401e-4,
                          0.0079013012611467520626, 0.071111038160875566622,
                          0.44444450319062699316, 1.7777777439146450067,
                          4.0000000132337935071, 3.9999999968569015366,
                          1.0000000003426703174,
                          1.5476870780515238488e-10, 1.2685004214732975355e-9,
                          9.2776861851114223267e-8, 1.9063070109379044378e-6,
                          4.0698004389917945832e-5, 6.4370447244298070713e-4,
                          0.0079044749458444976958, 0.071105052411749363882,
                          0.44445280640924755082, 1.7777694934432109713,
                          4.0000055808824003386, 3.9999977081165740932,
                          1.0000004333949319118,
                          2.0675200625006793075e-10, -6.1689554705125681442e-10,
                          1.2436765915401571654e-7, 1.5830429403520613423e-6,
                          4.2947227560776583326e-5, 6.3249861665073441312e-4,
                          0.0079454472840953930811, 0.070994327785661860575,
                          0.44467219586283000332, 1.7774588182255374745,
                          4.0003038986252717972, 3.9998233869142057195,
                          1.0000472932961288324,
                          2.7475684794982708655e-10, -3.8991472076521332023e-9,
                          1.9730170483976049388e-7, 5.9651531561967674521e-7,
                          5.1992971474748995357e-5, 5.7327338675433770752e-4,
                          0.0082293143836530412024, 0.069990934858728039037,
                          0.44726764292723985087, 1.7726685170014087784,
                          4.0062907863712704432, 3.9952750700487845355,
                          1.0016354346654179322
                        };
  static double b[70] = {
                          6.7852367144945531383e-8, 4.6266061382821826854e-7,
                          6.9703135812354071774e-6, 7.6637663462953234134e-5,
                          7.9113515222612691636e-4, 0.0073401204731103808981,
                          0.060677114958668837046, 0.43994941411651569622,
                          2.7420017097661750609, 14.289661921740860534,
                          59.820609640320710779, 188.78998681199150629,
                          399.8731367825601118, 427.56411572180478514,
                          1.8042097874891098754e-7, 1.2277164312044637357e-6,
                          1.8484393221474274861e-5, 2.0293995900091309208e-4,
                          0.0020918539850246207459, 0.019375315654033949297,
                          0.15985869016767185908, 1.1565260527420641724,
                          7.1896341224206072113, 37.354773811947484532,
                          155.80993164266268457, 489.5211371158540918,
                          1030.9147225169564806, 1093.5883545113746958,
                          4.8017305613187493564e-7, 3.261317843912380074e-6,
                          4.9073137508166159639e-5, 5.3806506676487583755e-4,
                          0.0055387918291051866561, 0.051223717488786549025,
                          0.42190298621367914765, 3.0463625987357355872,
                          18.895299447327733204, 97.915189029455461554,
                          407.13940115493494659, 1274.3088990480582632,
                          2670.9883037012547506, 2815.7166284662544712,
                          1.2789926338424623394e-6, 8.6718263067604918916e-6,
                          1.3041508821299929489e-4, 0.001428224737372747892,
                          0.014684070635768789378, 0.13561403190404185755,
                          1.1152592585977393953, 8.0387088559465389038,
                          49.761318895895479206, 257.2684232313529138,
                          1066.8543146269566231, 3328.3874581009636362,
                          6948.8586598121634874, 7288.4893398212481055,
                          3.409350368197032893e-6, 2.3079025203103376076e-5,
                          3.4691373283901830239e-4, 0.003794994977222908545,
                          0.038974209677945602145, 0.3594948380414878371,
                          2.9522878893539528226, 21.246564609514287056,
                          131.28727387146173141, 677.38107093296675421,
                          2802.3724744545046518, 8718.5731420798254081,
                          18141.348781638832286, 18948.925349296308859
                        };
  static double c[45] = {
                          2.5568678676452702768e-15, 3.0393953792305924324e-14,
                          6.3343751991094840009e-13, 1.5041298011833009649e-11,
                          4.4569436918556541414e-10, 1.746393051427167951e-8,
                          1.0059224011079852317e-6, 1.0729838945088577089e-4,
                          0.05150322693642527738,
                          5.2527963991711562216e-15, 7.202118481421005641e-15,
                          7.2561421229904797156e-13, 1.482312146673104251e-11,
                          4.4602670450376245434e-10, 1.7463600061788679671e-8,
                          1.005922609132234756e-6, 1.0729838937545111487e-4,
                          0.051503226936437300716,
                          1.3365917359358069908e-14, -1.2932643065888544835e-13,
                          1.7450199447905602915e-12, 1.0419051209056979788e-11,
                          4.58047881980598326e-10, 1.7442405450073548966e-8,
                          1.0059461453281292278e-6, 1.0729837434500161228e-4,
                          0.051503226940658446941,
                          5.3771611477352308649e-14, -1.1396193006413731702e-12,
                          1.2858641335221653409e-11, -5.9802086004570057703e-11,
                          7.3666894305929510222e-10, 1.6731837150730356448e-8,
                          1.0070831435812128922e-6, 1.0729733111203704813e-4,
                          0.051503227360726294675,
                          3.7819492084858931093e-14, -4.8600496888588034879e-13,
                          1.6898350504817224909e-12, 4.5884624327524255865e-11,
                          1.2521615963377513729e-10, 1.8959658437754727957e-8,
                          1.0020716710561353622e-6, 1.073037119856927559e-4,
                          0.05150322383300230775
                        };

  w = fabs(x);
  if (w < 8.5)
  {
    t = w * w * 0.0625;
    k = 13 * ((int) t);
    y = (((((((((((a[k] * t + a[k + 1]) * t +
                  a[k + 2]) * t + a[k + 3]) * t + a[k + 4]) * t +
               a[k + 5]) * t + a[k + 6]) * t + a[k + 7]) * t +
            a[k + 8]) * t + a[k + 9]) * t + a[k + 10]) * t +
         a[k + 11]) * t + a[k + 12];
  }
  else if (w < 12.5)
  {
    k = (int) w;
    t = w - k;
    k = 14 * (k - 8);
    y = ((((((((((((b[k] * t + b[k + 1]) * t +
                   b[k + 2]) * t + b[k + 3]) * t + b[k + 4]) * t +
                b[k + 5]) * t + b[k + 6]) * t + b[k + 7]) * t +
             b[k + 8]) * t + b[k + 9]) * t + b[k + 10]) * t +
          b[k + 11]) * t + b[k + 12]) * t + b[k + 13];
  }
  else
  {
    t = 60 / w;
    k = 9 * ((int) t);
    y = ((((((((c[k] * t + c[k + 1]) * t +
               c[k + 2]) * t + c[k + 3]) * t + c[k + 4]) * t +
            c[k + 5]) * t + c[k + 6]) * t + c[k + 7]) * t +
         c[k + 8]) * sqrt(t) * exp(w);
  }
  return y;
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
double Cssrc::win(double n, int len, double alp, double iza)
{
  return dbesi0(alp*sqrt(1 - 4*n*n / (((double)len - 1)*((double)len - 1)))) / iza;
}
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
double Cssrc::sinc(double x)
{
  return x == 0 ? 1 : sin(x) / x;
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
double Cssrc::hn_lpf(int n, double lpf, double fs)
{
  double t = 1 / fs;
  double omega = 2 * M_PI * lpf;
  return 2*lpf*t*sinc(n*omega*t);
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
int Cssrc::gcd(int x, int y)
{
  int t;

  while (y != 0)
  {
    t = x % y; x = y; y = t;
  }
  return x;
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
int Cssrc::extract_int(unsigned char *buf)
{
#ifndef BIGENDIAN
  return *(int *)buf;
#else
  return ((int)buf[0]) | (((int)buf[1]) << 8) |
         (((int)buf[2]) << 16) | (((int)((char *)buf)[3]) << 24);
#endif
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
short Cssrc::extract_short(unsigned char *buf)
{
#ifndef BIGENDIAN
  return *(short *)buf;
#else
  return ((short)buf[0]) | (((short)((char *)buf)[1]) << 8);
#endif
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void Cssrc::bury_int(unsigned char *buf, int i)
{
#ifndef BIGENDIAN
  *(int *)buf = i;
#else
  buf[0] = i & 0xff; i >>= 8;
  buf[1] = i & 0xff; i >>= 8;
  buf[2] = i & 0xff; i >>= 8;
  buf[3] = i & 0xff;
#endif
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void Cssrc::bury_short(unsigned char *buf, short s)
{
#ifndef BIGENDIAN
  *(short *)buf = s;
#else
  buf[0] = s & 0xff; s >>= 8;
  buf[1] = s & 0xff;
#endif
}

//---------------------------------------------------------------------------
// Inits Filter Stage 1 + 2, returns false if error
//---------------------------------------------------------------------------
bool Cssrc::InitFilters(void)
{
  //-----create FILTER 1----------
  if (UpSampling)
  {
    double aa = AA; // stop band attenuation(dB)
    double lpf, delta, d, df, alp, iza;
    double guard = 2;

    frqgcd = gcd(sfrq, dfrq);

    fs1 = sfrq / frqgcd * dfrq;

    if (fs1 / dfrq == 1) osf = 1;
    else if (fs1 / dfrq % 2 == 0) osf = 2;
    else if (fs1 / dfrq % 3 == 0) osf = 3;
    else
      return (false); //,"Resampling from %dHz to %dHz is not supported.\n",sfrq,dfrq);

    df = (dfrq * osf / 2 - sfrq / 2) * 2 / guard;
    lpf = sfrq / 2 + (dfrq * osf / 2 - sfrq / 2) / guard;

    delta = pow(10.0, -aa / 20);
    if (aa <= 21) d = 0.9222; else d = (aa - 7.95) / 14.36;

    n1 = (int)(fs1 / df * d + 1);
    if (n1 % 2 == 0) n1++;

    alp = alpha(aa);
    iza = dbesi0(alp);

    n1y = fs1 / sfrq;
    n1x = n1 / n1y + 1;

    f1order = new int[n1y * osf];
    for (int i = 0;i < n1y*osf;i++)
    {
      f1order[i] = fs1 / sfrq - (i * (fs1 / (dfrq * osf))) % (fs1 / sfrq);
      if (f1order[i] == fs1 / sfrq) f1order[i] = 0;
    }

    f1inc = new int [n1y * osf];
    for (int i = 0;i < n1y*osf;i++)
    {
      f1inc[i] = f1order[i] < fs1 / (dfrq * osf) ? nch : 0;
      if (f1order[i] == fs1 / sfrq) f1order[i] = 0;
    }

    stage1US = new REAL * [n1y];
    stage1US[0] = new REAL[n1x * n1y];

    for (int i = 1;i < n1y;i++)
    {
      stage1US[i] = &(stage1US[0][n1x * i]);
      for (int j = 0;j < n1x;j++) stage1US[i][j] = 0;
    }

    for (int i = -(n1 / 2);i <= n1 / 2;i++)
    {
      stage1US[(i + n1 / 2) % n1y][(i + n1 / 2) / n1y] = REAL(win(i, n1, alp, iza) * hn_lpf(i, lpf, fs1) * fs1 / sfrq);
    }
  }
  else
  {
    double aa = AA; // stop band attenuation(dB)
    double lpf, delta, d, df, alp, iza;
    int ipsize, wsize;

    frqgcd = gcd(sfrq, dfrq);

    if (dfrq / frqgcd == 1) osf = 1;
    else if (dfrq / frqgcd % 2 == 0) osf = 2;
    else if (dfrq / frqgcd % 3 == 0) osf = 3;
    else
      return (false); //,"Resampling from %dHz to %dHz is not supported.\n",sfrq,dfrq);

    fs1 = sfrq * osf;

    delta = pow(10.0, -aa / 20);
    if (aa <= 21) d = 0.9222; else d = (aa - 7.95) / 14.36;

    n1 = filter1len;
    for (int i = 1;;i = i * 2)
    {
      n1 = filter1len * i;
      if (n1 % 2 == 0) n1--;
      df = (fs1 * d) / (n1 - 1);
      lpf = (dfrq - df) / 2;
      if (df < DF) break;
    }

    alp = alpha(aa);

    iza = dbesi0(alp);

    for (n1b = 1;n1b < n1;n1b *= 2) {}
    n1b *= 2;

    stage1DS = new REAL[n1b];

    for (int i = 0;i < n1b;i++) stage1DS[i] = 0;

    for (int i = -(n1 / 2);i <= n1 / 2;i++)
    {
      stage1DS[i + n1 / 2] = REAL(win(i, n1, alp, iza) * hn_lpf(i, lpf, fs1) * fs1 / sfrq / n1b * 2);
    }

    ipsize = (int)(2 + sqrt((double)n1b));
    fft_ip = new int [ipsize];
    fft_ip[0] = 0;
    wsize = n1b / 2;
    fft_w = new REAL[wsize];

    rdft(n1b, 1, stage1DS, fft_ip, fft_w);
  }

  //------create Stage 2 filter---------
  // Make stage 2 filter
  if (UpSampling)
  {
    double aa = AA; // stop band attenuation(dB)
    double lpf, delta, d, df, alp, iza;
    int ipsize, wsize;

    delta = pow(10.0, -aa / 20);
    if (aa <= 21) d = 0.9222; else d = (aa - 7.95) / 14.36;

    fs2 = dfrq * osf;

    for (int i = 1;;i = i * 2)
    {
      n2 = filter2len * i;
      if (n2 % 2 == 0) n2--;
      df = (fs2 * d) / (n2 - 1);
      lpf = sfrq / 2;
      if (df < DF) break;
    }

    alp = alpha(aa);

    iza = dbesi0(alp);

    for (n2b = 1;n2b < n2;n2b *= 2) {}
    n2b *= 2;

    stage2US = new REAL[n2b];

    for (int i = 0;i < n2b;i++) stage2US[i] = 0;

    for (int i = -(n2 / 2);i <= n2 / 2;i++)
    {
      stage2US[i + n2 / 2] = REAL(win(i, n2, alp, iza) * hn_lpf(i, lpf, fs2) / n2b * 2);
    }

    ipsize = (int)(2 + sqrt((double)n2b));
    fft_ip = new int[ipsize];
    fft_ip[0] = 0;
    wsize = n2b / 2;
    fft_w = new REAL[wsize];

    rdft(n2b, 1, stage2US, fft_ip, fft_w);
  }
  else
  {
    if (osf == 1)
    {
      fs2 = sfrq / frqgcd * dfrq;
      n2 = 1;
      n2y = n2x = 1;
      f2order = new int[n2y];
      f2order[0] = 0;
      f2inc = new int[n2y];
      f2inc[0] = sfrq / dfrq;
      stage2DS = new REAL * [n2y];
      stage2DS[0] = new REAL[n2x * n2y];
      stage2DS[0][0] = 1;
    }
    else
    {
      double aa = AA; // stop band attenuation(dB)
      double lpf, delta, d, df, alp, iza;
      double guard = 2;

      fs2 = sfrq / frqgcd * dfrq ;

      df = (fs1 / 2 - sfrq / 2) * 2 / guard;
      lpf = sfrq / 2 + (fs1 / 2 - sfrq / 2) / guard;

      delta = pow(10.0, -aa / 20);
      if (aa <= 21) d = 0.9222; else d = (aa - 7.95) / 14.36;

      n2 = (int)(fs2 / df * d + 1);
      if (n2 % 2 == 0) n2++;

      alp = alpha(aa);
      iza = dbesi0(alp);

      n2y = fs2 / fs1;
      n2x = n2 / n2y + 1;

      f2order = new int[n2y];
      for (int i = 0;i < n2y;i++)
      {
        f2order[i] = fs2 / fs1 - (i * (fs2 / dfrq)) % (fs2 / fs1);
        if (f2order[i] == fs2 / fs1) f2order[i] = 0;
      }

      f2inc = new int[n2y];
      for (int i = 0;i < n2y;i++)
      {
        f2inc[i] = (fs2 / dfrq - f2order[i]) / (fs2 / fs1) + 1;
        if (f2order[i + 1 == n2y ? 0 : i + 1] == 0) f2inc[i]--;
      }

      stage2DS = new REAL * [n2y];
      stage2DS[0] = new REAL[n2x * n2y];

      for (int i = 1;i < n2y;i++)
      {
        stage2DS[i] = &(stage2DS[0][n2x * i]);
        for (int j = 0;j < n2x;j++) stage2DS[i][j] = 0;
      }

      for (int i = -(n2 / 2);i <= n2 / 2;i++)
      {
        stage2DS[(i + n2 / 2) % n2y][(i + n2 / 2) / n2y] = REAL(win(i, n2, alp, iza) * hn_lpf(i, lpf, fs2) * fs2 / fs1);
      }
    }
  }

  //---------init ready for data-------------------
  delay = 0;
  inbuflen = 0;
  if (UpSampling)
  {
    n2b2 = n2b / 2;

    buf1 = new REAL * [nch];
    for (int i = 0;i < nch;i++)
    {
      buf1[i] = new REAL[n2b2 / osf + 1];
      for (int j = 0;j < (n2b2 / osf + 1);j++) buf1[i][j] = 0;
    }

    buf2 = new REAL * [nch];
    for (int i = 0;i < nch;i++) buf2[i] = new REAL[n2b];

    //  rawinbuf  = new unsigned char [nch*(n2b2+n1x) * bps]; //calloc(nch*(n2b2+n1x),bps);
    //  ZeroMemory(rawinbuf, nch*(n2b2+n1x) * bps);
    rawinbuf = NULL;
    rawoutbuf = new unsigned char [dbps * nch * (n2b2 / osf + 1)];

    // Max Size of input we can take at once
    m_iMaxInputSize = nch * (n2b2 + n1x) * bps;
    // Our resampled output buffer
    m_iResampleBufferSize = m_iOutputBufferSize + dbps * nch * (n2b2 / osf + 1);
    m_pResampleBuffer = new unsigned char[m_iResampleBufferSize];
    m_iResampleBufferPos = 0;

    inbuf = new REAL [nch * (n2b2 + n1x)]; // calloc(nch*(n2b2+n1x),sizeof(REAL));
    ZeroMemory(inbuf, nch*(n2b2 + n1x) * sizeof(REAL));

    outbuf = new REAL [nch * (n2b2 / osf + 1)];

    s1p = 0;
    rp = 0;
    ds = 0;
    osc = 0;

    init = 1;
    ending = 0;
    inbuflen = n1 / 2 / (fs1 / sfrq) + 1;
    delay = (int)((double)n2 / 2 / (fs2 / dfrq));

    sumread = sumwrite = 0;

  }
  else
  {
    n1b2 = n1b / 2;

    //    |....B....|....C....|   buf1      n1b2+n1b2
    //|.A.|....D....|             buf2  n2x+n1b2
    //
    // まずinbufからBにosf買Tンプリングしながらコピー
    // Cはクリア
    // BCにstage 1 filterをかける
    // DにBを足す
    // ADにstage 2 filterをかける
    // Dの後ろをAに移動
    // CをDにコピー

    buf1 = new REAL * [nch];
    for (int i = 0;i < nch;i++)
      buf1[i] = new REAL[n1b];

    buf2 = new REAL * [nch];
    for (int i = 0;i < nch;i++)
    {
      buf2[i] = new REAL[n2x + 1 + n1b2];
      for (int j = 0;j < n2x + 1 + n1b2;j++) buf2[i][j] = 0;
    }

    //  rawinbuf  = new unsigned char [nch*(n1b2/osf+osf+1) * bps];
    //  ZeroMemory(rawinbuf, nch*(n1b2/osf+osf+1) * bps);
    rawinbuf = NULL;
    rawoutbuf = new unsigned char [(int)(dbps * nch * ((double)n1b2 * sfrq / dfrq + 1))];

    // Max Size of input we can take at once
    m_iMaxInputSize = nch * (n1b2 / osf + osf + 1) * bps;

    // Our resampled output buffer
    m_iResampleBufferSize = m_iOutputBufferSize + (int)(dbps * nch * ((double)n1b2 * sfrq / dfrq + 1));
    m_pResampleBuffer = new unsigned char[m_iResampleBufferSize];
    m_iResampleBufferPos = 0;

    inbuf = new REAL [nch * (n1b2 / osf + osf + 1)];
    ZeroMemory(inbuf, nch*(n1b2 / osf + osf + 1) * sizeof(REAL));

    outbuf = new REAL[(int)(nch * ((double)n1b2 * sfrq / dfrq + 1))];

    op = outbuf;

    s2p = 0;
    rp = 0;
    rps = 0;
    ds = 0;
    osc = 0;
    rp2 = 0;

    init = 1;
    ending = 0;
    delay = (int)((double)n1 / 2 / ((double)fs1 / dfrq) + (double)n2 / 2 / ((double)fs2 / dfrq));

    sumread = sumwrite = 0;
  }
  return (true);
}

//---------------------------------------------------------------------------
// Inits Freq Converter, returns false if cannot do
//---------------------------------------------------------------------------
bool Cssrc::InitConverter(int OldFreq, int OldBPS, int Channels, int NewFreq, int NewBPS, int OutputBufferSize)
{
  // The amount of data taken from the output buffer at a time
  m_iOutputBufferSize = OutputBufferSize;

  nch = Channels; // input channels
  sfrq = OldFreq; // input freq
  bps = OldBPS / 8; // input bits

  dfrq = NewFreq; // output sample rate
  dbps = NewBPS / 8; // output bytes per second

  if (dfrq == sfrq)
  {
    // setup output buffer size'd resample buffer
    m_pResampleBuffer = new unsigned char[m_iOutputBufferSize];
    m_iResampleBufferPos = 0;
    return (true); // nothing to change so exit
  }

  if (bps != 1 && bps != 2 && bps != 3 && bps != 4)  // only 8 16 24 supported
    return (false);

  if (sfrq < dfrq)
    UpSampling = true;
  else
    DownSampling = true;

  if (!InitFilters())
    return (false);

  return (true);
}

//---------------------------------------------------------------------------
// Upsamples a buffer full of rawindata
// returns the datalength
//---------------------------------------------------------------------------
int Cssrc::UpSampleRawIn(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int toberead2, int nsmplread)
{
  int i = 0;

  switch (bps)
  {
  case 1:
    for (i = 0; i < nsmplread * nch; i++)
      inbuf[nch * inbuflen + i] = (1 / (REAL)0x7f) * ((REAL)((unsigned char *)rawinbuf)[i] - 128);
    break;

  case 2:
#ifndef BIGENDIAN
    for (i = 0;i < nsmplread*nch;i++)
      inbuf[nch*inbuflen + i] = (1 / (REAL)0x7fff) * (REAL)((short *)rawinbuf)[i];
#else
    for (i = 0;i < nsmplread*nch;i++)
      inbuf[nch*inbuflen + i] = (1 / (REAL)0x7fff) * (((int)rawinbuf[i * 2]) | (((int)((char *)rawinbuf)[i * 2 + 1]) << 8));
#endif
    break;

  case 3:
    for (i = 0;i < nsmplread*nch;i++)
      inbuf[nch*inbuflen + i] = (1 / (REAL)0x7fffff) * ((((int)rawinbuf[i * 3 ]) << 0 ) | (((int)rawinbuf[i * 3 + 1]) << 8 ) | (((int)((char *)rawinbuf)[i * 3 + 2]) << 16));
    break;

  case 4:
    for (i = 0;i < nsmplread*nch;i++)
      inbuf[nch*inbuflen + i] = (1 / (REAL)0x7fffffff) * ((((int)rawinbuf[i * 4 ]) << 0 ) | (((int)rawinbuf[i * 4 + 1]) << 8 ) | (((int)rawinbuf[i * 4 + 2]) << 16) | (((int)((char *)rawinbuf)[i * 4 + 3]) << 24));
    break;
  }
  for (;i < nch*toberead2;i++)
    inbuf[nch*inbuflen + i] = 0;

  return UpSampleCommon(pRetDataPtr, IsEof, toberead, toberead2, nsmplread);
}

int Cssrc::UpSampleFloatIn(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int toberead2, int nsmplread)
{
  float *floatIn = (float *)rawinbuf;
  memcpy(inbuf + nch*inbuflen, floatIn, nsmplread*nch*sizeof(REAL));

  // pad with zeros
  for (int i = nsmplread*nch; i < nch*toberead2; i++)
    inbuf[nch*inbuflen + i] = 0;

  return UpSampleCommon(pRetDataPtr, IsEof, toberead, toberead2, nsmplread);
}

int Cssrc::UpSampleCommon(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int toberead2, int nsmplread)
{
  int i, j;
  double att = 0;
  double gain = pow(10.0, -att / 20);

  int ToRet = 0;

  bool BreakOut = false;

  inbuflen += toberead2;
  sumread += nsmplread;
  ending = IsEof;

  nsmplwrt1 = n2b2;


  // apply stage 1 filter
  ip = &inbuf[((sfrq * (rp - 1) + fs1) / fs1) * nch];
  s1p_backup = s1p;
  ip_backup = ip;
  osc_backup = osc;

  for (ch = 0;ch < nch;ch++)
  {
    REAL *op = &outbuf[ch];
    int no = n1y * osf;

    s1p = s1p_backup; ip = ip_backup + ch;

    switch (n1x)
    {
    case 7:
      for (p = 0;p < nsmplwrt1;p++)
      {
        int s1o = f1order[s1p];

        buf2[ch][p] = stage1US[s1o][0] * *(ip + 0 * nch) + stage1US[s1o][1] * *(ip + 1 * nch) + stage1US[s1o][2] * *(ip + 2 * nch) + stage1US[s1o][3] * *(ip + 3 * nch) + stage1US[s1o][4] * *(ip + 4 * nch) + stage1US[s1o][5] * *(ip + 5 * nch) + stage1US[s1o][6] * *(ip + 6 * nch);
        ip += f1inc[s1p];
        s1p++;
        if (s1p == no)
          s1p = 0;
      }
      break;

    case 9:
      for (p = 0;p < nsmplwrt1;p++)
      {
        int s1o = f1order[s1p];

        buf2[ch][p] = stage1US[s1o][0] * *(ip + 0 * nch) + stage1US[s1o][1] * *(ip + 1 * nch) + stage1US[s1o][2] * *(ip + 2 * nch) + stage1US[s1o][3] * *(ip + 3 * nch) + stage1US[s1o][4] * *(ip + 4 * nch) + stage1US[s1o][5] * *(ip + 5 * nch) + stage1US[s1o][6] * *(ip + 6 * nch) + stage1US[s1o][7] * *(ip + 7 * nch) + stage1US[s1o][8] * *(ip + 8 * nch);
        ip += f1inc[s1p];
        s1p++;
        if (s1p == no)
          s1p = 0;
      }
      break;

    default:
      for (p = 0;p < nsmplwrt1;p++)
      {
        REAL tmp = 0;
        REAL *ip2 = ip;

        int s1o = f1order[s1p];

        for (i = 0;i < n1x;i++)
        {
          tmp += stage1US[s1o][i] * *ip2;
          ip2 += nch;
        }
        buf2[ch][p] = tmp;
        ip += f1inc[s1p];
        s1p++;
        if (s1p == no)
          s1p = 0;
      }
      break;
    }
    osc = osc_backup;

    // apply stage 2 filter
    for (p = nsmplwrt1;p < n2b;p++)
      buf2[ch][p] = 0;

    rdft(n2b, 1, buf2[ch], fft_ip, fft_w);

    buf2[ch][0] = stage2US[0] * buf2[ch][0];
    buf2[ch][1] = stage2US[1] * buf2[ch][1];

    for (i = 1;i < n2b / 2;i++)
    {
      REAL re, im;

      re = stage2US[i * 2 ] * buf2[ch][i * 2] - stage2US[i * 2 + 1] * buf2[ch][i * 2 + 1];
      im = stage2US[i * 2 + 1] * buf2[ch][i * 2] + stage2US[i * 2 ] * buf2[ch][i * 2 + 1];

      buf2[ch][i*2 ] = re;
      buf2[ch][i*2 + 1] = im;
    }
    rdft(n2b, -1, buf2[ch], fft_ip, fft_w);

    for (i = osc, j = 0;i < n2b2;i += osf, j++)
    {
      REAL f = (buf1[ch][j] + buf2[ch][i]);
      op[j*nch] = f;
    }
    nsmplwrt2 = j;
    osc = i - n2b2;
    for (j = 0;i < n2b;i += osf, j++)
      buf1[ch][j] = buf2[ch][i];
  }
  rp += nsmplwrt1 * (sfrq / frqgcd) / osf;

  switch (dbps)
  {
  case 1:
    {
      REAL gain2 = REAL(gain * (REAL)0x7f);
      ch = 0;
      for (i = 0;i < nsmplwrt2*nch;i++)
      {
        int s = RINT(outbuf[i] * gain2);
        if (s < -0x80)
        {
          double d = (double)s / -0x80;
          peak = peak < d ? d : peak;
          s = -0x80;
        }
        if (0x7f < s)
        {
          double d = (double)s / 0x7f;
          peak = peak < d ? d : peak;
          s = 0x7f;
        }
        ((unsigned char *)rawoutbuf)[i] = s + 0x80;

        ch++;
        if (ch == nch)
          ch = 0;
      }
    }
    break;

  case 2:
    {
      REAL gain2 = REAL(gain * (REAL)0x7fff);
      ch = 0;

      for (i = 0;i < nsmplwrt2*nch;i++)
      {
        int s = RINT(outbuf[i] * gain2);

        if (s < -0x8000)
        {
          double d = (double)s / -0x8000;
          peak = peak < d ? d : peak;
          s = -0x8000;
        }
        if (0x7fff < s)
        {
          double d = (double)s / 0x7fff;
          peak = peak < d ? d : peak;
          s = 0x7fff;
        }

#ifndef BIGENDIAN
        ((short *)rawoutbuf)[i] = s;
#else
        ((char *)rawoutbuf)[i*2 ] = s & 255; s >>= 8;
        ((char *)rawoutbuf)[i*2 + 1] = s & 255;
#endif
        ch++;
        if (ch == nch)
          ch = 0;
      }
    }
    break;

  case 3:
    {
      REAL gain2 = REAL(gain * (REAL)0x7fffff);
      ch = 0;

      for (i = 0;i < nsmplwrt2*nch;i++)
      {
        int s = RINT(outbuf[i] * gain2);

        if (s < -0x800000)
        {
          double d = (double)s / -0x800000;
          peak = peak < d ? d : peak;
          s = -0x800000;
        }
        if (0x7fffff < s)
        {
          double d = (double)s / 0x7fffff;
          peak = peak < d ? d : peak;
          s = 0x7fffff;
        }

        ((char *)rawoutbuf)[i*3 ] = s & 255; s >>= 8;
        ((char *)rawoutbuf)[i*3 + 1] = s & 255; s >>= 8;
        ((char *)rawoutbuf)[i*3 + 2] = s & 255;

        ch++;
        if (ch == nch)
          ch = 0;
      }
    }
    break;

  }

  if (!init)
  {
    if (ending)
    {
      if ((double)sumread*dfrq / sfrq + 2 > sumwrite + nsmplwrt2)
      {
        //     FileO.WriteXBytesToFile(rawoutbuf, dbps*nch*nsmplwrt2);
        ToRet = dbps * nch * nsmplwrt2;
        *pRetDataPtr = rawoutbuf;

        sumwrite += nsmplwrt2;
      }
      else
      {
        int Writing = (int)(floor((double)sumread * dfrq / sfrq) + 2 - sumwrite);
        //     FileO.WriteXBytesToFile(rawoutbuf, dbps*nch*Writing);
        ToRet = dbps * nch * Writing;
        *pRetDataPtr = rawoutbuf;

        sumwrite += Writing;
        BreakOut = true;
      }
    }
    else
    {
      //    FileO.WriteXBytesToFile(rawoutbuf, dbps*nch*nsmplwrt2);
      ToRet = dbps * nch * nsmplwrt2;
      *pRetDataPtr = rawoutbuf;

      sumwrite += nsmplwrt2;
    }
  }
  else
  {
    if (nsmplwrt2 < delay)
    {
      delay -= nsmplwrt2;
    }
    else
    {
      if (ending)
      {
        if ((double)sumread*dfrq / sfrq + 2 > sumwrite + nsmplwrt2 - delay)
        {
          //      FileO.WriteXBytesToFile(rawoutbuf+dbps*nch*delay, dbps*nch*(nsmplwrt2-delay));
          ToRet = dbps * nch * (nsmplwrt2 - delay);
          *pRetDataPtr = (rawoutbuf + dbps * nch * delay);

          sumwrite += nsmplwrt2 - delay;
        }
        else
        {
          int ToWrite = (int)(floor((double)sumread * dfrq / sfrq) + 2 - sumwrite - delay);
          //      FileO.WriteXBytesToFile(rawoutbuf+dbps*nch*delay, dbps*nch*ToWrite);
          ToRet = dbps * nch * ToWrite;
          *pRetDataPtr = (rawoutbuf + dbps * nch * delay);

          sumwrite += ToWrite;
          BreakOut = true;
        }
      }
      else
      {
        //     FileO.WriteXBytesToFile(rawoutbuf+dbps*nch*delay, dbps*nch*(nsmplwrt2-delay));
        ToRet = dbps * nch * (nsmplwrt2 - delay);
        *pRetDataPtr = (rawoutbuf + dbps * nch * delay);

        sumwrite += nsmplwrt2 - delay;
        init = 0;
      }
    }
  }

  if (!BreakOut)
  {
    int ds = (rp - 1) / (fs1 / sfrq);

    memmove(inbuf, inbuf + nch*ds, sizeof(REAL)*nch*(inbuflen - ds));
    inbuflen -= ds;
    rp -= ds * (fs1 / sfrq);
  }

  return (ToRet);
}

//---------------------------------------------------------------------------
// Downsamples a buffer full of rawindata
// returns the datalength
//---------------------------------------------------------------------------
int Cssrc::DownSampleRawIn(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int nsmplread)
{
  int i = 0;

  switch (bps)
  {
  case 1:
    for (i = 0; i < nsmplread * nch; i++)
      inbuf[nch * inbuflen + i] = (1 / (REAL)0x7f) * ((REAL)((unsigned char *)rawinbuf)[i] - 128);
    break;

  case 2:
#ifndef BIGENDIAN
    for (i = 0;i < nsmplread*nch;i++)
      inbuf[nch*inbuflen + i] = (1 / (REAL)0x7fff) * (REAL)((short *)rawinbuf)[i];
#else
    for (i = 0;i < nsmplread*nch;i++)
      inbuf[nch*inbuflen + i] = (1 / (REAL)0x7fff) * (((int)rawinbuf[i * 2]) | (((int)((char *)rawinbuf)[i * 2 + 1]) << 8));
#endif
    break;

  case 3:
    for (i = 0;i < nsmplread*nch;i++)
      inbuf[nch*inbuflen + i] = (1 / (REAL)0x7fffff) * ((((int)rawinbuf[i * 3 ]) << 0 ) | (((int)rawinbuf[i * 3 + 1]) << 8 ) | (((int)((char *)rawinbuf)[i * 3 + 2]) << 16));
    break;

  case 4:
    for (i = 0;i < nsmplread*nch;i++)
      inbuf[nch*inbuflen + i] = (1 / (REAL)0x7fffffff) * ((((int)rawinbuf[i * 4 ]) << 0 ) | (((int)rawinbuf[i * 4 + 1]) << 8 ) | (((int)rawinbuf[i * 4 + 2]) << 16) | (((int)((char *)rawinbuf)[i * 4 + 3]) << 24));
    break;
  }

  for (; i < nch*toberead; i++)
    inbuf[i] = 0;

  return DownSampleCommon(pRetDataPtr, IsEof, toberead, nsmplread);
}

int Cssrc::DownSampleFloatIn(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int nsmplread)
{
  float *floatIn = (float *)rawinbuf;
  memcpy(inbuf + nch*inbuflen, floatIn, nsmplread*nch*sizeof(REAL));

  // pad with zeros
  for (int i = nsmplread*nch; i < nch*toberead; i++)
    inbuf[nch*inbuflen + i] = 0;

  return DownSampleCommon(pRetDataPtr, IsEof, toberead, nsmplread);
}

int Cssrc::DownSampleCommon(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int nsmplread)
{
  int i, j;
  double att = 0;
  double gain = pow(10.0, -att / 20);

  int ToRet = 0;

  bool BreakOut = false;

  sumread += nsmplread;
  ending = IsEof;
  rps_backup = rps;
  s2p_backup = s2p;

  for (ch = 0;ch < nch;ch++)
  {
    rps = rps_backup;
    for (k = 0;k < rps;k++)
      buf1[ch][k] = 0;

    for (i = rps, j = 0;i < n1b2;i += osf, j++)
    {
      buf1[ch][i] = inbuf[j * nch + ch];

      for (k = i + 1;k < i + osf;k++)
        buf1[ch][k] = 0;
    }

    for (k = n1b2;k < n1b;k++)
      buf1[ch][k] = 0;

    rps = i - n1b2;
    rp += j;

    rdft(n1b, 1, buf1[ch], fft_ip, fft_w);

    buf1[ch][0] = stage1DS[0] * buf1[ch][0];
    buf1[ch][1] = stage1DS[1] * buf1[ch][1];

    for (i = 1;i < n1b2;i++)
    {
      REAL re, im;

      re = stage1DS[i * 2 ] * buf1[ch][i * 2] - stage1DS[i * 2 + 1] * buf1[ch][i * 2 + 1];
      im = stage1DS[i * 2 + 1] * buf1[ch][i * 2] + stage1DS[i * 2 ] * buf1[ch][i * 2 + 1];

      buf1[ch][i*2 ] = re;
      buf1[ch][i*2 + 1] = im;
    }

    rdft(n1b, -1, buf1[ch], fft_ip, fft_w);

    for (i = 0;i < n1b2;i++)
      buf2[ch][n2x + 1 + i] += buf1[ch][i];

    {
      int t1 = rp2 / (fs2 / fs1);
      if (rp2 % (fs2 / fs1) != 0)
        t1++;
      bp = &(buf2[ch][t1]);
    }

    s2p = s2p_backup;

    for (p = 0;bp - buf2[ch] < n1b2 + 1;p++)
    {
      REAL tmp = 0;
      REAL *bp2;
      int s2o;

      bp2 = bp;
      s2o = f2order[s2p];
      bp += f2inc[s2p];
      s2p++;

      if (s2p == n2y)
        s2p = 0;

      for (i = 0;i < n2x;i++)
        tmp += stage2DS[s2o][i] * *bp2++;

      op[p*nch + ch] = tmp;
    }

    nsmplwrt2 = p;
  }

  rp2 += nsmplwrt2 * (fs2 / dfrq);

  switch (dbps)
  {
  case 1:
    {
      REAL gain2 = REAL(gain * (REAL)0x7f);
      ch = 0;

      for (i = 0;i < nsmplwrt2*nch;i++)
      {
        int s;

        s = RINT(outbuf[i] * gain2);
        if (s < -0x80)
        {
          double d = (double)s / -0x80;
          peak = peak < d ? d : peak;
          s = -0x80;
        }
        if (0x7f < s)
        {
          double d = (double)s / 0x7f;
          peak = peak < d ? d : peak;
          s = 0x7f;
        }

        ((unsigned char *)rawoutbuf)[i] = s + 0x80;

        ch++;
        if (ch == nch)
          ch = 0;
      }
    }
    break;

  case 2:
    {
      REAL gain2 = REAL(gain * (REAL)0x7fff);
      ch = 0;

      for (i = 0;i < nsmplwrt2*nch;i++)
      {
        int s;

        s = RINT(outbuf[i] * gain2);

        if (s < -0x8000)
        {
          double d = (double)s / -0x8000;
          peak = peak < d ? d : peak;
          s = -0x8000;
        }
        if (0x7fff < s)
        {
          double d = (double)s / 0x7fff;
          peak = peak < d ? d : peak;
          s = 0x7fff;
        }

#ifndef BIGENDIAN
        ((short *)rawoutbuf)[i] = s;
#else
        ((char *)rawoutbuf)[i*2 ] = s & 255; s >>= 8;
        ((char *)rawoutbuf)[i*2 + 1] = s & 255;
#endif

        ch++;
        if (ch == nch) ch = 0;
      }
    }
    break;

  case 3:
    {
      REAL gain2 = REAL(gain * (REAL)0x7fffff);
      ch = 0;

      for (i = 0;i < nsmplwrt2*nch;i++)
      {
        int s;

        s = RINT(outbuf[i] * gain2);

        if (s < -0x800000)
        {
          double d = (double)s / -0x800000;
          peak = peak < d ? d : peak;
          s = -0x800000;
        }
        if (0x7fffff < s)
        {
          double d = (double)s / 0x7fffff;
          peak = peak < d ? d : peak;
          s = 0x7fffff;
        }

        ((char *)rawoutbuf)[i*3 ] = s & 255; s >>= 8;
        ((char *)rawoutbuf)[i*3 + 1] = s & 255; s >>= 8;
        ((char *)rawoutbuf)[i*3 + 2] = s & 255;

        ch++;
        if (ch == nch) ch = 0;
      }
    }
    break;

  }

  if (!init)
  {
    if (ending)
    {
      if ((double)sumread*dfrq / sfrq + 2 > sumwrite + nsmplwrt2)
      {
        //     FileO.WriteXBytesToFile(rawoutbuf, dbps*nch*nsmplwrt2);
        sumwrite += nsmplwrt2;

        ToRet = dbps * nch * nsmplwrt2;
        *pRetDataPtr = rawoutbuf;
      }
      else
      {
        int ToWrite = (int)(floor((double)sumread * dfrq / sfrq) + 2 - sumwrite);
        //     FileO.WriteXBytesToFile(rawoutbuf, dbps*nch*ToWrite);
        sumwrite += ToWrite;

        ToRet = dbps * nch * ToWrite;
        *pRetDataPtr = rawoutbuf;
        BreakOut = true;
      }
    }
    else
    {
      //    FileO.WriteXBytesToFile(rawoutbuf, dbps*nch*nsmplwrt2);
      sumwrite += nsmplwrt2;
      ToRet = dbps * nch * nsmplwrt2;
      *pRetDataPtr = rawoutbuf;
    }
  }
  else
  {
    if (nsmplwrt2 < delay)
    {
      delay -= nsmplwrt2;
    }
    else
    {
      if (ending)
      {
        if ((double)sumread*dfrq / sfrq + 2 > sumwrite + nsmplwrt2 - delay)
        {
          //    FileO.WriteXBytesToFile(rawoutbuf+dbps*nch*delay, dbps*nch*(nsmplwrt2-delay));
          sumwrite += nsmplwrt2 - delay;
          ToRet = dbps * nch * (nsmplwrt2 - delay);
          *pRetDataPtr = rawoutbuf + dbps * nch * delay;
        }
        else
        {
          int Writing = (int)(floor((double)sumread * dfrq / sfrq) + 2 - sumwrite - delay);
          //    FileO.WriteXBytesToFile(rawoutbuf+dbps*nch*delay, dbps*nch*Writing);
          sumwrite += Writing;
          ToRet = dbps * nch * Writing;
          *pRetDataPtr = rawoutbuf + dbps * nch * delay;
          BreakOut = true;
        }
      }
      else
      {
        //     FileO.WriteXBytesToFile(rawoutbuf+dbps*nch*delay, dbps*nch*(nsmplwrt2-delay));
        ToRet = dbps * nch * (nsmplwrt2 - delay);
        *pRetDataPtr = rawoutbuf + dbps * nch * delay;
        sumwrite += nsmplwrt2 - delay;
        init = 0;
      }
    }
  }

  if (!BreakOut)
  {
    int ds = (rp2 - 1) / (fs2 / fs1);

    if (ds > n1b2)
      ds = n1b2;

    for (ch = 0;ch < nch;ch++)
      memmove(buf2[ch], buf2[ch] + ds, sizeof(REAL)*(n2x + 1 + n1b2 - ds));

    rp2 -= ds * (fs2 / fs1);

    for (ch = 0;ch < nch;ch++)
      memcpy(buf2[ch] + n2x + 1, buf1[ch] + n1b2, sizeof(REAL)*n1b2);
  }
  return (ToRet);
}

//---------------------------------------------------------------------------
// Converts some data-
// DataSize is adjusted
// returns a new buffer (could be NULL), *** needs to be DELETED (or passed back into)
//---------------------------------------------------------------------------
/*char *Cssrc::ConvertSomeData(char *InData, int &DataSize, bool IsEOF)
{
 if (dfrq == sfrq) // not changing just returning?
 {
  char *RetData = new char[DataSize];
  CopyMemory(RetData, InData, DataSize);
  return(RetData);
 }
 
 char *pReturnData = NULL;
 DWORD ReturnDataSize = 0;
 
 //------get what we have------
 DWORD DataLen = (DWORD)DataSize;
 DWORD Used = 0;
// char *pData = DataStream.ReturnData(InData, DataLen);
 
 DWORD Left = DataLen;
 //----are we upsampling?-------
 if (UpSampling)
 {
  //-----if have enough then encode chunks-----
  while (true)
  {
   int toberead,toberead2;
 
   toberead2 = toberead = (int)(floor((double)n2b2*sfrq/(dfrq*osf))+1+n1x-inbuflen);
   if (toberead == 0)
    break;
   if ((toberead * bps*nch) > (int)Left)
    if (!IsEOF)
     break; // next time (store it)
    else
     toberead = (int)Left / (bps*nch);
   if (toberead == 0)
    break;
   //-----we have a block - copy to rawinbuf-------
//   rawinbuf = pData;
//   CopyMemory(rawinbuf, (pData + Used), (toberead * bps*nch));
   int nsmplread = toberead;
   //-----inc counters------
   Used+=(toberead * bps*nch);
   Left-=(toberead * bps*nch);
 
   //---run upsample-----
   unsigned char *pOutData;
   int NewSamples = UpSampleRawIn(&pOutData, IsEOF, toberead, toberead2, nsmplread);
 
   //------take new data and add to pReturnData-------
   if (NewSamples > 0)
   {
    DWORD NewBufferLenNeeded = ReturnDataSize + NewSamples;
    //-----we can store it in existing?------
    char *pNewBuffer = new char[NewBufferLenNeeded]; // plus a bit to grow
 
    if (ReturnDataSize) // existing?
    {
     CopyMemory(pNewBuffer, pReturnData, ReturnDataSize);
     delete pReturnData;
    }
 
    CopyMemory((pNewBuffer + ReturnDataSize), pOutData, NewSamples);
    ReturnDataSize+=NewSamples;
    pReturnData = pNewBuffer;
   }
  }  
 } else { //========down sample
  //-----if have enough then encode chunks-----
  while (true)
  {
   int toberead = (n1b2-rps-1)/osf+1;
   if (toberead == 0)
    break;
   if ((toberead * bps*nch) > (int)Left)
    if (!IsEOF)
     break; // next time (store it)
    else
     toberead = (int)Left / (bps*nch);
   if (toberead == 0)
    break;
   //-----we have a block - copy to rawinbuf-------
//   CopyMemory(rawinbuf, (pData + Used), (toberead * bps*nch));
   int nsmplread = toberead;
   //-----inc counters------
   Used+=(toberead * bps*nch);
   Left-=(toberead * bps*nch);
 
   //---run upsample-----
   unsigned char *pOutData;
   int NewSamples = DownSampleRawIn(&pOutData, IsEOF, toberead, nsmplread);
 
   //------take new data and add to pReturnData-------
   if (NewSamples > 0)
   {
    DWORD NewBufferLenNeeded = ReturnDataSize + NewSamples;
    char *pNewBuffer = new char[NewBufferLenNeeded];
    if (ReturnDataSize) // existing?
    {
     CopyMemory(pNewBuffer, pReturnData, ReturnDataSize);
     delete [] pReturnData;
    }
    CopyMemory((pNewBuffer + ReturnDataSize), pOutData, NewSamples);
    ReturnDataSize+=NewSamples;
    pReturnData = pNewBuffer;
   }
  }  
 }
 //-------------store any unused-------------
 if (Left)
//  DataStream.StoreThisExtraData(pData + Used, Left);
// delete []pData;
 
 //-----return what we have--------
 DataSize = ReturnDataSize;
 
 return(pReturnData);
}*/

bool Cssrc::GetData(unsigned char *pOutData)
{
  // See if we have data to return
  if (m_iResampleBufferPos >= m_iOutputBufferSize)
  { // Yes.  Copy the data from our buffer
    memcpy(pOutData, m_pResampleBuffer, m_iOutputBufferSize);
    // Now move any extra data in our resample buffer to the front
    m_iResampleBufferPos -= m_iOutputBufferSize;
    if (m_iResampleBufferPos)
      memmove(m_pResampleBuffer, m_pResampleBuffer + m_iOutputBufferSize, m_iResampleBufferPos);
    return true;
  }
  return false;
}

int Cssrc::GetInputSamples()
{
  // First check whether we have enough space in our output buffer, or whether they
  // should take data out of it first
  if (m_iResampleBufferPos >= m_iOutputBufferSize)
    return 0;  // need to take data out first!

  if (UpSampling)
  {
    int toberead;
    toberead = (DWORD)(floor((double)n2b2 * sfrq / (dfrq * osf)) + 1 + n1x - inbuflen);
    return toberead * nch;
  }
  else if (DownSampling)
  {
    int toberead = (n1b2-rps-1)/osf+1;
    return toberead * nch;
  }
  else
  {
    return m_iOutputBufferSize / dbps;
  }
}

int Cssrc::GetInputSize()
{
  int size = GetInputSamples();
  if (size < 0) return size;
  return size * bps;
}

int Cssrc::PutFloatData(float *pInData, int numSamples)
{
  // First check whether we have enough space in our output buffer, or whether they
  // should take data out of it first
  if (m_iResampleBufferPos >= m_iOutputBufferSize)
    return 0;  // need to take data out first!

  // Copy the desired amount of data in, and resample it
  if (UpSampling)
  {
    int toberead, toberead2;
    toberead2 = toberead = (DWORD)(floor((double)n2b2 * sfrq / (dfrq * osf)) + 1 + n1x - inbuflen);

    int iAmountToRead = toberead * nch;
    // Check that we've got enough data
    if (numSamples < iAmountToRead)
      return -1;

    // Doesn't currently support EOF reading
    bool IsEOF(false);

    //---run upsample-----
    int nsmplread = toberead;
    rawinbuf = (unsigned char *)pInData;
    unsigned char *pOutData = NULL;
    int iNewSamples = UpSampleFloatIn(&pOutData, IsEOF, toberead, toberead2, nsmplread);

    // save data into our output buffer
    if (iNewSamples)
    {
      memcpy(m_pResampleBuffer + m_iResampleBufferPos, pOutData, iNewSamples);
      m_iResampleBufferPos += iNewSamples;
    }
    return iAmountToRead;
  }
  else if (DownSampling)
  { // unimplemented!
    int toberead = (n1b2-rps-1)/osf+1;

    int iAmountToRead = toberead * nch;
    // Check that we've got enough data
    if (numSamples < iAmountToRead)
      return -1;

    // Doesn't currently support EOF reading
    bool IsEOF(false);

    //---run downsample-----
    int nsmplread = toberead;
    rawinbuf = (unsigned char *)pInData;
    unsigned char *pOutData = NULL;
    int iNewSamples = DownSampleFloatIn(&pOutData, IsEOF, toberead, nsmplread);
    // save data into our output buffer
    if (iNewSamples)
    {
      memcpy(m_pResampleBuffer + m_iResampleBufferPos, pOutData, iNewSamples);
      m_iResampleBufferPos += iNewSamples;
    }
    return iAmountToRead;
  }
  else
  { // just convert to the output bits per sample
    if (dbps == 2)  // 16 bit
    { // most likely for us - convert float -> short with rounding
      short *pShort = (short *)m_pResampleBuffer;
      float *pInput = (float *)pInData;
      for (int i = 0; i < numSamples; i++)
      {
        float result = 32767.0f * pInput[i] + 0.5f;
        if (result > 32767.0f)
          *pShort++ = 32767;
        else if (result < -32768.0f)
          *pShort++ = -32768;
        else
          *pShort++ = (short)result;
      }
      m_iResampleBufferPos += numSamples * dbps;
    }
    else  // unimplemented
      return 0;
    return numSamples;
  }
  return 0;
}

// Need to alter for downsampling
int Cssrc::PutData(unsigned char *pInData, int iSize)
{
  // First check whether we have enough space in our output buffer, or whether they
  // should take data out of it first
  if (m_iResampleBufferPos >= m_iOutputBufferSize)
    return 0;  // need to take data out first!

  // Copy the desired amount of data in, and resample it
  if (UpSampling)
  {
    int toberead, toberead2;
    toberead2 = toberead = (DWORD)(floor((double)n2b2 * sfrq / (dfrq * osf)) + 1 + n1x - inbuflen);

    int iAmountToRead = toberead * bps * nch;
    // Check that we've got enough data
    if (iSize < iAmountToRead)
      return -1;

    // Doesn't currently support EOF reading
    bool IsEOF(false);

    //---run upsample-----
    int nsmplread = toberead;
    rawinbuf = pInData;
    unsigned char *pOutData = NULL;
    int iNewSamples = UpSampleRawIn(&pOutData, IsEOF, toberead, toberead2, nsmplread);

    // save data into our output buffer
    if (iNewSamples)
    {
      memcpy(m_pResampleBuffer + m_iResampleBufferPos, pOutData, iNewSamples);
      m_iResampleBufferPos += iNewSamples;
    }
    return iAmountToRead;
  }
  else if (DownSampling)
  { // unimplemented!
    int toberead = (n1b2-rps-1)/osf+1;

    int iAmountToRead = toberead * bps * nch;
    // Check that we've got enough data
    if (iSize < iAmountToRead)
      return -1;

    // Doesn't currently support EOF reading
    bool IsEOF(false);

    //---run downsample-----
    int nsmplread = toberead;
    rawinbuf = pInData;
    unsigned char *pOutData = NULL;
    int iNewSamples = DownSampleRawIn(&pOutData, IsEOF, toberead, nsmplread);
    // save data into our output buffer
    if (iNewSamples)
    {
      memcpy(m_pResampleBuffer + m_iResampleBufferPos, pOutData, iNewSamples);
      m_iResampleBufferPos += iNewSamples;
    }
    return iAmountToRead;
  }
  else
  { // just convert bitspersample (unimplemented - currently we just assume it's the same!)
    if (dbps == bps)
    {
      memcpy(m_pResampleBuffer, pInData, iSize);
      m_iResampleBufferPos += iSize;
    }
    else
    { // need to convert (UNTESTED!)
      if (dbps == 16)
      { // most likely for us
        short *pShort = (short *)m_pResampleBuffer;
        if (bps == 8)
        {
          for (int i = 0; i < iSize; i++)
            *pShort++ = ((short)pInData[i]) << 8;
          m_iResampleBufferPos += iSize * 2;
        }
        else // bps == 24
        {
          for (int i = 0; i < iSize/3; i++)
            *pShort++ = ((short)pInData[i]) >> 8;
          m_iResampleBufferPos += iSize / 3 * 2;
        }
      }
      // else unimplemented!
    }
    return iSize;
  }
  return 0;
}

int Cssrc::GetInputBitrate()
{
  return sfrq*bps*nch*8;
}
