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

#ifndef _PER_FRAME_EQN_H
#define _PER_FRAME_EQN_H

#define PER_FRAME_EQN_DEBUG 0

per_frame_eqn_t * new_per_frame_eqn(int index, param_t * param, struct GEN_EXPR_T * gen_expr);
void eval_per_frame_eqn(per_frame_eqn_t * per_frame_eqn);
void free_per_frame_eqn(per_frame_eqn_t * per_frame_eqn);
void eval_per_frame_init_eqn(per_frame_eqn_t * per_frame_eqn);

#endif /** !_PER_FRAME_EQN_H */
