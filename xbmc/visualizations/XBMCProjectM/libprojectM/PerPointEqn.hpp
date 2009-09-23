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
 * Per-point equation
 *
 * $Log$
 */

#ifndef _PER_POINT_EQN_H
#define _PER_POINT_EQN_H

class CustomWave;
class GenExpr;
class Param;
class PerPointEqn;

class PerPointEqn {
public:
    int index;
    int samples; // the number of samples to iterate over
    Param *param;
    GenExpr * gen_expr;
    ~PerPointEqn();
    void evaluate(int i);
    PerPointEqn( int index, Param *param, GenExpr *gen_expr, int samples);
 };


//inline void eval_per_point_eqn_helper( void *per_point_eqn ) {
//    ((PerPointEqn *)per_point_eqn)->evalPerPointEqn();
//  }

#endif /** !_PER_POINT_EQN_H */
