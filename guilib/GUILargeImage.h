/*!
\file GUILargeImage.h
\brief 
*/

#pragma once

#include "GUIImage.h"

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

protected:
  virtual void AllocateOnDemand();
  virtual void FreeTextures();
  virtual int GetOrientation() const;

  bool m_usingBundledTexture;
  int m_orientation;
};