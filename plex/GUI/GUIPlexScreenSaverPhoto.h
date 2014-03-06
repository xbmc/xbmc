#ifndef GUIPLEXSCREENSAVERPHOTO_H
#define GUIPLEXSCREENSAVERPHOTO_H

#include "guilib/GUIDialog.h"
#include "guilib/GUIMultiImage.h"

#include "JobManager.h"
#include "FileItem.h"

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
    CFileItemListPtr m_images;
};

#endif // GUIPLEXSCREENSAVERPHOTO_H
