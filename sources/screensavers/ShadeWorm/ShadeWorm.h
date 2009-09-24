#include <xtl.h>
#include <xgraphics.h>

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);

class ShadeWorm_c
{
public:
	ShadeWorm_c();
	~ShadeWorm_c();

	void Create(LPDIRECT3DDEVICE8 device, int width, int height, const char*saverName);
	bool Start();
	void Stop();
	void Render();

private:

	struct Worm_t
	{
		float x;		// Current position
		float y;
		float vx;		// Current velocity
		float vy;	
	};

	struct ColMapEntry_t
	{
		int m_index;	// Index in the palette (0-255)
		int m_col;		// Colour at this index
	};

	struct ColMap_t
	{
		int				m_numCols;	// Num of cols in this colour map
		ColMapEntry_t*	m_colMap;	// Pointer to the colour map cols
	};

	struct TEXVERTEX
	{
		FLOAT x, y, z, w;	// Position
		DWORD colour;		// Colour
		float u, v;			// Texture coords
	};

	enum State_e {STATE_NEWCOL, STATE_FADEUP, STATE_FADEDOWN, STATE_DRAWING};
	#define	D3DFVF_TEXVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

	#define MAX_WORMS	(32)
	#define MAX_COLMAPS (32)

	static const char*	m_shaderColourMapSrc;
	LPDIRECT3DDEVICE8	m_device;
	int					m_height;
	int					m_width;
	IDirect3DTexture8*	m_bufferTexture;	// Workspace area where worms are drawn
	IDirect3DTexture8*	m_colMapTexture;	// Used for remapping the colours
	IDirect3DTexture8*	m_spriteTexture;	// Texture for the worm sprite
	DWORD				m_pShader;			// Pixel shader
	LPXGBUFFER			m_pUcode;
	char				m_saverName[1024];
	Worm_t				m_worms[MAX_WORMS];
	int					m_numWorms;
	ColMap_t			m_colMaps[MAX_COLMAPS];
	int					m_numColMaps;
	int					m_colMap;
	int					m_drawTime;
	bool				m_randomColMap;
	int					m_timer;

	void LoadSettings(const char* name);
	void CreateColMap(int colIndex);
	void RenderSprite(float x, float y, float sizeX, float sizeY, int col);
	float GetRand();
	void Clamp(int a, int b, int& val);
	void Clamp(float a, float b, float& val);

};