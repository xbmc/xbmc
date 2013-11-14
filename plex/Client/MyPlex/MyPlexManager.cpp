#include "MyPlexManager.h"
#include "GUISettings.h"
#include "XBMCTinyXML.h"

#include "Client/PlexServerManager.h"
#include "GUIMessage.h"
#include "guilib/GUIWindowManager.h"

#include "utils/log.h"

#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "MyPlexScanner.h"

#include "FileSystem/PlexFile.h"

#include "LocalizeStrings.h"
#include "Client/PlexServerDataLoader.h"

#include "dialogs/GUIDialogKaiToast.h"

#include "PlexApplication.h"
#include "GUIUserMessages.h"


#define FAILURE_TMOUT 3600
#define SUCCESS_TMOUT 60 * 3

void
CMyPlexManager::Process()
{
  m_secToSleep = SUCCESS_TMOUT;
  m_myplex = g_plexApplication.serverManager->FindByUUID("myplex");

  while (true)
  {
    /* bye bye */
    if (m_bStop)
      break;

    switch(m_state)
    {
      case STATE_REFRESH:
        m_secToSleep = DoRefreshUserInfo();
        break;
      case STATE_NOT_LOGGEDIN:
        m_secToSleep = DoRemoveAllServers();
        break;
      case STATE_TRY_LOGIN:
        m_secToSleep = DoLogin();
        break;
      case STATE_FETCH_PIN:
        m_secToSleep = DoFetchPin();
        break;
      case STATE_WAIT_PIN:
        m_secToSleep = DoFetchWaitPin();
        break;
      case STATE_LOGGEDIN:
        m_secToSleep = DoScanMyPlex();
        break;
      case STATE_EXIT:
        return;
    }

    CLog::Log(LOGDEBUG, "CMyPlexManager::Process after a run our state is %d, will now sleep for %d seconds", m_state, m_secToSleep);

    m_wakeEvent.WaitMSec(m_secToSleep * 1000);
    m_wakeEvent.Reset();
  }
}

void CMyPlexManager::BroadcastState()
{
  CGUIMessage msg(GUI_MSG_MYPLEX_STATE_CHANGE, PLEX_MYPLEX_MANAGER, 0);
  msg.SetParam1((int)m_state);
  msg.SetParam2((int)m_lastError);
  m_lastError = ERROR_NOERROR;

  switch(m_state)
  {
    case STATE_LOGGEDIN:
    {
      g_guiSettings.SetString("myplex.status", g_localizeStrings.Get(44011) + " (" + CStdString(m_currentUserInfo.username) + ")");
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(44105), m_currentUserInfo.username, 5000, false);
      break;
    }
    case STATE_NOT_LOGGEDIN:
      g_guiSettings.SetString("myplex.status", g_localizeStrings.Get(44010));
      break;
    case STATE_REFRESH:
      g_guiSettings.SetString("myplex.status", "Trying...");
    default:
      break;
  }

  if (m_state == STATE_LOGGEDIN || m_state == STATE_NOT_LOGGEDIN)
  {
    /* Update settings */
    CGUIMessage msg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
    g_windowManager.SendThreadMessage(msg, WINDOW_SETTINGS_SYSTEM);
  }

  g_windowManager.SendThreadMessage(msg);
}

TiXmlElement *CMyPlexManager::GetXml(const CURL &url, bool POST)
{
  m_doc = CXBMCTinyXML();

  CStdString data;
  bool returnval;
  XFILE::CPlexFile file;

  if (POST)
    returnval = file.Post(url.Get(), "", data);
  else
    returnval = file.Get(url.Get(), data);

  if (!returnval)
  {
    CLog::Log(LOGERROR, "CMyPlexManager::GetXml failed to fetch %s", url.Get().c_str());

    if (file.GetLastHTTPResponseCode() == 401)
    {
      m_lastError = ERROR_WRONG_CREDS;
      m_state = STATE_NOT_LOGGEDIN;
    }
    else
    {
      m_lastError = ERROR_NETWORK;
      m_state = STATE_REFRESH;
    }

    BroadcastState();
    return NULL;
  }

  if (!m_doc.Parse(data) || !m_doc.RootElement())
  {
    CLog::Log(LOGERROR, "CMyPlexManager::GetXml failed to parse Xml from %s", url.Get().c_str());

    m_state = STATE_NOT_LOGGEDIN;
    m_lastError = ERROR_PARSE;
    BroadcastState();
    return NULL;
  }

  return m_doc.RootElement();
}

int CMyPlexManager::DoLogin()
{
  CURL url = m_myplex->BuildPlexURL("users/sign_in.xml");
  url.SetUserName(m_username);
  url.SetPassword(m_password);

  TiXmlElement *root = GetXml(url, true);

  if (!root)
    return FAILURE_TMOUT;

  if (!m_currentUserInfo.SetFromXmlElement(root))
  {
    m_lastError = ERROR_PARSE;
    m_state = STATE_NOT_LOGGEDIN;
    BroadcastState();
    return FAILURE_TMOUT;
  }

  m_state = STATE_REFRESH;
  BroadcastState();

  return 0;
}

int CMyPlexManager::DoFetchPin()
{
  CURL url = m_myplex->BuildPlexURL("pins.xml");

  TiXmlElement* root = GetXml(url, true);
  if (!root)
    return FAILURE_TMOUT;

  if (!m_currentPinInfo.SetFromXmlElement(root))
  {
    m_lastError = ERROR_PARSE;
    m_state = STATE_NOT_LOGGEDIN;
    BroadcastState();
    return FAILURE_TMOUT;
  }

  if (!m_currentPinInfo.code.empty() &&
      m_currentPinInfo.id != -1)
  {
    m_state = STATE_WAIT_PIN;
    BroadcastState();
    return 2;
  }

  m_state = STATE_NOT_LOGGEDIN;
  m_lastError = ERROR_PARSE;
  BroadcastState();

  return FAILURE_TMOUT;
}

int CMyPlexManager::DoFetchWaitPin()
{
  CStdString pinPath = "pins/" + boost::lexical_cast<std::string>(m_currentPinInfo.id) + ".xml";
  CURL url = m_myplex->BuildPlexURL(pinPath);

  TiXmlElement* root = GetXml(url);
  if (!root)
    return FAILURE_TMOUT;

  CMyPlexPinInfo pinInfo;
  pinInfo.SetFromXmlElement(root);

  if (pinInfo.authToken.empty())
    return 2;

  CLog::Log(LOGDEBUG, "CMyPlexManager::DoFetchWaitPin got auth_token now %s!", pinInfo.authToken.c_str());
  m_currentPinInfo = pinInfo;

  /* we now have to fetch user info */
  m_state = STATE_REFRESH;
  return 0;
}

int CMyPlexManager::DoScanMyPlex()
{
  if (g_guiSettings.GetBool("myplex.enablequeueandrec"))
    g_plexApplication.dataLoader->LoadDataFromServer(m_myplex);

  EMyPlexError err = CMyPlexScanner::DoScan();
  m_lastError = err;

  if (err == ERROR_WRONG_CREDS)
  {
    m_state = STATE_NOT_LOGGEDIN;
    BroadcastState();
    return FAILURE_TMOUT;
  }
  else if (err == ERROR_NETWORK)
  {
    m_state = STATE_REFRESH;
    BroadcastState();
    return FAILURE_TMOUT;
  }

  return SUCCESS_TMOUT;
}

int CMyPlexManager::DoRefreshUserInfo()
{
  CURL url = m_myplex->BuildPlexURL("users/account");

  TiXmlElement* root = GetXml(url);
  if (!root)
  {
    DoRemoveAllServers();
    return FAILURE_TMOUT;
  }

  CMyPlexUserInfo userInfo;

  if (!userInfo.SetFromXmlElement(root) ||
      userInfo.authToken.empty())
  {
    CLog::Log(LOGERROR, "CMyPlexManager::DoRefreshUserInfo failed to get token from account info");
    m_lastError = ERROR_PARSE;
    m_state = STATE_NOT_LOGGEDIN;
    BroadcastState();

    return FAILURE_TMOUT;
  }

  /* update the token in our global store */
  g_guiSettings.SetString("myplex.token", userInfo.authToken.c_str());

  /* reset pin information */
  m_currentPinInfo = CMyPlexPinInfo();

  /* update current user info */
  m_currentUserInfo = userInfo;

  /* hooray final state! */
  m_state = STATE_LOGGEDIN;

  BroadcastState();

  /* Also we want it to go scan directly */
  return 0;
}

int CMyPlexManager::DoRemoveAllServers()
{
  PlexServerList list;
  g_plexApplication.serverManager->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_MYPLEX);
  g_plexApplication.dataLoader->RemoveServer(m_myplex);

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, "Lost connection to myPlex", "You need to relogin", 5000, false);

  return FAILURE_TMOUT;
}

/////// Public Interface
void CMyPlexManager::StartPinLogin()
{
  CSingleLock lk(m_stateLock);

  m_state = STATE_FETCH_PIN;
  BroadcastState();
  m_wakeEvent.Set();
}

void CMyPlexManager::StopPinLogin()
{
  CSingleLock lk(m_stateLock);
  m_state = STATE_NOT_LOGGEDIN;
  m_currentPinInfo = CMyPlexPinInfo();
  m_wakeEvent.Set();
}

void CMyPlexManager::Login(const CStdString &username, const CStdString &password)
{
  CSingleLock lk(m_stateLock);
  m_state = STATE_TRY_LOGIN;

  m_username = username;
  m_password = password;

  m_wakeEvent.Set();
}

void CMyPlexManager::Logout()
{
  m_state = STATE_NOT_LOGGEDIN;
  m_currentUserInfo = CMyPlexUserInfo();
  g_guiSettings.SetString("myplex.token", "");

  m_wakeEvent.Set();

  BroadcastState();
}

CStdString CMyPlexManager::GetAuthToken() const
{
  /* First, if we have a authToken in the Pin info, we use that */
  if (!m_currentPinInfo.authToken.empty())
    return m_currentPinInfo.authToken;

  /* Ok, let's check if we have a token in our userInfo */
  if (!m_currentUserInfo.authToken.empty())
    return m_currentUserInfo.authToken;

  /* Failing all that, we need to check the settings ... */
  return g_guiSettings.GetString("myplex.token");
}

void CMyPlexManager::Stop()
{
  m_state = STATE_EXIT;
  m_wakeEvent.Set();
  StopThread(true);
}
