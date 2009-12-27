
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
 * MilkdropPreset
 *
 * $Log$
 */

#ifndef _MilkdropPreset_HPP
#define _MilkdropPreset_HPP

#include "Common.hpp"
#include <string>
#include <cassert>
#include <map>

#define MILKDROP_PRESET_DEBUG 0 /* 0 for no debugging, 1 for normal, 2 for insane */

#include "CustomShape.hpp"
#include "CustomWave.hpp"
#include "Expr.hpp"
#include "PerPixelEqn.hpp"
#include "PerFrameEqn.hpp"
#include "BuiltinParams.hpp"
#include "PresetFrameIO.hpp"
#include "InitCond.hpp"
#include "Preset.hpp"

class CustomWave;
class CustomShape;
class InitCond;


class MilkdropPreset : public Preset
{

public:


  ///  Load a MilkdropPreset by filename with input and output buffers specified.
  /// \param absoluteFilePath the absolute file path of a MilkdropPreset to load from the file system
  /// \param MilkdropPresetName a descriptive name for the MilkdropPreset. Usually just the file name
  /// \param MilkdropPresetInputs a reference to read only projectM engine variables
  /// \param MilkdropPresetOutputs initialized and filled with data parsed from a MilkdropPreset
  MilkdropPreset(const std::string & absoluteFilePath, const std::string & milkdropPresetName, PresetOutputs & presetOutputs);

  ///  Load a MilkdropPreset from an input stream with input and output buffers specified.
  /// \param in an already initialized input stream to read the MilkdropPreset file from
  /// \param MilkdropPresetName a descriptive name for the MilkdropPreset. Usually just the file name
  /// \param MilkdropPresetInputs a reference to read only projectM engine variables
  /// \param MilkdropPresetOutputs initialized and filled with data parsed from a MilkdropPreset
  MilkdropPreset(std::istream & in, const std::string & milkdropPresetName, PresetOutputs & presetOutputs);

  ~MilkdropPreset();

  /// All "builtin" parameters for this MilkdropPreset. Anything *but* user defined parameters and
  /// custom waves / shapes objects go here.
  /// @bug encapsulate
  BuiltinParams builtinParams;


  /// Used by parser to find/create custom waves and shapes. May be refactored
  template <class CustomObject>
  static CustomObject * find_custom_object(int id, std::vector<CustomObject*> & customObjects);


  int per_pixel_eqn_string_index;
  int per_frame_eqn_string_index;
  int per_frame_init_eqn_string_index;

  int per_frame_eqn_count,
  per_frame_init_eqn_count;


  /// Used by parser
  /// @bug refactor
  int add_per_pixel_eqn( char *name, GenExpr *gen_expr );

  /// Accessor method to retrieve the absolute file path of the loaded MilkdropPreset
  /// \returns a file path string
  std::string absoluteFilePath() const
  {
    return _absoluteFilePath;
  }


  /// Accessor method for the MilkdropPreset outputs instance associated with this MilkdropPreset
  /// \returns A MilkdropPreset output instance with values computed from most recent evaluateFrame()
  PresetOutputs & presetOutputs() const
  {

    return _presetOutputs;
  }

  const PresetInputs & presetInputs() const
  {

    return _presetInputs;
  }


// @bug encapsulate

  PresetOutputs::cwave_container customWaves;
  PresetOutputs::cshape_container customShapes;

  /// @bug encapsulate
  /* Data structures that contain equation and initial condition information */
  std::vector<PerFrameEqn*>  per_frame_eqn_tree;   /* per frame equations */
  std::map<int, PerPixelEqn*>  per_pixel_eqn_tree; /* per pixel equation tree */
  std::map<std::string,InitCond*>  per_frame_init_eqn_tree; /* per frame initial equations */
  std::map<std::string,InitCond*>  init_cond_tree; /* initial conditions */
  std::map<std::string,Param*> user_param_tree; /* user parameter splay tree */


  PresetOutputs & pipeline() { return _presetOutputs; } 

  void Render(const BeatDetect &music, const PipelineContext &context);

private:

  PresetInputs _presetInputs;
  /// Evaluates the MilkdropPreset for a frame given the current values of MilkdropPreset inputs / outputs
  /// All calculated values are stored in the associated MilkdropPreset outputs instance
  void evaluateFrame();

  // The absolute file path of the MilkdropPreset
  std::string _absoluteFilePath;

  // The absolute path of the MilkdropPreset
  std::string _absolutePath;

  void initialize(const std::string & pathname);
  void initialize(std::istream & in);

  int loadPresetFile(const std::string & pathname);

  void loadBuiltinParamsUnspecInitConds();
  void loadCustomWaveUnspecInitConds();
  void loadCustomShapeUnspecInitConds();

  void evalCustomWavePerFrameEquations();
  void evalCustomShapePerFrameEquations();
  void evalPerFrameInitEquations();
  void evalCustomWaveInitConditions();
  void evalCustomShapeInitConditions();
  void evalPerPixelEqns();
  void evalPerFrameEquations();
  void initialize_PerPixelMeshes();
  int readIn(std::istream & fs);

  void preloadInitialize();
  void postloadInitialize();

  PresetOutputs & _presetOutputs;

template <class CustomObject>
void transfer_q_variables(std::vector<CustomObject*> & customObjects);
};


template <class CustomObject>
void MilkdropPreset::transfer_q_variables(std::vector<CustomObject*> & customObjects)
{
 	CustomObject * custom_object;

	for (typename std::vector<CustomObject*>::iterator pos = customObjects.begin(); pos != customObjects.end();++pos) {

		custom_object = *pos;
		for (unsigned int i = 0; i < NUM_Q_VARIABLES; i++)
			custom_object->q[i] = _presetOutputs.q[i];
	}


}

template <class CustomObject>
CustomObject * MilkdropPreset::find_custom_object(int id, std::vector<CustomObject*> & customObjects)
{

  CustomObject * custom_object = NULL;


  for (typename std::vector<CustomObject*>::iterator pos = customObjects.begin(); pos != customObjects.end();++pos) {
	if ((*pos)->id == id) {
		custom_object = *pos;
		break;
	}
  }

  if (custom_object == NULL)
  {

    if ((custom_object = new CustomObject(id)) == NULL)
    {
      return NULL;
    }

      customObjects.push_back(custom_object);

  }

  assert(custom_object);
  return custom_object;
}



#endif /** !_MilkdropPreset_HPP */
