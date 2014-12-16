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
 * Per-pixel equation
 *
 * $Log$
 */

#ifndef _PER_PIXEL_EQN_H
#define _PER_PIXEL_EQN_H

#define PER_PIXEL_EQN_DEBUG 0

#define ZOOM_OP 0
#define ZOOMEXP_OP 1
#define ROT_OP 2
#define CX_OP 3
#define CY_OP 4
#define SX_OP 5
#define SY_OP  6
#define DX_OP 7
#define DY_OP 8
#define WARP_OP 9
#define NUM_OPS 10 /* obviously, this number is dependent on the number of existing per pixel operations */

class GenExpr;
class Param;
class PerPixelEqn;
class Preset;

class PerPixelEqn {
public:
    int index; /* used for splay tree ordering. */
    int flags; /* primarily to specify if this variable is user-defined */
    Param *param; 
    GenExpr *gen_expr;	

    void evalPerPixelEqns( Preset *preset );
    void evaluate(int mesh_i, int mesh_j);

    PerPixelEqn(int index, Param * param, GenExpr * gen_expr);

  };


#endif /** !_PER_PIXEL_EQN_H */
