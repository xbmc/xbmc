#ifndef GUIPLEXSCREENSAVERPHOTO_H
#define GUIPLEXSCREENSAVERPHOTO_H

#include "guilib/GUIDialog.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIMultiImage.h"
#include "guilib/GUILabelControl.h"

#include "JobManager.h"
#include "FileItem.h"
#include "addons/Addon.h"


class CGUIPlexScreenSaverPhoto : public CGUIDialog, public IJobCallback
{
  public:
    CGUIPlexScreenSaverPhoto();
    virtual void UpdateVisibility();

  private:
    virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
    virtual bool OnMessage(CGUIMessage &message);
    virtual void Render();

    void OnJobComplete(unsigned int jobID, bool success, CJob *job);

    CGUIMultiImage* m_multiImage;
    CGUIImage* m_overlayImage;
    CGUILabelControl* m_clockLabel;
    CFileItemListPtr m_images;
    CGUILabelControl* m_imageLabel;
};

#endif // GUIPLEXSCREENSAVERPHOTO_H
