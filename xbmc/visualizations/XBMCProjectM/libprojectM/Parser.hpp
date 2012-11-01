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
 * Preset parser
 *
 * $Log$
 */

#ifndef _PARSER_H
#define _PARSER_H
#define PARSE_DEBUG 0
//#define PARSE_DEBUG 0

#include <stdio.h>

#include "Expr.hpp"
#include "PerFrameEqn.hpp"
#include "InitCond.hpp"
#include "Preset.hpp"

/* Strings that prefix (and denote the type of) equations */
#define PER_FRAME_STRING "per_frame_"
#define PER_FRAME_STRING_LENGTH 10

#define PER_PIXEL_STRING "per_pixel_"
#define PER_PIXEL_STRING_LENGTH 10

#define PER_FRAME_INIT_STRING "per_frame_init_"
#define PER_FRAME_INIT_STRING_LENGTH 15

#define WAVECODE_STRING "wavecode_"
#define WAVECODE_STRING_LENGTH 9

#define WAVE_STRING "wave_"
#define WAVE_STRING_LENGTH 5

#define PER_POINT_STRING "per_point"
#define PER_POINT_STRING_LENGTH 9

#define PER_FRAME_STRING_NO_UNDERSCORE "per_frame"
#define PER_FRAME_STRING_NO_UNDERSCORE_LENGTH 9

#define SHAPECODE_STRING "shapecode_"
#define SHAPECODE_STRING_LENGTH 10

#define SHAPE_STRING "shape_"
#define SHAPE_STRING_LENGTH 6

#define SHAPE_INIT_STRING "init"
#define SHAPE_INIT_STRING_LENGTH 4

#define WAVE_INIT_STRING "init"
#define WAVE_INIT_STRING_LENGTH 4

typedef enum {
  UNSET_LINE_MODE,
  PER_FRAME_LINE_MODE,
  PER_PIXEL_LINE_MODE,
  PER_FRAME_INIT_LINE_MODE,
  INIT_COND_LINE_MODE,
  CUSTOM_WAVE_PER_POINT_LINE_MODE,
  CUSTOM_WAVE_PER_FRAME_LINE_MODE,
  CUSTOM_WAVE_WAVECODE_LINE_MODE,
  CUSTOM_SHAPE_SHAPECODE_LINE_MODE,
  CUSTOM_SHAPE_PER_FRAME_LINE_MODE,
  CUSTOM_SHAPE_PER_FRAME_INIT_LINE_MODE,
  CUSTOM_WAVE_PER_FRAME_INIT_LINE_MODE
} line_mode_t;

/** Token enumeration type */
typedef enum {
    tEOL,   /* end of a line, usually a '/n' or '/r' */
    tEOF,   /* end of file */
    tLPr,   /* ( */
    tRPr,   /* ) */
    tLBr,   /* [ */
    tRBr,   /* ] */
    tEq,    /* = */
    tPlus,  /* + */
    tMinus, /* - */
    tMult,  /* * */
    tMod,   /* % */
    tDiv,   /* / */
    tOr,    /* | */
    tAnd,   /* & */
    tComma, /* , */
    tPositive, /* + as a prefix operator */
    tNegative, /* - as a prefix operator */
    tSemiColon, /* ; */
    tStringTooLong, /* special token to indicate an invalid string length */
    tStringBufferFilled /* the string buffer for this line is maxed out */
  } token_type;

class CustomShape;
class CustomWave;
class GenExpr;
class InfixOp;
class PerFrameEqn;
class Preset;
class TreeExpr;

class Parser {
public:
    static std::string lastLinePrefix;
    static line_mode_t line_mode;
    static CustomWave *current_wave;
    static CustomShape *current_shape;
    static int string_line_buffer_index;
    static char string_line_buffer[STRING_LINE_SIZE];
    static unsigned int line_count;
    static int per_frame_eqn_count;
    static int per_frame_init_eqn_count;
    static int last_custom_wave_id;
    static int last_custom_shape_id;
    static char last_eqn_type[MAX_TOKEN_SIZE];
    static int last_token_size;
    static bool tokenWrapAroundEnabled;

    static PerFrameEqn *parse_per_frame_eqn( std::istream & fs, int index, 
                                             Preset * preset);
    static int parse_per_pixel_eqn( std::istream & fs, Preset * preset,
                                    char * init_string);
    static InitCond *parse_init_cond( std::istream & fs, char * name, Preset * preset );
    static int parse_preset_name( std::istream & fs, char * name );
    static int parse_top_comment( std::istream & fs );
    static int parse_line( std::istream & fs, Preset * preset );

    static int get_string_prefix_len(char * string);
    static TreeExpr * insert_gen_expr(GenExpr * gen_expr, TreeExpr ** root);
    static TreeExpr * insert_infix_op(InfixOp * infix_op, TreeExpr ** root);
    static token_type parseToken(std::istream & fs, char * string);
    static GenExpr ** parse_prefix_args(std::istream & fs, int num_args, Preset * preset);
    static GenExpr * parse_infix_op(std::istream & fs, token_type token, TreeExpr * tree_expr, Preset * preset);
    static GenExpr * parse_sign_arg(std::istream & fs);
    static int parse_float(std::istream & fs, float * float_ptr);
    static int parse_int(std::istream & fs, int * int_ptr);
    static int insert_gen_rec(GenExpr * gen_expr, TreeExpr * root);
    static int insert_infix_rec(InfixOp * infix_op, TreeExpr * root);
    static GenExpr * parse_gen_expr(std::istream & fs, TreeExpr * tree_expr, Preset * preset);
    static PerFrameEqn * parse_implicit_per_frame_eqn(std::istream & fs, char * param_string, int index, Preset * preset);
    static InitCond * parse_per_frame_init_eqn(std::istream & fs, Preset * preset, std::map<std::string,Param*> * database);
    static int parse_wavecode_prefix(char * token, int * id, char ** var_string);
    static int parse_wavecode(char * token, std::istream & fs, Preset * preset);
    static int parse_wave_prefix(char * token, int * id, char ** eqn_string);
    static int parse_wave_helper(std::istream & fs, Preset * preset, int id, char * eqn_type, char * init_string);
    static int parse_shapecode(char * eqn_string, std::istream & fs, Preset * preset);
    static int parse_shapecode_prefix(char * token, int * id, char ** var_string);
    
    static int parse_wave(char * eqn_string, std::istream & fs, Preset * preset);
    static int parse_shape(char * eqn_string, std::istream & fs, Preset * preset);
    static int parse_shape_prefix(char * token, int * id, char ** eqn_string);

    static int update_string_buffer(char * buffer, int * index);
    static int string_to_float(char * string, float * float_ptr);
    static int parse_shape_per_frame_init_eqn(std::istream & fs, CustomShape * custom_shape, Preset * preset);
    static int parse_shape_per_frame_eqn(std::istream & fs, CustomShape * custom_shape, Preset * preset);
    static int parse_wave_per_frame_eqn(std::istream & fs, CustomWave * custom_wave, Preset * preset);
    static bool wrapsToNextLine(const std::string & str);
  };

#endif /** !_PARSER_H */

