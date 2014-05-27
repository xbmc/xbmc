#ifndef GUIDIALOGPLEXERROR_H
#define GUIDIALOGPLEXERROR_H

#include "xbmc/dialogs/GUIDialogOK.h"

class CGUIDialogPlexError : public CGUIDialogOK
{
public:
  CGUIDialogPlexError() : CGUIDialogOK() {}

  static void ShowError(const CVariant &heading, const CVariant &line0,
                        const CVariant &line1, const CVariant &line2);

  static CStdString GetLocalizedString(const CVariant &var);
};

#endif // GUIDIALOGPLEXERROR_H
