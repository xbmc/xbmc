/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowWeather.h"
#include "guiImage.h"
#include "Util.h"
#include "utils/Weather.h"


#define CONTROL_BTNREFRESH  2
#define CONTROL_SELECTLOCATION 3
#define CONTROL_LABELUPDATED 11
#define CONTROL_IMAGELOGO  101

#define CONTROL_STATICTEMP  223
#define CONTROL_STATICFEEL  224
#define CONTROL_STATICUVID  225
#define CONTROL_STATICWIND  226
#define CONTROL_STATICDEWP  227
#define CONTROL_STATICHUMI  228

#define CONTROL_LABELD0DAY  31
#define CONTROL_LABELD0HI  32
#define CONTROL_LABELD0LOW  33
#define CONTROL_LABELD0GEN  34
#define CONTROL_IMAGED0IMG  35

#define PARTNER_ID    "1004124588"   //weather.com partner id
#define PARTNER_KEY    "079f24145f208494"  //weather.com partner key

#define MAX_LOCATION   3
#define LOCALIZED_TOKEN_FIRSTID 370
#define LOCALIZED_TOKEN_LASTID 395
/*
FIXME'S
>strings are not centered
>weather.com dev account is mine not a general xbmc one
*/

CGUIWindowWeather::CGUIWindowWeather(void)
    : CGUIWindow(WINDOW_WEATHER, "MyWeather.xml")
{
  m_iCurWeather = 0;
#ifdef _USE_ZIP_


#endif
  srand(timeGetTime());
}

CGUIWindowWeather::~CGUIWindowWeather(void)
{}

bool CGUIWindowWeather::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowWeather::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNREFRESH)
      {
        Refresh(); // Refresh clicked so do a complete update
      }
      else if (iControl == CONTROL_SELECTLOCATION)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_SELECTLOCATION);
        m_gWindowManager.SendMessage(msg);
        m_iCurWeather = msg.GetParam1();

        CStdString strLabel=g_weatherManager.GetLocation(m_iCurWeather);
        int iPos = strLabel.ReverseFind(", ");
        if (iPos)
        {
          CStdString strLabel2(strLabel);
          strLabel = strLabel2.substr(0,iPos);
        }

        SET_CONTROL_LABEL(CONTROL_SELECTLOCATION,strLabel);
        Refresh();
      }
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
    {
      g_weatherManager.Reset();
      return true;
    }
    else if (message.GetParam1() == GUI_MSG_WEATHER_FETCHED && IsActive())
    {
      UpdateLocations();
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowWeather::OnInitWindow()
{
  // call UpdateButtons() so that we start with our initial stuff already present
  UpdateButtons();
  UpdateLocations();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowWeather::UpdateLocations()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_SELECTLOCATION);
  g_graphicsContext.SendMessage(msg);
  CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_SELECTLOCATION);

  for (unsigned int i = 0; i < MAX_LOCATION; i++)
  {
    char *szLocation = g_weatherManager.GetLocation(i);
    if (!szLocation) continue;
    CStdString strLabel(szLocation);
    if (strlen(szLocation) > 1) //got the location string yet?
    {
      int iPos = strLabel.ReverseFind(", ");
      if (iPos)
      {
        CStdString strLabel2(strLabel);
        strLabel = strLabel2.substr(0,iPos);
      }
      msg2.SetParam1(i);
      msg2.SetLabel(strLabel);
      g_graphicsContext.SendMessage(msg2);
    }
    else
    {
      strLabel.Format("AreaCode %i", i + 1);

      msg2.SetLabel(strLabel);
      msg2.SetParam1(i);
      g_graphicsContext.SendMessage(msg2);
    }
    if (i==m_iCurWeather)
      SET_CONTROL_LABEL(CONTROL_SELECTLOCATION,strLabel);
  }

  CONTROL_SELECT_ITEM(CONTROL_SELECTLOCATION, m_iCurWeather);
}

void CGUIWindowWeather::UpdateButtons()
{
  // disable refresh button if internet lookups are disabled
  if (g_guiSettings.GetBool("network.enableinternet"))
  {
    CONTROL_ENABLE(CONTROL_BTNREFRESH);
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNREFRESH);
  }

  SET_CONTROL_LABEL(CONTROL_BTNREFRESH, 184);   //Refresh

  SET_CONTROL_LABEL(WEATHER_LABEL_LOCATION, g_weatherManager.GetLocation(m_iCurWeather));
  SET_CONTROL_LABEL(CONTROL_LABELUPDATED, g_weatherManager.GetLastUpdateTime());

  for (DWORD dwID = WEATHER_LABEL_CURRENT_COND; dwID <= WEATHER_LABEL_CURRENT_HUMI; dwID++)
  {
    SET_CONTROL_LABEL(dwID, g_weatherManager.GetInfo(dwID));
  }
  CGUIImage *pImage = (CGUIImage *)GetControl(WEATHER_IMAGE_CURRENT_ICON);
  if (pImage) pImage->SetFileName(g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));

  //static labels
  SET_CONTROL_LABEL(CONTROL_STATICTEMP, 401);  //Temperature
  SET_CONTROL_LABEL(CONTROL_STATICFEEL, 402);  //Feels Like
  SET_CONTROL_LABEL(CONTROL_STATICUVID, 403);  //UV Index
  SET_CONTROL_LABEL(CONTROL_STATICWIND, 404);  //Wind
  SET_CONTROL_LABEL(CONTROL_STATICDEWP, 405);  //Dew Point
  SET_CONTROL_LABEL(CONTROL_STATICHUMI, 406);  //Humidity

  for (int i = 0; i < NUM_DAYS; i++)
  {
    SET_CONTROL_LABEL(CONTROL_LABELD0DAY + (i*10), g_weatherManager.m_dfForcast[i].m_szDay);
    SET_CONTROL_LABEL(CONTROL_LABELD0HI + (i*10), g_weatherManager.m_dfForcast[i].m_szHigh);
    SET_CONTROL_LABEL(CONTROL_LABELD0LOW + (i*10), g_weatherManager.m_dfForcast[i].m_szLow);
    SET_CONTROL_LABEL(CONTROL_LABELD0GEN + (i*10), g_weatherManager.m_dfForcast[i].m_szOverview);
    pImage = (CGUIImage *)GetControl(CONTROL_IMAGED0IMG + (i * 10));
    if (pImage) pImage->SetFileName(g_weatherManager.m_dfForcast[i].m_szIcon);
  }
}


void CGUIWindowWeather::Render()
{
  // update our controls
  UpdateButtons();

  CGUIWindow::Render();
}

//Do a complete download, parse and update
void CGUIWindowWeather::Refresh()
{
  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("network.enableinternet")) return ;

  g_weatherManager.SetArea(m_iCurWeather);
  g_weatherManager.Refresh();
}
