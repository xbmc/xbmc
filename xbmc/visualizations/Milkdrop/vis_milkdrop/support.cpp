/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#include "support.h"
#include "utility.h"
#include "md_defines.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

bool g_bDebugOutput = false;
bool g_bDumpFileCleared = false;

//---------------------------------------------------
void PrepareFor3DDrawing(
        IDirect3DDevice9 *pDevice, 
        int viewport_width,
        int viewport_height,
        float fov_in_degrees, 
        float near_clip,
        float far_clip,
        D3DXVECTOR3* pvEye,
        D3DXVECTOR3* pvLookat,
        D3DXVECTOR3* pvUp
    )
{
#if 0

    // This function sets up DirectX up for 3D rendering.
    // Only call it once per frame, as it is VERY slow.
    // INPUTS:
    //    pDevice           a pointer to the D3D device
    //    viewport_width    the width of the client area of the window
    //    viewport_height   the height of the client area of the window
    //    fov_in_degrees    the field of view, in degrees
    //    near_clip         the distance to the near clip plane; should be > 0!
    //    far_clip          the distance to the far clip plane
    //    eye               the eyepoint coordinates, in world space
    //    lookat            the point toward which the eye is looking, in world space
    //    up                a vector indicating which dir. is up; usually <0,1,0>
    //
    // What this function does NOT do:
    //    1. set the current texture (SetTexture)
    //    2. set up the texture stages for texturing (SetTextureStageState)
    //    3. set the current vertex format (SetVertexShader)
    //    4. set up the world matrix (SetTransform(D3DTS_WORLD, &my_world_matrix))

    
    // set up render state to some nice defaults:
    {
        // some defaults
        d3dSetRenderState( D3DRS_ZENABLE, TRUE );
        d3dSetRenderState( D3DRS_ZWRITEENABLE, TRUE );
        d3dSetRenderState( D3DRS_ZFUNC,     D3DCMP_LESSEQUAL );
        d3dSetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
//        d3dSetRenderState( D3DRS_CLIPPING, TRUE );
        d3dSetRenderState( D3DRS_LIGHTING, FALSE );
        d3dSetRenderState( D3DRS_COLORVERTEX, TRUE );
        d3dSetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
        d3dSetRenderState( D3DRS_FILLMODE,  D3DFILL_SOLID );
        d3dSetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

        // turn fog off
        d3dSetRenderState( D3DRS_FOGENABLE, FALSE );
        d3dSetRenderState( D3DRS_RANGEFOGENABLE, FALSE );
    
        // turn on high-quality bilinear interpolations
        pDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR); 
        pDevice->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
        pDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
        pDevice->SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
    }    

    // set up view & projection matrices (but not the world matrix!)
    {
        // if the window is not square, instead of distorting the scene,
        // clip it so that the longer dimension of the window has the
        // regular FOV, and the shorter dimension has a reduced FOV.
        float fov_x = fov_in_degrees * 3.1415927f/180.0f;
        float fov_y = fov_in_degrees * 3.1415927f/180.0f;
        float aspect = (float)viewport_height / (float)viewport_width;
        if (aspect < 1)
            fov_y *= aspect;
        else
            fov_x /= aspect;
        
        if (near_clip < 0.1f)
            near_clip = 0.1f;
        if (far_clip < near_clip + 1.0f)
            far_clip = near_clip + 1.0f;

        D3DXMATRIX proj;
        MakeProjectionMatrix(&proj, near_clip, far_clip, fov_x, fov_y);
        pDevice->SetTransform(D3DTS_PROJECTION, &proj);
        
        D3DXMATRIX view;
        D3DXMatrixLookAtLH(&view, pvEye, pvLookat, pvUp);
        pDevice->SetTransform(D3DTS_VIEW, &view);

        // Optimization note: "You can minimize the number of required calculations 
        // by concatenating your world and view matrices into a world-view matrix 
        // that you set as the world matrix, and then setting the view matrix 
        // to the identity."
        //D3DXMatrixMultiply(&world, &world, &view);                
        //D3DXMatrixIdentity(&view);
    }
#endif
}

void PrepareFor2DDrawing(IDirect3DDevice9 *pDevice)
{
    // New 2D drawing area will have x,y coords in the range <-1,-1> .. <1,1>
    //         +--------+ Y=-1
    //         |        |
    //         | screen |             Z=0: front of scene
    //         |        |             Z=1: back of scene
    //         +--------+ Y=1
    //       X=-1      X=1
    // NOTE: After calling this, be sure to then call (at least):
    //  1. SetVertexShader()
    //  2. SetTexture(), if you need it
    // before rendering primitives!
    // Also, be sure your sprites have a z coordinate of 0.
    pDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    pDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
    pDevice->SetRenderState( D3DRS_ZFUNC,     D3DCMP_LESSEQUAL );
    pDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );
    pDevice->SetRenderState( D3DRS_FILLMODE,  D3DFILL_SOLID );
    pDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
//    d3dSetRenderState( D3DRS_CLIPPING, TRUE ); 
    pDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
    pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_LOCALVIEWER, FALSE );
    
    pDevice->SetTexture(0, NULL);
    pDevice->SetTexture(1, NULL);
//    SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);//D3DTEXF_LINEAR);
//    SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_POINT);//D3DTEXF_LINEAR);
	  pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	  pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    pDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    pDevice->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);    
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT );
    pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE );

    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
    pDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

    pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    
    // set up for 2D drawing:
    {
        D3DXMATRIX Ortho2D;    
        D3DXMATRIX Identity;
        
        D3DXMatrixOrthoLH(&Ortho2D, 2.0f, -2.0f, 0.0f, 1.0f);
        D3DXMatrixIdentity(&Identity);

        pDevice->SetTransform(D3DTS_PROJECTION, &Ortho2D);
        pDevice->SetTransform(D3DTS_WORLD, &Identity);
        pDevice->SetTransform(D3DTS_VIEW, &Identity);
    }
}

//---------------------------------------------------

void MakeWorldMatrix( D3DXMATRIX* pOut, 
                      float xpos, float ypos, float zpos, 
                      float sx,   float sy,   float sz, 
                      float pitch, float yaw, float roll)
{
#if 0
    /*
     * The m_xPos, m_yPos, m_zPos variables contain the model's
     * location in world coordinates.
     * The m_fPitch, m_fYaw, and m_fRoll variables are floats that 
     * contain the model's orientation in terms of pitch, yaw, and roll
     * angles, in radians.
     */

    D3DXMATRIX MatTemp;
    D3DXMatrixIdentity(pOut);

    // 1. first, rotation
    if (pitch || yaw || roll) 
    {
        D3DXMATRIX MatRot;
        D3DXMatrixIdentity(&MatRot);

        D3DXMatrixRotationX(&MatTemp, pitch);         // Pitch
        D3DXMatrixMultiply(&MatRot, &MatRot, &MatTemp);
        D3DXMatrixRotationY(&MatTemp, yaw);           // Yaw
        D3DXMatrixMultiply(&MatRot, &MatRot, &MatTemp);
        D3DXMatrixRotationZ(&MatTemp, roll);          // Roll
        D3DXMatrixMultiply(&MatRot, &MatRot, &MatTemp);
 
        D3DXMatrixMultiply(pOut, pOut, &MatRot);
    }

    // 2. then, scaling
    D3DXMatrixScaling(&MatTemp, sx, sy, sz);
    D3DXMatrixMultiply(pOut, pOut, &MatTemp);

    // 3. last, translation to final world pos.
    D3DXMatrixTranslation(&MatTemp, xpos, ypos, zpos);
    D3DXMatrixMultiply(pOut, pOut, &MatTemp);
#endif
}

void MakeProjectionMatrix( D3DXMATRIX* pOut,
                           const float near_plane, // Distance to near clipping plane
                           const float far_plane,  // Distance to far clipping plane
                           const float fov_horiz,  // Horizontal field of view angle, in radians
                           const float fov_vert)   // Vertical field of view angle, in radians
{
    float w = (float)1/tanf(fov_horiz*0.5f);  // 1/tan(x) == cot(x)
    float h = (float)1/tanf(fov_vert*0.5f);   // 1/tan(x) == cot(x)
    float Q = far_plane/(far_plane - near_plane);
 
    ZeroMemory(pOut, sizeof(D3DXMATRIX));
    pOut->_11 = w;
    pOut->_22 = h;
    pOut->_33 = Q;
    pOut->_43 = -Q*near_plane;
    pOut->_34 = 1;
}

//---------------------------------------------------

void GetWinampSongTitle(HWND hWndWinamp, char *szSongTitle, int nSize)
{
    szSongTitle[0] = 0;
#if 0
    if (::GetWindowText(hWndWinamp, szSongTitle, nSize))
    {
	    // remove ' - Winamp' if found at end
	    if (strlen(szSongTitle) > 9)
	    {
		    int check_pos = strlen(szSongTitle) - 9;
		    if (strcmp(" - Winamp", (char *)(szSongTitle + check_pos)) == 0)
			    szSongTitle[check_pos] = 0;
	    }

	    // remove ' - Winamp [Paused]' if found at end
	    if (strlen(szSongTitle) > 18)
	    {
		    int check_pos = strlen(szSongTitle) - 18;
		    if (strcmp(" - Winamp [Paused]", (char *)(szSongTitle + check_pos)) == 0)
			    szSongTitle[check_pos] = 0;
	    }

	    // remove song # and period from beginning
	    char *p = szSongTitle;
	    while (*p >= '0' && *p <= '9') p++;
	    if (*p == '.' && *(p+1) == ' ')
	    {
		    p += 2;
		    int pos = 0;
		    while (*p != 0)
		    {
			    szSongTitle[pos++] = *p;
			    p++;
		    }
		    szSongTitle[pos++] = 0;
	    }

	    // fix &'s for display
        // note: this is not necessary if you use the DT_NOPREFIX flag with text drawing functions!
	    /*
	    {
		    int pos = 0;
		    int len = strlen(szSongTitle);
		    while (szSongTitle[pos])
		    {
			    if (szSongTitle[pos] == '&')
			    {
				    for (int x=len; x>=pos; x--)
					    szSongTitle[x+1] = szSongTitle[x];
				    len++;
				    pos++;
			    }
			    pos++;
		    }
	    }*/
    }
#endif
}

void GetWinampSongPosAsText(HWND hWndWinamp, char *szSongPos)
{
    // note: size(szSongPos[]) must be at least 64.

    szSongPos[0] = 0;

#if 0
	int nSongPosMS = SendMessage(hWndWinamp,WM_USER,0,105);
    if (nSongPosMS > 0)
    {
		float time_s = nSongPosMS*0.001f;
		int minutes = (int)(time_s/60);
		time_s -= minutes*60;
		int seconds = (int)time_s;
		time_s -= seconds;
		int dsec = (int)(time_s*100);
		sprintf(szSongPos, "%d:%02d.%02d", minutes, seconds, dsec);
    }

#endif
}

void GetWinampSongLenAsText(HWND hWndWinamp, char *szSongLen)
{
    // note: size(szSongLen[]) must be at least 64.
    szSongLen[0] = 0;
#if 0
	int nSongLenMS = SendMessage(hWndWinamp,WM_USER,1,105)*1000;
    if (nSongLenMS > 0)
    {
		int len_s = nSongLenMS/1000;
		int minutes = len_s/60;
		int seconds = len_s - minutes*60;
		sprintf(szSongLen, "%d:%02d", minutes, seconds);
    }    
#endif
}

float GetWinampSongPos(HWND hWndWinamp)
{
    // returns answer in seconds
//    return (float)SendMessage(hWndWinamp,WM_USER,0,105)*0.001f;
	return 0;
}

float GetWinampSongLen(HWND hWndWinamp)
{
    // returns answer in seconds
//	return (float)SendMessage(hWndWinamp,WM_USER,1,105);
	return 0;
}

/*
void g_dumpmsg(char *s)
{
    if (g_bDebugOutput)
    {
        if (!g_bDumpFileCleared)
        {
            g_bDumpFileCleared = true;
	        FILE *infile = fopen(DEBUGFILE, "w");
            if (infile)
            {
	            fprintf(infile, DEBUGFILEHEADER);
	            fclose(infile);
            }
        }
    
        FILE *infile2;
        infile2 = fopen(DEBUGFILE, "a");
        if (infile2)
        {
	        fprintf(infile2, "%s\n", s);
            OutputDebugString(s);
            OutputDebugString("\n");
	        fclose(infile2);
        }
    }
}

void ClipWindowToScreen(RECT *rect)
{
	int screen_width  = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);

	// make sure it's not too small
	if (rect->right - rect->left < 64)
		rect->right = rect->left + 64;
	if (rect->bottom - rect->top < 64)
		rect->bottom = rect->top + 64;

	// make sure it's not too big
	if (rect->right - rect->left > screen_width)
		rect->right = rect->left + screen_width;
	if (rect->bottom - rect->top > screen_height)
		rect->bottom = rect->top + screen_height;

	// clip vs. screen edges
	if (rect->top    < 0)
	{
		rect->bottom -= rect->top;
		rect->top = 0;
	}

	if (rect->top    > screen_height - 64)
	{
		rect->bottom += (screen_height - 64) - rect->top;
		rect->top    = screen_height - 64;
	}

	if (rect->right < 64)
	{
		rect->left -= rect->right - 64;
		rect->right = 64;
	}

	if (rect->left   > screen_width - 64)
	{
		rect->right += (screen_width - 64) - rect->left;
		rect->left  = screen_width - 64;
	}
}
*/

const unsigned char LC2UC[256] = {
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
	17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,255,
	33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
	49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,
	97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
	113,114,115,116,117,118,119,120,121,122,91,92,93,94,95,96,
	97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
	113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
	129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,
	145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,
	161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,
	177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,
	193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,
	209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,
	225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,
	241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
};

int mystrcmpi(char *s1, char *s2)
{
	// returns  1 if s1 comes before s2
	// returns  0 if equal
	// returns -1 if s1 comes after s2
	// treats all characters/symbols by their ASCII values, 
	//    except that it DOES ignore case.

	int i=0;

	while (LC2UC[s1[i]] == LC2UC[s2[i]] && s1[i] != 0)
		i++;

	//FIX THIS!

	if (s1[i]==0 && s2[i]==0)
		return 0;
	else if (s1[i]==0)
		return -1;
	else if (s2[i]==0)
		return 1;
	else 
		return (LC2UC[s1[i]] < LC2UC[s2[i]]) ? -1 : 1;
}

void SetScrollLock(bool bNewState)
{
#if 0
	if (bNewState != (bool)(GetKeyState(VK_SCROLL) & 1))
	{
		// Simulate a key press
		keybd_event( VK_SCROLL,
					  0x45,
					  KEYEVENTF_EXTENDEDKEY | 0,
					  0 );

		// Simulate a key release
		keybd_event( VK_SCROLL,
					  0x45,
					  KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
					  0);
	}
#endif
}
