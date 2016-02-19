/*
 *      Copyright (C) 2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InterProcess.h"
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <iostream>       // std::cerr
#include <stdexcept>      // std::out_of_range
#include <p8-platform/util/StringUtils.h>
#include <p8-platform/util/timeutils.h>

using namespace P8PLATFORM;
using namespace V2;

extern "C"
{

std::unordered_map<std::thread::id, CKODIAddon_InterProcess*> g_threadMap;

std::thread::id g_mainThreadId = std::this_thread::get_id();

CKODIAddon_InterProcess g_interProcess;

CKODIAddon_InterProcess::CKODIAddon_InterProcess()
 : m_KodiAPILevel(2),
   m_socket(NULL),
   m_LoggedIn(false),
   m_sharedMemAvailable(false),
   m_parent(NULL)
{
}

CKODIAddon_InterProcess::CKODIAddon_InterProcess(CKODIAddon_InterProcess *parent)
 : m_KodiAPILevel(2),
   m_socket(NULL),
   m_LoggedIn(false),
   m_sharedMemAvailable(false),
   m_parent(parent)
{
}

CKODIAddon_InterProcess::~CKODIAddon_InterProcess()
{
  Disconnect();
}

//-- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - --

bool CKODIAddon_InterProcess::Connect(const std::string &strHostname)
{
  Disconnect();

  m_hostname        = strHostname;
  uint64_t iNow     = GetTimeMs();
  uint64_t iTarget  = iNow + KODI_API_ConnectionTimeout * 1000;

  if (!m_socket)
    m_socket = new CTcpConnection(m_hostname.c_str(), KODI_API_ConnectionPort);

  while (!m_socket->IsOpen() && iNow < iTarget)
  {
    if (!m_socket->Open(iTarget - iNow))
      CEvent::Sleep(100);
    iNow = GetTimeMs();
  }

  if (!m_socket->IsOpen())
  {
    KODI_API_lasterror = API_ERR_CONNECTION;
    return false;
  }

  return true;
}

void CKODIAddon_InterProcess::Disconnect()
{
  StopThread();

  if (m_socket)
  {
    if (m_socket->IsOpen())
      m_socket->Close();

    delete m_socket;
    m_socket = NULL;
  }
  m_LoggedIn = false;
}

int CKODIAddon_InterProcess::Init(int argc, char *argv[], addon_properties* props, const std::string &strHostname)
{
  try
  {
    if (!props)
      throw API_ERR_BUFFER;

    CLockObject lock(m_callMutex);

    if (!Connect(strHostname))
      throw API_ERR_COMM;

    memcpy(&m_props, props, sizeof(addon_properties));

    bool isSubThread = false;
    uint64_t pid = GetThreadId();
    CRequestPacket vrp(KODICall_LoginVerify, this);
    vrp.push(API_UINT32_T, &KODI_API_Level);
    vrp.push(API_STRING,    KODI_API_Version);
    vrp.push(API_UINT64_T, &pid);
    vrp.push(API_STRING,    m_props.id);
    vrp.push(API_STRING,    m_props.type);
    vrp.push(API_STRING,    m_props.version);
    vrp.push(API_STRING,    m_props.name);
    vrp.push(API_STRING,    m_props.license);
    vrp.push(API_STRING,    m_props.summary);
    vrp.push(API_STRING,    m_props.description);
    vrp.push(API_STRING,    m_props.path);
    vrp.push(API_STRING,    m_props.libname);
    vrp.push(API_STRING,    m_props.author);
    vrp.push(API_STRING,    m_props.source);
    vrp.push(API_STRING,    m_props.icon);
    vrp.push(API_STRING,    m_props.disclaimer);
    vrp.push(API_STRING,    m_props.changelog);
    vrp.push(API_STRING,    m_props.fanart);
    vrp.push(API_BOOLEAN,  &m_props.is_independent);
    vrp.push(API_BOOLEAN,  &m_props.use_net_only);
    vrp.push(API_BOOLEAN,  &isSubThread);

    // read welcome
    std::unique_ptr<CResponsePacket> vresp(ReadResult(&vrp));
    if (!vresp)
      throw API_ERR_COMM;

    char* str;
    uint8_t retErr;
    int sharedMemAvailable;
    vresp->pop(API_UINT32_T, &retErr);
    vresp->pop(API_INT,      &m_randomConnectionNumber);
    vresp->pop(API_UINT32_T, &m_KodiAPILevel);
    m_kodiName    = (char*)vresp->pop(API_STRING, &str);
    m_kodiVersion = (char*)vresp->pop(API_STRING, &str);
    vresp->pop(API_INT,      &sharedMemAvailable);
    vresp->pop(API_INT,      &m_sharedMem_Size);
    m_sharedMemAvailable = (sharedMemAvailable > 0) ? true : false;

    if (retErr != API_SUCCESS)
      throw API_ERR_UNKNOWN;

    if (m_KodiAPILevel < KODI_API_Level)
      throw API_ERR_REQUEST;

    g_threadMap.insert(std::make_pair<std::thread::id, CKODIAddon_InterProcess*>(std::this_thread::get_id(), this));

    if (m_sharedMemAvailable && CreateSharedMemory())
    {
      SetFunctions_SharedMem();
      m_sharedMemUsed = true;
    }
    else
    {
      SetFunctions_LocalNet();
      m_sharedMemUsed = false;
    }
    m_LoggedIn = true;

    KodiAPI::Log(ADDON_LOG_INFO, "Logged in to '%s' Version: '%s' with protocol version '%d'",
                              m_kodiName.c_str(), m_kodiVersion.c_str(), m_KodiAPILevel);

    CreateThread(false);
    usleep(10);
  }
  catch (KODI_API_Errortype error)
  {
    KODI_API_lasterror = error;
    return error;
  }

  return API_SUCCESS;
}

int CKODIAddon_InterProcess::InitLibAddon(void* hdl)
{
  KODI_API_lasterror = API_ERR_TYPE;
  return API_ERR_TYPE;
}

int CKODIAddon_InterProcess::Finalize()
{
  uint32_t retCode = API_ERR_UNKNOWN;

  try
  {
    std::thread::id thread_id = std::this_thread::get_id();
    if (g_mainThreadId != thread_id)
    {
      CKODIAddon_InterProcess* process = g_threadMap.at(thread_id);
      retCode = g_interProcess.FinalizeThread(process);
      delete process;
      g_threadMap.erase(thread_id);
      return retCode;
    }

    CLockObject lock(m_callMutex);

    m_LoggedIn      = false;
    m_sharedMemUsed = false;
    if (m_sharedMemUsed)
      Unlock_KodiToAddon_Addon();
    StopThread();

    CRequestPacket vrp(KODICall_Logout, this);
    auto vresp(ReadResult(&vrp));
    if (!vresp)
      throw "failed to logout Kodi";

    if (m_sharedMemUsed)
      DestroySharedMemory();

    uint32_t retCode;
    vresp->pop(API_UINT32_T, &retCode);
    if (retCode != API_SUCCESS)
    {
      throw StringUtils::Format("Addon received error after logout to Kodi: %s", errorTranslator[retCode].errorName).c_str();
    }
    g_threadMap.erase(thread_id);
  }
  catch (const std::out_of_range& oor)
  {
    std::cerr << __FUNCTION__ << " - 1 Thread out of Range error: " << oor.what() << '\n';
    exit(1);
  }
  catch (const char * str)
  {
    Log_Console(ADDON_LOG_ERROR, StringUtils::Format("Binary AddOn: %s - %s", __FUNCTION__, str).c_str());
  }
  Disconnect();
  return retCode;
}

//-- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - --

void *CKODIAddon_InterProcess::Process(void)
{
  std::unique_ptr<CResponsePacket> vresp;

  while (!IsStopped() && m_LoggedIn)
  {
    if (m_sharedMem_KodiToAddon)
    {
      if (Lock_KodiToAddon_Addon())
      {
        if (IsStopped() || !m_sharedMem_KodiToAddon || !m_LoggedIn)
          break;

        /*switch (m_sharedMem_KodiToAddon->message.messageId)
        {
          case KODICall_Ping:
          {
            m_sharedMem_KodiToAddon->message.retValue.bool_par = true;
            break;
          }
          default:
          {
            break;
          }
        }*/

        Unlock_KodiToAddon_Kodi();
      }
    }
    else
    {
      // try to reconnect
      if (m_connectionLost && !TryReconnect())
      {
        Sleep(1000);
        continue;
      }

      // if there's anything in the buffer, read it
      if ((vresp = ReadMessage(5)) == NULL)
      {
        Sleep(5);
        continue;
      }

      if (vresp->getChannelID() == m_randomConnectionNumber)
      {
        int requestID = vresp->getRequestID();
        switch (requestID)
        {
          case KODICall_Ping:
          {
            Ping();
            break;
          }
          default:
          {
            KodiAPI::Log(ADDON_LOG_ERROR, "Binary AddOn: Rxd a unknown response packet '%i' on thread %lu !!",
                          requestID, vresp->getChannelID());
            break;
          }
        }
      }
    }
  }
}

//-- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - --

void CKODIAddon_InterProcess::SetFunctions_LocalNet()
{
  InitThread      = LocalNet_InitThread;
  FinalizeThread  = LocalNet_FinalizeThread;
}

bool CKODIAddon_InterProcess::Ping()
{
  CKODIAddon_InterProcess *session = nullptr;

  try
  {
    session = g_threadMap.at(std::this_thread::get_id());

    CRequestPacket vrp(KODICall_Ping, session);

    auto vresp(session->ReadResult(&vrp));
    if (!vresp)
      throw API_ERR_COMM;

    uint32_t retCode;
    vresp->pop(API_UINT32_T, &retCode);
    if (retCode != API_SUCCESS)
      throw retCode;

    return true;
  }
  catch (const std::out_of_range& oor)
  {
    std::cerr << __FUNCTION__ << " - 2 Thread out of Range error: " << oor.what() << '\n';
    exit(1);
  }
  catch (KODI_API_Errortype error)
  {
    if (session)
      session->Disconnect();
    KODI_API_lasterror = error;
  }

  return false;
}

void CKODIAddon_InterProcess::Log(const addon_log loglevel, const char* text)
{
  CKODIAddon_InterProcess *session = nullptr;

  try
  {
    session = g_threadMap.at(std::this_thread::get_id());
    if (!session->m_LoggedIn)
    {
      Log_Console(loglevel, text);
      return;
    }

    uint32_t level = loglevel;
    CRequestPacket vrp(KODICall_Log, session);
    vrp.push(API_UINT32_T, &level);
    vrp.push(API_STRING, text);

    CLockObject lock(session->m_callMutex);

    std::unique_ptr<CResponsePacket> vresp(session->ReadResult(&vrp));
    if (!vresp)
      throw "Failed to send log entry in Kodi";

    uint32_t retCode;
    vresp->pop(API_UINT32_T, &retCode);

    if (retCode != API_SUCCESS)
    {
      throw StringUtils::Format("Addon received error after send of log entry to Kodi: %s",
                                  errorTranslator[retCode].errorName).c_str();
    }
  }
  catch (const std::out_of_range& oor)
  {
    std::cerr << __FUNCTION__ << " - 3 Thread out of Range error: " << oor.what() << '\n';
    //exit(1);
  }
  catch (const char * str)
  {
    Log_Console(ADDON_LOG_ERROR, StringUtils::Format("Binary AddOn: %s", str).c_str());
    session->Disconnect();
  }
}


int CKODIAddon_InterProcess::LocalNet_InitThread()
{
  uint32_t retCode = API_ERR_UNKNOWN;
  CKODIAddon_InterProcess *session = nullptr;
  try
  {
    std::thread::id thread_id = std::this_thread::get_id();
    if (g_mainThreadId == thread_id)
      throw "Not possible to call 'KodiAPI_InitThread' from main thread! Add-on becomes closed!";

    session = g_threadMap.at(g_mainThreadId);

    srand(clock());
    int randomConnectionNumber = std::rand();

    CLockObject lock(g_interProcess.m_callMutex);

    CKODIAddon_InterProcess *newProcess = new CKODIAddon_InterProcess(&g_interProcess);
    newProcess->Connect(g_interProcess.m_hostname.c_str());

    bool isSubThread = true;
    uint64_t pid = GetThreadId();
    CRequestPacket vrp(KODICall_LoginVerify, session);
    vrp.push(API_UINT32_T, &KODI_API_Level);
    vrp.push(API_STRING,    KODI_API_Version);
    vrp.push(API_UINT64_T, &pid);
    vrp.push(API_STRING,    g_interProcess.m_props.id);
    vrp.push(API_STRING,    g_interProcess.m_props.type);
    vrp.push(API_STRING,    g_interProcess.m_props.version);
    std::string name = StringUtils::Format("%s - Subthread '%lu'", g_interProcess.m_props.name, pid);
    vrp.push(API_STRING,    name.c_str());
    vrp.push(API_STRING,    g_interProcess.m_props.license);
    vrp.push(API_STRING,    g_interProcess.m_props.summary);
    vrp.push(API_STRING,    g_interProcess.m_props.description);
    vrp.push(API_STRING,    g_interProcess.m_props.path);
    vrp.push(API_STRING,    g_interProcess.m_props.libname);
    vrp.push(API_STRING,    g_interProcess.m_props.author);
    vrp.push(API_STRING,    g_interProcess.m_props.source);
    vrp.push(API_STRING,    g_interProcess.m_props.icon);
    vrp.push(API_STRING,    g_interProcess.m_props.disclaimer);
    vrp.push(API_STRING,    g_interProcess.m_props.changelog);
    vrp.push(API_STRING,    g_interProcess.m_props.fanart);
    vrp.push(API_BOOLEAN,  &g_interProcess.m_props.is_independent);
    vrp.push(API_BOOLEAN,  &g_interProcess.m_props.use_net_only);
    vrp.push(API_BOOLEAN,  &isSubThread);

    // read welcome
    auto vresp(newProcess->ReadResult(&vrp));
    if (!vresp)
      throw "failed to read greeting from Kodi for subthread";

    char* str;
    int sharedMemAvailable;
    vresp->pop(API_UINT32_T, &retCode);
    vresp->pop(API_INT,      &newProcess->m_randomConnectionNumber);
    vresp->pop(API_UINT32_T, &newProcess->m_KodiAPILevel);
    newProcess->m_kodiName    = (char*)vresp->pop(API_STRING, &str);
    newProcess->m_kodiVersion = (char*)vresp->pop(API_STRING, &str);
    vresp->pop(API_INT,      &sharedMemAvailable);
    vresp->pop(API_INT,      &newProcess->m_sharedMem_Size);
    newProcess->m_sharedMemAvailable = (sharedMemAvailable > 0) ? true : false;
    newProcess->m_LoggedIn = true;
    newProcess->m_sharedMemUsed = false;

    if (retCode != API_SUCCESS)
    {
      throw StringUtils::Format("Addon received error after send of log entry to Kodi: %s",
                                  errorTranslator[retCode].errorName).c_str();
    }

    newProcess->SetFunctions_LocalNet();
    g_threadMap.insert(std::make_pair<std::thread::id, CKODIAddon_InterProcess*>(std::thread::id(thread_id), &*newProcess));
    newProcess->CreateThread(false);
  }
  catch (const std::out_of_range& oor)
  {
    std::cerr << __FUNCTION__ << " - 4 Thread out of Range error: " << oor.what() << '\n';
    exit(1);
  }
  catch (const char * str)
  {
    KodiAPI::Log(ADDON_LOG_FATAL, "Binary AddOn: %s", str);
    g_interProcess.Disconnect();
    exit(1);
  }

  return retCode;
}

int CKODIAddon_InterProcess::LocalNet_FinalizeThread(CKODIAddon_InterProcess* thread)
{
  uint32_t retCode = API_ERR_UNKNOWN;
  try
  {
    CLockObject lock(g_interProcess.m_callMutex);

    thread->m_LoggedIn = false;
    thread->StopThread();

    CRequestPacket vrp(KODICall_Logout, thread);
    auto vresp(thread->ReadResult(&vrp));
    if (!vresp)
      throw "failed to logout Kodi";

    uint32_t retCode;
    vresp->pop(API_UINT32_T, &retCode);
    if (retCode != API_SUCCESS)
    {
      throw StringUtils::Format("Addon received error after logout to Kodi: %s", errorTranslator[retCode].errorName).c_str();
    }
  }
  catch (const char * str)
  {
    Log_Console(ADDON_LOG_ERROR, StringUtils::Format("Binary AddOn: %s - %s", __FUNCTION__, str).c_str());
  }
  thread->Disconnect();
  return retCode;
}

//-- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - --

void CKODIAddon_InterProcess::SetFunctions_SharedMem()
{
  InitThread      = SharedMem_InitThread;
  FinalizeThread  = SharedMem_FinalizeThread;
}

int CKODIAddon_InterProcess::SharedMem_InitThread()
{
  int retCode = API_ERR_UNKNOWN;
  CKODIAddon_InterProcess *session = nullptr;

  try
  {
    std::thread::id thread_id = std::this_thread::get_id();
    if (g_mainThreadId == thread_id)
      throw "Not possible to call 'KodiAPI_InitThread' from main thread! Add-on becomes closed!";

    session = g_threadMap.at(g_mainThreadId);

    srand(clock());
    int randomConnectionNumber = std::rand();

    CLockObject lock(g_interProcess.m_callMutex);

    CRequestPacket vrp(KODICall_CreateSubThread, session);
    vrp.push(API_INT, &randomConnectionNumber);

    auto vresp(session->ReadResult(&vrp));
    if (!vresp)
      throw API_ERR_COMM;

    uint32_t retCode;
    vresp->pop(API_UINT32_T, &retCode);

    if (retCode != API_SUCCESS)
    {
      throw StringUtils::Format("No sub thread from Kodi for Add-on created! Error: %s",
                                  errorTranslator[retCode].errorName).c_str();
    }

    CKODIAddon_InterProcess *newProcess = new CKODIAddon_InterProcess(&g_interProcess);
    newProcess->m_LoggedIn = true;
    newProcess->m_randomConnectionNumber = randomConnectionNumber;
    newProcess->m_sharedMemUsed = true;
    if (newProcess->CreateSharedMemory())
      newProcess->SetFunctions_SharedMem();
    else
      throw "Creation of shared Memory for new thread failed!";

    g_threadMap.insert(std::make_pair<std::thread::id, CKODIAddon_InterProcess*>(std::thread::id(thread_id), &*newProcess));
    newProcess->CreateThread(false);
  }
  catch (const std::out_of_range& oor)
  {
    std::cerr << __FUNCTION__ << " - 5 Thread out of Range error: " << oor.what() << '\n';
    exit(1);
  }
  catch (const char * str)
  {
    KodiAPI::Log(ADDON_LOG_FATAL, "Binary AddOn: %s", str);
    g_interProcess.Disconnect();
    exit(1);
  }

  return retCode;
}

int CKODIAddon_InterProcess::SharedMem_FinalizeThread(CKODIAddon_InterProcess* thread)
{
  try
  {
    CLockObject lock(g_interProcess.m_callMutex);

    CRequestPacket vrp(KODICall_DeleteSubThread, &g_interProcess);
    vrp.push(API_INT, &thread->m_randomConnectionNumber);
    std::unique_ptr<CResponsePacket> vresp(g_interProcess.ReadResult(&vrp));
    if (!vresp)
      throw API_ERR_COMM;

    uint32_t retCode;
    vresp->pop(API_UINT32_T, &retCode);

    thread->m_LoggedIn = false;
    thread->Unlock_KodiToAddon_Addon();
    thread->StopThread();
    if (retCode != API_SUCCESS)
    {
      throw "Sub thread from Kodi for Add-on failed to stop!";
    }

    return API_SUCCESS;
  }
  catch (const char * str)
  {
    KodiAPI::Log(ADDON_LOG_FATAL, "Binary AddOn: %s", str);
  }
  return API_ERR_UNKNOWN;
}

//-- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - --

void CKODIAddon_InterProcess::Log_Console(const addon_log loglevel, const char* text)
{
  fprintf(stderr, "LOG: Level %02u : %s\n", loglevel, text);
}

//-- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - --

bool CKODIAddon_InterProcess::IsConnected()
{
  return m_socket && m_socket->IsOpen();
}

bool CKODIAddon_InterProcess::TryReconnect()
{
  // @todo
  return false;
}

std::unique_ptr<CResponsePacket> CKODIAddon_InterProcess::ReadMessage(int iInitialTimeout /*= 10000*/,
                                                                      int iDatapacketTimeout /*= 10000*/)
{
  uint32_t          channelID       = 0;
  uint32_t          userDataLength  = 0;
  uint8_t*          userData        = NULL;
  CResponsePacket*  vresp           = NULL;

  CLockObject lock(m_readMutex);

  if (!readData((uint8_t*)&channelID, sizeof(uint32_t), iInitialTimeout))
    return NULL;

  // Data was read
  channelID = ntohl(channelID);
  switch (channelID)
  {
    case KODIPacket_Status:
    case KODIPacket_RequestedResponse:
    {
      vresp = new CResponsePacket();

      if (!readData(vresp->getHeader(), vresp->getHeaderLength(), iDatapacketTimeout))
      {
        delete vresp;
        KodiAPI::Log(ADDON_LOG_ERROR, "Binary AddOn: %s - lost sync on response packet", __FUNCTION__);
        Disconnect();
        return NULL;
      }
      vresp->popHeader();
      userDataLength = vresp->getUserDataLength();

      userData = NULL;
      if (userDataLength > 0)
      {
        userData = (uint8_t*)malloc(userDataLength);
        if (!userData)
          return NULL;

        if (!readData(userData, userDataLength, iDatapacketTimeout))
        {
          free(userData);
          delete vresp;
          KodiAPI::Log(ADDON_LOG_ERROR, "Binary AddOn: %s - lost sync on additional response packet", __FUNCTION__);
          Disconnect();
          return NULL;
        }
      }

      if (channelID == KODIPacket_Status)
        vresp->setStatus(userData, userDataLength);
      else
        vresp->setResponse(userData, userDataLength);
    }
  }

  return std::unique_ptr<CResponsePacket>(vresp);
}

bool CKODIAddon_InterProcess::readData(uint8_t* buffer, int totalBytes, int timeout)
{
  int bytesRead = m_socket->Read(buffer, totalBytes, timeout);
  if (bytesRead == totalBytes)
  {
    return true;
  }
  else if (m_socket->GetErrorNumber() == ETIMEDOUT && bytesRead > 0)
  {
    // we did read something. try to finish the read
    bytesRead += m_socket->Read(buffer+bytesRead, totalBytes-bytesRead, timeout);
    if (bytesRead == totalBytes)
      return true;
  }
  else if (m_socket->GetErrorNumber() == ETIMEDOUT)
  {
    return false;
  }

  Disconnect();
  return false;
}

std::unique_ptr<CResponsePacket> CKODIAddon_InterProcess::ReadResult(CRequestPacket* vrp)
{
  if (vrp->SharedMemUsed())
  {
    Unlock_AddonToKodi_Kodi();
    Lock_AddonToKodi_Addon();
    CResponsePacket* vresp = new CResponsePacket(true);
    memcpy(vresp->getHeader(), (uint8_t*)&m_sharedMem_AddonToKodi->message.out+4, vresp->getHeaderLength());
    vresp->popHeader();
    vresp->setResponse((uint8_t*)&m_sharedMem_AddonToKodi->message.out.data, vresp->getUserDataLength());
    return std::unique_ptr<CResponsePacket>(vresp);
  }
  else
  {
    usleep(1);

    if (!TransmitMessage(vrp))
    {
      Disconnect();
      return NULL;
    }

    std::unique_ptr<CResponsePacket> pkt;

    while ((pkt = ReadMessage(10000, 10000)))
    {
      /* Discard everything other as response packets until it is received */
      if (pkt->getChannelID() == KODIPacket_RequestedResponse && pkt->getRequestID() == vrp->getSerial())
      {
        return pkt;
      }
    }

    Disconnect();
  }
  return NULL;
}

bool CKODIAddon_InterProcess::ReadSuccess(CRequestPacket* vrp)
{
  std::unique_ptr<CResponsePacket> pkt;
  if ((pkt = ReadResult(vrp)) == NULL)
  {
    return false;
  }

  uint32_t retCode;
  pkt->pop(API_UINT32_T, &retCode);
  if (retCode != API_SUCCESS)
  {
    KodiAPI::Log(ADDON_LOG_ERROR, "Binary AddOn: %s - failed with error code '%i': %s", __FUNCTION__, retCode, errorTranslator[retCode].errorName);
    return false;
  }
  return true;
}

bool CKODIAddon_InterProcess::TransmitMessage(CRequestPacket* vrp)
{
  if (!IsConnected())
    return false;

  ssize_t iWriteResult = m_socket->Write(vrp->getPtr(), vrp->getLen());
  if (iWriteResult != (ssize_t)vrp->getLen())
  {
    KodiAPI::Log(ADDON_LOG_ERROR, "Binary AddOn: %s - Failed to write packet (%s), bytes written: %ld of total: %ld",
              __FUNCTION__, m_socket->GetError().c_str(), iWriteResult, vrp->getLen());
    return false;
  }
  return true;
}

uint64_t CKODIAddon_InterProcess::GetThreadId()
{
#if (defined TARGET_WINDOWS)
  return (unit64_t)_getpid();
#elif (defined TARGET_POSIX)
  return (uint64_t)getpid();
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

}; /* extern "C" */
