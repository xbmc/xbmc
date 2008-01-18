/*!
\file GUILargeImage.h
\brief 
*/

#pragma once

#include "guiImage.h"

/*!
 \ingroup controls
 \brief 
 */

class CGUILargeImage : public CGUIImage
{
public:
  CGUILargeImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture);
  virtual ~CGUILargeImage(void);

  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void Render();
  virtual void SetAspectRatio(GUIIMAGE_ASPECT_RATIO ratio, DWORD align = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER);

protected:
  virtual void SetFileName(const CStdString &strFileName);
  virtual void AllocateOnDemand();
  virtual void FreeTextures();
  virtual int GetOrientation() const;

  bool m_usingBundledTexture;
  int m_orientation;
  CGUIImage m_fallbackImage;
};

