#ifndef GUIPLEXSCREENSAVERPHOTO_H
#define GUIPLEXSCREENSAVERPHOTO_H

#include "guilib/GUIDialog.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIMultiImage.h"
#include "guilib/GUILabelControl.h"

#include "JobManager.h"
#include "FileItem.h"
#include "addons/Addon.h"
#include "PlexTimer.h"

class CGUIPlexScreenSaverPhoto : public CGUIDialog, public IJobCallback
{
public:
  CGUIPlexScreenSaverPhoto();
  virtual void UpdateVisibility();

  enum POSITION
  {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
  };

private:
  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();

  void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  void moveLabels();
  void getXYPosition(int &x, int &y, int &alignment);

  CGUIMultiImage* m_multiImage;
  CGUIImage* m_overlayImage;
  CGUILabelControl* m_clockLabel;
  CFileItemListPtr m_images;
  CGUILabelControl* m_imageLabel;

  CPlexTimer m_moveTimer;
  POSITION m_currentPosition;
};

#endif // GUIPLEXSCREENSAVERPHOTO_H
