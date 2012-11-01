/*
*      Copyright (C) 2005-2012 Team XBMC
*      http://www.xbmc.org
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

#include "system.h"
#include "D3DResource.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"

#ifdef HAS_DX

using namespace std;

CD3DTexture::CD3DTexture()
{
  m_width = 0;
  m_height = 0;
  m_mipLevels = 0;
  m_usage = 0;
  m_format = D3DFMT_A8R8G8B8;
  m_pool = D3DPOOL_DEFAULT;
  m_texture = NULL;
  m_data = NULL;
  m_pitch = 0;
}

CD3DTexture::~CD3DTexture()
{
  Release();
  delete[] m_data;
}

bool CD3DTexture::Create(UINT width, UINT height, UINT mipLevels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
  m_width = width;
  m_height = height;
  m_mipLevels = mipLevels;
  m_usage = usage;
  m_format = format;
  m_pool = pool;
  // create the texture
  Release();
  HRESULT hr = D3DXCreateTexture(g_Windowing.Get3DDevice(), m_width, m_height, m_mipLevels, m_usage, m_format, m_pool, &m_texture);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed 0x%08X", hr);
  }
  else
  {
    D3DSURFACE_DESC desc;
    if( D3D_OK == m_texture->GetLevelDesc(0, &desc))
    {
      if(desc.Format != m_format)
        CLog::Log(LOGWARNING, "CD3DTexture::Create - format changed from %d to %d", m_format, desc.Format);
      if(desc.Height != m_height || desc.Width != m_width)
        CLog::Log(LOGWARNING, "CD3DTexture::Create - size changed from %ux%u to %ux%u", m_width, m_height, desc.Width, desc.Height);
    }

    g_Windowing.Register(this);
    return true;
  }
  return false;
}

void CD3DTexture::Release()
{
  g_Windowing.Unregister(this);
  SAFE_RELEASE(m_texture);
}

bool CD3DTexture::LockRect(UINT level, D3DLOCKED_RECT *lr, const RECT *rect, DWORD flags)
{
  if (m_texture)
  {
    if ((flags & D3DLOCK_DISCARD) && !(m_usage & D3DUSAGE_DYNAMIC))
      flags &= ~D3DLOCK_DISCARD;
    return (D3D_OK == m_texture->LockRect(level, lr, rect, flags));
  }
  return false;
}

bool CD3DTexture::UnlockRect(UINT level)
{
  if (m_texture)
    return (D3D_OK == m_texture->UnlockRect(level));
  return false;
}

bool CD3DTexture::GetLevelDesc(UINT level, D3DSURFACE_DESC *desc)
{
  if (m_texture)
    return (D3D_OK == m_texture->GetLevelDesc(level, desc));
  return false;
}

bool CD3DTexture::GetSurfaceLevel(UINT level, LPDIRECT3DSURFACE9 *surface)
{
  if (m_texture)
    return (D3D_OK == m_texture->GetSurfaceLevel(level, surface));
  return false;
}

void CD3DTexture::SaveTexture()
{
  if (m_texture)
  {
    delete[] m_data;
    m_data = NULL;
    if(!(m_usage & D3DUSAGE_RENDERTARGET)
    && !(m_usage & D3DUSAGE_DEPTHSTENCIL)
    && !(m_pool == D3DPOOL_DEFAULT && (m_usage & D3DUSAGE_DYNAMIC) == 0))
    {
      D3DLOCKED_RECT lr;
      if (LockRect( 0, &lr, NULL, D3DLOCK_READONLY ))
      {
        m_pitch = lr.Pitch;
        unsigned int memUsage = GetMemoryUsage(lr.Pitch);
        m_data = new unsigned char[memUsage];
        memcpy(m_data, lr.pBits, memUsage);
        UnlockRect(0);
      }
    }
  }
  SAFE_RELEASE(m_texture);
}

void CD3DTexture::OnDestroyDevice()
{
  SaveTexture();
}

void CD3DTexture::OnLostDevice()
{
  if (m_pool == D3DPOOL_DEFAULT)
    SaveTexture();
}

void CD3DTexture::RestoreTexture()
{
  // yay, we're back - make a new copy of the texture
  if (!m_texture)
  {
    HRESULT hr = D3DXCreateTexture(g_Windowing.Get3DDevice(), m_width, m_height, m_mipLevels, m_usage, m_format, m_pool, &m_texture);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": D3DXCreateTexture failed 0x%08X", hr);
    }
    else
    {
      // copy the data to the texture
      D3DLOCKED_RECT lr;
      if (m_texture && m_data && LockRect(0, &lr, NULL, D3DLOCK_DISCARD ))
      {
        if (lr.Pitch == m_pitch)
          memcpy(lr.pBits, m_data, GetMemoryUsage(lr.Pitch));
        else
        {
          UINT minpitch = ((UINT)lr.Pitch < m_pitch) ? lr.Pitch : m_pitch;
        
          for(UINT i = 0; i < m_height; ++i)
          {
            // Get pointers to the "rows" of pixels in texture
            BYTE* pBits = (BYTE*)lr.pBits + i*lr.Pitch;
            BYTE* pData = m_data + i*m_pitch;
            memcpy(pBits, pData, minpitch);
          }
        }
        UnlockRect(0);
      }
    }

    delete[] m_data;
    m_data = NULL;
    m_pitch = 0;
  }
}

void CD3DTexture::OnCreateDevice()
{
  RestoreTexture();
}

void CD3DTexture::OnResetDevice()
{
  if (m_pool == D3DPOOL_DEFAULT)
    RestoreTexture();
}


unsigned int CD3DTexture::GetMemoryUsage(unsigned int pitch) const
{
  switch (m_format)
  {
  case D3DFMT_DXT1:
  case D3DFMT_DXT3:
  case D3DFMT_DXT5:
    return pitch * m_height / 4;
  default:
    return pitch * m_height;
  }
}

CD3DEffect::CD3DEffect()
{
  m_effect = NULL;
}

CD3DEffect::~CD3DEffect()
{
  Release();
}

bool CD3DEffect::Create(const CStdString &effectString, DefinesMap* defines)
{
  Release();
  m_effectString = effectString;
  m_defines.clear();
  if (defines != NULL)
    m_defines = *defines; //FIXME: is this a copy of all members?
  if (CreateEffect())
  {
    g_Windowing.Register(this);
    return true;
  }
  return false;
}

void CD3DEffect::Release()
{
  g_Windowing.Unregister(this);
  SAFE_RELEASE(m_effect);
}

void CD3DEffect::OnDestroyDevice()
{
  SAFE_RELEASE(m_effect);
}

void CD3DEffect::OnCreateDevice()
{
  CreateEffect();
}

bool CD3DEffect::SetFloatArray(D3DXHANDLE handle, const float* val, unsigned int count)
{
  if(m_effect)
    return (D3D_OK == m_effect->SetFloatArray(handle, val, count));
  return false;
}

bool CD3DEffect::SetMatrix(D3DXHANDLE handle, const D3DXMATRIX* mat)
{
  if (m_effect)
    return (D3D_OK == m_effect->SetMatrix(handle, mat));
  return false;
}

bool CD3DEffect::SetTechnique(D3DXHANDLE handle)
{
  if (m_effect)
    return (D3D_OK == m_effect->SetTechnique(handle));
  return false;
}

bool CD3DEffect::SetTexture(D3DXHANDLE handle, CD3DTexture &texture)
{
  if (m_effect)
    return (D3D_OK == m_effect->SetTexture(handle, texture.Get()));
  return false;
}

bool CD3DEffect::Begin(UINT *passes, DWORD flags)
{
  if (m_effect)
    return (D3D_OK == m_effect->Begin(passes, flags));
  return false;
}

bool CD3DEffect::BeginPass(UINT pass)
{
  if (m_effect)
    return (D3D_OK == m_effect->BeginPass(pass));
  return false;
}

bool CD3DEffect::EndPass()
{
  if (m_effect)
    return (D3D_OK == m_effect->EndPass());
  return false;
}

bool CD3DEffect::End()
{
  if (m_effect)
    return (D3D_OK == m_effect->End());
  return false;
}

bool CD3DEffect::CreateEffect()
{
  HRESULT hr;
  LPD3DXBUFFER pError = NULL;

  std::vector<D3DXMACRO> definemacros;

  for( DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it )
	{
		D3DXMACRO m;
		m.Name = it->first.c_str();
    if (it->second.IsEmpty())
      m.Definition = NULL;
    else
		  m.Definition = it->second.c_str();
		definemacros.push_back( m );
	}

  definemacros.push_back(D3DXMACRO());
	definemacros.back().Name = 0;
	definemacros.back().Definition = 0;

  hr = D3DXCreateEffect(g_Windowing.Get3DDevice(),  m_effectString, m_effectString.length(), &definemacros[0], NULL, 0, NULL, &m_effect, &pError );
  if(hr == S_OK)
    return true;
  else if(pError)
  {
    CStdString error;
    error.assign((const char*)pError->GetBufferPointer(), pError->GetBufferSize());
    CLog::Log(LOGERROR, "%s", error.c_str());
  }
  return false;
}

void CD3DEffect::OnLostDevice()
{
  if (m_effect)
    m_effect->OnLostDevice();
}

void CD3DEffect::OnResetDevice()
{
  if (m_effect)
    m_effect->OnResetDevice();
}

CD3DVertexBuffer::CD3DVertexBuffer()
{
  m_length = 0;
  m_usage = 0;
  m_fvf = 0;
  m_pool = D3DPOOL_DEFAULT;
  m_vertex = NULL;
  m_data = NULL;
}

CD3DVertexBuffer::~CD3DVertexBuffer()
{
  Release();
  delete[] m_data;
}

bool CD3DVertexBuffer::Create(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool)
{
  m_length = length;
  m_usage = usage;
  m_fvf = fvf;
  m_pool = pool;

  // create the vertex buffer
  Release();
  if (CreateVertexBuffer())
  {
    g_Windowing.Register(this);
    return true;
  }
  return false;
}

void CD3DVertexBuffer::Release()
{
  g_Windowing.Unregister(this);
  SAFE_RELEASE(m_vertex);
}

bool CD3DVertexBuffer::Lock(UINT level, UINT size, void **data, DWORD flags)
{
  if (m_vertex)
    return (D3D_OK == m_vertex->Lock(level, size, data, flags));
  return false;
}

bool CD3DVertexBuffer::Unlock()
{
  if (m_vertex)
    return (D3D_OK == m_vertex->Unlock());
  return false;
}

void CD3DVertexBuffer::OnDestroyDevice()
{
  if (m_vertex)
  {
    delete[] m_data;
    m_data = NULL;
    void* data;
    if (Lock(0, 0, &data, 0))
    {
      m_data = new BYTE[m_length];
      memcpy(m_data, data, m_length);
      Unlock();
    }
  }
  SAFE_RELEASE(m_vertex);
}

void CD3DVertexBuffer::OnCreateDevice()
{
  // yay, we're back - make a new copy of the vertices
  if (!m_vertex && m_data && CreateVertexBuffer())
  {
    void *data = NULL;
    if (Lock(0, 0, &data, 0))
    {
      memcpy(data, m_data, m_length);
      Unlock();
    }
    delete[] m_data;
    m_data = NULL;
  }
}

bool CD3DVertexBuffer::CreateVertexBuffer()
{
  if (D3D_OK == g_Windowing.Get3DDevice()->CreateVertexBuffer(m_length, m_usage, m_fvf, m_pool, &m_vertex, NULL))
    return true;
  return false;
}

#endif
