#pragma once

#include "utils/Thread.h"
#include "utils/CriticalSection.h"

class CGUILargeTextureManager : public CThread
{
public:
  CGUILargeTextureManager();
  virtual ~CGUILargeTextureManager();

  virtual void Process();

  LPDIRECT3DTEXTURE8 GetImage(const CStdString &path, int &width, int &height, int &orientation, bool firstRequest);
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
      SAFE_RELEASE(m_texture);
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

    void SetTexture(LPDIRECT3DTEXTURE8 texture, int width, int height, int orientation)
    {
      assert(m_texture == NULL);
      m_texture = texture;
      m_width = width;
      m_height = height;
      m_orientation = orientation;
    };

    LPDIRECT3DTEXTURE8 GetTexture() const { return m_texture; };
    int GetWidth() const { return m_width; };
    int GetHeight() const { return m_height; };
    int GetOrientation() const { return m_orientation; };
    const CStdString &GetPath() const { return m_path; };

  private:
    static const unsigned int TIME_TO_DELETE = 2000;

    unsigned int m_refCount;
    CStdString m_path;
    LPDIRECT3DTEXTURE8 m_texture;
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

