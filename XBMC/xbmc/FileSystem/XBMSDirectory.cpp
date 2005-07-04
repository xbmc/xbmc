
#include "../stdafx.h"
#include "xbmsdirectory.h"
#include "../util.h"
#include "directorycache.h"

extern "C"
{
#include "../lib/libxbms/ccincludes.h"
 #include "../lib/libxbms/ccbuffer.h"
 #include "../lib/libxbms/ccxclient.h"
 #include "../lib/libxbms/ccxmltrans.h"
}

struct DiscoveryCallbackContext
{
  CFileItemList *items;
  const char *username;
  const char *password;
};

static void DiscoveryCallback(const char *addr, const char *port, const char *version,
                              const char *comment, void *context);

CXBMSDirectory::CXBMSDirectory(void)
{
  CSectionLoader::Load("LIBXBMS");
}

CXBMSDirectory::~CXBMSDirectory(void)
{
  CSectionLoader::Unload("LIBXBMS");
}



bool CXBMSDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  unsigned long handle;
  char *filename, *fileinfo;
  bool rv = false;
  CURL url(strPath);

  CStdString strRoot = strPath;
  if (!CUtil::HasSlashAtEnd(strPath) )
    strRoot += "/";

  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath);

  CcXstreamServerConnection conn = NULL;

  if (strPath == "xbms://")
  {
    if (url.GetFileName() == "")
    {
      // Let's do the automatic discovery.
      struct DiscoveryCallbackContext dc_context;
      CStdString strPassword = url.GetPassWord();
      CStdString strUserName = url.GetUserName();

      dc_context.items = &items;
      dc_context.username = ((strUserName.c_str() != NULL) && (strlen(strUserName.c_str()) > 0)) ? strUserName.c_str() : NULL;
      dc_context.password = ((strPassword.c_str() != NULL) && (strlen(strPassword.c_str()) > 0)) ? strPassword.c_str() : NULL;
      ccx_client_discover_servers(DiscoveryCallback, (void *)(&dc_context));
      rv = S_OK;

      return true;
    }
  }

  if (cc_xstream_client_connect(url.GetHostName().c_str(),
                                (url.HasPort()) ? url.GetPort() : 1400, &conn) != CC_XSTREAM_CLIENT_OK)
  {
    if (conn != NULL) cc_xstream_client_disconnect(conn);

    return false;
  }

  if (cc_xstream_client_version_handshake(conn) != CC_XSTREAM_CLIENT_OK)
  {
    if (conn != NULL) cc_xstream_client_disconnect(conn);

    return false;
  }

  // Authenticate here!
  CStdString strPassword = url.GetPassWord();
  CStdString strUserName = url.GetUserName();
  if (strPassword.size() && strUserName.size())
  {
    // We don't check the return value here.  If authentication
    // step fails, let's try if server lets us log in without
    // authentication.
    cc_xstream_client_password_authenticate(conn,
                                            strUserName.c_str(),
                                            strPassword.c_str() );
  }
  CStdString strFileName = url.GetFileName();
  CStdString strDir, strFile;
  strDir = "";
  if (cc_xstream_client_setcwd(conn, "/") == CC_XSTREAM_CLIENT_OK)
  {
    strFile = url.GetFileName();
    for (int i = 0; i < (int)strFile.size(); ++i)
    {
      if (strFile[i] == '/' || strFile[i] == '\\')
      {
        if (strDir != "")
        {
          if (cc_xstream_client_setcwd(conn, strDir.c_str()) != CC_XSTREAM_CLIENT_OK)
          {
            if (conn != NULL) cc_xstream_client_disconnect(conn);
            return false;
          }
        }
        strDir = "";
      }
      else
      {
        strDir += strFile[i];
      }
    }
  }
  else
  {
    if (conn != NULL) cc_xstream_client_disconnect(conn);
    return false;
  }
  if (strDir.size() > 0)
  {
    if (cc_xstream_client_setcwd(conn, strDir.c_str()) != CC_XSTREAM_CLIENT_OK)
    {
      if (conn != NULL) cc_xstream_client_disconnect(conn);

      return false;
    }
  }

  if (cc_xstream_client_dir_open(conn, &handle) != CC_XSTREAM_CLIENT_OK)
  {
    if (conn != NULL) cc_xstream_client_disconnect(conn);

    return false;
  }

  while (cc_xstream_client_dir_read(conn, handle, &filename, &fileinfo) == CC_XSTREAM_CLIENT_OK)
  {
    if (*filename == '\0')
    {
      free(filename);
      free(fileinfo);
      break;
    }
    bool bIsDirectory = false;

    if (strstr(fileinfo, "<ATTRIB>directory</ATTRIB>"))
      bIsDirectory = true;

    CFileItem* pItem = new CFileItem(filename);

    char* pstrSizeStart = strstr(fileinfo, "<SIZE>");
    char* pstrSizeEnd = strstr(fileinfo, "</SIZE>");
    if (pstrSizeStart && pstrSizeEnd)
    {
      char szSize[128];
      pstrSizeStart += strlen("<SIZE>");
      strncpy(szSize, pstrSizeStart, pstrSizeEnd - pstrSizeStart);
      szSize[pstrSizeEnd - pstrSizeStart] = 0;
      pItem->m_dwSize = _atoi64(szSize);
    }

    char* pstrModificationStart = strstr(fileinfo, "<MODIFICATION>");
    char* pstrModificationEnd = strstr(fileinfo, "</MODIFICATION>");
    if (pstrModificationStart && pstrModificationEnd)
    {
      char szModification[128];
      pstrModificationStart += strlen("<MODIFICATION>");
      strncpy(szModification, pstrModificationStart, pstrModificationEnd - pstrModificationStart);
      szModification[pstrModificationEnd - pstrModificationStart] = 0;
      __int64 lTimeDate = _atoi64(szModification);

      FILETIME fileTime, localTime;
      LONGLONG ll = Int32x32To64(lTimeDate, 10000000) + 116444736000000000;
      fileTime.dwLowDateTime = (DWORD) (ll & 0xFFFFFFFF);
      fileTime.dwHighDateTime = (DWORD)(ll >> 32);

      FileTimeToLocalFileTime(&fileTime, &localTime);
      FileTimeToSystemTime(&localTime, &pItem->m_stTime);

    }


    pItem->m_strPath = strRoot;
    pItem->m_strPath += filename;
    pItem->m_bIsFolder = bIsDirectory;

    if ( bIsDirectory || IsAllowed( filename) )
    {
      items.Add(new CFileItem(*pItem));
    }

    vecCacheItems.Add(pItem);

    free(filename);
    free(fileinfo);
  }
  cc_xstream_client_close_all(conn);
  rv = true;

  if (conn != NULL)
    cc_xstream_client_disconnect(conn);


  g_directoryCache.SetDirectory(strPath, vecCacheItems);
  return true;
}

static void DiscoveryCallback(const char *addr, const char *port, const char *version,
                              const char *comment, void *context)
{
  struct DiscoveryCallbackContext *c = (struct DiscoveryCallbackContext *)context;

  //Construct name
  CStdString itemName = "Server: ";
  CStdString strPath = "xbms://";
  itemName += addr;

  if (strcmp(port, "1400") != 0)
  {
    itemName += " Port: ";
    itemName += port;
  }
  if (strlen(comment) > 0)
  {
    itemName += " (";
    itemName += comment;
    itemName += ")";
  }

  // Construct URL
  if (c->username != NULL)
  {
    strPath += c->username;
    if (c->password != NULL)
    {
      strPath += ":";
      strPath += c->password;
    }
    strPath += "@";
  }
  strPath += addr;
  strPath += ":";
  strPath += port;
  strPath += "/";

  // Add to items
  CFileItem* pItem = new CFileItem(itemName);
  pItem->m_strPath = strPath;
  pItem->m_bIsFolder = true;
  pItem->m_bIsShareOrDrive = true;
  pItem->SetIconImage("defaultNetwork.png");
  c->items->Add(pItem);
}
