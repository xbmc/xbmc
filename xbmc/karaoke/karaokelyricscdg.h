//
// C++ Interface: karaokelyricscdg
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KARAOKELYRICSCDG_H
#define KARAOKELYRICSCDG_H

#if defined(HAS_SDL_OPENGL)
  #include "TextureManager.h"
#endif


#include "karaokelyrics.h"
#include "Cdg.h"

class CKaraokeLyricsCDG : public CKaraokeLyrics
{
  public:
      CKaraokeLyricsCDG( const CStdString& cdgFile );
    ~CKaraokeLyricsCDG();

    //! Parses the lyrics or song file, and loads the lyrics into memory. Returns true if the
    //! lyrics are successfully loaded, false otherwise.
    bool Load();

    //! Virtually all CDG lyrics have some kind of background
    virtual bool HasBackground();

    //! This function is called when the karoke visualisation window created. It may
    //! be called after Start(), but is guaranteed to be called before Render()
    //! Default implementation does nothing.
    virtual bool InitGraphics();

    //! This function is called when the karoke visualisation window is destroyed.
    //! Default implementation does nothing.
    virtual void Shutdown();

    //! This function is called to render the lyrics (each frame(?))
    virtual void Render();

  protected:
    void RenderIntoBuffer( unsigned char *pixels, unsigned int width, unsigned int height, unsigned int pitch ) const;
    SubCode* GetCurSubCode() const;
    bool SetNextSubCode();

  private:
    //! CDG file name
    CStdString  m_cdgFile;

    //! CDG parser
    CCdg    * m_pCdg;
    CCdgReader  * m_pReader;
    CCdgLoader  * m_pLoader;
    errCode      m_FileState;

    //! Rendering stuff
#if defined(HAS_SDL_OPENGL)
    CGLTexture * m_pCdgTexture;
#elif defined(HAS_SDL_2D)
    SDL_Surface *m_pCdgTexture;
#else
    LPDIRECT3DDEVICE8 m_pd3dDevice;
    LPDIRECT3DTEXTURE8 m_pCdgTexture;
#endif
    DWORD  m_bgAlpha;  //!< background alpha
    DWORD  m_fgAlpha;  //!< foreground alpha
};

#endif
