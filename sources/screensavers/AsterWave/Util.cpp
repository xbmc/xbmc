#include "Util.h"

D3DCOLOR HSVtoRGB( float h, float s, float v )
{
  int i;
  float f;
  int r, g, b, p, q, t, m;

  if( s == 0 ) { // achromatic (grey)
    r = g = b = (int)(255*v);
    return D3DCOLOR_RGBA(r,g,b,255);
  }

  h /= 60;      // sector 0 to 5
  i = (int)( h );
  f = h - i;      // frational part of h
  m = (int)(255*v);
  p = (int)(m * ( 1 - s ));
  q = (int)(m * ( 1 - s * f ));
  t = (int)(m * ( 1 - s * ( 1 - f ) ));

  switch( i ) {
    case 0: return D3DCOLOR_RGBA(m,t,p,255);
    case 1: return D3DCOLOR_RGBA(q,m,p,255);
    case 2: return D3DCOLOR_RGBA(p,m,t,255);
    case 3: return D3DCOLOR_RGBA(p,q,m,255);
    case 4: return D3DCOLOR_RGBA(t,p,m,255);
    default: break;    // case 5:
  }
  return D3DCOLOR_RGBA(m,p,q,255);
}

int g_colorType = 0;

void incrementColor()
{
  float val = frand();
  if (val < 0.65)
    g_colorType = 0;
  else if (val < 0.87)
    g_colorType = 1;
  else
    g_colorType = 2;
}

DWORD randColor()
{
  float h = (float)(rand()%360),s,v;
  switch(g_colorType)
  {
    case 0: 
      h = (float)(rand()%360);
      s = 0.3f + 0.7f*frand();
      v = 0.67f + 0.25f*frand();;
      break;
    case 1: 
      s = 0.9f + 0.1f*frand(); 
      v = 0.67f + 0.3f*frand();
      break;
    default:
      s = 1.0f*frand(); 
      v = 0.3f+0.7f*frand();
  }
  return HSVtoRGB(h,s,v);
}

DWORD LerpColor3(DWORD s,DWORD e,float r)
{
  Color res = { 255,0,0,0};
  Color sa = * ((Color *) &s);
  Color ea = * ((Color *) &e);
  res.r = sa.r + (ea.r - sa.r)*r;
  res.g = sa.g + (ea.g - sa.g)*r;
  res.b = sa.b + (ea.b - sa.b)*r;
  return (DWORD)*(DWORD*)&res;
};

void TransformCoord(D3DXVECTOR3 * pOut, D3DXVECTOR3 * pIn, D3DXMATRIX * pMat)
{
  D3DXMATRIX tran;
  D3DXMatrixTranslation(&tran,pIn->x,pIn->y,pIn->z);
  D3DXMatrixMultiply(&tran,&tran,pMat);
  pOut->x=tran._41;
  pOut->y=tran._42;
  pOut->z=tran._43;
}