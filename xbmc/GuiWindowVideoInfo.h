#pragma once
#include "GUIDialog.h"
#include "utils/imdb.h"

class CGUIWindowVideoInfo :
      public CGUIDialog
{
public:
  CGUIWindowVideoInfo(void);
  virtual ~CGUIWindowVideoInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();
  void SetMovie(CIMDBMovie& movie);
  bool NeedRefresh() const;

protected:
  void Refresh();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);
  CIMDBMovie* m_pMovie;
  bool m_bViewReview;
  bool m_bRefresh;
};
