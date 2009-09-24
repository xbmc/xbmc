/*
 * BioGenesis Screensaver for XBox Media Center
 * Copyright (c) 2004 Team XBMC
 *
 * Ver 1.0 2007-02-12 by Asteron  http://asteron.projects.googlepages.com/home
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2  of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "Life.h"
#include "XmlDocument.h"
// use the 'dummy' dx8 lib - this allow you to make
// DX8 calls which XBMC will emulate for you.
#pragma comment (lib, "lib/xbox_dx8.lib" )

struct Cell
{
  DWORD color; // The cell color.
  short lifetime;
  BYTE nextstate, state; 
};


#define DEAD 0
#define ALIVE 1
#define COLOR_TIME 0
#define COLOR_COLONY 1
#define COLOR_NEIGHBORS 2

struct Grid
{
  int minSize;
  int maxSize;
  int width;
  int height;
  int spacing;
  int resetTime;
  int cellSizeX;
  int cellSizeY;
  int colorType;
  int ruleset;
  int frameCounter;
  int maxColor;
  int presetChance;
  int allowedColoring;
  int cellLineLimit;
  DWORD palette[800];
  Cell * cells;
  Cell * fullGrid;
};

Grid grid;
int PALETTE_SIZE = sizeof(grid.palette)/sizeof(DWORD);
DWORD COLOR_TIMES[] = {  
  D3DCOLOR_RGBA(30,30,200,255),
  D3DCOLOR_RGBA(120,10,255,255),
  D3DCOLOR_RGBA(50,100,250,255),
  D3DCOLOR_RGBA(0,250,200,255),
  D3DCOLOR_RGBA(60,250,40,255),
  D3DCOLOR_RGBA(244,200,40,255),
  D3DCOLOR_RGBA(250,100,30,255),
  D3DCOLOR_RGBA(255,10,20,255)
  };
int MAX_COLOR = sizeof(COLOR_TIMES)/sizeof(DWORD);

LPDIRECT3DVERTEXBUFFER8 g_pVertexBuffer = NULL; // Vertices Buffer


float frand(){return ((float) rand() / (float) RAND_MAX);}

DWORD randColor()
{
  float h=(float)(rand()%360), s = 0.3f + 0.7f*frand(), v=0.67f+0.25f*frand();
  if (grid.colorType == COLOR_NEIGHBORS || grid.colorType == COLOR_TIME)
  {
    s = 0.9f + 0.1f*frand();
    //v = 0.5f + 0.3f*frand();
  }
  return HSVtoRGB(h,s,v);
}

void SeedGrid()
{
  memset(grid.cells,0, grid.width*grid.height*sizeof(Cell));
  for ( int i = 0; i<grid.width*grid.height; i++ )
  {
    grid.cells[i].lifetime = 0;
    if (rand() % 4 == 0)
    {
      grid.cells[i].state = ALIVE;
      grid.cells[i].nextstate = grid.cells[i].state;
      if (grid.colorType == COLOR_TIME)
        grid.cells[i].color = grid.palette[grid.cells[i].lifetime];
      else
      {
        grid.cells[i].color = randColor();
      }
    }
  }
}


void presetPalette()
{
  grid.palette[11] = 0xFF2222FF; //block

  grid.palette[2]  = 0xFFFF0066;  //blinker
  grid.palette[24] = 0xFFFF33FF;  //inside

  grid.palette[12] = 0xFFAA00FF; //behive
  
  grid.palette[36] = 0xFF008800; //dot
  grid.palette[5]  = 0xFFDDDD00; //cross

  grid.palette[10]  = 0xFFAA0000; //ship
  grid.palette[13]  = 0xFF0099CC; //ship

}

void CreateGrid()
{
  int i, cellmin, cellmax;
  cellmin = (int)sqrt((float)(g_iWidth*g_iHeight/(int)(grid.maxSize*grid.maxSize*g_fRatio)));
  cellmax = (int)sqrt((float)(g_iWidth*g_iHeight/(int)(grid.minSize*grid.minSize*g_fRatio)));
  grid.cellSizeX = rand()%(cellmax - cellmin + 1) + cellmin;
  grid.cellSizeY = grid.cellSizeX > 5 ? (int)(g_fRatio * grid.cellSizeX) : grid.cellSizeX;
  grid.width = g_iWidth/grid.cellSizeX;
  grid.height = g_iHeight/grid.cellSizeY;

  if (grid.cellSizeX <= grid.cellLineLimit )
    grid.spacing = 0;
  else grid.spacing = 1;


  if (grid.fullGrid)
    delete grid.fullGrid;
  grid.fullGrid = new Cell[grid.width*(grid.height+2)+2]; 
  memset(grid.fullGrid,0, (grid.width*(grid.height+2)+2) * sizeof(Cell));
  grid.cells = &grid.fullGrid[grid.width + 1];
  grid.frameCounter = 0;
  do
  {
    grid.colorType = rand()%3;
  } while (!(grid.allowedColoring & (1 << grid.colorType)) && grid.allowedColoring != 0);
  grid.ruleset = 0;

  for (i=0; i< PALETTE_SIZE; i++)
    grid.palette[i] = randColor();

  grid.maxColor = MAX_COLOR;
  if (grid.colorType == COLOR_TIME && (rand()%100 < grid.presetChance))
    for (i=0; i< MAX_COLOR; i++)
      grid.palette[i] = COLOR_TIMES[i];
  else
    grid.maxColor += (rand()%2)*(rand()%60);  //make it shimmer sometimes
  if (grid.colorType == COLOR_TIME && rand()%3)
  {
    for (i=grid.maxColor-1; i<PALETTE_SIZE; i++)
      grid.palette[i] = LerpColor(grid.palette[grid.maxColor-1],grid.palette[PALETTE_SIZE-1],(float)(i-grid.maxColor+1)/(float)(PALETTE_SIZE-grid.maxColor));
    grid.maxColor = PALETTE_SIZE;
  }
  if (grid.colorType == COLOR_NEIGHBORS)
  {
    if (rand()%100 < grid.presetChance)
      presetPalette();
    reducePalette();
  }  
  SeedGrid();
}



int * rotateBits(int * bits)
{
  int temp;
  temp = bits[0];
  bits[0] = bits[2];
  bits[2] = bits[7];
  bits[7] = bits[5];
  bits[5] = temp;  
  temp = bits[1];
  bits[1] = bits[4];
  bits[4] = bits[6];
  bits[6] = bits[3];
  bits[3] = temp;
  return bits;
}
int * flipBits(int * bits)
{
  int temp;
  temp = bits[0];
  bits[0] = bits[2];
  bits[2] = temp;
  temp = bits[3];
  bits[3] = bits[4];
  bits[4] = temp;
  temp = bits[5];
  bits[5] = bits[7];
  bits[7] = temp;
  return bits;
}
int packBits(int * bits)
{
  int packed = 0;
  for(int j = 0; j<8; j++)
    packed |= bits[j] << j;
  return packed;
}
void unpackBits(int num, int * bits)
{  
  for(int i=0; i<8; i++)
    bits[i] = (num & (1<<i))>>i ;
}
// This simplifies the neighbor palette based off of symmetry
void reducePalette()
{
  int i = 0, bits[8], inf, temp;
  for(i = 0; i < 256; i++)
  {
    inf = i;
    unpackBits(i, bits);
    for(int k = 0; k < 2; k++)
    {
      for(int j = 0; j<4; j++)
        if ((temp = packBits(rotateBits(bits))) < inf)
          inf = temp;
      flipBits(bits);
    }
    grid.palette[i] = grid.palette[inf];
  }
}



void DrawGrid()
{
  for(int i = 0; i<grid.width*grid.height; i++ )
    if (grid.cells[i].state != DEAD)
      DrawRectangle((i%grid.width)*grid.cellSizeX,(i/grid.width)*grid.cellSizeY, 
        grid.cellSizeX - grid.spacing, grid.cellSizeY - grid.spacing, grid.cells[i].color);
}

void UpdateStates()
{
  for(int i = 0; i<grid.width*grid.height; i++ )
    grid.cells[i].state = grid.cells[i].nextstate;
}

void StepLifetime()
{
  int i;
  for(i = 0; i<grid.width*grid.height; i++ )
  {
    int count = 0;
    if(grid.cells[i-grid.width-1].state) count++;
    if(grid.cells[i-grid.width  ].state) count++;
    if(grid.cells[i-grid.width+1].state) count++;
    if(grid.cells[i           -1].state) count++;
    if(grid.cells[i           +1].state) count++;
    if(grid.cells[i+grid.width-1].state) count++;
    if(grid.cells[i+grid.width  ].state) count++;
    if(grid.cells[i+grid.width+1].state) count++;

    if(grid.cells[i].state == DEAD)
    {
      grid.cells[i].lifetime = 0;
      if (count == 3 || (grid.ruleset && count == 6))
      {
        grid.cells[i].nextstate = ALIVE;
        grid.cells[i].color = grid.palette[0];
      }
    }
    else 
    {
      if (count == 2 || count == 3)
      {
        grid.cells[i].lifetime++;
        if (grid.cells[i].lifetime >= grid.maxColor)
          grid.cells[i].lifetime = grid.maxColor - 1;
        grid.cells[i].color = grid.palette[grid.cells[i].lifetime];
      }
      else
        grid.cells[i].nextstate = DEAD;
    }
  }
  UpdateStates();
}

void StepNeighbors()
{
  UpdateStates();
  int i;
  for(i = 0; i<grid.width*grid.height; i++ )
  {
    int count = 0;
    int neighbors = 0;
    if(grid.cells[i-grid.width-1].state) {count++; neighbors |=  1;}
    if(grid.cells[i-grid.width  ].state) {count++; neighbors |=  2;}
    if(grid.cells[i-grid.width+1].state) {count++; neighbors |=  4;}
    if(grid.cells[i           -1].state) {count++; neighbors |=  8;}
    if(grid.cells[i           +1].state) {count++; neighbors |= 16;}
    if(grid.cells[i+grid.width-1].state) {count++; neighbors |= 32;}
    if(grid.cells[i+grid.width  ].state) {count++; neighbors |= 64;}
    if(grid.cells[i+grid.width+1].state) {count++; neighbors |=128;}

    if(grid.cells[i].state == DEAD)
    {
      if (count == 3 || (grid.ruleset && (neighbors == 0x7E || neighbors == 0xDB)))
      {
        grid.cells[i].nextstate = ALIVE;
        grid.cells[i].color = grid.palette[neighbors];
      }
    }
    else 
    {
      if (count != 2 && count != 3)
        grid.cells[i].nextstate = DEAD;
      grid.cells[i].color = grid.palette[neighbors];
    }
  }
}


void StepColony()
{
  D3DCOLOR foundColors[8];
  int i;
  for(i = 0; i<grid.width*grid.height; i++ )
  {
    int count = 0;
    if(grid.cells[i-grid.width-1].state) foundColors[count++] = grid.cells[i-grid.width-1].color;
    if(grid.cells[i-grid.width  ].state) foundColors[count++] = grid.cells[i-grid.width  ].color;
    if(grid.cells[i-grid.width+1].state) foundColors[count++] = grid.cells[i-grid.width+1].color;
    if(grid.cells[i           -1].state) foundColors[count++] = grid.cells[i           -1].color;
    if(grid.cells[i           +1].state) foundColors[count++] = grid.cells[i           +1].color;
    if(grid.cells[i+grid.width-1].state) foundColors[count++] = grid.cells[i+grid.width-1].color;
    if(grid.cells[i+grid.width  ].state) foundColors[count++] = grid.cells[i+grid.width  ].color;
    if(grid.cells[i+grid.width+1].state) foundColors[count++] = grid.cells[i+grid.width+1].color;

    if(grid.cells[i].state == DEAD)
    {
      if (count == 3 || (grid.ruleset && count == 6))
      {
        if (foundColors[0] == foundColors[2])
          grid.cells[i].color = foundColors[0];
        else 
          grid.cells[i].color = foundColors[1];
        grid.cells[i].nextstate = ALIVE;
      }
    }
    else if (count != 2 && count != 3)
      grid.cells[i].nextstate = DEAD;
    
  }
  UpdateStates();
}

void Step()
{
  switch(grid.colorType)
  {
    case COLOR_COLONY:    StepColony(); break;
    case COLOR_TIME:    StepLifetime(); break;
    case COLOR_NEIGHBORS:  StepNeighbors(); break;
  }
}

D3DCOLOR HSVtoRGB( float h, float s, float v )
{
  int i;
  float f;
  int r, g, b, p, q, t, m;

  if( s == 0 ) { // achromatic (grey)
    r = g = b = (int)(255*v);
    return D3DCOLOR_RGBA(r,g,b,255);
  }

  h /= 60;      // sector 0 to 5
  i = (int)( h );
  f = h - i;      // frational part of h
  m = (int)(255*v);
  p = (int)(m * ( 1 - s ));
  q = (int)(m * ( 1 - s * f ));
  t = (int)(m * ( 1 - s * ( 1 - f ) ));
  

  switch( i ) {
    case 0: return D3DCOLOR_RGBA(m,t,p,255);
    case 1: return D3DCOLOR_RGBA(q,m,p,255);
    case 2: return D3DCOLOR_RGBA(p,m,t,255);
    case 3: return D3DCOLOR_RGBA(p,q,m,255);
    case 4: return D3DCOLOR_RGBA(t,p,m,255);
    default: break;    // case 5:
  }
  return D3DCOLOR_RGBA(m,p,q,255);
}


void DrawRectangle(int x, int y, int w, int h, D3DCOLOR dwColour)
{
  VOID* pVertices;
    //Store each point of the triangle together with it's colour
    CUSTOMVERTEX cvVertices[] =
    {
        {(float) x, (float) y+h, 0.0f, 0.5, dwColour,},
        {(float) x, (float) y, 0.0f, 0.5, dwColour,},
    {(float) x+w, (float) y+h, 0.0f, 0.5, dwColour,},
        {(float) x+w, (float) y, 0.0f, 0.5, dwColour,},
    };



    //Get a pointer to the vertex buffer vertices and lock the vertex buffer
  g_pVertexBuffer->Lock(0, 4*sizeof(CUSTOMVERTEX), (BYTE**)&pVertices, 0);

    //Copy our stored vertices values into the vertex buffer
    memcpy(pVertices, cvVertices, sizeof(cvVertices));

    //Unlock the vertex buffer
    g_pVertexBuffer->Unlock();

  // Draw it
	g_pd3dDevice->SetVertexShader(D3DFVF_CUSTOMVERTEX);
  g_pd3dDevice->SetStreamSource(0, g_pVertexBuffer, sizeof(CUSTOMVERTEX));
  g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

  // Every time we Create a vertex buffer, we must release one!.

  return;
}

// XBMC has loaded us into memory,
// we should set our core values
// here and load any settings we
// may have from our config file
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreenSaverName, float ratio)
{
  strcpy(g_szScrName,szScreenSaverName);
  g_pd3dDevice = pd3dDevice;
  g_iWidth = iWidth;
  g_iHeight = iHeight;
  g_fRatio = ratio;
  if (g_fRatio < 0.1f) // backwards compatible
    g_fRatio = 1.0f;
  // Load the settings
  //Create the vertex buffer from our device
    g_pd3dDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX),
                                               D3DUSAGE_WRITEONLY, 
                         D3DFVF_CUSTOMVERTEX,
                                               D3DPOOL_MANAGED, 
                         &g_pVertexBuffer);
  g_pd3dDevice->SetVertexShader(D3DFVF_CUSTOMVERTEX);
    g_pd3dDevice->SetStreamSource(0, g_pVertexBuffer, sizeof(CUSTOMVERTEX));

  memset(&grid,0, sizeof(Grid));

  LoadSettings();
  CreateGrid();
  
}



// XBMC tells us we should get ready
// to start rendering. This function
// is called once when the screensaver
// is activated by XBMC.
extern "C" void Start()
{
  SeedGrid();
  return;
}

// XBMC tells us to render a frame of
// our screensaver. This is called on
// each frame render in XBMC, you should
// render a single frame only - the DX
// device will already have been cleared.
extern "C" void Render()
{
  D3DXMATRIX m_World;
  D3DXMatrixIdentity( &m_World );
	d3dSetTransform(D3DTS_WORLD, &m_World);

  if (grid.frameCounter++ == grid.resetTime)
    CreateGrid();
  Step();
  DrawGrid();
}

// XBMC tells us to stop the screensaver
// we should free any memory and release
// any resources we have created.
extern "C" void Stop()
{
  g_pVertexBuffer->Release();
  delete grid.fullGrid;
  return;
}

void SetDefaults()
{
  grid.minSize = 50;
  grid.maxSize = 250;
  grid.spacing = 1;
  grid.resetTime = 2000;
  grid.presetChance = 30;
  grid.allowedColoring = 7;
  grid.cellLineLimit = 3;
}

// Load settings from the [screensavername].xml configuration file
// the name of the screensaver (filename) is used as the name of
// the xml file - this is sent to us by XBMC when the Init func
// is called.
void LoadSettings()
{
  XmlNode node, childNode;
  CXmlDocument doc;
  
  // Set up the defaults
  SetDefaults();

  char szXMLFile[1024];
  strcpy(szXMLFile, "Q:\\screensavers\\");
  strcat(szXMLFile, g_szScrName);
  strcat(szXMLFile, ".xml");

  OutputDebugString("Loading XML: ");
  OutputDebugString(szXMLFile);

  // Load the config file
  if (doc.Load(szXMLFile) >= 0)
  {
    node = doc.GetNextNode(XML_ROOT_NODE);
    while(node > 0)
    {
      if (strcmpi(doc.GetNodeTag(node),"screensaver"))
      {
        node = doc.GetNextNode(node);
        continue;
      }
      if (childNode = doc.GetChildNode(node,"mingridsize")){
        grid.minSize = atoi(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"maxgridsize")){
        grid.maxSize = atoi(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"resettime")){
        grid.resetTime = atoi(doc.GetNodeText(childNode)); 
      }
      if (childNode = doc.GetChildNode(node,"presetchance")){
        grid.presetChance = atoi(doc.GetNodeText(childNode)); 
      }
      if (childNode = doc.GetChildNode(node,"gridlinesminsize")){
        grid.cellLineLimit = atoi(doc.GetNodeText(childNode)); 
      }
      if (childNode = doc.GetChildNode(node,"usecolonycoloring"))
        if(strcmpi(doc.GetNodeText(childNode),"true"))
          grid.allowedColoring ^= (1 << COLOR_COLONY);
      if (childNode = doc.GetChildNode(node,"uselifetimecoloring"))
        if(strcmpi(doc.GetNodeText(childNode),"true"))
          grid.allowedColoring ^= (1 << COLOR_TIME);
      if (childNode = doc.GetChildNode(node,"useneighborcoloring"))
        if(strcmpi(doc.GetNodeText(childNode),"true"))
          grid.allowedColoring ^= (1 << COLOR_NEIGHBORS);

      
      node = doc.GetNextNode(node);
    }
    doc.Close();
  }
}


extern "C" void GetInfo(SCR_INFO* pInfo)
{
  // not used, but can be used to pass info
  // back to XBMC if required in the future
  return;
}

extern "C" 
{

  struct ScreenSaver
  {
  public:
    void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float ratio);
    void (__cdecl* Start) ();
    void (__cdecl* Render) ();
    void (__cdecl* Stop) ();
    void (__cdecl* GetInfo)(SCR_INFO *info);
  } ;


  void __declspec(dllexport) get_module(struct ScreenSaver* pScr)
  {
    pScr->Create = Create;
    pScr->Start = Start;
    pScr->Render = Render;
    pScr->Stop = Stop;
    pScr->GetInfo = GetInfo;
  }
};