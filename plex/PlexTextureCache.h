#ifndef PLEXTEXTURECACHE_H
#define PLEXTEXTURECACHE_H

#include "TextureCache.h"

class CPlexTextureCache : public CTextureCache
{
private:

public:
  CPlexTextureCache() {}
  ~CPlexTextureCache() {}

  virtual void Initialize() {}
  virtual void Deinitialize();
  virtual bool HasCachedImage(const CStdString &image);
  virtual CStdString GetCachedImage(const CStdString &image, CTextureDetails &details, bool trackUsage = false);
  virtual CStdString CheckCachedImage(const CStdString &image, bool returnDDS, bool &needsRecaching);
  virtual void BackgroundCacheImage(const CStdString &image);
  virtual void ClearCachedImage(const CStdString &image, bool deleteSource = false);
  virtual bool GetCachedTexture(const CStdString &url, CTextureDetails &details);
  virtual bool AddCachedTexture(const CStdString &image, const CTextureDetails &details);
  virtual void IncrementUseCount(const CTextureDetails &details);
  virtual bool SetCachedTextureValid(const CStdString &url, bool updateable);
  virtual bool ClearCachedTexture(const CStdString &url, CStdString &cacheFile);
  virtual void OnCachingComplete(bool success, CTextureCacheJob *job);
};

#endif // PLEXTEXTURECACHE_H
