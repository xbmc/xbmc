/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIControlProfiler.h"
#include "utils/XBMCTinyXML.h"
#include "utils/TimeUtils.h"
#include "utils/StringUtils.h"

bool CGUIControlProfiler::m_bIsRunning = false;

CGUIControlProfilerItem::CGUIControlProfilerItem(CGUIControlProfiler *pProfiler, CGUIControlProfilerItem *pParent, CGUIControl *pControl)
: m_pProfiler(pProfiler), m_pParent(pParent), m_pControl(pControl), m_visTime(0), m_renderTime(0), m_i64VisStart(0), m_i64RenderStart(0)
{
  if (m_pControl)
  {
    m_controlID = m_pControl->GetID();
    m_ControlType = m_pControl->GetControlType();
    m_strDescription = m_pControl->GetDescription();
  }
  else
  {
    m_controlID = 0;
    m_ControlType = CGUIControl::GUICONTROL_UNKNOWN;
  }
}

CGUIControlProfilerItem::~CGUIControlProfilerItem(void)
{
  Reset(NULL);
}

void CGUIControlProfilerItem::Reset(CGUIControlProfiler *pProfiler)
{
  m_controlID = 0;
  m_ControlType = CGUIControl::GUICONTROL_UNKNOWN;
  m_pControl = NULL;

  m_visTime = 0;
  m_renderTime = 0;
  const unsigned int dwSize = m_vecChildren.size();
  for (unsigned int i=0; i<dwSize; ++i)
    delete m_vecChildren[i];
  m_vecChildren.clear();

  m_pProfiler = pProfiler;
}

void CGUIControlProfilerItem::BeginVisibility(void)
{
  m_i64VisStart = CurrentHostCounter();
}

void CGUIControlProfilerItem::EndVisibility(void)
{
  m_visTime += (unsigned int)(m_pProfiler->m_fPerfScale * (CurrentHostCounter() - m_i64VisStart));
}

void CGUIControlProfilerItem::BeginRender(void)
{
  m_i64RenderStart = CurrentHostCounter();
}

void CGUIControlProfilerItem::EndRender(void)
{
  m_renderTime += (unsigned int)(m_pProfiler->m_fPerfScale * (CurrentHostCounter() - m_i64RenderStart));
}

void CGUIControlProfilerItem::SaveToXML(TiXmlElement *parent)
{
  TiXmlElement *xmlControl = new TiXmlElement("control");
  parent->LinkEndChild(xmlControl);

  const char *lpszType = NULL;
  switch (m_ControlType)
  {
  case CGUIControl::GUICONTROL_BUTTON:
    lpszType = "button"; break;
  case CGUIControl::GUICONTROL_FADELABEL:
    lpszType = "fadelabel"; break;
  case CGUIControl::GUICONTROL_IMAGE:
  case CGUIControl::GUICONTROL_BORDEREDIMAGE:
    lpszType = "image"; break;
  case CGUIControl::GUICONTROL_LABEL:
    lpszType = "label"; break;
  case CGUIControl::GUICONTROL_LISTGROUP:
    lpszType = "group"; break;
  case CGUIControl::GUICONTROL_PROGRESS:
    lpszType = "progress"; break;
  case CGUIControl::GUICONTROL_RADIO:
    lpszType = "radiobutton"; break;
  case CGUIControl::GUICONTROL_RSS:
    lpszType = "rss"; break;
  case CGUIControl::GUICONTROL_SLIDER:
    lpszType = "slider"; break;
  case CGUIControl::GUICONTROL_SETTINGS_SLIDER:
    lpszType = "sliderex"; break;
  case CGUIControl::GUICONTROL_SPIN:
    lpszType = "spincontrol"; break;
  case CGUIControl::GUICONTROL_SPINEX:
    lpszType = "spincontrolex"; break;
  case CGUIControl::GUICONTROL_TEXTBOX:
    lpszType = "textbox"; break;
  case CGUIControl::GUICONTROL_TOGGLEBUTTON:
    lpszType = "togglebutton"; break;
  case CGUIControl::GUICONTROL_VIDEO:
    lpszType = "videowindow"; break;
  case CGUIControl::GUICONTROL_MOVER:
    lpszType = "mover"; break;
  case CGUIControl::GUICONTROL_RESIZE:
    lpszType = "resize"; break;
  case CGUIControl::GUICONTROL_EDIT:
    lpszType = "edit"; break;
  case CGUIControl::GUICONTROL_VISUALISATION:
    lpszType = "visualisation"; break;
  case CGUIControl::GUICONTROL_MULTI_IMAGE:
    lpszType = "multiimage"; break;
  case CGUIControl::GUICONTROL_GROUP:
    lpszType = "group"; break;
  case CGUIControl::GUICONTROL_GROUPLIST:
    lpszType = "grouplist"; break;
  case CGUIControl::GUICONTROL_SCROLLBAR:
    lpszType = "scrollbar"; break;
  case CGUIControl::GUICONTROL_LISTLABEL:
    lpszType = "label"; break;
  case CGUIControl::GUICONTAINER_LIST:
    lpszType = "list"; break;
  case CGUIControl::GUICONTAINER_WRAPLIST:
    lpszType = "wraplist"; break;
  case CGUIControl::GUICONTAINER_FIXEDLIST:
    lpszType = "fixedlist"; break;
  case CGUIControl::GUICONTAINER_PANEL:
    lpszType = "panel"; break;
  //case CGUIControl::GUICONTROL_UNKNOWN:
  default:
    break;
  }

  if (lpszType)
    xmlControl->SetAttribute("type", lpszType);
  if (m_controlID != 0)
  {
    std::string str = StringUtils::Format("%u", m_controlID);
    xmlControl->SetAttribute("id", str.c_str());
  }

  float pct = (float)GetTotalTime() / (float)m_pProfiler->GetTotalTime();
  if (pct > 0.01f)
  {
    std::string str = StringUtils::Format("%.0f", pct * 100.0f);
    xmlControl->SetAttribute("percent", str.c_str());
  }

  if (!m_strDescription.empty())
  {
    TiXmlElement *elem = new TiXmlElement("description");
    xmlControl->LinkEndChild(elem);
    TiXmlText *text = new TiXmlText(m_strDescription.c_str());
    elem->LinkEndChild(text);
  }

  // Note time is stored in 1/100 milliseconds but reported in ms
  unsigned int vis = m_visTime / 100;
  unsigned int rend = m_renderTime / 100;
  if (vis || rend)
  {
    std::string val;
    TiXmlElement *elem = new TiXmlElement("rendertime");
    xmlControl->LinkEndChild(elem);
    val = StringUtils::Format("%u", rend);
    TiXmlText *text = new TiXmlText(val.c_str());
    elem->LinkEndChild(text);

    elem = new TiXmlElement("visibletime");
    xmlControl->LinkEndChild(elem);
    val = StringUtils::Format("%u", vis);
    text = new TiXmlText(val.c_str());
    elem->LinkEndChild(text);
  }

  if (m_vecChildren.size())
  {
    TiXmlElement *xmlChilds = new TiXmlElement("children");
    xmlControl->LinkEndChild(xmlChilds);
    const unsigned int dwSize = m_vecChildren.size();
    for (unsigned int i=0; i<dwSize; ++i)
      m_vecChildren[i]->SaveToXML(xmlChilds);
  }
}

CGUIControlProfilerItem *CGUIControlProfilerItem::AddControl(CGUIControl *pControl)
{
  m_vecChildren.push_back(new CGUIControlProfilerItem(m_pProfiler, this, pControl));
  return m_vecChildren.back();
}

CGUIControlProfilerItem *CGUIControlProfilerItem::FindOrAddControl(CGUIControl *pControl, bool recurse)
{
  const unsigned int dwSize = m_vecChildren.size();
  for (unsigned int i=0; i<dwSize; ++i)
  {
    CGUIControlProfilerItem *p = m_vecChildren[i];
    if (p->m_pControl == pControl)
      return p;
    if (recurse && (p = p->FindOrAddControl(pControl, true)))
      return p;
  }

  if (pControl->GetParentControl() == m_pControl)
    return AddControl(pControl);

  return NULL;
}

CGUIControlProfiler::CGUIControlProfiler(void)
: m_ItemHead(NULL, NULL, NULL), m_pLastItem(NULL), m_iMaxFrameCount(200), m_iFrameCount(0)
// m_bIsRunning(false), no isRunning because it is static
{
  m_fPerfScale = 100000.0f / CurrentHostFrequency();
}

CGUIControlProfiler &CGUIControlProfiler::Instance(void)
{
  static CGUIControlProfiler _instance;
  return _instance;
}

bool CGUIControlProfiler::IsRunning(void)
{
  return m_bIsRunning;
}

void CGUIControlProfiler::Start(void)
{
  m_iFrameCount = 0;
  m_bIsRunning = true;
  m_pLastItem = NULL;
  m_ItemHead.Reset(this);
}

void CGUIControlProfiler::BeginVisibility(CGUIControl *pControl)
{
  CGUIControlProfilerItem *item = FindOrAddControl(pControl);
  item->BeginVisibility();
}

void CGUIControlProfiler::EndVisibility(CGUIControl *pControl)
{
  CGUIControlProfilerItem *item = FindOrAddControl(pControl);
  item->EndVisibility();
}

void CGUIControlProfiler::BeginRender(CGUIControl *pControl)
{
  CGUIControlProfilerItem *item = FindOrAddControl(pControl);
  item->BeginRender();
}

void CGUIControlProfiler::EndRender(CGUIControl *pControl)
{
  CGUIControlProfilerItem *item = FindOrAddControl(pControl);
  item->EndRender();
}

CGUIControlProfilerItem *CGUIControlProfiler::FindOrAddControl(CGUIControl *pControl)
{
  if (m_pLastItem)
  {
    // Typically calls come in pairs so the last control we found is probably
    // the one we want again next time
    if (m_pLastItem->m_pControl == pControl)
      return m_pLastItem;
    // If that control is not a match, usually the one we want is the next
    // sibling of that control, or the parent of that control so check
    // the parent first as it is more convenient
    m_pLastItem = m_pLastItem->m_pParent;
    if (m_pLastItem && m_pLastItem->m_pControl == pControl)
      return m_pLastItem;
    // continued from above, this searches the original control's siblings
    if (m_pLastItem)
      m_pLastItem = m_pLastItem->FindOrAddControl(pControl, false);
    if (m_pLastItem)
      return m_pLastItem;
  }

  m_pLastItem = m_ItemHead.FindOrAddControl(pControl, true);
  if (!m_pLastItem)
    m_pLastItem = m_ItemHead.AddControl(pControl);

  return m_pLastItem;
}

void CGUIControlProfiler::EndFrame(void)
{
  m_iFrameCount++;
  if (m_iFrameCount >= m_iMaxFrameCount)
  {
    const unsigned int dwSize = m_ItemHead.m_vecChildren.size();
    for (unsigned int i=0; i<dwSize; ++i)
    {
      CGUIControlProfilerItem *p = m_ItemHead.m_vecChildren[i];
      m_ItemHead.m_visTime += p->m_visTime;
      m_ItemHead.m_renderTime += p->m_renderTime;
    }

    m_bIsRunning = false;
    if (SaveResults())
      m_ItemHead.Reset(this);
  }
}

bool CGUIControlProfiler::SaveResults(void)
{
  if (m_strOutputFile.empty())
    return false;

  CXBMCTinyXML doc;
  TiXmlDeclaration decl("1.0", "", "yes");
  doc.InsertEndChild(decl);

  TiXmlElement *root = new TiXmlElement("guicontrolprofiler");
  std::string str = StringUtils::Format("%d", m_iFrameCount);
  root->SetAttribute("framecount", str.c_str());
  root->SetAttribute("timeunit", "ms");
  doc.LinkEndChild(root);

  m_ItemHead.SaveToXML(root);
  return doc.SaveFile(m_strOutputFile);
}
