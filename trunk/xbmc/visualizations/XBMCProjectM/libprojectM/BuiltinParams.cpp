#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdio.h>

#include "fatal.h"
#include "BuiltinParams.hpp"
#include "Algorithms.hpp"
#include "InitCondUtils.hpp"


using namespace Algorithms;

BuiltinParams::BuiltinParams() {}

BuiltinParams::BuiltinParams(const PresetInputs & presetInputs, PresetOutputs & presetOutputs)
{

 int ret;
  if ((ret = init_builtin_param_db(presetInputs, presetOutputs)) != PROJECTM_SUCCESS)
  {
	std::cout << "failed to allocate builtin parameter database with error " << ret << std::endl;;
    throw ret;
  }

}

BuiltinParams::~BuiltinParams()
{
  destroy_builtin_param_db();
}

/* Loads a float parameter into the builtin database */
int BuiltinParams::load_builtin_param_float(const std::string & name, void * engine_val, void * matrix, short int flags,
    float init_val, float upper_bound, float lower_bound, const std::string & alt_name)
{

  Param * param = NULL;
  CValue iv, ub, lb;

  iv.float_val = init_val;
  ub.float_val = upper_bound;
  lb.float_val = lower_bound;

  /* Create new parameter of type float */
  if (BUILTIN_PARAMS_DEBUG == 2)
  {
    printf("load_builtin_param_float: (name \"%s\") (alt_name = \"%s\") ", name.c_str(), alt_name.c_str());
    fflush(stdout);
  }

std::string lowerName(name);
std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), tolower);

  if ((param = new Param(lowerName, P_TYPE_DOUBLE, flags, engine_val, matrix, iv, ub, lb)) == NULL)
  {
    return PROJECTM_OUTOFMEM_ERROR;
  }

  if (BUILTIN_PARAMS_DEBUG == 2)
  {
    printf("created...");
    fflush(stdout);
  }

  /* Insert the paremeter into the database */

  if (insert_builtin_param( param ) < 0)
  {
    delete param;
    return PROJECTM_ERROR;
  }

  if (BUILTIN_PARAMS_DEBUG == 2)
  {
    printf("inserted...");
    fflush(stdout);
  }

  /* If this parameter has an alternate name, insert it into the database as link */

  if (alt_name != "")
  {
    std::string alt_lower_name(alt_name);
    std::transform(alt_lower_name.begin(), alt_lower_name.end(), alt_lower_name.begin(), tolower);
    insert_param_alt_name(param,alt_lower_name);
 
    if (BUILTIN_PARAMS_DEBUG == 2)
    {
      printf("alt_name inserted...");
      fflush(stdout);
    }


  }

  if (BUILTIN_PARAMS_DEBUG == 2) printf("finished\n");

  /* Finished, return success */
  return PROJECTM_SUCCESS;
}



/* Destroy the builtin parameter database.
   Generally, do this on projectm exit */
int BuiltinParams::destroy_builtin_param_db()
{

  Algorithms::traverse<TraverseFunctors::DeleteFunctor<Param> >(builtin_param_tree);
  return PROJECTM_SUCCESS;
}


/* Insert a parameter into the database with an alternate name */
int BuiltinParams::insert_param_alt_name(Param * param, const std::string & alt_name)
{

  assert(param);
  
  aliasMap.insert(std::make_pair(alt_name, param->name));

  return PROJECTM_SUCCESS;
}

Param * BuiltinParams::find_builtin_param(const std::string & name)
{


  
  AliasMap::iterator pos = aliasMap.find(name);
  Param * param = 0;
  //std::cerr << "[BuiltinParams] find_builtin_param: name is " << name << std::endl;
  if (pos == aliasMap.end())
  {
    std::map<std::string, Param*>::iterator builtinPos = builtin_param_tree.find(name);

    if (builtinPos != builtin_param_tree.end()) {
    //  std::cerr << "[BuiltinParams] find_builtin_param: found it directly." << std::endl;
      param = builtinPos->second;
     }
  }
  else
  {

    std::map<std::string, Param*>::iterator builtinPos = builtin_param_tree.find(pos->second);
	
    if (builtinPos != builtin_param_tree.end()) {
      //std::cerr << "[BuiltinParams] find_builtin_param: found it indirectly." << std::endl;
      param = builtinPos->second;

}
  }
  return param;
}


/* Loads a integer parameter into the builtin database */
int BuiltinParams::load_builtin_param_int(const std::string & name, void * engine_val, short int flags,
    int init_val, int upper_bound, int lower_bound, const std::string &alt_name)
{

  Param * param;
  CValue iv, ub, lb;

  iv.int_val = init_val;
  ub.int_val = upper_bound;
  lb.int_val = lower_bound;

std::string lowerName(name);
std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), tolower);
  param = new Param(lowerName, P_TYPE_INT, flags, engine_val, NULL, iv, ub, lb);

  if (param == NULL)
  {
    return PROJECTM_OUTOFMEM_ERROR;
  }

  if (insert_builtin_param( param ) < 0)
  {
    delete param;
    return PROJECTM_ERROR;
  }

  if (alt_name != "")
  {
    std::string alt_lower_name(alt_name);
    std::transform(alt_lower_name.begin(), alt_lower_name.end(), alt_lower_name.begin(), tolower);
    insert_param_alt_name(param,alt_lower_name);
 
  }

  return PROJECTM_SUCCESS;

}

/* Loads a boolean parameter */
int BuiltinParams::load_builtin_param_bool(const std:: string & name, void * engine_val, short int flags,
    int init_val, const std::string &alt_name)
{

  Param * param;
  CValue iv, ub, lb;

  iv.int_val = init_val;
  ub.int_val = TRUE;
  lb.int_val = false;

std::string lowerName(name);
std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), tolower);

  param = new Param(lowerName, P_TYPE_BOOL, flags, engine_val, NULL, iv, ub, lb);

  if (param == NULL)
  {
    return PROJECTM_OUTOFMEM_ERROR;
  }

  if (insert_builtin_param(param) < 0)
  {
    delete param;
    return PROJECTM_ERROR;
  }

  if (alt_name != "")
  {
    std::string alt_lower_name(alt_name);
    std::transform(alt_lower_name.begin(), alt_lower_name.end(), alt_lower_name.begin(), tolower);
    insert_param_alt_name(param,alt_lower_name);
  }

  return PROJECTM_SUCCESS;

}

/* Inserts a parameter into the builtin database */
int BuiltinParams::insert_builtin_param( Param *param )
{
  std::pair<std::map<std::string, Param*>::iterator, bool> inserteePos = builtin_param_tree.insert(std::make_pair(param->name, param));

  return inserteePos.second;
}



/* Initialize the builtin parameter database.
   Should only be necessary once */
int BuiltinParams::init_builtin_param_db(const PresetInputs & presetInputs, PresetOutputs & presetOutputs)
{

  if (BUILTIN_PARAMS_DEBUG)
  {
    printf("init_builtin_param: loading database...");
    fflush(stdout);
  }

  /* Loads all builtin parameters into the database */
  if (load_all_builtin_param(presetInputs, presetOutputs) < 0)
  {
    if (BUILTIN_PARAMS_DEBUG) printf("failed loading builtin parameters (FATAL)\n");
    return PROJECTM_ERROR;
  }

  if (BUILTIN_PARAMS_DEBUG) printf("success!\n");

  /* Finished, no errors */
  return PROJECTM_SUCCESS;
}



/* Loads all builtin parameters, limits are also defined here */
int BuiltinParams::load_all_builtin_param(const PresetInputs & presetInputs, PresetOutputs & presetOutputs)
{

  load_builtin_param_float("frating", (void*)&presetOutputs.fRating, NULL, P_FLAG_NONE, 0.0 , 5.0, 0.0, "");
  load_builtin_param_float("fwavescale", (void*)&presetOutputs.fWaveScale, NULL, P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("gamma", (void*)&presetOutputs.fGammaAdj, NULL, P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, 0, "fGammaAdj");
  load_builtin_param_float("echo_zoom", (void*)&presetOutputs.fVideoEchoZoom, NULL, P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, 0, "fVideoEchoZoom");
  load_builtin_param_float("echo_alpha", (void*)&presetOutputs.fVideoEchoAlpha, NULL, P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, 0, "fvideoechoalpha");
  load_builtin_param_float("wave_a", (void*)&presetOutputs.fWaveAlpha, NULL, P_FLAG_NONE, 0.0, 1.0, 0, "fwavealpha");
  load_builtin_param_float("fwavesmoothing", (void*)&presetOutputs.fWaveSmoothing, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, "");
  load_builtin_param_float("fmodwavealphastart", (void*)&presetOutputs.fModWaveAlphaStart, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, "");
  load_builtin_param_float("fmodwavealphaend", (void*)&presetOutputs.fModWaveAlphaEnd, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, "");
  load_builtin_param_float("fWarpAnimSpeed",  (void*)&presetOutputs.fWarpAnimSpeed, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, "");
  load_builtin_param_float("fWarpScale",  (void*)&presetOutputs.fWarpScale, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, "");
  //  load_builtin_param_float("warp", (void*)&presetOutputs.warp, warp_mesh, P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, 0, "");

  load_builtin_param_float("fshader", (void*)&presetOutputs.fShader, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, "");
  load_builtin_param_float("decay", (void*)&presetOutputs.decay, NULL, P_FLAG_NONE, 0.0, 1.0, 0, "fdecay");

  load_builtin_param_int("echo_orient", (void*)&presetOutputs.nVideoEchoOrientation, P_FLAG_NONE, 0, 3, 0, "nVideoEchoOrientation");
  load_builtin_param_int("wave_mode", (void*)&presetOutputs.nWaveMode, P_FLAG_NONE, 0, 7, 0, "nwavemode");

  load_builtin_param_bool("wave_additive", (void*)&presetOutputs.bAdditiveWaves, P_FLAG_NONE, false, "bAdditiveWaves");
  load_builtin_param_bool("bmodwavealphabyvolume", (void*)&presetOutputs.bModWaveAlphaByVolume, P_FLAG_NONE, false, "");
  load_builtin_param_bool("wave_brighten", (void*)&presetOutputs.bMaximizeWaveColor, P_FLAG_NONE, false, "bMaximizeWaveColor");
  load_builtin_param_bool("wrap", (void*)&presetOutputs.bTexWrap, P_FLAG_NONE, false, "btexwrap");
  load_builtin_param_bool("darken_center", (void*)&presetOutputs.bDarkenCenter, P_FLAG_NONE, false, "bdarkencenter");
  load_builtin_param_bool("bredbluestereo", (void*)&presetOutputs.bRedBlueStereo, P_FLAG_NONE, false, "");
  load_builtin_param_bool("brighten", (void*)&presetOutputs.bBrighten, P_FLAG_NONE, false, "bbrighten");
  load_builtin_param_bool("darken", (void*)&presetOutputs.bDarken, P_FLAG_NONE, false, "bdarken");
  load_builtin_param_bool("solarize", (void*)&presetOutputs.bSolarize, P_FLAG_NONE, false, "bsolarize");
  load_builtin_param_bool("invert", (void*)&presetOutputs.bInvert, P_FLAG_NONE, false, "binvert");
  load_builtin_param_bool("bmotionvectorson", (void*)&presetOutputs.bMotionVectorsOn, P_FLAG_NONE, false, "");
  load_builtin_param_bool("wave_dots", (void*)&presetOutputs.bWaveDots, P_FLAG_NONE, false, "bwavedots");
  load_builtin_param_bool("wave_thick", (void*)&presetOutputs.bWaveThick, P_FLAG_NONE, false, "bwavethick");
  load_builtin_param_float("warp", (void*)&presetOutputs.warp, presetOutputs.warp_mesh,  P_FLAG_PER_PIXEL |P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, "");
  load_builtin_param_float("zoom", (void*)&presetOutputs.zoom, presetOutputs.zoom_mesh,  P_FLAG_PER_PIXEL |P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, "");
  load_builtin_param_float("rot", (void*)&presetOutputs.rot, presetOutputs.rot_mesh,  P_FLAG_PER_PIXEL |P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, "");
  /// @note added huge bug fix here potentially by prevening zoomexp_mesh from being freed when presets dealloc
  load_builtin_param_float("zoomexp", (void*)&presetOutputs.zoomexp, presetOutputs.zoomexp_mesh,  P_FLAG_PER_PIXEL |P_FLAG_NONE , 0.0, MAX_DOUBLE_SIZE, 0, "fzoomexponent");

  load_builtin_param_float("cx", (void*)&presetOutputs.cx, presetOutputs.cx_mesh, P_FLAG_PER_PIXEL | P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, "");
  load_builtin_param_float("cy", (void*)&presetOutputs.cy, presetOutputs.cy_mesh, P_FLAG_PER_PIXEL | P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, "");
  load_builtin_param_float("dx", (void*)&presetOutputs.dx, presetOutputs.dx_mesh,  P_FLAG_PER_PIXEL | P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, "");
  load_builtin_param_float("dy", (void*)&presetOutputs.dy, presetOutputs.dy_mesh,  P_FLAG_PER_PIXEL |P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, "");
  load_builtin_param_float("sx", (void*)&presetOutputs.sx, presetOutputs.sx_mesh,  P_FLAG_PER_PIXEL |P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, "");
  load_builtin_param_float("sy", (void*)&presetOutputs.sy, presetOutputs.sy_mesh,  P_FLAG_PER_PIXEL |P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, "");

  load_builtin_param_float("wave_r", (void*)&presetOutputs.wave_r, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("wave_g", (void*)&presetOutputs.wave_g, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("wave_b", (void*)&presetOutputs.wave_b, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("wave_x", (void*)&presetOutputs.wave_x, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("wave_y", (void*)&presetOutputs.wave_y, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("wave_mystery", (void*)&presetOutputs.wave_mystery, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, "fWaveParam");

  load_builtin_param_float("ob_size", (void*)&presetOutputs.ob_size, NULL, P_FLAG_NONE, 0.0, 0.5, 0, "");
  load_builtin_param_float("ob_r", (void*)&presetOutputs.ob_r, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("ob_g", (void*)&presetOutputs.ob_g, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("ob_b", (void*)&presetOutputs.ob_b, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("ob_a", (void*)&presetOutputs.ob_a, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, "");

  load_builtin_param_float("ib_size", (void*)&presetOutputs.ib_size,  NULL,P_FLAG_NONE, 0.0, .5, 0.0, "");
  load_builtin_param_float("ib_r", (void*)&presetOutputs.ib_r,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("ib_g", (void*)&presetOutputs.ib_g,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("ib_b", (void*)&presetOutputs.ib_b,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("ib_a", (void*)&presetOutputs.ib_a,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, "");

  load_builtin_param_float("mv_r", (void*)&presetOutputs.mv_r,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("mv_g", (void*)&presetOutputs.mv_g,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("mv_b", (void*)&presetOutputs.mv_b,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("mv_x", (void*)&presetOutputs.mv_x,  NULL,P_FLAG_NONE, 0.0, 64.0, 0.0, "nmotionvectorsx");
  load_builtin_param_float("mv_y", (void*)&presetOutputs.mv_y,  NULL,P_FLAG_NONE, 0.0, 48.0, 0.0, "nmotionvectorsy");
  load_builtin_param_float("mv_l", (void*)&presetOutputs.mv_l,  NULL,P_FLAG_NONE, 0.0, 5.0, 0.0, "");
  load_builtin_param_float("mv_dy", (void*)&presetOutputs.mv_dy, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, "");
  load_builtin_param_float("mv_dx", (void*)&presetOutputs.mv_dx,  NULL,P_FLAG_NONE, 0.0, 1.0, -1.0, "");
  load_builtin_param_float("mv_a", (void*)&presetOutputs.mv_a,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, "");

  load_builtin_param_float("time", (void*)&presetInputs.time,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0.0, "");
  load_builtin_param_float("bass", (void*)&presetInputs.bass,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0.0, "");
  load_builtin_param_float("mid", (void*)&presetInputs.mid,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0, "");

  load_builtin_param_float("treb", (void*)&presetInputs.treb,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0, "");


  load_builtin_param_float("mood_r", (void*)&presetInputs.mood_r,  NULL,P_FLAG_READONLY, 0.0, 1.0, 0.0, "");
  load_builtin_param_float("mood_g", (void*)&presetInputs.mood_g,  NULL,P_FLAG_READONLY, 0.0, 1.0, 0, "");
  load_builtin_param_float("mood_b", (void*)&presetInputs.mood_b,  NULL,P_FLAG_READONLY, 0.0, 1.0, 0, "");

  load_builtin_param_float("bass_att", (void*)&presetInputs.bass_att,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0, "");
  load_builtin_param_float("mid_att", (void*)&presetInputs.mid_att,  NULL, P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0, "");
  load_builtin_param_float("treb_att", (void*)&presetInputs.treb_att,  NULL, P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0, "");
  load_builtin_param_int("frame", (void*)&presetInputs.frame, P_FLAG_READONLY, 0, MAX_INT_SIZE, 0, "");
  load_builtin_param_float("progress", (void*)&presetInputs.progress,  NULL,P_FLAG_READONLY, 0.0, 1, 0, "");
  load_builtin_param_int("fps", (void*)&presetInputs.fps, P_FLAG_READONLY, 15, MAX_INT_SIZE, 0, "");

  load_builtin_param_float("x", (void*)&presetInputs.x_per_pixel, presetInputs.x_mesh,  P_FLAG_PER_PIXEL |P_FLAG_ALWAYS_MATRIX | P_FLAG_READONLY | P_FLAG_NONE,
                           0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("y", (void*)&presetInputs.y_per_pixel, presetInputs.y_mesh,  P_FLAG_PER_PIXEL |P_FLAG_ALWAYS_MATRIX |P_FLAG_READONLY | P_FLAG_NONE,
                           0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("ang", (void*)&presetInputs.ang_per_pixel, presetInputs.theta_mesh,  P_FLAG_PER_PIXEL |P_FLAG_ALWAYS_MATRIX | P_FLAG_READONLY | P_FLAG_NONE,
                           0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("rad", (void*)&presetInputs.rad_per_pixel, presetInputs.rad_mesh,  P_FLAG_PER_PIXEL |P_FLAG_ALWAYS_MATRIX | P_FLAG_READONLY | P_FLAG_NONE,
                           0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");

  load_builtin_param_float("q1", (void*)&presetOutputs.q1,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("q2", (void*)&presetOutputs.q2,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("q3", (void*)&presetOutputs.q3,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("q4", (void*)&presetOutputs.q4,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("q5", (void*)&presetOutputs.q5,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("q6", (void*)&presetOutputs.q6,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("q7", (void*)&presetOutputs.q7,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");
  load_builtin_param_float("q8", (void*)&presetOutputs.q8,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, "");

  /* variables added in 1.04 */
  load_builtin_param_int("meshx", (void*)&presetInputs.gx, P_FLAG_READONLY, 32, 96, 8, "");
  load_builtin_param_int("meshy", (void*)&presetInputs.gy, P_FLAG_READONLY, 24, 72, 6, "");

  return PROJECTM_SUCCESS;

}

