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


#include "karaokelyrics.h"
#include "Cdg.h"

class CBaseTexture;
typedef uint32_t color_t;

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

    //! Should return true if the lyrics have video file to play
    virtual bool HasVideo();

    //! Should return video parameters if HasVideo() returned true
    virtual void GetVideoParameters( CStdString& path, __int64& offset  );

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
    CBaseTexture* m_pCdgTexture;
    color_t  m_bgAlpha;  //!< background alpha
    color_t  m_fgAlpha;  //!< foreground alpha
};

#endif
