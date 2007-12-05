#pragma once

#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#ifdef HAS_SDL
#include "SDL/SDL.h"
#include "TextureManager.h"
#endif

class CGUILargeTextureManager : public CThread
{
public:
  CGUILargeTextureManager();
  virtual ~CGUILargeTextureManager();

  virtual void Process();

#ifndef HAS_SDL
  LPDIRECT3DTEXTURE8 GetImage(const CStdString &path, int &width, int &height, int &orientation, bool firstRequest);
#elif defined (HAS_SDL_2D)
  SDL_Surface * GetImage(const CStdString &path, int &width, int &height, int &orientation, bool firstRequest);
#else
  CGLTexture  * GetImage(const CStdString &path, int &width, int &height, int &orientation, bool firstRequest);
#endif
  void ReleaseImage(const CStdString &path);

  void CleanupUnusedImages();

protected:
  class CLargeTexture
  {
  public:
    CLargeTexture(const CStdString &path)
    {
      m_path = path;
      m_width = 0;
      m_height = 0;
      m_orientation = 0;
      m_texture = NULL;
      m_refCount = 1;
      m_timeToDelete = 0;
    };

    virtual ~CLargeTexture()
    {
      assert(m_refCount == 0);
#ifndef HAS_SDL
      SAFE_RELEASE(m_texture);
#elif defined (HAS_SDL_2D)
      if (m_texture) 
         SDL_FreeSurface(m_texture);
#else
      delete m_texture;
#endif
      m_texture = NULL;
    };

    void AddRef() { m_refCount++; };
    bool DecrRef(bool deleteImmediately)
    {
      assert(m_refCount);
      m_refCount--;
      if (m_refCount == 0)
      {
        if (deleteImmediately)
          delete this;
        else
          m_timeToDelete = timeGetTime() + TIME_TO_DELETE;
        return true;
      }
      return false;
    };

    bool DeleteIfRequired()
    {
      if (m_refCount == 0 && m_timeToDelete < timeGetTime())
      {
        delete this;
        return true;
      }
      return false;
    };

#ifndef HAS_SDL
    void SetTexture(LPDIRECT3DTEXTURE8 texture, int width, int height, int orientation)
#elif defined (HAS_SDL_2D)
    void SetTexture(SDL_Surface * texture, int width, int height, int orientation)
#else
    void SetTexture(CGLTexture * texture, int width, int height, int orientation)
#endif
    {
      assert(m_texture == NULL);
      m_texture = texture;
      m_width = width;
      m_height = height;
      m_orientation = orientation;
    };

#ifndef HAS_SDL
    LPDIRECT3DTEXTURE8 GetTexture() const { return m_texture; };
#elif defined (HAS_SDL_2D)
    SDL_Surface * GetTexture() const { return m_texture; };
#else
    CGLTexture * GetTexture() const { return m_texture; };
#endif
    int GetWidth() const { return m_width; };
    int GetHeight() const { return m_height; };
    int GetOrientation() const { return m_orientation; };
    const CStdString &GetPath() const { return m_path; };

  private:
    static const unsigned int TIME_TO_DELETE = 2000;

    unsigned int m_refCount;
    CStdString m_path;
#ifndef HAS_SDL
    LPDIRECT3DTEXTURE8 m_texture;
#elif defined (HAS_SDL_2D)
    SDL_Surface * m_texture;
#else
    CGLTexture * m_texture;
#endif
    int m_width;
    int m_height;
    int m_orientation;
    unsigned int m_timeToDelete;
  };

  void QueueImage(CLargeTexture *image);

private:
  vector<CLargeTexture *> m_queued;
  vector<CLargeTexture *> m_allocated;
  typedef vector<CLargeTexture *>::iterator listIterator;

  CCriticalSection m_listSection;
  bool m_running;
};

extern CGUILargeTextureManager g_largeTextureManager;

