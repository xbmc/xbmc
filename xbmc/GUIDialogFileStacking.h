#pragma once
#include "GUIDialog.h"

class CGUIDialogFileStacking :
      public CGUIDialog
{
public:
  CGUIDialogFileStacking(void);
  virtual ~CGUIDialogFileStacking(void);
  virtual bool OnMessage(CGUIMessage& message);

  int GetSelectedFile() const;
  void SetNumberOfFiles(int iFiles);
  virtual void Render();
protected:
  int m_iSelectedFile;
  int m_iNumberOfFiles;
  int m_iFrames;

};
