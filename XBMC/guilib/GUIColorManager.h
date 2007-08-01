/*!
\file GUIColorManager.h
\brief 
*/

#ifndef GUILIB_COLORMANAGER_H
#define GUILIB_COLORMANAGER_H

#pragma once

/*!
 \ingroup textures
 \brief 
 */
class CGUIColorManager
{
public:
  CGUIColorManager(void);
  virtual ~CGUIColorManager(void);

  void Load(const CStdString &colorFile);

  DWORD GetColor(const CStdString &color) const;

  void Clear();

protected:
  bool LoadXML(TiXmlDocument &xmlDoc);

  map<CStdString, DWORD> m_colors;
  typedef map<CStdString, DWORD>::iterator iColor;
  typedef map<CStdString, DWORD>::const_iterator icColor;
};

/*!
 \ingroup textures
 \brief 
 */
extern CGUIColorManager g_colorManager;
#endif
