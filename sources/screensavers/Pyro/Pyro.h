// include file for screensaver template

#include <xtl.h>

struct projectile {
	int x, y;	/* position */
	int dx, dy;	/* velocity */
	int decay;
	int size;
	int fuse;
	bool primary;
	bool dead;
	//XColor color;
	//D3DCOLORVALUE colour;
	D3DCOLOR colour;
	struct projectile *next_free;
};

static struct projectile *projectiles, *free_projectiles;
static unsigned int default_fg_pixel;
static int how_many, frequency, scatter;

static struct projectile *get_projectile(void);
static void free_projectile(struct projectile *p);
static void launch(int xlim, int ylim, int g);
static struct projectile *shrapnel(struct projectile *parent);

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);

#define D3DFVF_TLVERTEX D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1

typedef struct _D3DTLVERTEX {
    float sx; /* Screen coordinates */
    float sy;
    float sz;
    float rhw; /* Reciprocal of homogeneous w */
    D3DCOLOR color; /* Vertex color */
    float tu; /* Texture coordinates */
    float tv;
    _D3DTLVERTEX() { }
    _D3DTLVERTEX(const D3DVECTOR& v, float _rhw,
                 D3DCOLOR _color, 
                 float _tu, float _tv)
    { sx = v.x; sy = v.y; sz = v.z; rhw = _rhw;
      color = _color; 
      tu = _tu; tv = _tv;
    }
} D3DTLVERTEX, *LPD3DTLVERTEX;

extern "C" 
{
	struct SCR_INFO 
	{
		int	dummy;
	};
};

//struct VERTEX { D3DXVECTOR4 p; D3DCOLOR col; FLOAT tu, tv; };
//static const DWORD FVF_VERTEX = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1;

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)
LPDIRECT3DVERTEXBUFFER8 g_pVertexBuffer = NULL; // Vertices Buffer

struct MYCUSTOMVERTEX
{
    FLOAT x, y, z; // The transformed position for the vertex.
	//FLOAT x, y, z, rhw; // The transformed position for the vertex.
    DWORD colour; // The vertex colour.
};

//

SCR_INFO vInfo;

void LoadSettings();
void SetDefaults();
void hsv_to_rgb(double hue, double saturation, double value, double *red, double *green, double *blue);
void DrawRectangle(int x, int y, int w, int h, D3DCOLOR dwColour);

static  char m_szVisName[1024];
int	m_iWidth;
int m_iHeight;

LPDIRECT3DDEVICE8 m_pd3dDevice;