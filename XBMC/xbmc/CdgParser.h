#ifndef CDGREADER_H
#define CDGREADER_H

#include "utils/thread.h"
#include "utils/criticalsection.h"
#include  "FileSystem/File.h"
#include "Cdg.h"
#include "xbstopwatch.h"

//////////////////////
//////CdgLoader///////
//////////////////////
#define STREAM_CHUNK	32768	// Cdg File is streamed in chunks of 32Kb
typedef enum
{
	FILE_ERR_NOT_FOUND,
	FILE_ERR_OPENING,
	FILE_ERR_NO_MEM,
	FILE_ERR_LOADING,
	FILE_LOADING,
	FILE_LOADED,
	FILE_NOT_LOADED
} errCode;

class CCdgLoader	: public CThread
{
public:
	CCdgLoader();
	~CCdgLoader();
	void StreamFile(CStdString strFileName);
	void StopStream();
	SubCode*	GetCurSubCode();
	bool	SetNextSubCode();
	errCode	GetFileState();
	CStdString	GetFileName();
protected:
	CFile m_File;
	CStdString	m_strFileName;
	BYTE	*m_pBuffer;
	SubCode	*m_pSubCode;
	errCode	m_CdgFileState;
	UINT	m_uiFileLength;
	UINT	m_uiLoadedBytes;
	UINT	m_uiStreamChunk;
	CCriticalSection	m_CritSection;

	SubCode* GetFirstLoaded();
	SubCode* GetLastLoaded();
	virtual void	OnStartup();
	virtual void	OnExit();
	virtual void	Process();
};


//////////////////////
//////CdgReader///////
//////////////////////
//CDG data packets should be read at a fixed frequency to assure sync with audio:
//Audio content on cd :
//(44100 samp/(chan*second)*2 chan*16 bit/samp)/(2352*8 bits/sector)=75 sectors/second.
// CDG data on cd :
//4 packets/sector*75 sectors/second=300 packets/second = 300 Hz 
#define PARSING_FREQ 300.0f  
#define DEBUG_AVDELAY_MOD 0.3f  //Adjustment for AV delay  in debug mode

class CCdgReader : public CThread
{
public:
	CCdgReader();
	~CCdgReader();
	bool		Attach(CCdgLoader*	pLoader);
	void		DetachLoader();
	bool		Start(float fTime);
	void		Pause();
	void		SetAVDelay(float fDelay);
	float		GetAVDelay();
	errCode GetFileState();
	CStdString	GetFileName();
	CCdg*	GetCdg();

protected:
	CCdgLoader*	m_pLoader;
	CCdg		m_Cdg;
	CXBStopWatch	m_Timer;
	float		m_fStartingTime;
	float		m_fAVDelay;
	UINT	m_uiNumReadSubCodes;
	CCriticalSection	m_CritSection;
	
	void	ReadUpToTime(float secs);
	virtual void	OnStartup();
	virtual void	OnExit();
	virtual void	Process();
};

//////////////////////
//////CdgRenderer/////
//////////////////////
#define TEXWIDTH    512	 // Texture size = power of 2
#define TEXHEIGHT  256
#define TEX_COLOR DWORD  //Texture color format is A8R8G8B8
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_TEX1)
//Message strings:
#define KARAOKE 13327
#define NOT_FOUND 13328
#define ERROR_OPENING 13329
#define UNABLE_TO_LOAD 13330
#define ERROR_OUT_OF_MEMORY 13331
typedef struct 
{
    FLOAT x, y, z, rhw;
    FLOAT u, v;
} CUSTOMVERTEX;

class CCdgRenderer
{
public:
	CCdgRenderer();
	~CCdgRenderer();
	bool Attach(CCdgReader* pReader);
	void DetachReader();
	bool InitGraphics();
	void ReleaseGraphics();
	void	Render();

protected:
	LPDIRECT3DDEVICE8				m_pd3dDevice;
	LPDIRECT3DVERTEXBUFFER8	m_pVertexBuffer;
	LPDIRECT3DTEXTURE8			m_pCdgTexture;
	CCdgReader*	m_pReader;
	CCdg*	m_pCdg;
	bool	m_bRender;
	errCode	m_FileState;
	CCriticalSection	m_CritSection;

	void	UpdateTexture();
	void	DrawTexture();
	TEX_COLOR GetBgAlpha();
	TEX_COLOR GetFgAlpha();
	TEX_COLOR ConvertColor(CDG_COLOR);
};


//////////////////////
//////CdgParser///////
//////////////////////
class CCdgParser
{
public:
	CCdgParser();
	~CCdgParser();
	bool		AllocGraphics();
	void		FreeGraphics();
	bool		Start(CStdString strSongPath);
	void		Stop();
	void		Pause();
	void		Free();
	void		SetAVDelay(float fDelay);
	float		GetAVDelay();
	void		Render();

protected:
	CCdgLoader*	m_pLoader;
	CCdgReader*	m_pReader;
	CCdgRenderer*	m_pRenderer;
	CCriticalSection	m_CritSection;

	bool		AllocRenderer();
	bool		AllocLoader();
	bool		AllocReader();
	void		FreeLoader();
	void		FreeReader();
	bool		StartLoader(CStdString strSongPath);
	void		StopLoader();
	bool		StartReader();
	void		StopReader();
};

#endif