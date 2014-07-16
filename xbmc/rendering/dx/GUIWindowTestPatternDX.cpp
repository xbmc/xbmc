/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *      Test patterns designed by Ofer LaOr - hometheater.co.il
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowTestPatternDX.h"
#include "windowing/WindowingFactory.h"
#ifndef M_PI
  #define M_PI       3.14159265358979323846
#endif

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

CGUIWindowTestPatternDX::CGUIWindowTestPatternDX(void) : CGUIWindowTestPattern()
{
}

CGUIWindowTestPatternDX::~CGUIWindowTestPatternDX(void)
{
}

void CGUIWindowTestPatternDX::DrawVerticalLines(int top, int left, int bottom, int right)
{
  CUSTOMVERTEX* vert = new CUSTOMVERTEX[2+(right-left)];
  int p = 0;
  for (int i = left; i <= right; i += 2)
  {
    vert[p].x = (float)i;
    vert[p].y = (float)top;
    vert[p].z = 0.5f;
    vert[p].rhw = 1.0f;
    vert[p].color = D3DCOLOR_COLORVALUE(m_white, m_white, m_white, 1.0f);
    ++p;
    vert[p].x = (float)i;
    vert[p].y = (float)bottom;
    vert[p].z = 0.5f;
    vert[p].rhw = 1.0f;
    vert[p].color = D3DCOLOR_COLORVALUE(m_white, m_white, m_white, 1.0f);
    ++p;
  }
  g_Windowing.Get3DDevice()->SetFVF(D3DFVF_CUSTOMVERTEX);
  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_LINELIST, p/2, vert, sizeof(CUSTOMVERTEX));

  delete [] vert;
}

void CGUIWindowTestPatternDX::DrawHorizontalLines(int top, int left, int bottom, int right)
{
  CUSTOMVERTEX* vert = new CUSTOMVERTEX[2+(bottom-top)];
  int p = 0;
  for (int i = top; i <= bottom; i += 2)
  {
    vert[p].x = (float)left;
    vert[p].y = (float)i;
    vert[p].z = 0.5f;
    vert[p].rhw = 1.0f;
    vert[p].color = D3DCOLOR_COLORVALUE(m_white, m_white, m_white, 1.0f);
    ++p;
    vert[p].x = (float)right;
    vert[p].y = (float)i;
    vert[p].z = 0.5f;
    vert[p].rhw = 1.0f;
    vert[p].color = D3DCOLOR_COLORVALUE(m_white, m_white, m_white, 1.0f);
    ++p;
  }
  g_Windowing.Get3DDevice()->SetFVF(D3DFVF_CUSTOMVERTEX);
  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_LINELIST, p/2, vert, sizeof(CUSTOMVERTEX));

  delete [] vert;
}

void CGUIWindowTestPatternDX::DrawCheckers(int top, int left, int bottom, int right)
{
  int c = (bottom-top+1)*(1+(right-left)/2);
  if (c < 1)
    return;
  CUSTOMVERTEX* vert = new CUSTOMVERTEX[c];
  int i=0;
  for (int y = top; y <= bottom; y++)
  {
    for (int x = left; x <= right; x += 2)
    {
      if (y % 2 == 0)
      {
        vert[i].x = (float)x;
        vert[i].y = (float)y;
        vert[i].z = 0.5f;
        vert[i].rhw = 1.0f;
        vert[i].color = D3DCOLOR_COLORVALUE(m_white, m_white, m_white, 1.0f);
      }
      else
      {
        vert[i].x = (float)x+1.0f;
        vert[i].y = (float)y;
        vert[i].z = 0.5f;
        vert[i].rhw = 1.0f;
        vert[i].color = D3DCOLOR_COLORVALUE(m_white, m_white, m_white, 1.0f);
      }
      ++i;
    }
  }
  g_Windowing.Get3DDevice()->SetFVF(D3DFVF_CUSTOMVERTEX);
  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_POINTLIST, i, vert, sizeof(CUSTOMVERTEX));

  delete [] vert;
}

void CGUIWindowTestPatternDX::DrawBouncingRectangle(int top, int left, int bottom, int right)
{
  m_bounceX += m_bounceDirectionX;
  m_bounceY += m_bounceDirectionY;

  if ((m_bounceDirectionX == 1 && m_bounceX + TEST_PATTERNS_BOUNCE_SQUARE_SIZE == right) || (m_bounceDirectionX == -1 && m_bounceX == left))
    m_bounceDirectionX = -m_bounceDirectionX;

  if ((m_bounceDirectionY == 1 && m_bounceY + TEST_PATTERNS_BOUNCE_SQUARE_SIZE == bottom) || (m_bounceDirectionY == -1 && m_bounceY == top))
    m_bounceDirectionY = -m_bounceDirectionY;

  DrawRectangle((float)m_bounceX, (float)m_bounceY, (float)(m_bounceX + TEST_PATTERNS_BOUNCE_SQUARE_SIZE), (float)(m_bounceY + TEST_PATTERNS_BOUNCE_SQUARE_SIZE), D3DCOLOR_COLORVALUE(m_white, m_white, m_white, 1.0f));
}

void CGUIWindowTestPatternDX::DrawContrastBrightnessPattern(int top, int left, int bottom, int right)
{
  DWORD color;
  DWORD color_white = D3DCOLOR_COLORVALUE(m_white, m_white, m_white, 1.0f);
  DWORD color_black = D3DCOLOR_COLORVALUE(m_black, m_black, m_black, 1.0f);
  float x5p = (float) (left + (0.05f * (right - left)));
  float y5p = (float) (top + (0.05f * (bottom - top)));
  float x12p = (float) (left + (0.125f * (right - left)));
  float y12p = (float) (top + (0.125f * (bottom - top)));
  float x25p = (float) (left + (0.25f * (right - left)));
  float y25p = (float) (top + (0.25f * (bottom - top)));
  float x37p = (float) (left + (0.375f * (right - left)));
  float y37p = (float) (top + (0.375f * (bottom - top)));
  float x50p = (float)(left + (right - left) / 2);
  float y50p = (float)(top + (bottom - top) / 2);
  float x62p = (float) (left + (0.625f * (right - left)));
  float y62p = (float) (top + (0.625f * (bottom - top)));
  float x75p = (float) (left + (0.75f * (right - left)));
  float y75p = (float) (top + (0.75f * (bottom - top)));
  float x87p = (float) (left + (0.875f * (right - left)));
  float y87p = (float) (top + (0.875f * (bottom - top)));
  float x95p = (float) (left + (0.95f * (right - left)));
  float y95p = (float) (top + (0.95f * (bottom - top)));

  m_blinkFrame = (m_blinkFrame + 1) % TEST_PATTERNS_BLINK_CYCLE;

  // draw main quadrants
  DrawRectangle(x50p, (float)top, (float)right, y50p, color_white);
  DrawRectangle((float)left, (float)y50p, x50p, (float)bottom, color_white);

  // draw border lines
  CUSTOMVERTEX vert[] = 
  {
    {(float)left, y5p, 0.5f, 1.0f, color_white},
    {x50p, y5p, 0.5f, 1.0f, color_white},
    {x5p, (float)top, 0.5f, 1.0f, color_white},
    {x5p, y50p, 0.5f, 1.0f, color_white},
    {x50p, y95p, 0.5f, 1.0f, color_white},
    {(float)right, y95p, 0.5f, 1.0f, color_white},
    {x95p, y50p, 0.5f, 1.0f, color_white},
    {x95p, (float)bottom, 0.5f, 1.0f, color_white},

    {x50p, y5p, 0.5f, 1.0f, color_black},
    {(float)right, y5p, 0.5f, 1.0f, color_black},
    {x5p, y50p, 0.5f, 1.0f, color_black},
    {x5p, (float)bottom, 0.5f, 1.0f, color_black},
    {(float)left, y95p, 0.5f, 1.0f, color_black},
    {x50p, y95p, 0.5f, 1.0f, color_black},
    {x95p, (float)top, 0.5f, 1.0f, color_black},
    {x95p, y50p, 0.5f, 1.0f, color_black}
  };
  g_Windowing.Get3DDevice()->SetFVF(D3DFVF_CUSTOMVERTEX);
  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_LINELIST, 8, vert, sizeof(CUSTOMVERTEX));

  // draw inner rectangles
  DrawRectangle(x12p, y12p, x37p, y37p, color_white);
  DrawRectangle(x62p, y62p, x87p, y87p, color_white);

  DrawRectangle(x62p, y12p, x87p, y37p, color_black);
  DrawRectangle(x12p, y62p, x37p, y87p, color_black);

  // draw inner circles
  if (m_blinkFrame < TEST_PATTERNS_BLINK_CYCLE / 2)
    color = D3DCOLOR_COLORVALUE(m_black + 0.05f, m_black + 0.05f, m_black + 0.05f, 1.0f);
  else
    color = D3DCOLOR_COLORVALUE(0.0f, 0.0f, 0.0f, 1.0f); // BTB
  DrawCircleEx(x25p, y75p, (y37p - y12p) / 3, color);
  DrawCircleEx(x75p, y25p, (y37p - y12p) / 3, color);

  if (m_blinkFrame < TEST_PATTERNS_BLINK_CYCLE / 2)
    color = D3DCOLOR_COLORVALUE(m_white - 0.05f, m_white - 0.05f, m_white - 0.05f, 1.0f);
  else
    color = D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, 1.0f); // WTW
  DrawCircleEx(x25p, y25p, (y37p - y12p) / 3, color);
  DrawCircleEx(x75p, y75p, (y37p - y12p) / 3, color);
}

void CGUIWindowTestPatternDX::DrawCircle(int originX, int originY, int radius)
{
  DrawCircleEx((float)originX, (float)originY, (float)radius, D3DCOLOR_COLORVALUE(m_white, m_white, m_white, 1.0f));
}

void CGUIWindowTestPatternDX::DrawCircleEx(float originX, float originY, float radius, DWORD color)
{
  float angle;
  float vectorX;
  float vectorY;
  float vectorY1 = originY;
  float vectorX1 = originX;
  CUSTOMVERTEX vert[1083]; // 361*3
  int p = 0;

  for (int i = 0; i <= 360; i++)
  {
    angle = (float)(((double)i)/57.29577957795135);
    vectorX = originX + (radius*(float)sin((double)angle));
    vectorY = originY + (radius*(float)cos((double)angle));
    vert[p].x = originX;
    vert[p].y = originY;
    vert[p].z = 0.5f;
    vert[p].rhw = 1.0f;
    vert[p].color = color;
    ++p;
    vert[p].x = vectorX1;
    vert[p].y = vectorY1;
    vert[p].z = 0.5f;
    vert[p].rhw = 1.0f;
    vert[p].color = color;
    ++p;
    vert[p].x = vectorX;
    vert[p].y = vectorY;
    vert[p].z = 0.5f;
    vert[p].rhw = 1.0f;
    vert[p].color = color;
    ++p;
    vectorY1 = vectorY;
    vectorX1 = vectorX;
  }
  g_Windowing.Get3DDevice()->SetFVF(D3DFVF_CUSTOMVERTEX);
  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 361, vert, sizeof(CUSTOMVERTEX));
}

void CGUIWindowTestPatternDX::BeginRender()
{
  g_Windowing.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
}

void CGUIWindowTestPatternDX::EndRender()
{
}

void CGUIWindowTestPatternDX::DrawRectangle(float x, float y, float x2, float y2, DWORD color)
{
  CUSTOMVERTEX vert[] = 
  {
    {x, y, 0.5f, 1.0f, color},
    {x2, y, 0.5f, 1.0f, color},
    {x2, y2, 0.5f, 1.0f, color},
    {x, y2, 0.5f, 1.0f, color},
    {x, y, 0.5f, 1.0f, color},
  };
  g_Windowing.Get3DDevice()->SetFVF(D3DFVF_CUSTOMVERTEX);
  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 3, vert, sizeof(CUSTOMVERTEX));
}
