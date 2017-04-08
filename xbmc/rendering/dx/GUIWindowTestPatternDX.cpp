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

CGUIWindowTestPatternDX::CGUIWindowTestPatternDX(void) : CGUIWindowTestPattern()
{
  m_vb = NULL;
  m_bufferWidth = 0;
}

CGUIWindowTestPatternDX::~CGUIWindowTestPatternDX(void)
{
  SAFE_RELEASE(m_vb);
  m_bufferWidth = 0;
}

void CGUIWindowTestPatternDX::DrawVerticalLines(int top, int left, int bottom, int right)
{
  Vertex* vert = new Vertex[2 + (right - left)];
  int p = 0;
  for (int i = left; i <= right; i += 2)
  {
    vert[p].pos.x = (float)i;
    vert[p].pos.y = (float)top;
    vert[p].pos.z = 0.5f;
    vert[p].color = XMFLOAT4(m_white, m_white, m_white, 1.0f);
    ++p;
    vert[p].pos.x = (float)i;
    vert[p].pos.y = (float)bottom;
    vert[p].pos.z = 0.5f;
    vert[p].color = XMFLOAT4(m_white, m_white, m_white, 1.0f);
    ++p;
  }
  UpdateVertexBuffer(vert, p);

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  CGUIShaderDX* pGUIShader = g_Windowing.GetGUIShader();

  pGUIShader->Begin(SHADER_METHOD_RENDER_DEFAULT);
  unsigned stride = sizeof(Vertex), offset = 0;
  pContext->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
  pGUIShader->Draw(p, 0);

  delete [] vert;
}

void CGUIWindowTestPatternDX::DrawHorizontalLines(int top, int left, int bottom, int right)
{
  Vertex* vert = new Vertex[2 + (bottom - top)];
  int p = 0;
  for (int i = top; i <= bottom; i += 2)
  {
    vert[p].pos.x = (float)left;
    vert[p].pos.y = (float)i;
    vert[p].pos.z = 0.5f;
    vert[p].color = XMFLOAT4(m_white, m_white, m_white, 1.0f);
    ++p;
    vert[p].pos.x = (float)right;
    vert[p].pos.y = (float)i;
    vert[p].pos.z = 0.5f;
    vert[p].color = XMFLOAT4(m_white, m_white, m_white, 1.0f);
    ++p;
  }
  UpdateVertexBuffer(vert, p);

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  CGUIShaderDX* pGUIShader = g_Windowing.GetGUIShader();

  pGUIShader->Begin(SHADER_METHOD_RENDER_DEFAULT);
  unsigned stride = sizeof(Vertex), offset = 0;
  pContext->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
  pGUIShader->Draw(p, 0);

  delete [] vert;
}

void CGUIWindowTestPatternDX::DrawCheckers(int top, int left, int bottom, int right)
{
  int c = (bottom-top+1)*(1+(right-left)/2);
  if (c < 1)
    return;
  Vertex* vert = new Vertex[c];
  int i=0;
  for (int y = top; y <= bottom; y++)
  {
    for (int x = left; x <= right; x += 2)
    {
      if (y % 2 == 0)
      {
        vert[i].pos.x = (float)x;
        vert[i].pos.y = (float)y;
        vert[i].pos.z = 0.5f;
        vert[i].color = XMFLOAT4(m_white, m_white, m_white, 1.0f);
      }
      else
      {
        vert[i].pos.x = (float)x+1.0f;
        vert[i].pos.y = (float)y;
        vert[i].pos.z = 0.5f;
        vert[i].color = XMFLOAT4(m_white, m_white, m_white, 1.0f);
      }
      ++i;
    }
  }
  UpdateVertexBuffer(vert, i);

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  CGUIShaderDX* pGUIShader = g_Windowing.GetGUIShader();

  pGUIShader->Begin(SHADER_METHOD_RENDER_DEFAULT);
  unsigned stride = sizeof(Vertex), offset = 0;
  pContext->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
  pGUIShader->Draw(i, 0);

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

  XMFLOAT4 xcolor_white, xcolor_black;
  CD3DHelper::XMStoreColor(&xcolor_white, color_white);
  CD3DHelper::XMStoreColor(&xcolor_black, color_black);

  // draw border lines
  Vertex vert[] = 
  {
    { XMFLOAT3((float)left, y5p, 0.5f), xcolor_white, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x50p, y5p, 0.5f), xcolor_white, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x5p, (float)top, 0.5f), xcolor_white, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x5p, y50p, 0.5f), xcolor_white, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x50p, y95p, 0.5f), xcolor_white, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3((float)right, y95p, 0.5f), xcolor_white, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x95p, y50p, 0.5f), xcolor_white, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x95p, (float)bottom, 0.5f), xcolor_white, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },

    { XMFLOAT3(x50p, y5p, 0.5f), xcolor_black, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3((float)right, y5p, 0.5f), xcolor_black, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x5p, y50p, 0.5f), xcolor_black, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x5p, (float)bottom, 0.5f), xcolor_black, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3((float)left, y95p, 0.5f), xcolor_black, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x50p, y95p, 0.5f), xcolor_black, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x95p, (float)top, 0.5f), xcolor_black, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x95p, y50p, 0.5f), xcolor_black, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
  };
  UpdateVertexBuffer(vert, ARRAYSIZE(vert));

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  CGUIShaderDX* pGUIShader = g_Windowing.GetGUIShader();

  pGUIShader->Begin(SHADER_METHOD_RENDER_DEFAULT);
  unsigned stride = sizeof(Vertex), offset = 0;
  pContext->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
  pGUIShader->Draw(ARRAYSIZE(vert), 0);

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
  Vertex vert[1084]; // 361*3 + 1
  int p = 0;

  for (int i = 0; i <= 360; i++)
  {
    angle = (float)(((double)i)/57.29577957795135);
    vectorX = originX + (radius*(float)sin((double)angle));
    vectorY = originY + (radius*(float)cos((double)angle));
    vert[p].pos.x = originX;
    vert[p].pos.y = originY;
    vert[p].pos.z = 0.5f;
    CD3DHelper::XMStoreColor(&vert[p].color, color);
    ++p;
    vert[p].pos.x = vectorX1;
    vert[p].pos.y = vectorY1;
    vert[p].pos.z = 0.5f;
    CD3DHelper::XMStoreColor(&vert[p].color, color);
    ++p;
    vert[p].pos.x = vectorX;
    vert[p].pos.y = vectorY;
    vert[p].pos.z = 0.5f;
    CD3DHelper::XMStoreColor(&vert[p].color, color);
    ++p;
    vectorY1 = vectorY;
    vectorX1 = vectorX;
  }
  vert[1083] = vert[0];

  UpdateVertexBuffer(vert, ARRAYSIZE(vert));

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  CGUIShaderDX* pGUIShader = g_Windowing.GetGUIShader();

  pGUIShader->Begin(SHADER_METHOD_RENDER_DEFAULT);
  unsigned stride = sizeof(Vertex), offset = 0;
  pContext->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  pGUIShader->Draw(ARRAYSIZE(vert), 0);
}

void CGUIWindowTestPatternDX::BeginRender()
{
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ID3D11RenderTargetView* renderTarget;

  pContext->OMGetRenderTargets(1, &renderTarget, NULL);
  float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  pContext->ClearRenderTargetView(renderTarget, color);
  renderTarget->Release();
}

void CGUIWindowTestPatternDX::EndRender()
{
  g_Windowing.GetGUIShader()->RestoreBuffers();
}

void CGUIWindowTestPatternDX::DrawRectangle(float x, float y, float x2, float y2, DWORD color)
{
  XMFLOAT4 float4;
  CD3DHelper::XMStoreColor(&float4, color);

  Vertex vert[] = 
  {
    { XMFLOAT3( x, y, 0.5f), float4, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x2, y, 0.5f), float4, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x2, y2, 0.5f), float4, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x2, y2, 0.5f), float4, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3(x, y2, 0.5f), float4, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
    { XMFLOAT3( x, y, 0.5f), float4, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
  };

  UpdateVertexBuffer(vert, ARRAYSIZE(vert));

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  CGUIShaderDX* pGUIShader = g_Windowing.GetGUIShader();

  pGUIShader->Begin(SHADER_METHOD_RENDER_DEFAULT);
  unsigned stride = sizeof(Vertex), offset = 0;
  pContext->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pGUIShader->Draw(ARRAYSIZE(vert), 0);
}

void CGUIWindowTestPatternDX::UpdateVertexBuffer(Vertex *vertices, unsigned count)
{
  unsigned width = sizeof(Vertex) * count;

  if (!m_vb || width > m_bufferWidth) // create new
  {
    SAFE_RELEASE(m_vb);

    CD3D11_BUFFER_DESC desc(width, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
    initData.SysMemPitch = width;
    if (SUCCEEDED(g_Windowing.Get3D11Device()->CreateBuffer(&desc, &initData, &m_vb)))
    {
      m_bufferWidth = width;
    }
    return;
  }
  else // update 
  {
    ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
    D3D11_MAPPED_SUBRESOURCE res;
    if (SUCCEEDED(pContext->Map(m_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
    {
      memcpy(res.pData, vertices, sizeof(Vertex) * count);
      pContext->Unmap(m_vb, 0);
    }
  }
}
