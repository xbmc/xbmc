#include "GUIPlexFilterFactory.h"

CGUIPlexFilterFactory::CGUIPlexFilterFactory(CGUIPlexMediaWindow *window) : m_window(window)
{
}

CGUIButtonControl *CGUIPlexFilterFactory::getPrimaryFilterButton(const std::string &label)
{
  if (!m_window || label.empty())
    return NULL;

  CGUIRadioButtonControl *originalRadioButton = (CGUIRadioButtonControl*)m_window->GetControl(FILTER_RADIO_BUTTON);
  if (!originalRadioButton)
    return NULL;

  CGUIRadioButtonControl *radioButton = new CGUIRadioButtonControl(*originalRadioButton);
  if (radioButton)
  {
    radioButton->SetLabel(label);
    radioButton->AllocResources();
    radioButton->SetVisible(true);
  }

  return radioButton;
}

CGUIButtonControl *CGUIPlexFilterFactory::getSecondaryFilterButton(CPlexSecondaryFilterPtr filter)
{
  if (!m_window || !filter)
    return NULL;

  CGUIButtonControl* originalButton;
  CGUIButtonControl* newButton;
  originalButton = (CGUIButtonControl*)m_window->GetControl(FILTER_RADIO_BUTTON);
  if (!originalButton)
    return NULL;

  newButton = (CGUIButtonControl*)(new CGUIRadioButtonControl(*(CGUIRadioButtonControl*)originalButton));
  newButton->SetSelected(filter->isSelected());

  if (filter->isSelected() && filter->getFilterType() != CPlexSecondaryFilter::FILTER_TYPE_BOOLEAN)
    newButton->SetLabel(filter->getFilterTitle() + ": " + filter->getCurrentValueLabel());
  else
    newButton->SetLabel(filter->getFilterTitle());
  newButton->AllocResources();
  newButton->SetVisible(true);

  return newButton;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUILabelControl *CGUIPlexFilterFactory::getSecondaryFilterLabel(CPlexSecondaryFilterPtr filter)
{
  if (!m_window || !filter || !filter->isSelected())
    return NULL;

  CGUILabelControl* originalLabel = (CGUILabelControl*)m_window->GetControl(FILTER_ACTIVE_LABEL);
  if (!originalLabel)
    return NULL;

  CGUILabelControl* newLabel = new CGUILabelControl(*originalLabel);
  newLabel->SetLabel(filter->getCurrentValue());
  newLabel->AllocResources();
  newLabel->SetVisible(true);

  return newLabel;
}

CGUIFilterOrderButtonControl *CGUIPlexFilterFactory::getSortButton(const std::string &label, CGUIFilterOrderButtonControl::FilterOrderButtonState state)
{
  if (!m_window || label.empty())
    return NULL;

  CGUIFilterOrderButtonControl* originalButton = (CGUIFilterOrderButtonControl*)m_window->GetControl(SORT_ORDER_BUTTON);
  if (!originalButton)
    return NULL;

  CGUIFilterOrderButtonControl* newButton = new CGUIFilterOrderButtonControl(*originalButton);
  newButton->SetLabel(label);
  newButton->SetTristate(state);
  newButton->SetVisible(true);

  /* TODO: the /sorts endpoint actually tells us what the default should be, but for now, just hack it :P */
  if (label == "Name")
    newButton->SetStartState(CGUIFilterOrderButtonControl::ASCENDING);

  return newButton;
}

