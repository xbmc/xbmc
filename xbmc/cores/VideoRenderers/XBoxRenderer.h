#ifndef XBOX_RENDERER
#define XBOX_RENDERER

#define NUM_BUFFERS      2

typedef struct YV12Image
{
  BYTE *plane[3];
  unsigned int stride[3];
  unsigned int width;
  unsigned int height;
} YV12Image;

struct DRAWRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

static enum EFIELDSYNC
{
  FS_NONE,
  FS_ODD,
  FS_EVEN,
};

static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
static const DWORD FVF_Y8A8VERTEX = D3DFVF_XYZRHW | D3DFVF_TEX2;

class CXBoxRenderer : private CThread
{
public:
  CXBoxRenderer(LPDIRECT3DDEVICE8 pDevice);
  ~CXBoxRenderer();

  virtual void GetVideoRect(RECT &rs, RECT &rd);
  virtual float GetAspectRatio();
  virtual void Update(bool bPauseDrawing);
  virtual void RenderUpdate(bool clear);
  virtual void CheckScreenSaver() {};
  virtual void SetupScreenshot() {};
  virtual void SetViewMode(int iViewMode);
  void CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height);

  // Functions called from mplayer
  virtual void WaitForFlip();
  virtual unsigned int QueryFormat(unsigned int format) { return 0; };
  virtual unsigned int Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps);
  virtual unsigned int GetImage(YV12Image *image);
  // ReleaseImage().  Called when the player has finished with the image from GetImage()
  // useful to do any postprocessing stuff (change of data formats etc.) in preparation
  // for FlipPage() without loading FlipPage() with code.
  virtual void ReleaseImage();
  virtual unsigned int PutImage(YV12Image *image);
  virtual unsigned int DrawFrame(unsigned char *src[]);
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y);
  virtual void FlipPage();
  virtual unsigned int PreInit();
  virtual void UnInit();
  virtual void DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride);
  virtual int GetNormalDisplayWidth() { return m_iNormalDestWidth; }
  virtual int GetNormalDisplayHeight() { return (int)(m_iNormalDestWidth / (m_iSourceWidth / (float)m_iSourceHeight)); }
  virtual void RenderBlank();
  void AutoCrop(bool bCrop);
  int GetBuffersCount() { return m_NumYV12Buffers; };

  void SetFieldSync(EFIELDSYNC mSync);
  void FlipPageAsync();

  int GetAsyncFlipTime() { return m_iAsyncFlipTime; } ;

protected:
  virtual void Render() {};
  virtual void CalcNormalDisplayRect(float fOffsetX1, float fOffsetY1, float fScreenWidth, float fScreenHeight, float fUserPixelRatio, float fZoomAmount);
  void CalculateFrameAspectRatio(int desired_width, int desired_height);
  virtual void ManageDisplay();
  virtual void CreateTextures();
  void ChooseBestResolution(float fps);
  RESOLUTION GetResolution();
  void CopyAlpha(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta, int dststride);
  virtual void ManageTextures();
  void DeleteOSDTextures(int index);
  void Setup_Y8A8Render();
  void RenderOSD();
  void SetupSubtitles();
  void DeleteYV12Texture(int index);
  void ClearYV12Texture(int index);
  bool CreateYV12Texture(int index);
  void CopyYV12Texture(int dest);

  // low memory renderer (default PixelShaderRenderer)
  void RenderLowMem();
  static const DWORD FVF_YV12VERTEX = D3DFVF_XYZRHW | D3DFVF_TEX3;
  int m_iYV12DecodeBuffer;
  int m_NumYV12Buffers;

  float m_fSourceFrameRatio; // the frame aspect ratio of the source (corrected for pixel ratio)
  RESOLUTION m_iResolution;    // the resolution we're running in
  bool m_bFlipped;      // true as soon as we've flipped screens
  float m_fps;        // fps of movie
  RECT rd;          // destination rect
  RECT rs;          // source rect
  unsigned int m_iSourceWidth;    // width
  unsigned int m_iSourceHeight;   // height
  unsigned int m_iNormalDestWidth;  // initial destination width in normal view

  bool m_bConfigured;
  
  EFIELDSYNC m_iFieldSync;

  // OSD stuff
  LPDIRECT3DTEXTURE8 m_pOSDYTexture[NUM_BUFFERS];
  LPDIRECT3DTEXTURE8 m_pOSDATexture[NUM_BUFFERS];
  float m_OSDWidth;
  float m_OSDHeight;
  DRAWRECT m_OSDRect;
  int m_iOSDBuffer;
  bool m_OSDRendered;
  int m_iOSDTextureWidth;
  int m_iOSDTextureHeight[NUM_BUFFERS];
  int m_NumOSDBuffers;

  // YV12 decoder textures
  LPDIRECT3DTEXTURE8 m_YTexture[NUM_BUFFERS];
  LPDIRECT3DTEXTURE8 m_UTexture[NUM_BUFFERS];
  LPDIRECT3DTEXTURE8 m_VTexture[NUM_BUFFERS];

  // Deinterlace/reinterlace fields for upsampling
  D3DTexture m_YFieldTexture[NUM_BUFFERS];
  D3DTexture m_UFieldTexture[NUM_BUFFERS];
  D3DTexture m_VFieldTexture[NUM_BUFFERS];
  unsigned int m_YFieldPitch;
  unsigned int m_UVFieldPitch;

  // render device
  LPDIRECT3DDEVICE8 m_pD3DDevice;

  // pixel shader (low memory shader used in all renderers while in GUI)
  DWORD m_hLowMemShader;

  // clear colour for "black" bars
  DWORD m_clearColour;


  // render thread
  CEvent m_eventFrame;
  int m_iAsyncFlipTime; //Time of an average flip in milliseconds

  void Process();
};

#endif
