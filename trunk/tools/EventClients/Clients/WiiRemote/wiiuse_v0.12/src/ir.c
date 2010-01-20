/*
 *	wiiuse
 *
 *	Written By:
 *		Michael Laforest	< para >
 *		Email: < thepara (--AT--) g m a i l [--DOT--] com >
 *
 *	Copyright 2006-2007
 *
 *	This file is part of wiiuse.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	$Header$
 *
 */

/**
 *	@file
 *	@brief Handles IR data.
 */

#include <stdio.h>
#include <math.h>

#ifndef WIN32
	#include <unistd.h>
#endif

#include "definitions.h"
#include "wiiuse_internal.h"
#include "ir.h"

static int get_ir_sens(struct wiimote_t* wm, char** block1, char** block2);
static void interpret_ir_data(struct wiimote_t* wm);
static void fix_rotated_ir_dots(struct ir_dot_t* dot, float ang);
static void get_ir_dot_avg(struct ir_dot_t* dot, int* x, int* y);
static void reorder_ir_dots(struct ir_dot_t* dot);
static float ir_distance(struct ir_dot_t* dot);
static int ir_correct_for_bounds(int* x, int* y, enum aspect_t aspect, int offset_x, int offset_y);
static void ir_convert_to_vres(int* x, int* y, enum aspect_t aspect, int vx, int vy);


/**
 *	@brief	Set if the wiimote should track IR targets.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param status	1 to enable, 0 to disable.
 */
void wiiuse_set_ir(struct wiimote_t* wm, int status) {
	byte buf;
	char* block1 = NULL;
	char* block2 = NULL;
	int ir_level;

	if (!wm)
		return;

	/*
	 *	Wait for the handshake to finish first.
	 *	When it handshake finishes and sees that
	 *	IR is enabled, it will call this function
	 *	again to actually enable IR.
	 */
	if (!WIIMOTE_IS_SET(wm, WIIMOTE_STATE_HANDSHAKE_COMPLETE)) {
		WIIUSE_DEBUG("Tried to enable IR, will wait until handshake finishes.");
		WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_IR);
		return;
	}

	/*
	 *	Check to make sure a sensitivity setting is selected.
	 */
	ir_level = get_ir_sens(wm, &block1, &block2);
	if (!ir_level) {
		WIIUSE_ERROR("No IR sensitivity setting selected.");
		return;
	}

	if (status) {
		/* if already enabled then stop */
		if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_IR))
			return;
		WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_IR);
	} else {
		/* if already disabled then stop */
		if (!WIIMOTE_IS_SET(wm, WIIMOTE_STATE_IR))
			return;
		WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_IR);
	}

	/* set camera 1 and 2 */
	buf = (status ? 0x04 : 0x00);
	wiiuse_send(wm, WM_CMD_IR, &buf, 1);
	wiiuse_send(wm, WM_CMD_IR_2, &buf, 1);

	if (!status) {
		WIIUSE_DEBUG("Disabled IR cameras for wiimote id %i.", wm->unid);
		wiiuse_set_report_type(wm);
		return;
	}

	/* enable IR, set sensitivity */
	buf = 0x08;
	wiiuse_write_data(wm, WM_REG_IR, &buf, 1);

	/* wait for the wiimote to catch up */
	#ifndef WIN32
		usleep(50000);
	#else
		Sleep(50);
	#endif

	/* write sensitivity blocks */
	wiiuse_write_data(wm, WM_REG_IR_BLOCK1, (byte*)block1, 9);
	wiiuse_write_data(wm, WM_REG_IR_BLOCK2, (byte*)block2, 2);

	/* set the IR mode */
	if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_EXP))
		buf = WM_IR_TYPE_BASIC;
	else
		buf = WM_IR_TYPE_EXTENDED;
	wiiuse_write_data(wm, WM_REG_IR_MODENUM, &buf, 1);

	#ifndef WIN32
		usleep(50000);
	#else
		Sleep(50);
	#endif

	/* set the wiimote report type */
	wiiuse_set_report_type(wm);

	WIIUSE_DEBUG("Enabled IR camera for wiimote id %i (sensitivity level %i).", wm->unid, ir_level);
}


/**
 *	@brief	Get the IR sensitivity settings.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param block1	[out] Pointer to where block1 will be set.
 *	@param block2	[out] Pointer to where block2 will be set.
 *
 *	@return Returns the sensitivity level.
 */
static int get_ir_sens(struct wiimote_t* wm, char** block1, char** block2) {
	if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_IR_SENS_LVL1)) {
		*block1 = WM_IR_BLOCK1_LEVEL1;
		*block2 = WM_IR_BLOCK2_LEVEL1;
		return 1;
	} else if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_IR_SENS_LVL2)) {
		*block1 = WM_IR_BLOCK1_LEVEL2;
		*block2 = WM_IR_BLOCK2_LEVEL2;
		return 2;
	} else if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_IR_SENS_LVL3)) {
		*block1 = WM_IR_BLOCK1_LEVEL3;
		*block2 = WM_IR_BLOCK2_LEVEL3;
		return 3;
	} else if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_IR_SENS_LVL4)) {
		*block1 = WM_IR_BLOCK1_LEVEL4;
		*block2 = WM_IR_BLOCK2_LEVEL4;
		return 4;
	} else if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_IR_SENS_LVL5)) {
		*block1 = WM_IR_BLOCK1_LEVEL5;
		*block2 = WM_IR_BLOCK2_LEVEL5;
		return 5;
	}

	*block1 = NULL;
	*block2 = NULL;
	return 0;
}


/**
 *	@brief	Set the virtual screen resolution for IR tracking.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param status	1 to enable, 0 to disable.
 */
void wiiuse_set_ir_vres(struct wiimote_t* wm, unsigned int x, unsigned int y) {
	if (!wm)	return;

	wm->ir.vres[0] = (x-1);
	wm->ir.vres[1] = (y-1);
}


/**
 *	@brief	Set the XY position for the IR cursor.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 */
void wiiuse_set_ir_position(struct wiimote_t* wm, enum ir_position_t pos) {
	if (!wm)	return;

	wm->ir.pos = pos;

	switch (pos) {

		case WIIUSE_IR_ABOVE:
			wm->ir.offset[0] = 0;

			if (wm->ir.aspect == WIIUSE_ASPECT_16_9)
				wm->ir.offset[1] = WM_ASPECT_16_9_Y/2 - 70;
			else if (wm->ir.aspect == WIIUSE_ASPECT_4_3)
				wm->ir.offset[1] = WM_ASPECT_4_3_Y/2 - 100;

			return;

		case WIIUSE_IR_BELOW:
			wm->ir.offset[0] = 0;

			if (wm->ir.aspect == WIIUSE_ASPECT_16_9)
				wm->ir.offset[1] = -WM_ASPECT_16_9_Y/2 + 100;
			else if (wm->ir.aspect == WIIUSE_ASPECT_4_3)
				wm->ir.offset[1] = -WM_ASPECT_4_3_Y/2 + 70;

			return;

		default:
			return;
	};
}


/**
 *	@brief	Set the aspect ratio of the TV/monitor.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param aspect	Either WIIUSE_ASPECT_16_9 or WIIUSE_ASPECT_4_3
 */
void wiiuse_set_aspect_ratio(struct wiimote_t* wm, enum aspect_t aspect) {
	if (!wm)	return;

	wm->ir.aspect = aspect;

	if (aspect == WIIUSE_ASPECT_4_3) {
		wm->ir.vres[0] = WM_ASPECT_4_3_X;
		wm->ir.vres[1] = WM_ASPECT_4_3_Y;
	} else {
		wm->ir.vres[0] = WM_ASPECT_16_9_X;
		wm->ir.vres[1] = WM_ASPECT_16_9_Y;
	}

	/* reset the position offsets */
	wiiuse_set_ir_position(wm, wm->ir.pos);
}


/**
 *	@brief	Set the IR sensitivity.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param level	1-5, same as Wii system sensitivity setting.
 *
 *	If the level is < 1, then level will be set to 1.
 *	If the level is > 5, then level will be set to 5.
 */
void wiiuse_set_ir_sensitivity(struct wiimote_t* wm, int level) {
	char* block1 = NULL;
	char* block2 = NULL;

	if (!wm)	return;

	if (level > 5)		level = 5;
	if (level < 1)		level = 1;

	WIIMOTE_DISABLE_STATE(wm, (WIIMOTE_STATE_IR_SENS_LVL1 |
								WIIMOTE_STATE_IR_SENS_LVL2 |
								WIIMOTE_STATE_IR_SENS_LVL3 |
								WIIMOTE_STATE_IR_SENS_LVL4 |
								WIIMOTE_STATE_IR_SENS_LVL5));

	switch (level) {
		case 1:
			WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_IR_SENS_LVL1);
			break;
		case 2:
			WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_IR_SENS_LVL2);
			break;
		case 3:
			WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_IR_SENS_LVL3);
			break;
		case 4:
			WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_IR_SENS_LVL4);
			break;
		case 5:
			WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_IR_SENS_LVL5);
			break;
		default:
			return;
	}

	/* set the new sensitivity */
	get_ir_sens(wm, &block1, &block2);

	wiiuse_write_data(wm, WM_REG_IR_BLOCK1, (byte*)block1, 9);
	wiiuse_write_data(wm, WM_REG_IR_BLOCK2, (byte*)block2, 2);

	WIIUSE_DEBUG("Set IR sensitivity to level %i (unid %i)", level, wm->unid);
}


/**
 *	@brief Calculate the data from the IR spots.  Basic IR mode.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param data		Data returned by the wiimote for the IR spots.
 */
void calculate_basic_ir(struct wiimote_t* wm, byte* data) {
	struct ir_dot_t* dot = wm->ir.dot;
	int i;

	dot[0].rx = 1023 - (data[0] | ((data[2] & 0x30) << 4));
	dot[0].ry = data[1] | ((data[2] & 0xC0) << 2);

	dot[1].rx = 1023 - (data[3] | ((data[2] & 0x03) << 8));
	dot[1].ry = data[4] | ((data[2] & 0x0C) << 6);

	dot[2].rx = 1023 - (data[5] | ((data[7] & 0x30) << 4));
	dot[2].ry = data[6] | ((data[7] & 0xC0) << 2);

	dot[3].rx = 1023 - (data[8] | ((data[7] & 0x03) << 8));
	dot[3].ry = data[9] | ((data[7] & 0x0C) << 6);

	/* set each IR spot to visible if spot is in range */
	for (i = 0; i < 4; ++i) {
		if (dot[i].ry == 1023)
			dot[i].visible = 0;
		else {
			dot[i].visible = 1;
			dot[i].size = 0;		/* since we don't know the size, set it as 0 */
		}
	}

	interpret_ir_data(wm);
}


/**
 *	@brief Calculate the data from the IR spots.  Extended IR mode.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param data		Data returned by the wiimote for the IR spots.
 */
void calculate_extended_ir(struct wiimote_t* wm, byte* data) {
	struct ir_dot_t* dot = wm->ir.dot;
	int i;

	for (i = 0; i < 4; ++i) {
		dot[i].rx = 1023 - (data[3*i] | ((data[(3*i)+2] & 0x30) << 4));
		dot[i].ry = data[(3*i)+1] | ((data[(3*i)+2] & 0xC0) << 2);

		dot[i].size = data[(3*i)+2] & 0x0F;

		/* if in range set to visible */
		if (dot[i].ry == 1023)
			dot[i].visible = 0;
		else
			dot[i].visible = 1;
	}

	interpret_ir_data(wm);
}


/**
 *	@brief Interpret IR data into more user friendly variables.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 */
static void interpret_ir_data(struct wiimote_t* wm) {
	struct ir_dot_t* dot = wm->ir.dot;
	int i;
	float roll = 0.0f;
	int last_num_dots = wm->ir.num_dots;

	if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_ACC))
		roll = wm->orient.roll;

	/* count visible dots */
	wm->ir.num_dots = 0;
	for (i = 0; i < 4; ++i) {
		if (dot[i].visible)
			wm->ir.num_dots++;
	}

	switch (wm->ir.num_dots) {
		case 0:
		{
			wm->ir.state = 0;

			/* reset the dot ordering */
			for (i = 0; i < 4; ++i)
				dot[i].order = 0;

			wm->ir.x = 0;
			wm->ir.y = 0;
			wm->ir.z = 0.0f;

			return;
		}
		case 1:
		{
			fix_rotated_ir_dots(wm->ir.dot, roll);

			if (wm->ir.state < 2) {
				/*
				 *	Only 1 known dot, so use just that.
				 */
				for (i = 0; i < 4; ++i) {
					if (dot[i].visible) {
						wm->ir.x = dot[i].x;
						wm->ir.y = dot[i].y;

						wm->ir.ax = wm->ir.x;
						wm->ir.ay = wm->ir.y;

						/*	can't calculate yaw because we don't have the distance */
						//wm->orient.yaw = calc_yaw(&wm->ir);

						ir_convert_to_vres(&wm->ir.x, &wm->ir.y, wm->ir.aspect, wm->ir.vres[0], wm->ir.vres[1]);
						break;
					}
				}
			} else {
				/*
				 *	Only see 1 dot but know theres 2.
				 *	Try to estimate where the other one
				 *	should be and use that.
				 */
				for (i = 0; i < 4; ++i) {
					if (dot[i].visible) {
						int ox = 0;
						int x, y;

						if (dot[i].order == 1)
							/* visible is the left dot - estimate where the right is */
							ox = dot[i].x + wm->ir.distance;
						else if (dot[i].order == 2)
							/* visible is the right dot - estimate where the left is */
							ox = dot[i].x - wm->ir.distance;

						x = ((signed int)dot[i].x + ox) / 2;
						y = dot[i].y;

						wm->ir.ax = x;
						wm->ir.ay = y;
						wm->orient.yaw = calc_yaw(&wm->ir);

						if (ir_correct_for_bounds(&x, &y, wm->ir.aspect, wm->ir.offset[0], wm->ir.offset[1])) {
							ir_convert_to_vres(&x, &y, wm->ir.aspect, wm->ir.vres[0], wm->ir.vres[1]);
							wm->ir.x = x;
							wm->ir.y = y;
						}

						break;
					}
				}
			}

			break;
		}
		case 2:
		case 3:
		case 4:
		{
			/*
			 *	Two (or more) dots known and seen.
			 *	Average them together to estimate the true location.
			 */
			int x, y;
			wm->ir.state = 2;

			fix_rotated_ir_dots(wm->ir.dot, roll);

			/* if there is at least 1 new dot, reorder them all */
			if (wm->ir.num_dots > last_num_dots) {
				reorder_ir_dots(dot);
				wm->ir.x = 0;
				wm->ir.y = 0;
			}

			wm->ir.distance = ir_distance(dot);
			wm->ir.z = 1023 - wm->ir.distance;

			get_ir_dot_avg(wm->ir.dot, &x, &y);

			wm->ir.ax = x;
			wm->ir.ay = y;
			wm->orient.yaw = calc_yaw(&wm->ir);

			if (ir_correct_for_bounds(&x, &y, wm->ir.aspect, wm->ir.offset[0], wm->ir.offset[1])) {
				ir_convert_to_vres(&x, &y, wm->ir.aspect, wm->ir.vres[0], wm->ir.vres[1]);
				wm->ir.x = x;
				wm->ir.y = y;
			}

			break;
		}
		default:
		{
			break;
		}
	}

	#ifdef WITH_WIIUSE_DEBUG
	{
	int ir_level;
	WIIUSE_GET_IR_SENSITIVITY(wm, &ir_level);
	WIIUSE_DEBUG("IR sensitivity: %i", ir_level);
	WIIUSE_DEBUG("IR visible dots: %i", wm->ir.num_dots);
	for (i = 0; i < 4; ++i)
		if (dot[i].visible)
			WIIUSE_DEBUG("IR[%i][order %i] (%.3i, %.3i) -> (%.3i, %.3i)", i, dot[i].order, dot[i].rx, dot[i].ry, dot[i].x, dot[i].y);
	WIIUSE_DEBUG("IR[absolute]: (%i, %i)", wm->ir.x, wm->ir.y);
	}
	#endif
}



/**
 *	@brief Fix the rotation of the IR dots.
 *
 *	@param dot		An array of 4 ir_dot_t objects.
 *	@param ang		The roll angle to correct by (-180, 180)
 *
 *	If there is roll then the dots are rotated
 *	around the origin and give a false cursor
 *	position. Correct for the roll.
 *
 *	If the accelerometer is off then obviously
 *	this will not do anything and the cursor
 *	position may be inaccurate.
 */
static void fix_rotated_ir_dots(struct ir_dot_t* dot, float ang) {
	float s, c;
	int x, y;
	int i;

	if (!ang) {
		for (i = 0; i < 4; ++i) {
			dot[i].x = dot[i].rx;
			dot[i].y = dot[i].ry;
		}
		return;
	}

	s = sin(DEGREE_TO_RAD(ang));
	c = cos(DEGREE_TO_RAD(ang));

	/*
	 *	[ cos(theta)  -sin(theta) ][ ir->rx ]
	 *	[ sin(theta)  cos(theta)  ][ ir->ry ]
	 */

	for (i = 0; i < 4; ++i) {
		if (!dot[i].visible)
			continue;

		x = dot[i].rx - (1024/2);
		y = dot[i].ry - (768/2);

		dot[i].x = (c * x) + (-s * y);
		dot[i].y = (s * x) + (c * y);

		dot[i].x += (1024/2);
		dot[i].y += (768/2);
	}
}


/**
 *	@brief Average IR dots.
 *
 *	@param dot		An array of 4 ir_dot_t objects.
 *	@param x		[out] Average X
 *	@param y		[out] Average Y
 */
static void get_ir_dot_avg(struct ir_dot_t* dot, int* x, int* y) {
	int vis = 0, i = 0;

	*x = 0;
	*y = 0;

	for (; i < 4; ++i) {
		if (dot[i].visible) {
			*x += dot[i].x;
			*y += dot[i].y;
			++vis;
		}
	}

	*x /= vis;
	*y /= vis;
}


/**
 *	@brief Reorder the IR dots.
 *
 *	@param dot		An array of 4 ir_dot_t objects.
 */
static void reorder_ir_dots(struct ir_dot_t* dot) {
	int i, j, order;

	/* reset the dot ordering */
	for (i = 0; i < 4; ++i)
		dot[i].order = 0;

	for (order = 1; order < 5; ++order) {
		i = 0;

		for (; !dot[i].visible || dot[i].order; ++i)
		if (i > 4)
			return;

		for (j = 0; j < 4; ++j) {
			if (dot[j].visible && !dot[j].order && (dot[j].x < dot[i].x))
				i = j;
		}

		dot[i].order = order;
	}
}


/**
 *	@brief Calculate the distance between the first 2 visible IR dots.
 *
 *	@param dot		An array of 4 ir_dot_t objects.
 */
static float ir_distance(struct ir_dot_t* dot) {
	int i1, i2;
	int xd, yd;

	for (i1 = 0; i1 < 4; ++i1)
		if (dot[i1].visible)
			break;
	if (i1 == 4)
		return 0.0f;

	for (i2 = i1+1; i2 < 4; ++i2)
		if (dot[i2].visible)
			break;
	if (i2 == 4)
		return 0.0f;

	xd = dot[i2].x - dot[i1].x;
	yd = dot[i2].y - dot[i1].y;

	return sqrt(xd*xd + yd*yd);
}


/**
 *	@brief Correct for the IR bounding box.
 *
 *	@param x		[out] The current X, it will be updated if valid.
 *	@param y		[out] The current Y, it will be updated if valid.
 *	@param aspect	Aspect ratio of the screen.
 *	@param offset_x	The X offset of the bounding box.
 *	@param offset_y	The Y offset of the bounding box.
 *
 *	@return Returns 1 if the point is valid and was updated.
 *
 *	Nintendo was smart with this bit. They sacrifice a little
 *	precision for a big increase in usability.
 */
static int ir_correct_for_bounds(int* x, int* y, enum aspect_t aspect, int offset_x, int offset_y) {
	int x0, y0;
	int xs, ys;

	if (aspect == WIIUSE_ASPECT_16_9) {
		xs = WM_ASPECT_16_9_X;
		ys = WM_ASPECT_16_9_Y;
	} else {
		xs = WM_ASPECT_4_3_X;
		ys = WM_ASPECT_4_3_Y;
	}

	x0 = ((1024 - xs) / 2) + offset_x;
	y0 = ((768 - ys) / 2) + offset_y;

	if ((*x >= x0)
		&& (*x <= (x0 + xs))
		&& (*y >= y0)
		&& (*y <= (y0 + ys)))
	{
		*x -= offset_x;
		*y -= offset_y;

		return 1;
	}

	return 0;
}


/**
 *	@brief Interpolate the point to the user defined virtual screen resolution.
 */
static void ir_convert_to_vres(int* x, int* y, enum aspect_t aspect, int vx, int vy) {
	int xs, ys;

	if (aspect == WIIUSE_ASPECT_16_9) {
		xs = WM_ASPECT_16_9_X;
		ys = WM_ASPECT_16_9_Y;
	} else {
		xs = WM_ASPECT_4_3_X;
		ys = WM_ASPECT_4_3_Y;
	}

	*x -= ((1024-xs)/2);
	*y -= ((768-ys)/2);

	*x = (*x / (float)xs) * vx;
	*y = (*y / (float)ys) * vy;
}


/**
 *	@brief Calculate yaw given the IR data.
 *
 *	@param ir	IR data structure.
 */
float calc_yaw(struct ir_t* ir) {
	float x;

	x = ir->ax - 512;
	x = x * (ir->z / 1024.0f);

	return RAD_TO_DEGREE( atanf(x / ir->z) );
}
