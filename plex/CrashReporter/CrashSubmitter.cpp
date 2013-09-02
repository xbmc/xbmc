#include "filesystem/CurlFile.h"
#include "filesystem/File.h"
#include "Directory.h"

#include "CrashSubmitter.h"
#include "Breakpad.h"

#include "log.h"
#include "PlexUtils.h"
#include "settings/GUISettings.h"

#include <boost/lexical_cast.hpp>

#include "utils/URIUtils.h"

#include "URL.h"
#include "utils/Base64.h"

#define SUBMITTER_URL "http://crashreport.plexapp.com:8881"

using namespace std;

#ifdef _WIN32
#include "config.h"
#include <process.h>
#define getpid _getpid
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
void CrashSubmitter::Process()
{
  CrashSubmitter::UploadCrashReports();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CrashSubmitter::UploadCrashReports()
{
  CLog::Log(LOGDEBUG,"CrashSubmitter::UploadCrashReports Starting up.");
  try
  {
    Upload();
  }
  catch (std::exception& ex)
  {
    CLog::Log(LOGERROR,"CrashSubmitter::UploadCrashReports Failure uploading crash reports: %s", ex.what());
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CrashSubmitter::Upload()
{
  // If the directory isn't there, we're done.
  CStdString p = PlexUtils::GetPlexCrashPath();
  if (!XFILE::CFile::Exists(p))
    return;

  // Make the upload path if we need to.
  CStdString uploadPath = URIUtils::AddFileToFolder(p, "processing" + boost::lexical_cast<string>(getpid()));
  if (!XFILE::CFile::Exists(uploadPath))
    XFILE::CDirectory::Create(uploadPath);

  CFileItemList list;
  XFILE::CDirectory dir;
  if (dir.GetDirectory(p, list))
  {
    for (int i = 0; i < list.Size(); i ++)
    {
      struct __stat64 st;
      CFileItemPtr file = list.Get(i);

      /* skip directories */
      if (XFILE::CFile::Stat(file->GetPath(), &st) == 0)
      {
#ifdef TARGET_WINDOWS
		    if (st.st_mode & S_IFDIR)
#else
        if (S_ISDIR(st.st_mode))
#endif
          continue;
      }
      else
        continue;

      CStdString fname = URIUtils::AddFileToFolder(uploadPath, URIUtils::GetFileName(file->GetPath()));
      CLog::Log(LOGDEBUG, "CrashReporter::Upload moving %s to %s", file->GetPath().c_str(), fname.c_str());

      XFILE::CFile::Rename(file->GetPath(), fname);

      if (UploadFile(fname) == false)
        XFILE::CFile::Rename(fname, file->GetPath());
      else
        XFILE::CFile::Delete(fname);
    }
  }

  // Now we can get rid of the processing place, files in there are uploaded or have been moved back.
  XFILE::CDirectory::Remove(uploadPath);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CrashSubmitter::GetDumpData(const CStdString &path)
{
  XFILE::CFile file;
  file.Open(path);

  int64_t size = file.GetLength();

  std::vector<char> data(size);
  if (!file.Read(&data[0], size))
    return "";

  return CURL::Encode(Base64::Encode(&data[0], size));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CrashSubmitter::UploadFile(const CStdString& p)
{
  CLog::Log(LOGDEBUG,"CrashSubmitter::UploadFile Uploading crash report %s", p.c_str());
  XFILE::CCurlFile http;
  CURL u(SUBMITTER_URL);

  u.SetOption("version", ExtractVersionFromCrashDump(p));
  u.SetOption("platform", PlexUtils::GetMachinePlatform());

  // Strip off the version number, if present
  CStdString crashUuid = URIUtils::GetFileName(p);
  CStdString ext = URIUtils::GetExtension(crashUuid);
  crashUuid.Replace(ext, "");

  size_t index = crashUuid.find("-v-");
  if (index != string::npos)
    crashUuid = crashUuid.substr(0, index);

  u.SetOption("uuid", crashUuid);

  // Server UUID.
  u.SetOption("serverUuid", g_guiSettings.GetString("system.uuid"));

  CStdString b64data = GetDumpData(p);
  CStdString data;
  if (!http.Post(u.Get(), "dumpfileb64=" + b64data, data))
  {
    CLog::Log(LOGDEBUG, "CrashSubmitter::UploadFile failed to upload to %s", SUBMITTER_URL);
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
string CrashSubmitter::ExtractVersionFromCrashDump(const CStdString &path)
{
  // We currently smuggle the version in the crash dump name
  // E.g. b1507f0c-82e4-42b2-81c8-4bf382c50ee0-v-0.9.8.1.dev-0b66fa4.dmp
  string filename = URIUtils::GetFileName(path);
  size_t indexToV = filename.find("-v-");
  size_t indexToDmp = filename.find(".dmp");
  if (indexToV != string::npos && indexToDmp != string::npos)
  {
    size_t indexToVersion = indexToV + 3;
    return filename.substr(indexToVersion, indexToDmp - indexToVersion);
  }

  // We have no idea, so don't even try
  return "";
}
