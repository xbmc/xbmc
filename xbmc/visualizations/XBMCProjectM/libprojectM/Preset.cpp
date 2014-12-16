/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef WIN32
#include "win32-dirent.h"
#else
#include <dirent.h>
#endif /** WIN32 */
#include <time.h>

#include "Preset.hpp"
#include "Parser.hpp"
#include "ParamUtils.hpp"
#include "InitCondUtils.hpp"
#include "fatal.h"
#include <iostream>
#include <sstream>

Preset::Preset(std::istream & in, const std::string & presetName, PresetInputs & presetInputs, PresetOutputs & presetOutputs):
    builtinParams(presetInputs, presetOutputs),
    m_presetName(presetName),
    m_presetOutputs(presetOutputs),
    m_presetInputs(presetInputs)
{

  m_presetOutputs.customWaves.clear();
  m_presetOutputs.customShapes.clear();

  initialize(in);

}


Preset::Preset(const std::string & absoluteFilePath, const std::string & presetName,  PresetInputs & presetInputs, PresetOutputs & presetOutputs):
    builtinParams(presetInputs, presetOutputs),
    m_absoluteFilePath(absoluteFilePath),
    m_presetName(presetName),
    m_presetOutputs(presetOutputs),
    m_presetInputs(presetInputs)
{

  m_presetOutputs.customWaves.clear();
  m_presetOutputs.customShapes.clear();
  
  initialize(absoluteFilePath);

}

Preset::~Preset()
{

  Algorithms::traverse<Algorithms::TraverseFunctors::DeleteFunctor<InitCond> >(init_cond_tree);

  Algorithms::traverse<Algorithms::TraverseFunctors::DeleteFunctor<InitCond> >(per_frame_init_eqn_tree);

  Algorithms::traverse<Algorithms::TraverseFunctors::DeleteFunctor<PerPixelEqn> >(per_pixel_eqn_tree);

  Algorithms::traverseVector<Algorithms::TraverseFunctors::DeleteFunctor<PerFrameEqn> >(per_frame_eqn_tree);

  Algorithms::traverse<Algorithms::TraverseFunctors::DeleteFunctor<Param> >(user_param_tree);

  for (PresetOutputs::cwave_container::iterator pos = customWaves.begin(); pos != customWaves.end(); ++pos)
  {
    	delete(*pos);
  }

  for (PresetOutputs::cshape_container::iterator pos = customShapes.begin(); pos != customShapes.end(); ++pos)
  {
    	delete(*pos);
  }

}

/* Adds a per pixel equation according to its string name. This
   will be used only by the parser */

int Preset::add_per_pixel_eqn(char * name, GenExpr * gen_expr)
{

  PerPixelEqn * per_pixel_eqn = NULL;
  int index;
  Param * param = NULL;

  assert(gen_expr);
  assert(name);

  if (PER_PIXEL_EQN_DEBUG) printf("add_per_pixel_eqn: per pixel equation (name = \"%s\")\n", name);
 

  /* Search for the parameter so we know what matrix the per pixel equation is referencing */

  param = ParamUtils::find(name, &this->builtinParams, &this->user_param_tree);
  if ( !param )
  {
    if (PER_PIXEL_EQN_DEBUG) printf("add_per_pixel_eqn: failed to allocate a new parameter!\n");
    return PROJECTM_FAILURE;
  }

  index = per_pixel_eqn_tree.size();

  /* Create the per pixel equation given the index, parameter, and general expression */
  if ((per_pixel_eqn = new PerPixelEqn(index, param, gen_expr)) == NULL)
  {
    if (PER_PIXEL_EQN_DEBUG) printf("add_per_pixel_eqn: failed to create new per pixel equation!\n");
    return PROJECTM_FAILURE;
  }




  /* Insert the per pixel equation into the preset per pixel database */
  std::pair<std::map<int, PerPixelEqn*>::iterator, bool> inserteeOption = per_pixel_eqn_tree.insert
      (std::make_pair(per_pixel_eqn->index, per_pixel_eqn));

  if (!inserteeOption.second)
  {
    printf("failed to add per pixel eqn!\n");
    delete(per_pixel_eqn);
    return PROJECTM_FAILURE;
  }

  /* Done */
  return PROJECTM_SUCCESS;
}

void Preset::evalCustomShapeInitConditions()
{

  for (PresetOutputs::cshape_container::iterator pos = customShapes.begin(); pos != customShapes.end(); ++pos) {
    assert(*pos);
    (*pos)->evalInitConds();
  }
}


void Preset::evalCustomWaveInitConditions()
{

  for (PresetOutputs::cwave_container::iterator pos = customWaves.begin(); pos != customWaves.end(); ++pos) {
    assert(*pos);
   (*pos)->evalInitConds();
}
}


void Preset::evalCustomWavePerFrameEquations()
{

  for (PresetOutputs::cwave_container::iterator pos = customWaves.begin(); pos != customWaves.end(); ++pos)
  {

    std::map<std::string, InitCond*> & init_cond_tree = (*pos)->init_cond_tree;
    for (std::map<std::string, InitCond*>::iterator _pos = init_cond_tree.begin(); _pos != init_cond_tree.end(); ++_pos)
    {
      assert(_pos->second);
      _pos->second->evaluate();
    }

    std::vector<PerFrameEqn*> & per_frame_eqn_tree = (*pos)->per_frame_eqn_tree;
    for (std::vector<PerFrameEqn*>::iterator _pos = per_frame_eqn_tree.begin(); _pos != per_frame_eqn_tree.end(); ++_pos)
    {
      (*_pos)->evaluate();
    }
  }

}

void Preset::evalCustomShapePerFrameEquations()
{

  for (PresetOutputs::cshape_container::iterator pos = customShapes.begin(); pos != customShapes.end(); ++pos)
  {

    std::map<std::string, InitCond*> & init_cond_tree = (*pos)->init_cond_tree;
    for (std::map<std::string, InitCond*>::iterator _pos = init_cond_tree.begin(); _pos != init_cond_tree.end(); ++_pos)
    {
      assert(_pos->second);
      _pos->second->evaluate();
    }

    std::vector<PerFrameEqn*> & per_frame_eqn_tree = (*pos)->per_frame_eqn_tree;
    for (std::vector<PerFrameEqn*>::iterator _pos = per_frame_eqn_tree.begin(); _pos != per_frame_eqn_tree.end(); ++_pos)
    {
      (*_pos)->evaluate();
    }
  }

}

void Preset::evalPerFrameInitEquations()
{

  for (std::map<std::string, InitCond*>::iterator pos = per_frame_init_eqn_tree.begin(); pos != per_frame_init_eqn_tree.end(); ++pos)
  {
    assert(pos->second);
    pos->second->evaluate();
  }

}

void Preset::evalPerFrameEquations()
{

  for (std::map<std::string, InitCond*>::iterator pos = init_cond_tree.begin(); pos != init_cond_tree.end(); ++pos)
  {
    assert(pos->second);
    pos->second->evaluate();
  }

  for (std::vector<PerFrameEqn*>::iterator pos = per_frame_eqn_tree.begin(); pos != per_frame_eqn_tree.end(); ++pos)
  {
    (*pos)->evaluate();
  }

}

void Preset::preloadInitialize() {
 
  /// @note commented this out because it should be unnecessary
  // Clear equation trees
  //init_cond_tree.clear();
  //user_param_tree.clear();
  //per_frame_eqn_tree.clear();
  //per_pixel_eqn_tree.clear();
  //per_frame_init_eqn_tree.clear();


}

void Preset::postloadInitialize() {

  /* It's kind of ugly to reset these values here. Should definitely be placed in the parser somewhere */
  this->per_frame_eqn_count = 0;
  this->per_frame_init_eqn_count = 0;

  this->loadBuiltinParamsUnspecInitConds();
  this->loadCustomWaveUnspecInitConds();
  this->loadCustomShapeUnspecInitConds();


/// @bug are you handling all the q variables conditions? in particular, the un-init case?
//m_presetOutputs.q1 = 0;
//m_presetOutputs.q2 = 0;
//m_presetOutputs.q3 = 0;
//m_presetOutputs.q4 = 0;
//m_presetOutputs.q5 = 0;
//m_presetOutputs.q6 = 0;
//m_presetOutputs.q7 = 0;
//m_presetOutputs.q8 = 0;

}

void Preset::initialize(const std::string & pathname)
{
  int retval;

  preloadInitialize();

if (PRESET_DEBUG)
  std::cerr << "[Preset] loading file \"" << pathname << "\"..." << std::endl;

  if ((retval = loadPresetFile(pathname)) < 0)
  {
if (PRESET_DEBUG)
     std::cerr << "[Preset] failed to load file \"" <<
      pathname << "\"!" << std::endl;

    /// @bug how should we handle this problem? a well define exception?
    throw retval;
  }

  postloadInitialize();
}

void Preset::initialize(std::istream & in)
{
  int retval;

  preloadInitialize();

  if ((retval = readIn(in)) < 0)
  {

	if (PRESET_DEBUG)
     std::cerr << "[Preset] failed to load from stream " << std::endl; 

    /// @bug how should we handle this problem? a well define exception?
    throw retval;
  }

  postloadInitialize();
}

void Preset::loadBuiltinParamsUnspecInitConds() {

  InitCondUtils::LoadUnspecInitCond loadUnspecInitCond(this->init_cond_tree, this->per_frame_init_eqn_tree);

  this->builtinParams.traverse(loadUnspecInitCond);
  Algorithms::traverse(user_param_tree, loadUnspecInitCond);

}

void Preset::loadCustomWaveUnspecInitConds()
{


  for (PresetOutputs::cwave_container::iterator pos = customWaves.begin(); pos != customWaves.end(); ++pos)
  {
    assert(*pos);
    (*pos)->loadUnspecInitConds();
  }

}

void Preset::loadCustomShapeUnspecInitConds()
{

  for (PresetOutputs::cshape_container::iterator pos = customShapes.begin(); pos != customShapes.end(); ++pos)
  {
    assert(*pos);
    (*pos)->loadUnspecInitConds();
  }
}


void Preset::evaluateFrame()
{

  // Evaluate all equation objects according to milkdrop flow diagram 

  evalPerFrameInitEquations();
  
  evalPerFrameEquations();


  // Important step to ensure custom shapes and waves don't stamp on the q variable values 
  // calculated by the per frame (init) and per pixel equations.
  transfer_q_variables(customWaves);
  transfer_q_variables(customShapes);

  initialize_PerPixelMeshes();

  evalPerPixelEqns();

  evalCustomWaveInitConditions();
  evalCustomWavePerFrameEquations();

  evalCustomShapeInitConditions();
  evalCustomShapePerFrameEquations();

  // Setup pointers of the custom waves and shapes to the preset outputs instance
  /// @slow an extra O(N) per frame, could do this during eval
  m_presetOutputs.customWaves = PresetOutputs::cwave_container(customWaves); 
  m_presetOutputs.customShapes = PresetOutputs::cshape_container(customShapes);

}

void Preset::initialize_PerPixelMeshes()
{

  int x,y;
      for (x=0;x<m_presetInputs.gx;x++){       
	for(y=0;y<m_presetInputs.gy;y++){
	  m_presetOutputs.cx_mesh[x][y]=m_presetOutputs.cx;
	}}
	
     
   

      for (x=0;x<m_presetInputs.gx;x++){
	for(y=0;y<m_presetInputs.gy;y++){
	  m_presetOutputs.cy_mesh[x][y]=m_presetOutputs.cy;
	}}
    
  
 
      for (x=0;x<m_presetInputs.gx;x++){
	for(y=0;y<m_presetInputs.gy;y++){
	  m_presetOutputs.sx_mesh[x][y]=m_presetOutputs.sx;
	}}
    
  

    
      for (x=0;x<m_presetInputs.gx;x++){
	for(y=0;y<m_presetInputs.gy;y++){
	  m_presetOutputs.sy_mesh[x][y]=m_presetOutputs.sy;
	}}
    

     
      for (x=0;x<m_presetInputs.gx;x++){
	for(y=0;y<m_presetInputs.gy;y++){
	  m_presetOutputs.dx_mesh[x][y]=m_presetOutputs.dx;
	}}
    
  
     
      for (x=0;x<m_presetInputs.gx;x++){
	for(y=0;y<m_presetInputs.gy;y++){
	  m_presetOutputs.dy_mesh[x][y]=m_presetOutputs.dy;
	}}
    

    
      for (x=0;x<m_presetInputs.gx;x++){
	for(y=0;y<m_presetInputs.gy;y++){
	  m_presetOutputs.zoom_mesh[x][y]=m_presetOutputs.zoom;
	}}
    
 

    
      for (x=0;x<m_presetInputs.gx;x++){
	for(y=0;y<m_presetInputs.gy;y++){
	  m_presetOutputs.zoomexp_mesh[x][y]=m_presetOutputs.zoomexp; 
	}}
    

  
      for (x=0;x<m_presetInputs.gx;x++){
	for(y=0;y<m_presetInputs.gy;y++){
	  m_presetOutputs.rot_mesh[x][y]=m_presetOutputs.rot;
	}}
    

      for (x=0;x<m_presetInputs.gx;x++){
	for(y=0;y<m_presetInputs.gy;y++){
	  m_presetOutputs.warp_mesh[x][y]=m_presetOutputs.warp;
	}}
    


}
// Evaluates all per-pixel equations 
void Preset::evalPerPixelEqns()
{

  /* Evaluate all per pixel equations in the tree datastructure */
  for (int mesh_x = 0; mesh_x < m_presetInputs.gx; mesh_x++)
	  for (int mesh_y = 0; mesh_y < m_presetInputs.gy; mesh_y++)
  for (std::map<int, PerPixelEqn*>::iterator pos = per_pixel_eqn_tree.begin();
       pos != per_pixel_eqn_tree.end(); ++pos)
    pos->second->evaluate(mesh_x, mesh_y);

}

int Preset::readIn(std::istream & fs) {

  line_mode_t line_mode;

  /* Parse any comments */
  if (Parser::parse_top_comment(fs) < 0)
  {
	if (PRESET_DEBUG)
    		std::cerr << "[Preset::readIn] no left bracket found..." << std::endl;
    return PROJECTM_FAILURE;
  }

  /* Parse the preset name and a left bracket */
  char tmp_name[MAX_TOKEN_SIZE];

  if (Parser::parse_preset_name(fs, tmp_name) < 0)
  {
    std::cerr <<  "[Preset::readIn] loading of preset name failed" << std::endl;
    return PROJECTM_ERROR;
  }

  /// @note  We ignore the preset name because [preset00] is just not so useful

  // Loop through each line in file, trying to succesfully parse the file. 
  // If a line does not parse correctly, keep trucking along to next line.
  int retval;
  while ((retval = Parser::parse_line(fs, this)) != EOF)
  {
    if (retval == PROJECTM_PARSE_ERROR)
    {
      line_mode = UNSET_LINE_MODE;
      // std::cerr << "[Preset::readIn()] parse error in file \"" << this->absoluteFilePath() << "\"" << std::endl;
    }
  }

//  std::cerr << "loadPresetFile: finished line parsing successfully" << std::endl;

  /* Now the preset has been loaded.
     Evaluation calls can be made at appropiate
     times in the frame loop */

return PROJECTM_SUCCESS;
}

/* loadPresetFile: private function that loads a specific preset denoted
   by the given pathname */
int Preset::loadPresetFile(const std::string & pathname)
{
  /* Open the file corresponding to pathname */
  FILE* f = fopen(pathname.c_str(), "r");
  if (!f) {
    if (PRESET_DEBUG)
    	std::cerr << "loadPresetFile: loading of file \"" << pathname << "\" failed!\n";
    return PROJECTM_ERROR;
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  rewind(f);
  std::vector<char> buffer(fsize);

  int err = fread(&buffer[0], 1, fsize, f);
  if (!err)
  {
    printf("read failed\n");
    fclose(f);
    return PROJECTM_ERROR;
  }

  fclose(f);
  std::stringstream stream;
  stream.rdbuf()->pubsetbuf(&buffer[0],buffer.size());
  return readIn(stream);
}

