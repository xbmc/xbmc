/*   fishBMC visualization plugin
 *   Copyright (C) 2012 Marcel Ebmer

 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifdef __GNUC__
#define __cdecl
#endif

#include "addons/include/xbmc_vis_types.h"
#include "addons/include/xbmc_vis_dll.h"

#include "fische.h"

#include <cmath>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>


// global variables
FISCHE*     g_fische;
double      g_aspect;
bool        g_isrotating;
double      g_angle;
double      g_lastangle;
bool        g_errorstate;
int         g_framedivisor;
double      g_angleincrement;
double      g_texright;
double      g_texleft;
bool        g_filemode;
int         g_size;
uint8_t*    g_axis = 0;





// render functions
// must be included AFTER global variables
#if defined(_WIN32)
#include "fishbmc_directx.hpp"
#else // _WIN32
#include "fishbmc_opengl.hpp"
#endif // _WIN32





void on_beat (double frames_per_beat)
{
    if (!g_isrotating) {
        g_isrotating = true;
        if (frames_per_beat < 1) frames_per_beat = 12;
        g_angleincrement = 180 / 4 / frames_per_beat;
    }
}

void write_vectors (const void* data, size_t bytes)
{
    char const * homedir = getenv ("HOME");
    if (!homedir)
        return;

    std::string dirname = std::string (homedir) + "/.fishBMC-data";
    mkdir (dirname.c_str(), 0755);

    std::ostringstream filename;
    filename << dirname << "/" << g_fische->height;

    // open the file
    std::fstream vectorsfile (filename.str().c_str(), std::fstream::out | std::fstream::binary);
    if (!vectorsfile.good())
        return;

    // write it
    vectorsfile.write (reinterpret_cast<const char*> (data), bytes);
    vectorsfile.close();
}

size_t read_vectors (void** data)
{
    char const * homedir = getenv ("HOME");
    if (!homedir)
        return 0;

    std::string dirname = std::string (homedir) + "/.fishBMC-data";
    mkdir (dirname.c_str(), 0755);

    std::ostringstream filename;
    filename << dirname << "/" << g_fische->height;

    // open the file
    std::fstream vectorsfile (filename.str().c_str(), std::fstream::in);
    if (!vectorsfile.good())
        return 0;

    vectorsfile.seekg (0, std::ios::end);
    size_t n = vectorsfile.tellg();
    vectorsfile.seekg (0, std::ios::beg);

    *data = malloc (n);
    vectorsfile.read (reinterpret_cast<char*> (*data), n);
    vectorsfile.close();

    return n;
}

void delete_vectors()
{
    char const * homedir = getenv ("HOME");
    if (!homedir)
        return;

    std::string dirname = std::string (homedir) + "/.fishBMC-data";
    mkdir (dirname.c_str(), 0755);

    for (int i = 64; i <= 2048; i *= 2) {
        std::ostringstream filename;
        filename << dirname << "/" << i;
        unlink (filename.str().c_str());
    }
}

extern "C" ADDON_STATUS ADDON_Create (void* hdl, void* props)
{
    if (!props)
        return ADDON_STATUS_UNKNOWN;

    VIS_PROPS* visProps = (VIS_PROPS*) props;

    init (visProps);

    g_fische = fische_new();
    g_fische->on_beat = &on_beat;
    g_fische->pixel_format = FISCHE_PIXELFORMAT_0xAABBGGRR;
    g_fische->line_style = FISCHE_LINESTYLE_THICK;
    g_aspect = double (visProps->width) / double (visProps->height);
    g_texleft = (2 - g_aspect) / 4;
    g_texright = 1 - g_texleft;
    g_framedivisor = 1;
    g_filemode = false;
    g_size = 128;

    return ADDON_STATUS_NEED_SETTINGS;
}

extern "C" void Start (int, int, int, const char*)
{
    g_errorstate = false;

    g_fische->audio_format = FISCHE_AUDIOFORMAT_FLOAT;

    g_fische->height = g_size;
    g_fische->width = 2 * g_size;

    if (g_filemode) {
        g_fische->read_vectors = &read_vectors;
        g_fische->write_vectors = &write_vectors;
    }

    else {
        delete_vectors();
    }

    if (fische_start (g_fische) != 0) {
        std::cerr << "fische failed to start" << std::endl;
        g_errorstate = true;
        return;
    }

    uint32_t* pixels = fische_render (g_fische);

    init_texture (g_fische->width, g_fische->height, pixels);

    g_isrotating = false;
    g_angle = 0;
    g_lastangle = 0;
    g_angleincrement = 0;
}

extern "C" void AudioData (const float* pAudioData, int iAudioDataLength, float*, int)
{
    fische_audiodata (g_fische, pAudioData, iAudioDataLength * 4);
}

extern "C" void Render()
{
    static int frame = 0;

    // check if this frame is to be skipped
    if (++ frame % g_framedivisor == 0) {
        uint32_t* pixels = fische_render (g_fische);
        replace_texture (g_fische->width, g_fische->height, pixels);
        if (g_isrotating)
            g_angle += g_angleincrement;
    }

    // stop rotation if required
    if (g_isrotating) {
        if (g_angle - g_lastangle > 180) {
            g_lastangle = g_lastangle ? 0 : 180;
            g_angle = g_lastangle;
            g_isrotating = false;
        }
    }

    // how many quads will there be?
    int n_Y = 8;
    int n_X = (g_aspect * 8 + 0.5);

    // one-time initialization of rotation axis array
    if (!g_axis) {
        g_axis = new uint8_t[n_X * n_Y];
        for (int i = 0; i < n_X * n_Y; ++ i) {
            g_axis[i] = rand() % 2;
        }
    }

    start_render();

    // loop over and draw all quads
    int quad_count = 0;
    double quad_width = 4.0 / n_X;
    double quad_height = 4.0 / n_Y;
    double tex_width = (g_texright - g_texleft);

    for (double X = 0; X < n_X; X += 1) {
        for (double Y = 0; Y < n_Y; Y += 1) {
            double center_x = -2 + (X + 0.5) * 4 / n_X;
            double center_y = -2 + (Y + 0.5) * 4 / n_Y;
            double tex_left = g_texleft + tex_width * X / n_X;
            double tex_right = g_texleft + tex_width * (X + 1) / n_X;
            double tex_top = Y / n_Y;
            double tex_bottom = (Y + 1) / n_Y;
            double angle = (g_angle - g_lastangle) * 4 - (X + Y * n_X) / (n_X * n_Y) * 360;
            if (angle < 0) angle = 0;
            if (angle > 360) angle = 360;

            textured_quad (center_x,
                              center_y,
                              angle,
                              g_axis[quad_count ++],
                              quad_width,
                              quad_height,
                              tex_left,
                              tex_right,
                              tex_top,
                              tex_bottom);
        }
    }

    finish_render();
}

extern "C" void GetInfo (VIS_INFO* pInfo)
{
    // std::cerr << "fishBMC::GetInfo" << std::endl;
    pInfo->bWantsFreq = false;
    pInfo->iSyncDelay = 0;
}

extern "C" bool OnAction (long flags, const void *param)
{
    return false;
}

extern "C" unsigned int GetPresets (char ***presets)
{
    return 0;
}

extern "C" unsigned GetPreset()
{
    return 0;
}

extern "C" bool IsLocked()
{
    return false;
}

extern "C" unsigned int GetSubModules (char ***names)
{
    return 0;
}

extern "C" void ADDON_Stop()
{
    fische_free (g_fische);
    g_fische = 0;
    delete_texture();
    delete [] g_axis;
    g_axis = 0;
}

extern "C" void ADDON_Destroy()
{
    return;
}

extern "C" bool ADDON_HasSettings()
{
    return false;
}

extern "C" ADDON_STATUS ADDON_GetStatus()
{
    if (g_errorstate)
        return ADDON_STATUS_UNKNOWN;

    return ADDON_STATUS_OK;
}

extern "C" unsigned int ADDON_GetSettings (ADDON_StructSetting ***sSet)
{
    return 0;
}

extern "C" void ADDON_FreeSettings()
{
}

extern "C" ADDON_STATUS ADDON_SetSetting (const char *strSetting, const void* value)
{
    if (!strSetting || !value)
        return ADDON_STATUS_UNKNOWN;

    if (!strncmp (strSetting, "nervous", 7)) {
        bool nervous = * ( (bool*) value);
        g_fische->nervous_mode = nervous ? 1 : 0;
    }

    else if (!strncmp (strSetting, "filemode", 7)) {
        bool filemode = * ( (bool*) value);
        g_filemode = filemode;
    }

    else if (!strncmp (strSetting, "detail", 6)) {
        int detail = * ( (int*) value);
        g_size = 128;
        while (detail--) {
            g_size *= 2;
        }
    }

    else if (!strncmp (strSetting, "divisor", 7)) {
        int divisor = * ( (int*) value);
        g_framedivisor = 8;
        while (divisor--) {
            g_framedivisor /= 2;
        }
    }

    return ADDON_STATUS_OK;
}

extern "C" void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}
