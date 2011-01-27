/*
 *      Copyright (C) 2005-2008 Team XBMC
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
// Shortcut.cpp: implementation of the CShortcut class.
//
//////////////////////////////////////////////////////////////////////

#include "Shortcut.h"
#include "Util.h"
#include "tinyXML/tinyxml.h"
#include "filesystem/File.h"

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShortcut::CShortcut()
{
}

CShortcut::~CShortcut()
{
}

bool CShortcut::Create(const CStdString& szPath)
{
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( szPath ) )
    return FALSE;

  bool bPath = false;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if ( strValue != "shortcut")
    return false;
  const TiXmlNode *pChild = pRootElement->FirstChild();

  m_strCustomGame.Empty();
  while (pChild > 0)
  {
    CStdString strValue = pChild->Value();
    if (strValue == "path")
    {
      if (pChild->FirstChild())
      {
        m_strPath = pChild->FirstChild()->Value();
        bPath = true;
      }
    }

    if (strValue == "video")
    {
      if (pChild->FirstChild())
      {
        m_strVideo = pChild->FirstChild()->Value();
      }
    }

    if (strValue == "parameters")
    {
      if (pChild->FirstChild())
      {
        m_strParameters = pChild->FirstChild()->Value();
      }
    }

    if (strValue == "thumb")
    {
      if (pChild->FirstChild())
      {
        m_strThumb = pChild->FirstChild()->Value();
      }
    }

    if (strValue == "label")
    {
      if (pChild->FirstChild())
      {
        m_strLabel = pChild->FirstChild()->Value();
      }
    }

    if (strValue == "custom")
    {
      const TiXmlNode* pCustomElement = pChild->FirstChildElement();
      while (pCustomElement > 0)
      {
        CStdString strCustomValue = pCustomElement->Value();
        if (strCustomValue == "game")
          m_strCustomGame = pCustomElement->FirstChild()->Value();

        pCustomElement = pCustomElement->NextSibling();
      }
    }

    pChild = pChild->NextSibling();

  }

  return bPath ? true : false;
}

bool CShortcut::Save(const CStdString& strFileName)
{
  // Make shortcut filename compatible
  CStdString strTotalPath = CUtil::MakeLegalPath(strFileName);

  // Remove old file
  CFile::Delete(strTotalPath);

  // Create shortcut document:
  // <shortcut>
  //   <path>F:\App\default.xbe</path>
  // </shortcut>
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("shortcut");
  TiXmlNode *pRootNode = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRootNode) return false;

  TiXmlElement newElement("path");
  TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
  if (!pNewNode) return false;

  TiXmlText value(m_strPath);
  pNewNode->InsertEndChild(value);

  if (!m_strThumb.IsEmpty())
  {
    TiXmlElement newElement("thumb");
    TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
    if (!pNewNode) return false;

    TiXmlText thumbValue(m_strThumb);
    pNewNode->InsertEndChild(thumbValue);
  }
  if (!m_strLabel.IsEmpty())
  {
    TiXmlElement newElement("label");
    TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
    if (!pNewNode) return false;

    TiXmlText labelValue(m_strLabel);
    pNewNode->InsertEndChild(labelValue);
  }
  if (!m_strVideo.IsEmpty())
  {
    TiXmlElement newElement("video");
    TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
    if (!pNewNode) return false;

    TiXmlText labelValue(m_strVideo);
    pNewNode->InsertEndChild(labelValue);
  }
  if (!m_strParameters.IsEmpty())
  {
    TiXmlElement newElement("parameters");
    TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
    if (!pNewNode) return false;

    TiXmlText labelValue(m_strParameters);
    pNewNode->InsertEndChild(labelValue);
  }
  if (!m_strCustomGame.IsEmpty())
  {
    TiXmlElement customElement("custom");
    TiXmlNode* pCustomNode = pRootNode->InsertEndChild(customElement);
    TiXmlText game(m_strCustomGame);
    TiXmlElement gameElement("game");
    TiXmlNode* pGameNode = pCustomNode->InsertEndChild(gameElement);
    pGameNode->InsertEndChild(game);
  }

  return xmlDoc.SaveFile(strTotalPath);
}
