/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <fstream>
#include <string>

#if defined(_WIN32)
#include <filesystem>
#include <random>
#else
#include <cstdlib>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <Platinum/Source/Devices/MediaServer/PltFileMediaServer.h>
#include <gtest/gtest.h>

namespace
{

#if defined(_WIN32)
using FilePath = std::filesystem::path;

std::string PathToUtf8(const FilePath& path)
{
  const std::u8string value = path.u8string();
  return {reinterpret_cast<const char*>(value.data()), value.size()};
}

std::string GenericPathToUtf8(const FilePath& path)
{
  const std::u8string value = path.generic_u8string();
  return {reinterpret_cast<const char*>(value.data()), value.size()};
}

FilePath PathFromUtf8(const NPT_String& path)
{
  const char8_t* begin = reinterpret_cast<const char8_t*>(path.GetChars());
  return FilePath(std::u8string(begin, begin + path.GetLength()));
}

#else
using FilePath = std::string;

const std::string& PathToUtf8(const FilePath& path)
{
  return path;
}

FilePath PathFromUtf8(const NPT_String& path)
{
  return {path.GetChars(), path.GetLength()};
}
#endif

FilePath ChildPath(const FilePath& parent, const char* child)
{
#if defined(_WIN32)
  return parent / child;
#else
  return parent + "/" + child;
#endif
}

FilePath FilesystemRoot(const FilePath& path)
{
#if defined(_WIN32)
  return path.root_path();
#else
  NPT_COMPILER_UNUSED(path);
  return "/";
#endif
}

bool PathsEquivalent(const FilePath& first, const FilePath& second)
{
#if defined(_WIN32)
  return std::filesystem::equivalent(first, second);
#else
  struct stat firstInfo;
  struct stat secondInfo;
  return stat(first.c_str(), &firstInfo) == 0 && stat(second.c_str(), &secondInfo) == 0 &&
         firstInfo.st_dev == secondInfo.st_dev && firstInfo.st_ino == secondInfo.st_ino;
#endif
}

class TestFileMediaServerDelegate : public PLT_FileMediaServerDelegate
{
public:
  explicit TestFileMediaServerDelegate(const FilePath& root)
    : PLT_FileMediaServerDelegate("/", PathToUtf8(root).c_str())
  {
  }

  using PLT_FileMediaServerDelegate::ProcessFileRequest;

  NPT_Result GetObjectPath(const char* objectId, NPT_String& path)
  {
    return GetFilePath(objectId, path);
  }

  bool CanBuildBrowseObject(const FilePath& path)
  {
    NPT_HttpUrl url;
    NPT_HttpRequest request(url, NPT_HTTP_METHOD_GET);
    PLT_HttpRequestContext context(request);
    PLT_MediaObjectReference object(BuildFromFilePath(PathToUtf8(path).c_str(), context));
    return !object.IsNull();
  }

  NPT_Result GetBrowseResourceUri(const FilePath& path, NPT_String& resourceUri)
  {
    NPT_HttpUrl url;
    NPT_HttpRequest request(url, NPT_HTTP_METHOD_GET);
    NPT_SocketAddress localAddress(NPT_IpAddress(127, 0, 0, 1), 1234);
    NPT_HttpRequestContext httpContext(&localAddress, nullptr);
    PLT_HttpRequestContext context(request, httpContext);
    PLT_MediaObjectReference object(BuildFromFilePath(PathToUtf8(path).c_str(), context));
    if (object.IsNull() || object->m_Resources.GetItemCount() == 0)
      return NPT_FAILURE;

    resourceUri = object->m_Resources[0].m_Uri;
    return NPT_SUCCESS;
  }

  NPT_Result OnUpdateObject(PLT_ActionReference&,
                            const char*,
                            NPT_Map<NPT_String, NPT_String>&,
                            NPT_Map<NPT_String, NPT_String>&,
                            const PLT_HttpRequestContext&) override
  {
    return NPT_ERROR_NOT_IMPLEMENTED;
  }
};

struct RequestResult
{
  NPT_Result result;
  NPT_HttpStatusCode status;
  std::string body;
  std::string mimeType;
  std::string contentFeatures;
};

class TestUPnPFileMediaServer : public testing::Test
{
protected:
  void SetUp() override
  {
#if defined(_WIN32)
    std::random_device random;
    const FilePath base =
        std::filesystem::temp_directory_path() /
        ("kodi-upnp-file-server-" + std::to_string(random()) + std::to_string(random()));
    ASSERT_TRUE(std::filesystem::create_directory(base));
    m_base = base;
#else
    const char* temp = std::getenv("TMPDIR");
    std::string directory =
        std::string(temp && *temp ? temp : "/tmp") + "/kodi-upnp-file-server-XXXXXX";
    ASSERT_NE(nullptr, mkdtemp(directory.data()));
    m_base = directory;
    m_directories.push_back(m_base);
#endif
    m_root = ChildPath(m_base, "root");
    m_root2 = ChildPath(m_base, "root2");

    ASSERT_NO_FATAL_FAILURE(CreateDirectory(m_root));
    ASSERT_NO_FATAL_FAILURE(CreateDirectory(ChildPath(m_root, "nested")));
    ASSERT_NO_FATAL_FAILURE(CreateDirectory(m_root2));
    WriteFile(ChildPath(m_root, "movie..part.mkv"), "normal");
    WriteFile(ChildPath(m_root, "..hidden"), "hidden");
    WriteFile(ChildPath(m_root, "nested/song.flac"), "nested");
    WriteFile(ChildPath(m_root2, "outside.txt"), "outside");
    WriteFile(ChildPath(m_base, "outside.txt"), "outside");
  }

  void TearDown() override
  {
#if defined(_WIN32)
    if (!m_base.empty())
    {
      std::error_code error;
      std::filesystem::remove_all(m_base, error);
      EXPECT_FALSE(error) << error.message();
    }
#else
    if (m_workingDirectory >= 0)
    {
      EXPECT_EQ(0, fchdir(m_workingDirectory));
      EXPECT_EQ(0, close(m_workingDirectory));
    }
    for (auto path = m_files.rbegin(); path != m_files.rend(); ++path)
      EXPECT_EQ(0, unlink(path->c_str())) << *path;
    for (auto path = m_directories.rbegin(); path != m_directories.rend(); ++path)
      EXPECT_EQ(0, rmdir(path->c_str())) << *path;
#endif
  }

  void CreateDirectory(const FilePath& path)
  {
#if defined(_WIN32)
    ASSERT_TRUE(std::filesystem::create_directory(path));
#else
    ASSERT_EQ(0, mkdir(path.c_str(), 0700));
    m_directories.push_back(path);
#endif
  }

#if !defined(_WIN32)
  int CreateSymlink(const FilePath& target, const FilePath& path)
  {
    const int result = symlink(target.c_str(), path.c_str());
    if (result == 0)
      m_files.push_back(path);
    return result;
  }
#endif

  void WriteFile(const FilePath& path, const char* contents)
  {
    std::ofstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
#if !defined(_WIN32)
    m_files.push_back(path);
#endif
    stream << contents;
    ASSERT_TRUE(stream.good());
  }

  RequestResult Request(const char* rawPath) { return Request(m_root, rawPath); }

  RequestResult Request(const FilePath& root,
                        const char* rawPath,
                        bool requestContentFeatures = false)
  {
    TestFileMediaServerDelegate delegate(root);
    NPT_HttpUrl url;
    EXPECT_EQ(NPT_SUCCESS, url.SetPath(rawPath, true));
    NPT_HttpRequest request(url, NPT_HTTP_METHOD_GET);
    if (requestContentFeatures)
      request.GetHeaders().SetHeader("getcontentFeatures.dlna.org", "1");
    NPT_HttpRequestContext context;
    NPT_HttpResponse response(200, "OK");
    response.SetEntity(new NPT_HttpEntity());

    RequestResult requestResult{delegate.ProcessFileRequest(request, context, response),
                                response.GetStatusCode(),
                                {},
                                {},
                                {}};
    requestResult.mimeType = response.GetEntity()->GetContentType().GetChars();
    const NPT_String* contentFeatures =
        response.GetHeaders().GetHeaderValue("ContentFeatures.DLNA.ORG");
    if (contentFeatures)
      requestResult.contentFeatures = contentFeatures->GetChars();
    if (requestResult.result == NPT_SUCCESS && requestResult.status == 200)
    {
      NPT_DataBuffer data;
      EXPECT_EQ(NPT_SUCCESS, response.GetEntity()->Load(data));
      requestResult.body.assign(reinterpret_cast<const char*>(data.GetData()), data.GetDataSize());
    }
    return requestResult;
  }

  void ExpectRejected(const char* rawPath)
  {
    const RequestResult result = Request(rawPath);
    EXPECT_EQ(NPT_SUCCESS, result.result);
    EXPECT_EQ(404, result.status);
  }

  void ExpectServeFileRejected(const char* rawPath)
  {
    const RequestResult result = Request(rawPath);
    EXPECT_EQ(NPT_ERROR_NO_SUCH_ITEM, result.result);
    EXPECT_EQ(200, result.status);
  }

#if !defined(_WIN32)
  int m_workingDirectory{-1};
  std::vector<FilePath> m_files;
  std::vector<FilePath> m_directories;
#endif
  FilePath m_base;
  FilePath m_root;
  FilePath m_root2;
};

TEST_F(TestUPnPFileMediaServer, ServesNormalFileBelowRoot)
{
  const RequestResult result = Request("/%25/movie..part.mkv");

  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("normal", result.body);
}

TEST_F(TestUPnPFileMediaServer, ServesFilenameBeginningWithTwoDots)
{
  const RequestResult result = Request("/%25/..hidden");

  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("hidden", result.body);
}

TEST_F(TestUPnPFileMediaServer, ServesNestedFileBelowRoot)
{
  const RequestResult result = Request("/%25/nested/song.flac");

  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("nested", result.body);
}

TEST_F(TestUPnPFileMediaServer, TrimsRedundantSeparatorsFromNonRoot)
{
  FilePath root = m_root;
  root += NPT_FilePath::Separator;
  root += NPT_FilePath::Separator;

  const RequestResult result = Request(root, "/%25/movie..part.mkv");
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("normal", result.body);
}

TEST_F(TestUPnPFileMediaServer, RoundTripsBrowseResourceUriFromFilesystemRoot)
{
  const FilePath filesystemRoot = FilesystemRoot(m_root);
  const FilePath file = ChildPath(m_root, "nested/song.flac");
  const std::string relativePath = PathToUtf8(file).substr(PathToUtf8(filesystemRoot).size());
  const NPT_String encodedPath =
      NPT_Uri::PercentEncode(relativePath.c_str(), NPT_Uri::PathCharsToEncode);
  TestFileMediaServerDelegate delegate(filesystemRoot);
  NPT_String resourceUri;

  ASSERT_EQ(NPT_SUCCESS, delegate.GetBrowseResourceUri(file, resourceUri));
  NPT_HttpUrl url(resourceUri.GetChars());
  EXPECT_NE(NPT_STRING_SEARCH_FAILED, url.GetPath().Find(encodedPath));

  const RequestResult result = Request(filesystemRoot, url.GetPath());
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("nested", result.body);
}

#if !defined(_WIN32)
TEST_F(TestUPnPFileMediaServer, TrimsMixedSeparatorsFromRelativeRoot)
{
  m_workingDirectory = open(".", O_RDONLY);
  ASSERT_GE(m_workingDirectory, 0);
  ASSERT_EQ(0, chdir(m_base.c_str()));

  const RequestResult result = Request("root/\\/", "/%25/nested/song.flac");
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("nested", result.body);

  TestFileMediaServerDelegate delegate("root/\\/");
  NPT_String path;
  ASSERT_EQ(NPT_SUCCESS, delegate.GetObjectPath("0/nested/song.flac", path));
  EXPECT_STREQ("root/nested/song.flac", path.GetChars());
}

TEST_F(TestUPnPFileMediaServer, PreservesPosixRootWithRedundantSeparators)
{
  const std::string relativePath = ChildPath(m_root, "nested/song.flac").substr(1);
  const NPT_String encodedPath =
      NPT_Uri::PercentEncode(relativePath.c_str(), NPT_Uri::PathCharsToEncode);
  const std::string rawPath = "/%25/" + std::string(encodedPath.GetChars());

  const RequestResult result = Request("///\\/", rawPath.c_str());
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("nested", result.body);

  TestFileMediaServerDelegate delegate("///\\/");
  NPT_String path;
  ASSERT_EQ(NPT_SUCCESS, delegate.GetObjectPath("0", path));
  EXPECT_STREQ("/", path.GetChars());
}

TEST_F(TestUPnPFileMediaServer, ResolvesSymlinkedRoot)
{
  const FilePath root = ChildPath(m_base, "root-link");
  ASSERT_EQ(0, CreateSymlink(m_root, root));

  const RequestResult result = Request(root, "/%25/movie..part.mkv");
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("normal", result.body);
}

TEST_F(TestUPnPFileMediaServer, AllowsInRootDirectorySymlinkForRequestsAndBrowse)
{
  ASSERT_EQ(0, CreateSymlink("nested", ChildPath(m_root, "nested-link")));
  const RequestResult result = Request("/%25/nested-link/song.flac");
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("nested", result.body);

  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;
  ASSERT_EQ(NPT_SUCCESS, delegate.GetObjectPath("0/nested-link/song.flac", path));
  EXPECT_EQ(PathToUtf8(ChildPath(m_root, "nested-link/song.flac")), path.GetChars());
  EXPECT_TRUE(delegate.CanBuildBrowseObject(ChildPath(m_root, "nested-link/song.flac")));
}

TEST_F(TestUPnPFileMediaServer, RejectsDanglingAndLoopingSymlinks)
{
  ASSERT_EQ(0, CreateSymlink("missing", ChildPath(m_root, "dangling")));
  ASSERT_EQ(0, CreateSymlink("loop", ChildPath(m_root, "loop")));
  ExpectServeFileRejected("/%25/dangling");
  ExpectServeFileRejected("/%25/loop");

  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;
  EXPECT_EQ(NPT_ERROR_NO_SUCH_ITEM, delegate.GetObjectPath("0/dangling", path));
  EXPECT_EQ(NPT_ERROR_NO_SUCH_ITEM, delegate.GetObjectPath("0/loop", path));
  EXPECT_FALSE(delegate.CanBuildBrowseObject(ChildPath(m_root, "dangling")));
}
#endif

TEST_F(TestUPnPFileMediaServer, RejectsUnresolvableRoot)
{
  const FilePath root = ChildPath(m_base, "missing-root");
  const RequestResult result = Request(root, "/%25/movie..part.mkv");
  EXPECT_EQ(NPT_ERROR_NO_SUCH_ITEM, result.result);
  EXPECT_EQ(200, result.status);

  TestFileMediaServerDelegate delegate(root);
  NPT_String path;
  EXPECT_EQ(NPT_ERROR_NO_SUCH_ITEM, delegate.GetObjectPath("0", path));
  EXPECT_FALSE(delegate.CanBuildBrowseObject(ChildPath(m_root, "movie..part.mkv")));
}

TEST_F(TestUPnPFileMediaServer, RejectsMetadataOutsideRoot)
{
  TestFileMediaServerDelegate delegate(m_root);
  EXPECT_FALSE(delegate.CanBuildBrowseObject(ChildPath(m_root2, "outside.txt")));
  EXPECT_FALSE(delegate.CanBuildBrowseObject(ChildPath(m_base, "outside.txt")));
  EXPECT_FALSE(delegate.CanBuildBrowseObject(FilesystemRoot(m_root)));
}

#if defined(_WIN32)
TEST_F(TestUPnPFileMediaServer, PreservesWindowsDriveRoot)
{
  const FilePath driveRoot = FilesystemRoot(m_root);
  ASSERT_FALSE(driveRoot.empty());
  ASSERT_TRUE(driveRoot.is_absolute());

  const FilePath file = ChildPath(m_root, "nested/song.flac");
  const std::string relativePath = GenericPathToUtf8(file.lexically_relative(driveRoot));
  const NPT_String encodedPath =
      NPT_Uri::PercentEncode(relativePath.c_str(), NPT_Uri::PathCharsToEncode);
  const std::string rawPath = "/%25/" + std::string(encodedPath.GetChars());
  const RequestResult result = Request(driveRoot, rawPath.c_str());

  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("nested", result.body);
}
#endif

TEST_F(TestUPnPFileMediaServer, UsesRequestedPathForMimeTypeOfInRootSymlink)
{
#if defined(_WIN32)
  GTEST_SKIP() << "Windows reparse points are intentionally rejected";
#else
  WriteFile(ChildPath(m_root, "extensionless-target"), "symlink");
  ASSERT_EQ(0, CreateSymlink("extensionless-target", ChildPath(m_root, "movie.mp4")));

  const RequestResult result = Request(m_root, "/%25/movie.mp4", true);
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("symlink", result.body);
  EXPECT_EQ("video/mp4", result.mimeType);
  EXPECT_NE(std::string::npos, result.contentFeatures.find("MPEG4_P2_SP_AAC"));
#endif
}

TEST_F(TestUPnPFileMediaServer, AcceptsCaseVariantInRootSymlinkOnCaseInsensitiveFilesystem)
{
#if defined(_WIN32)
  GTEST_SKIP() << "Windows reparse points are intentionally rejected";
#else
  const FilePath alternateRoot = ChildPath(m_base, "ROOT");
  if (!PathsEquivalent(m_root, alternateRoot))
    GTEST_SKIP() << "Filesystem is case-sensitive";

  WriteFile(ChildPath(m_root, "case-target"), "case");
  ASSERT_EQ(0,
            CreateSymlink(ChildPath(alternateRoot, "case-target"), ChildPath(m_root, "case-link")));

  const RequestResult result = Request("/%25/case-link");
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("case", result.body);
#endif
}

TEST_F(TestUPnPFileMediaServer, RejectsTraversalPaths)
{
  ExpectRejected("/%25/../outside.txt");
  ExpectRejected("/%25/..\\outside.txt");
  ExpectRejected("/%25/%2e%2e%2foutside.txt");
  ExpectRejected("/%25/%2e%2e%5coutside.txt");
  ExpectRejected("/%25/.%2e/outside.txt");
  ExpectRejected("/%25/nested/../../outside.txt");
  ExpectRejected("/%/../outside.txt");
}

TEST_F(TestUPnPFileMediaServer, RejectsAbsolutePaths)
{
  ExpectRejected("/%25//etc/passwd");
#if defined(_WIN32)
  ExpectRejected("/%25/C:%5cWindows%5cwin.ini");
#endif
  ExpectRejected("/%25/%5c%5cserver%5cshare%5cfile");
}

TEST_F(TestUPnPFileMediaServer, RejectsRootPrefixCollision)
{
  ExpectRejected("/%25/../root2/outside.txt");
}

TEST_F(TestUPnPFileMediaServer, DoesNotDecodePercentEncodingTwice)
{
  WriteFile(ChildPath(m_root, "%2e%2e%2fpercent.txt"), "percent");

  const RequestResult result = Request("/%25/%252e%252e%252fpercent.txt");
  EXPECT_EQ(NPT_SUCCESS, result.result);
  EXPECT_EQ(200, result.status);
  EXPECT_EQ("percent", result.body);
}

TEST_F(TestUPnPFileMediaServer, RejectsMalformedPathEncodingWithoutThrowing)
{
  EXPECT_NO_THROW(ExpectServeFileRejected("/%25/%ff"));
  EXPECT_NO_THROW(ExpectServeFileRejected("/%25/%c3%28"));
}

TEST_F(TestUPnPFileMediaServer, PreservesServeFileFailureSemantics)
{
  ExpectServeFileRejected("/%25/missing-file");
}

TEST_F(TestUPnPFileMediaServer, RejectsSymlinkEscape)
{
#if defined(_WIN32)
  std::error_code error;
  std::filesystem::create_directory_symlink(m_root2, ChildPath(m_root, "outside-link"), error);
  if (error)
    GTEST_SKIP() << "Cannot create directory symlink: " << error.message();
#else
  ASSERT_EQ(0, CreateSymlink(m_root2, ChildPath(m_root, "outside-link")));
#endif

  ExpectServeFileRejected("/%25/outside-link/outside.txt");
}

TEST_F(TestUPnPFileMediaServer, ResolvesNormalBrowseObjectIdsBelowRoot)
{
  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;

  ASSERT_EQ(NPT_SUCCESS, delegate.GetObjectPath("0/nested/song.flac", path));
  EXPECT_TRUE(PathsEquivalent(PathFromUtf8(path), ChildPath(m_root, "nested/song.flac")));
}

TEST_F(TestUPnPFileMediaServer, RejectsBrowseObjectIdTraversal)
{
  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;

  EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath("0/../outside.txt", path)));
  EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath("0/..\\outside.txt", path)));
  EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath("0/nested/../../outside.txt", path)));
  EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath("0/../root2/outside.txt", path)));
}

TEST_F(TestUPnPFileMediaServer, RejectsMalformedBrowseObjectIdWithoutThrowing)
{
  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;
  const char malformedObjectId[] = {'0', '/', static_cast<char>(0xff), '\0'};

  EXPECT_NO_THROW(EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath(malformedObjectId, path))));
}

TEST_F(TestUPnPFileMediaServer, DoesNotUrlDecodeBrowseObjectIds)
{
  WriteFile(ChildPath(m_root, "%2e%2e"), "percent-object-id");
  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;

  ASSERT_EQ(NPT_SUCCESS, delegate.GetObjectPath("0/%2e%2e", path));
  EXPECT_TRUE(PathsEquivalent(PathFromUtf8(path), ChildPath(m_root, "%2e%2e")));
}

TEST_F(TestUPnPFileMediaServer, RejectsBrowseObjectIdSymlinkEscape)
{
#if defined(_WIN32)
  std::error_code error;
  std::filesystem::create_directory_symlink(m_root2, ChildPath(m_root, "browse-link"), error);
  if (error)
    GTEST_SKIP() << "Cannot create directory symlink: " << error.message();
#else
  ASSERT_EQ(0, CreateSymlink(m_root2, ChildPath(m_root, "browse-link")));
#endif

  TestFileMediaServerDelegate delegate(m_root);
  NPT_String path;
  EXPECT_TRUE(NPT_FAILED(delegate.GetObjectPath("0/browse-link/outside.txt", path)));
  EXPECT_FALSE(delegate.CanBuildBrowseObject(ChildPath(m_root, "browse-link")));
}

} // namespace
