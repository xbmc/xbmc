#pragma once
#include "GUIWindowVideo.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"
#include "videodatabase.h"

#include "stdstring.h"
#include <vector>
using namespace std;
using namespace DIRECTORY;

class CGUIWindowVideoYear : 	public CGUIWindowVideo
{
public:
	CGUIWindowVideoYear(void);
	virtual ~CGUIWindowVideoYear(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  virtual void    Render();

private:
  void              SetIMDBThumbs(VECFILEITEMS& items);
protected:
  void							Clear();
  virtual void			OnSort();
  virtual void			UpdateButtons();
	virtual void			Update(const CStdString &strDirectory);
  virtual void			OnClick(int iItem);
  virtual void			OnInfo(int iItem);
	virtual int				GetSelectedItem();

};
