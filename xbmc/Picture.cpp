
#include "stdafx.h"
#include "picture.h"
#include "util.h"


CPicture::CPicture(void)
{
  ZeroMemory(&m_info, sizeof(ImageInfo));
}

CPicture::~CPicture(void)
{

}

IDirect3DTexture8* CPicture::Load(const CStdString& strFileName, int iMaxWidth, int iMaxHeight)
{
  if (!m_dll.Load()) return NULL;

  memset(&m_info, 0, sizeof(ImageInfo));
  if (!m_dll.LoadImage(strFileName.c_str(), iMaxWidth, iMaxHeight, &m_info))
  {
    CLog::Log(LOGERROR, "PICTURE: Error loading image %s", strFileName.c_str());
    return NULL;
  }
  // loaded successfully
  return m_info.texture;
}

bool CPicture::DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, bool checkExistence /*= false*/)
{
  // don't create the thumb if it already exists
  if (checkExistence && CFile::Exists(strThumbFileName))
    return true;

  CLog::Log(LOGINFO, "Creating thumb from: %s as: %s", strFileName.c_str(),strThumbFileName.c_str());
  
  // load our dll
  if (!m_dll.Load()) return false;

  memset(&m_info, 0, sizeof(ImageInfo));
  if (!m_dll.CreateThumbnail(strFileName.c_str(), strThumbFileName.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize, g_guiSettings.GetBool("pictures.useexifrotation")))
  {
    CLog::Log(LOGERROR, "PICTURE::DoCreateThumbnail: Unable to create thumbfile %s from image %s", strThumbFileName.c_str(), strFileName.c_str());
    return false;
  }
  return true;
}

bool CPicture::CreateThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName)
{
  CLog::Log(LOGINFO, "Creating album thumb from memory: %s", strThumbFileName.c_str());
  if (!m_dll.Load()) return false;
  if (!m_dll.CreateThumbnailFromMemory((BYTE *)pBuffer, nBufSize, strExtension.c_str(), strThumbFileName.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "PICTURE::CreateAlbumThumbnailFromMemory: exception: memfile FileType: %s\n", strExtension.c_str());
    return false;
  }
  return true;
}

void CPicture::CreateFolderThumb(const CStdString *strThumbs, const CStdString &folderThumbnail)
{ // we want to mold the thumbs together into one single one
  if (!m_dll.Load()) return;
  CStdString strThumbnails[4];
  const char *szThumbs[4];
  for (int i=0; i < 4; i++)
  {
    if (strThumbs[i].IsEmpty())
      strThumbnails[i] = "";
    else
    {
      CFileItem item(strThumbs[i], false);
      strThumbnails[i] = item.GetCachedPictureThumb();
      DoCreateThumbnail(strThumbs[i], strThumbnails[i], true);
    }
    szThumbs[i] = strThumbnails[i].c_str();
  }
  if (!m_dll.CreateFolderThumbnail(szThumbs, folderThumbnail.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "PICTURE::CreateFolderThumb() failed for folder thumb %s", folderThumbnail.c_str());
  }
}

bool CPicture::CreateThumbnailFromSurface(BYTE* pBuffer, int width, int height, int stride, const CStdString &strThumbFileName)
{
  if (!pBuffer || !m_dll.Load()) return false;
  return m_dll.CreateThumbnailFromSurface(pBuffer, width, height, stride, strThumbFileName.c_str());
}

int CPicture::ConvertFile(const CStdString &srcFile, const CStdString &destFile, float rotateDegrees, int width, int height, unsigned int quality)
{ 
  if (!m_dll.Load()) return false;
  int ret;
  ret=m_dll.ConvertFile(srcFile.c_str(), destFile.c_str(), rotateDegrees, width, height, quality);
  if (ret!=0)
  {
    CLog::Log(LOGERROR, "PICTURE: Error %i converting image %s", ret, srcFile.c_str());
    return ret;
  }
  return ret;
}
