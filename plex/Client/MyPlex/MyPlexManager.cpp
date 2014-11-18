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
#include "PlexFilterManager.h"
#include "PlexPlayQueueManager.h"

#include "dialogs/GUIDialogKaiToast.h"

#include "PlexApplication.h"
#include "GUIUserMessages.h"
#include "Application.h"
#include "Directory.h"

#include "File.h"
#include "PlexAES.h"
#include "Base64.h"
#include "Third-Party/hash-library/sha256.h"


#define FAILURE_TMOUT 3600
#define SUCCESS_TMOUT 30 * 60

///////////////////////////////////////////////////////////////////////////////////////////////////
CMyPlexManager::CMyPlexManager() : CThread("MyPlexManager"), m_state(STATE_REFRESH), m_homeId(-1), m_havePlexServers(false)
{
  g_guiSettings.SetString("myplex.status", g_localizeStrings.Get(44010));
  
  if (!g_guiSettings.GetString("myplex.uid").IsEmpty())
  {
    CStdString cachePath = "special://plexprofile/plexuserdata.exml";
    if (XFILE::CFile::Exists(cachePath))
    {
      CPlexAES aes(g_guiSettings.GetString("system.uuid"));
      std::string xmlData = aes.decryptFile(cachePath);
      
      CXBMCTinyXML doc;
      if (xmlData.length() > 0 && doc.Parse(xmlData.c_str()) != NULL)
      {
        m_currentUserInfo.SetFromXmlElement(doc.RootElement());
        CLog::Log(LOGINFO, "MyPlexManager::init using cached userinfo for %s", m_currentUserInfo.username.c_str());
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CMyPlexManager::HashPin(const std::string& pin)
{
  SHA256 sha;
  sha.add(pin.c_str(), pin.length());
  std::string token = GetAuthToken();
  sha.add(token.c_str(), token.length());
  return sha.getHash();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CMyPlexManager::Process()
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

///////////////////////////////////////////////////////////////////////////////////////////////////
void CMyPlexManager::BroadcastState()
{
  CGUIMessage msg(GUI_MSG_MYPLEX_STATE_CHANGE, PLEX_MYPLEX_MANAGER, 0);
  msg.SetParam1((int)m_state);
  msg.SetParam2((int)m_lastError);

  switch(m_state)
  {
    case STATE_LOGGEDIN:
    {
      g_guiSettings.SetString("myplex.status", g_localizeStrings.Get(44011) + " (" + CStdString(m_currentUserInfo.username) + ")");

      if (!g_application.IsPlayingFullScreenVideo())
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(44105), m_currentUserInfo.username, 5000, false);
      break;
    }
    case STATE_NOT_LOGGEDIN:
      g_guiSettings.SetString("myplex.status", g_localizeStrings.Get(44010));
      if (m_lastError == ERROR_WRONG_CREDS && g_windowManager.GetActiveWindow() != WINDOW_SETTINGS_SYSTEM && m_homeId != -1)
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(20117), "Please try again", 3000, false);
      break;
    case STATE_REFRESH:
      g_guiSettings.SetString("myplex.status", "Trying...");
    default:
      break;
  }

  m_lastError = ERROR_NOERROR;

  if (m_state == STATE_LOGGEDIN || m_state == STATE_NOT_LOGGEDIN)
  {
    /* Update settings */
    CGUIMessage msg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
    g_windowManager.SendThreadMessage(msg, WINDOW_SETTINGS_SYSTEM);
  }

  g_windowManager.SendThreadMessage(msg);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
TiXmlElement* CMyPlexManager::GetXml(const CURL &url, bool POST)
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
    CLog::Log(LOGERROR, "CMyPlexManager::GetXml failed to fetch %s : %ld", url.Get().c_str(), file.GetLastHTTPResponseCode());

    // we need to check for 401 or 422, both can mean that the token is wrong
    if (file.GetLastHTTPResponseCode() == 401 || file.GetLastHTTPResponseCode() == 422)
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

///////////////////////////////////////////////////////////////////////////////////////////////////
int CMyPlexManager::DoLogin()
{
  CURL url;

  if (m_homeId == -1)
  {
    url = m_myplex->BuildPlexURL("users/sign_in.xml");
    url.SetUserName(m_username);
    url.SetPassword(m_password);
  }
  else
  {
    std::string id = boost::lexical_cast<std::string>(m_homeId);
    url = m_myplex->BuildPlexURL("api/home/users/" + id + "/switch");

    if (!m_homePin.empty())
      url.SetOption("pin", m_homePin);
  }

  TiXmlElement *root = GetXml(url, true);

  if (!root)
    return FAILURE_TMOUT;

  std::string currentToken = m_currentUserInfo.authToken;

  if (!m_currentUserInfo.SetFromXmlElement(root))
  {
    m_lastError = ERROR_PARSE;
    m_state = STATE_NOT_LOGGEDIN;
    BroadcastState();
    return FAILURE_TMOUT;
  }

  // if we get a new token we want to refresh all
  // servers again.
  if (currentToken != m_currentUserInfo.authToken)
    DoRemoveAllServers();

  m_state = STATE_REFRESH;
  BroadcastState();

  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////////////
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

  m_havePlexServers = true;

  return SUCCESS_TMOUT;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CMyPlexManager::DoRefreshUserInfo()
{
  if (GetAuthToken().empty())
    return FAILURE_TMOUT;

  CURL url = m_myplex->BuildPlexURL("users/account");

  TiXmlElement* root = GetXml(url);
  if (!root)
  {
    if (m_lastError != ERROR_NETWORK)
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
  
  /* update the uid in our global store */
  g_guiSettings.SetString("myplex.uid", boost::lexical_cast<std::string>(userInfo.id).c_str());
  
  /* reset pin information */
  m_currentPinInfo = CMyPlexPinInfo();

  /* update current user info */
  m_currentUserInfo = userInfo;

  /* hooray final state! */
  m_state = STATE_LOGGEDIN;
  
  // now cache the user data
  XFILE::CDirectory::Create("special://plexprofile");
  CacheUserInfo(root);
  
  // check if we have a legacy token in settings and remove it.
  if (!g_guiSettings.GetString("myplex.token").IsEmpty())
    g_guiSettings.SetString("myplex.token", "");
  
  // our restricted flag might have been updated
  // so let's refresh all our shares
  //
  g_plexApplication.dataLoader->Refresh();
  g_plexApplication.filterManager->loadFiltersFromDisk();

  BroadcastState();

  /* Also we want it to go scan directly */
  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CMyPlexManager::CacheUserInfo(TiXmlElement *userXml)
{
  CXBMCTinyXML doc;
  
  TiXmlDeclaration decl("1.0", "utf-8", "");
  doc.InsertEndChild(decl);
  doc.InsertEndChild(*userXml);
  
  TiXmlPrinter printer;
  doc.Accept(&printer);
  
  CPlexAES aes(g_guiSettings.GetString("system.uuid"));
  std::string outdata = Base64::Encode(aes.encrypt(printer.Str()));
  
  XFILE::CFile file;
  if (file.OpenForWrite("special://plexprofile/plexuserdata.exml"))
  {
    file.Write(outdata.c_str(), outdata.length());
    file.Close();
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CMyPlexManager::DoRemoveAllServers()
{
  // remove ALL servers, since we want to make sure that no
  // local connections with tokens are still around
  //
  if (m_havePlexServers)
  {
    g_plexApplication.serverManager->RemoveAllServers();

    // Clear out queue and recommendations
    g_plexApplication.dataLoader->RemoveServer(m_myplex);

    // clear out playqueues
    g_plexApplication.playQueueManager->clear();

    if (g_application.IsPlayingFullScreenVideo())
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, "Lost connection to myPlex", "You need to relogin", 5000, false);

    m_havePlexServers = false;
  }

  return FAILURE_TMOUT;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/////// Public Interface
///////////////////////////////////////////////////////////////////////////////////////////////////
void CMyPlexManager::StartPinLogin()
{
  CSingleLock lk(m_stateLock);

  m_state = STATE_FETCH_PIN;
  BroadcastState();
  m_wakeEvent.Set();
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void CMyPlexManager::StopPinLogin()
{
  CSingleLock lk(m_stateLock);
  m_state = STATE_NOT_LOGGEDIN;
  m_currentPinInfo = CMyPlexPinInfo();
  m_wakeEvent.Set();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CMyPlexManager::Login(const CStdString &username, const CStdString &password)
{
  CSingleLock lk(m_stateLock);
  m_state = STATE_TRY_LOGIN;

  m_username = username;
  m_password = password;

  m_wakeEvent.Set();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CMyPlexManager::SwitchHomeUser(int id, const std::string& pin)
{
  CSingleLock lk(m_stateLock);
  m_state = STATE_TRY_LOGIN;

  m_homeId = id;
  m_homePin = pin;

  m_wakeEvent.Set();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CMyPlexManager::Logout()
{
  m_state = STATE_NOT_LOGGEDIN;
  m_currentUserInfo = CMyPlexUserInfo();
  g_guiSettings.SetString("myplex.uid", "");

  m_wakeEvent.Set();

  BroadcastState();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CMyPlexManager::GetAuthToken() const
{
  /* First, if we have a authToken in the Pin info, we use that */
  if (!m_currentPinInfo.authToken.empty())
    return m_currentPinInfo.authToken;

  /* Ok, let's check if we have a token in our userInfo */
  if (!m_currentUserInfo.authToken.empty())
    return m_currentUserInfo.authToken;
  
  /* look for old style token in settings */
  if (!g_guiSettings.GetString("myplex.token").IsEmpty())
    return g_guiSettings.GetString("myplex.token");
  
  return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CMyPlexManager::VerifyPin(const std::string& pin, int userId)
{
  if (pin.length() != 4)
    return false;
  
  int uid = userId;
  if (uid == -1)
    uid = m_currentUserInfo.id;
  
  if (IsSignedIn())
  {
    // this just checks if we have a connection to plex.tv
    // we always want to try to make a connection there first
    //
    std::string id = boost::lexical_cast<std::string>(uid);
    CURL url = m_myplex->BuildPlexURL("api/home/users/" + id + "/switch");
    url.SetOption("pin", pin);
    
    XFILE::CPlexFile plex;
    CStdString data;
    bool verify = plex.Post(url.Get(), "", data);
    
    return verify;
  }
  else if (uid == m_currentUserInfo.id && !m_currentUserInfo.pin.empty())
  {
    // if we don't have a connection but we have a cached PIN number,
    // let's try to use that as our verification instead
    //
    return HashPin(pin) == m_currentUserInfo.pin;
  }
  else
  {
    // if we don't have access to plex.tv,
    // don't have a cached pin we can't really
    // do a PIN check, so here we default to
    // just allowing the user through if they
    // are the current user. All other users
    // we need to to deny.
    //
    return (uid == m_currentUserInfo.id);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CMyPlexManager::Stop()
{
  m_state = STATE_EXIT;
  m_wakeEvent.Set();
  StopThread(true);
}
