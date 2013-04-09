#pragma once

#include <boost/shared_ptr.hpp>

class PlexAsyncUrlResolver;
typedef boost::shared_ptr<PlexAsyncUrlResolver> PlexAsyncUrlResolverPtr;

#include "FileItem.h"
#include "PlexLog.h"
#include "PlexUtils.h"
#include "HTTP.h"
#include "filesystem/CurlFile.h"
#include "FileSystem/PlexDirectory.h"
#include "Application.h"
#include <boost/thread.hpp>
#include "URL.h"

class PlexAsyncUrlResolver
{
 public:

  static PlexAsyncUrlResolverPtr Resolve(const CFileItem& item)
  {
    // Pass in a reference to the shared pointer, so ownership is maintained between caller and thread.
    PlexAsyncUrlResolverPtr self = PlexAsyncUrlResolverPtr(new PlexAsyncUrlResolver(item));
    boost::thread t(boost::bind(&PlexAsyncUrlResolver::Process, self.get(), self));
    t.detach();

    return self;
  }

  static PlexAsyncUrlResolverPtr ResolveFirst(const CFileItem& item)
  {
    PlexAsyncUrlResolverPtr self = PlexAsyncUrlResolverPtr(new PlexAsyncUrlResolver(item));
    boost::thread t(boost::bind(&PlexAsyncUrlResolver::ProcessFirst, self.get(), self));
    t.detach();

    return self;
  }

  PlexAsyncUrlResolver(const CFileItem& item)
    : m_item(item)
    , m_bSuccess(false)
    , m_bStop(false)
    , m_indirect(false) {}

  bool WaitForCompletion(int ms)
  {
    return m_downloadEvent.WaitMSec(ms);
  }

  CStdString GetFinalPath()
  {
    return m_finalPath;
  }

  CFileItem& GetFinalItem()
  {
    return *(m_finalItem.get());
  }

  CFileItemPtr GetFinalItemPtr()
  {
    return m_finalItem;
  }

  bool IsIndirect() const
  {
    return m_indirect;
  }

  bool Success()
  {
    return m_bSuccess;
  }

  void Cancel()
  {
    m_bStop = true;
  }

 protected:

  void ProcessFirst(PlexAsyncUrlResolverPtr me)
  {
    CStdString url = m_item.GetProperty("key").asString();
    if (m_bStop == false)
    {
      CFileItemList list;
      XFILE::CPlexDirectory dir;

      if (dir.GetDirectory(url, list))
      {
        if (list.Size() > 0 && m_bStop == false)
        {
          m_finalItem = list.Get(0);
          m_bSuccess = true;
        }
      }
      else
      {
        m_bStop = true;
      }
    }

    m_downloadEvent.Set();
  }

  void Process(PlexAsyncUrlResolverPtr me)
  {
    CStdString body;
    CStdString url = m_item.GetPath();
    CLog::Log(LOGNOTICE, "Resolving indirect URL: %s", url.c_str());

    // See if we need to send data to resolve the indirect.
    if (m_item.HasProperty("postURL"))
    {
      // Go get the page, clearing cookies (b/c we want to get all needed ones back in the headers.
      // FIXME, we should look at postHeaders as well.
      //
      CLog::Log(LOGNOTICE, "Found a POST URL, going to fetch %s", m_item.GetProperty("postURL").c_str());
      XFILE::CCurlFile curl;
      curl.ClearCookies();
      if (curl.Get(m_item.GetProperty("postURL").asString(), body))
      {
        // Get the headers and prepend them to the request.
        CLog::Log(LOGNOTICE, "POST URL was retrieved successfully, processing.");
        CHttpHeader header = curl.GetHttpHeader();
        body = header.GetHeaders() + body;

        // Add the postURL to the request.
        CStdString param = m_item.GetProperty("postURL").asString();
        CURL::Encode(param);
        url = url + "&postURL=" + param;
      }
      else
      {
        m_bStop = true;
      }
    }

    if (m_bStop == false)
    {
      CFileItemList  fileItems;
      XFILE::CPlexDirectory plexDir;

      plexDir.SetBody(body);
      if (plexDir.GetDirectory(url, fileItems))
      {
        if (fileItems.Size() == 1)
        {
          if (m_bStop == false)
          {
            CFileItemPtr finalFile = fileItems.Get(0);

            // See if we ran smack into another indirect item.
            if (finalFile->GetProperty("indirect").asInteger() == 1)
            {
              m_indirect = true;
              m_finalItem = finalFile;
            }

            g_application.CurrentFileItem().SetPath(finalFile->GetPath());
            g_application.CurrentFileItem().SetProperty("httpCookies", finalFile->GetProperty("httpCookies"));
            g_application.CurrentFileItem().SetProperty("userAgent", finalFile->GetProperty("userAgent"));

            // Set the final path, by reference.
            m_finalPath = finalFile->GetPath();
            m_bSuccess = true;
          }
        }
      }
    }

    // Notify of completeness.
    m_downloadEvent.Set();
  }

 private:

  CEvent     m_downloadEvent;
  bool       m_bSuccess;
  bool       m_bStop;
  CStdString m_finalPath;
  CFileItemPtr  m_finalItem;
  bool       m_indirect;
  CFileItem  m_item;
  PlexAsyncUrlResolverPtr m_me;
};
