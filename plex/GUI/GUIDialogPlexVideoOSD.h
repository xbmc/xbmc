#ifndef GUIDIALOGPLEXVIDEOOSD_H
#define GUIDIALOGPLEXVIDEOOSD_H

#include "video/dialogs/GUIDialogVideoOSD.h"
#include "PlexGlobalTimer.h"

class CGUIDialogPlexVideoOSD : public CGUIDialogVideoOSD, public IPlexGlobalTimeout
{
public:
  CGUIDialogPlexVideoOSD();
  bool OnAction(const CAction &action);
  bool OnMessage(CGUIMessage &message);

  void OnTimeout();
  CStdString TimerName() const { return "videoosd"; }

  bool IsOpenedFromPause() const { return m_openedFromPause; }
  void SetOpenedFromPause(bool onOff) { m_openedFromPause = onOff; }

private:
  bool m_openedFromPause;
};

#endif // GUIDIALOGPLEXVIDEOOSD_H
