/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#ifdef HAS_DS_PLAYER
 
#include "ExternalPixelShader.h"
#include "PixelShaderCompiler.h"
#include "utils/StdString.h"
#include "FileSystem\File.h"
#include "utils/XMLUtils.h"
#include "settings/Settings.h"

HRESULT CExternalPixelShader::Compile(CPixelShaderCompiler *pCompiler)
{
  if (! pCompiler)
    return E_FAIL;

  if (m_SourceData.empty())
  {
    if (! Load())
      return E_FAIL;
  }
  CStdString errorMsg;
  HRESULT hr = pCompiler->CompileShader(m_SourceData, "main", m_SourceTarget, 0, &m_pPixelShader, NULL, &errorMsg);
  if(FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s Shader's compilation failed : %s", __FUNCTION__, errorMsg.c_str());
    return hr;
  }

  // Delete buffer
  m_SourceData.SetBuf(0);

  CLog::Log(LOGINFO, "Pixel shader \"%s\" compiled", m_name.c_str());
  return S_OK;
}

CExternalPixelShader::CExternalPixelShader(TiXmlElement* xml)
  : m_id(-1), m_valid(false), m_enabled(false)
{
  m_name = xml->Attribute("name");
  xml->Attribute("id", & m_id);

  if (! XMLUtils::GetString(xml, "path", m_SourceFile))
    return;

  if (! XMLUtils::GetString(xml, "profile", m_SourceTarget))
    return;

  if (! XFILE::CFile::Exists(m_SourceFile))
  {
    CStdString originalFile = m_SourceFile;
    m_SourceFile = g_settings.GetUserDataItem("dsplayer/shaders/" + originalFile);
    if (! XFILE::CFile::Exists(m_SourceFile))
    {
      m_SourceFile = "special://xbmc/system/players/dsplayer/shaders/" + originalFile;
      if (! XFILE::CFile::Exists(m_SourceFile))
      {
        m_SourceFile = "";
        return;
      }
    }
  }

  m_SourceTarget.ToLower();
  if ( !m_SourceTarget.Equals("ps_1_1") && !m_SourceTarget.Equals("ps_1_2") && !m_SourceTarget.Equals("ps_1_3")
    && !m_SourceTarget.Equals("ps_1_4") && !m_SourceTarget.Equals("ps_2_0") && !m_SourceTarget.Equals("ps_2_a")
    && !m_SourceTarget.Equals("ps_2_b") && !m_SourceTarget.Equals("ps_3_0") )
    return;

  m_valid = true;
}

CExternalPixelShader::CExternalPixelShader(CStdString strFile, CStdString strProfile)
  : m_id(-1), m_valid(false), m_enabled(false), m_SourceFile(strFile),
  m_SourceTarget(strProfile)
{
  if (! XFILE::CFile::Exists(m_SourceFile))
  {
    CStdString originalFile = m_SourceFile;
    m_SourceFile = g_settings.GetUserDataItem("dsplayer/shaders/" + originalFile);
    if (! XFILE::CFile::Exists(m_SourceFile))
    {
      m_SourceFile = "special://xbmc/system/players/dsplayer/shaders/" + originalFile;
      if (! XFILE::CFile::Exists(m_SourceFile))
      {
        m_SourceFile = "";
        return;
      }
    }
  }

  m_SourceTarget.ToLower();
  if ( !m_SourceTarget.Equals("ps_1_1") && !m_SourceTarget.Equals("ps_1_2") && !m_SourceTarget.Equals("ps_1_3")
    && !m_SourceTarget.Equals("ps_1_4") && !m_SourceTarget.Equals("ps_2_0") && !m_SourceTarget.Equals("ps_2_a")
    && !m_SourceTarget.Equals("ps_2_b") && !m_SourceTarget.Equals("ps_3_0") )
    return;

  m_valid = true;
}

bool CExternalPixelShader::Load()
{
  XFILE::CFile file;
  if (! file.Open(m_SourceFile))
    return false;

  int64_t length = file.GetLength();
  m_SourceData.SetBuf((int) length);

  if (file.Read(m_SourceData.GetBuffer((int) length), length) != length)
  {
    m_SourceData.Empty();
    return false;
  }

  return true;
}

TiXmlElement CExternalPixelShader::ToXML()
{
  TiXmlElement shader("shader");

  shader.SetAttribute("name", GetName().c_str());
  shader.SetAttribute("id", GetId());

  TiXmlText text("");
    
  {
    TiXmlElement path("path");
    text.SetValue( m_SourceFile );
    path.InsertEndChild(text);
    shader.InsertEndChild(path);
  }

  {
    TiXmlElement profile("profile");
    text.SetValue( m_SourceTarget );
    profile.InsertEndChild(text);
    shader.InsertEndChild(profile);
  }

  return shader;
}
 
#endif