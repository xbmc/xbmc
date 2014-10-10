/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2007 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/**
 * $Id$
 *
 * Encapsulation of a custom wave
 *
 * $Log$
 */

#ifndef _CUSTOM_WAVE_H
#define _CUSTOM_WAVE_H

#define CUSTOM_WAVE_DEBUG 0

class CustomWave;
class GenExpr;
class PerPointEqn;
class Preset;

#include <vector>

#include "Common.hpp"
#include "Param.hpp"
#include "PerFrameEqn.hpp"

#include <map>

class CustomWave {
public:

     /** Empty constructor leaves wave in undefined state **/
     CustomWave() {}

     /** Initializes a custom wave id given the integer id */
     CustomWave(int id);

    /** Destructor is necessary so we can free the per point matrices **/
    ~CustomWave();

    /* Numerical id */
    int id;
    int per_frame_count;

    /* Parameter tree associated with this custom wave */
    std::map<std::string,Param*> param_tree;

    /* Engine variables */
    float x; /* x position for per point equations */
    float y; /* y position for per point equations */
    float r; /* red color value */
    float g; /* green color value */
    float b; /* blue color value */
    float a; /* alpha color value */
    float * x_mesh;
    float * y_mesh;
    float * r_mesh;
    float * b_mesh;
    float * g_mesh;
    float * a_mesh;
    float * value1;
    float * value2;
    float * sample_mesh;
    
    bool enabled; /* if true then wave is visible, hidden otherwise */
    int samples; /* number of samples associated with this wave form. Usually powers of 2 */
    float sample;
    bool bSpectrum; /* spectrum data or pcm data */
    bool bUseDots; /* draw wave as dots or lines */
    bool bDrawThick; /* draw thicker lines */
    bool bAdditive; /* add color values together */
    
    float scaling; /* scale factor of waveform */
    float smoothing; /* smooth factor of waveform */
    int sep;  /* no idea what this is yet... */
    
    /* stupid t variables */
    float t1;
    float t2;
    float t3;
    float t4;
    float t5;
    float t6;
    float t7;
    float t8;


    /* stupider q variables */
    float q1;
    float q2;
    float q3;
    float q4;
    float q5;
    float q6;
    float q7;
    float q8;

    float v1,v2;

    /* Data structures to hold per frame and per point equations */
    std::map<std::string,InitCond*>  init_cond_tree;
    std::vector<PerFrameEqn*>  per_frame_eqn_tree;
    std::vector<PerPointEqn*>  per_point_eqn_tree;
    std::map<std::string,InitCond*>  per_frame_init_eqn_tree;

    /* Denotes the index of the last character for each string buffer */
    int per_point_eqn_string_index;
    int per_frame_eqn_string_index;
    int per_frame_init_eqn_string_index;

    int add_per_point_eqn(char * name, GenExpr * gen_expr);
    void evalCustomWaveInitConditions(Preset *preset);
    void evalPerPointEqns();

    void loadUnspecInitConds();
    
    void evalInitConds();

};

#endif /** !_CUSTOM_WAVE_H */
