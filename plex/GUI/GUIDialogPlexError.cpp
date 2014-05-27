
#include "GUIDialogPlexError.h"
#include "xbmc/dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexError::ShowError(const CVariant &heading, const CVariant &line0,
                                    const CVariant &line1, const CVariant &line2)
{
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, GetLocalizedString(heading), GetLocalizedString(line0), 10000, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CGUIDialogPlexError::GetLocalizedString(const CVariant &var)
{
  if (var.isString())
    return var.asString();
  else if (var.isInteger() && var.asInteger())
    return g_localizeStrings.Get((uint32_t)var.asInteger());
  return "";
}
