#ifndef GUIPLEXFILTERFACTORY_H
#define GUIPLEXFILTERFACTORY_H

#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"

#include "Filters/PlexSecondaryFilter.h"
#include "GUI/GUIPlexMediaWindow.h"
#include "guilib/GUILabelControl.h"
#include "GUI/GUIFilterOrderButtonControl.h"

class CGUIPlexFilterFactory
{
  public:
    CGUIPlexFilterFactory(CGUIPlexMediaWindow *window);
    CGUIButtonControl* getPrimaryFilterButton(const std::string& label);
    CGUIButtonControl* getSecondaryFilterButton(CPlexSecondaryFilterPtr filter);
    CGUILabelControl* getSecondaryFilterLabel(CPlexSecondaryFilterPtr filter);

    CGUIPlexMediaWindow* m_window;
    CGUIFilterOrderButtonControl *getSortButton(const std::string &label, CGUIFilterOrderButtonControl::FilterOrderButtonState state);
};

#endif // GUIPLEXFILTERFACTORY_H
