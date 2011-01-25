/* 
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    quantity.c

	string -> quantity -> native value convertion
	by Kentaro Sato	<kentaro@ranvis.com>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdlib.h>
#include <math.h>

#include "timidity.h"
#include "common.h"
#include "controls.h"
#include "instrum.h"
#include "output.h"
#include "playmidi.h"
#include "tables.h"

#define ___QUANTITY_C_
#include "quantity.h"

/*
	Guide To Add New Unit Types/Units
	
	append QUANTITY_UNIT_TYPE(<TYPE>)
	           QUANTITY_UNIT_NAME(<UNIT>)
	           ... to enum quantity_units (in quantity.h)
	append QUANTITY_TYPE_INT/FLOAT(<TYPE>)
	           REGISTER_TYPE_INT/FLOAT("<SUFFIX>", <UNIT>);
	           ...
	           END_QUANTITY_TYPE; to GetQuantityHints()
	write convert_<TYPE>_NUM(int32/FLOAT_T value, int32 param)
	          convert_<UNIT>(int32/FLOAT_T value, int32 param)
	          ... functions.
*/

/*************** conversion functions ***************/

static int32 convert_DIRECT_INT_NUM(int32 value, int32 param)
{
	return value;
}

static FLOAT_T convert_DIRECT_FLOAT_NUM(FLOAT_T value, int32 param)
{
	return value;
}

/* from instrum.c, convert_tremolo_sweep() */
static int32 convert_TREMOLO_SWEEP_NUM(int32 value, int32 param)
{
	uint8	sweep = value;
  if (!sweep)
    return 0;

  return
    ((control_ratio * SWEEP_TUNING) << SWEEP_SHIFT) /
      (play_mode->rate * sweep);
}

static int32 convert_TREMOLO_SWEEP_MS(int32 value, int32 param)
{
	if (value <= 0)
		return 0;
	#if SWEEP_SHIFT <= 16
	return ((uint32)(control_ratio * (1000 >> 2)) << SWEEP_SHIFT) / ((play_mode->rate * value) >> 2);
	#else
		#error "overflow"
	#endif
}

/* from instrum.c, convert_tremolo_rate() */
static int32 convert_TREMOLO_RATE_NUM(int32 value, int32 param)
{
	uint8	rate = value;
  return
    ((SINE_CYCLE_LENGTH * control_ratio * rate) << RATE_SHIFT) /
      (TREMOLO_RATE_TUNING * play_mode->rate);
}

static int32 convert_TREMOLO_RATE_MS(int32 value, int32 param)
{
	#if RATE_SHIFT <= 5
	return ((SINE_CYCLE_LENGTH * control_ratio * (1000 >> 1)) << RATE_SHIFT) /
		((play_mode->rate * (uint32)value) >> 1);
	#else
		#error "overflow"
	#endif
}

static FLOAT_T convert_TREMOLO_RATE_HZ(FLOAT_T value, int32 param)
{
	if (value <= 0)
		return 0;
	return ((SINE_CYCLE_LENGTH * control_ratio) << RATE_SHIFT) * value / play_mode->rate;
}

/* from instrum.c, convert_vibrato_sweep() */
static int32 convert_VIBRATO_SWEEP_NUM(int32 value, int32 vib_control_ratio)
{
	uint8	sweep = value;
  if (!sweep)
    return 0;

  return (int32)(TIM_FSCALE((double) (vib_control_ratio)
			    * SWEEP_TUNING, SWEEP_SHIFT)
		 / (double)(play_mode->rate * sweep));

  /* this was overflowing with seashore.pat

      ((vib_control_ratio * SWEEP_TUNING) << SWEEP_SHIFT) /
      (play_mode->rate * sweep); */
}

static int32 convert_VIBRATO_SWEEP_MS(int32 value, int32 vib_control_ratio)
{
	if (value <= 0)
		return 0;
	return (TIM_FSCALE((double)vib_control_ratio * 1000, SWEEP_SHIFT)
		/ (double)(play_mode->rate * value));
}

/* from instrum.c, to_control() */
static int32 convert_VIBRATO_RATE_NUM(int32 control, int32 param)
{
	return (int32) (0x2000 / pow(2.0, control / 31.0));
}

static int32 convert_VIBRATO_RATE_MS(int32 value, int32 param)
{
	return 1000 * play_mode->rate / ((2 * VIBRATO_SAMPLE_INCREMENTS) * value);
}

static FLOAT_T convert_VIBRATO_RATE_HZ(FLOAT_T value, int32 param)
{
	return play_mode->rate / ((2 * VIBRATO_SAMPLE_INCREMENTS) * value);
}

/*************** core functions ***************/

#define MAX_QUANTITY_UNITS_PER_UNIT_TYPES	8

static int GetQuantityHints(uint16 type, QuantityHint *units)
{
	QuantityHint		*unit;
	
	unit = units;
	#define QUANTITY_TYPE_INT(type)	\
			case QUANTITY_UNIT_TYPE(type):		REGISTER_TYPE_INT("", type##_NUM)
	#define QUANTITY_TYPE_FLOAT(type)	\
			case QUANTITY_UNIT_TYPE(type):		REGISTER_TYPE_FLOAT("", type##_NUM)
	#define REGISTER_TYPE_INT(ustr, utype)					REGISTER_TYPE_ENTITY_INT(ustr, utype, convert_##utype)
	#define REGISTER_TYPE_FLOAT(ustr, utype)				REGISTER_TYPE_ENTITY_FLOAT(ustr, utype, convert_##utype)
	#define REGISTER_TYPE_ALIAS_INT(ustr, utype, atype)		REGISTER_TYPE_ENTITY_INT(ustr, utype, convert_##atype)
	#define REGISTER_TYPE_ALIAS_FLOAT(ustr, utype, atype)	REGISTER_TYPE_ENTITY_FLOAT(ustr, utype, convert_##atype)
	#define REGISTER_TYPE_ENTITY_INT(ustr, utype, ucvt)	\
				unit->suffix = ustr, unit->type = type, unit->id = QUANTITY_UNIT_NAME(utype), unit->float_type = 0, unit->convert.i = ucvt, unit++
	#define REGISTER_TYPE_ENTITY_FLOAT(ustr, utype, ucvt)	\
				unit->suffix = ustr, unit->type = type, unit->id = QUANTITY_UNIT_NAME(utype), unit->float_type = 1, unit->convert.f = ucvt, unit++
	#define END_QUANTITY_TYPE		unit->suffix = NULL; break
	switch (type)
	{
		QUANTITY_TYPE_INT(DIRECT_INT);
			END_QUANTITY_TYPE;
		QUANTITY_TYPE_FLOAT(DIRECT_FLOAT);
			END_QUANTITY_TYPE;
		QUANTITY_TYPE_INT(TREMOLO_SWEEP);
			REGISTER_TYPE_INT("ms", TREMOLO_SWEEP_MS);
			END_QUANTITY_TYPE;
		QUANTITY_TYPE_INT(TREMOLO_RATE);
			REGISTER_TYPE_INT("ms", TREMOLO_RATE_MS);
			REGISTER_TYPE_FLOAT("Hz", TREMOLO_RATE_HZ);
			END_QUANTITY_TYPE;
		QUANTITY_TYPE_INT(VIBRATO_RATE);
			REGISTER_TYPE_INT("ms", VIBRATO_RATE_MS);
			REGISTER_TYPE_FLOAT("Hz", VIBRATO_RATE_HZ);
			END_QUANTITY_TYPE;
		QUANTITY_TYPE_INT(VIBRATO_SWEEP);
			REGISTER_TYPE_INT("ms", VIBRATO_SWEEP_MS);
			END_QUANTITY_TYPE;
		default:
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Internal parameter error (%d)", type);
			return 0;
	}
	return 1;
}

/* quantity is unchanged if an error occurred */
static const char *number_to_quantity(int32 number_i, const char *suffix_i, FLOAT_T number_f, const char *suffix_f, Quantity *quantity, uint16 type)
{
	QuantityHint		units[MAX_QUANTITY_UNITS_PER_UNIT_TYPES], *unit;
	
	if (!GetQuantityHints(type, units))
		return "Parameter error";
	unit = units;
	while(unit->suffix != NULL)
	{
		if (suffix_i != NULL && strcmp(suffix_i, unit->suffix) == 0)	/* number_i, suffix_i was valid */
		{
			quantity->type = unit->type;
			quantity->unit = unit->id;
			if (unit->float_type)
				quantity->value.f = number_i;
			else
				quantity->value.i = number_i;
			return NULL;
		}
		else if (suffix_f != NULL && strcmp(suffix_f, unit->suffix) == 0)	/* number_f, suffix_f was valid */
		{
			if (unit->float_type)
			{
				quantity->type = unit->type;
				quantity->unit = unit->id;
				quantity->value.f = number_f;
				return NULL;
			}
			else
				return "integer expected";
		}
		unit++;
	}
	return "invalid parameter";
}

const char *string_to_quantity(const char *string, Quantity *quantity, uint16 type)
{
	int32				number_i;
	FLOAT_T				number_f;
	char				*suffix_i, *suffix_f;
	
	number_i = strtol(string, &suffix_i, 10);	/* base == 10 for compatibility with atoi() */
	if (string == suffix_i)	/* doesn't start with valid character */
		return "Number expected";
	number_f = strtod(string, &suffix_f);
	return number_to_quantity(number_i, suffix_i, number_f, suffix_f, quantity, type);
}

void int_to_quantity(int32 number, Quantity *quantity, uint16 type)
{
	/* pass suffix_f NULL to warn if unit type is float type */
	if (number_to_quantity(number, "", number, NULL, quantity, type) != NULL)	/* error */
	{
		quantity->type = QUANTITY_UNIT_TYPE(DIRECT_INT);
		quantity->unit = QUANTITY_UNIT_NAME(DIRECT_INT_NUM);
		quantity->value.i = 0;
	}
}

void float_to_quantity(FLOAT_T number, Quantity *quantity, uint16 type)
{
	/* pass suffix_i NULL to warn if unit type is integer type */
	if (number_to_quantity(number, NULL, number, "", quantity, type) != NULL)	/* error */
	{
		quantity->type = QUANTITY_UNIT_TYPE(DIRECT_FLOAT);
		quantity->unit = QUANTITY_UNIT_NAME(DIRECT_FLOAT_NUM);
		quantity->value.f = 0;
	}
}

static int GetQuantityConvertProc(const Quantity *quantity, QuantityConvertProc *proc)
{
	QuantityHint		units[MAX_QUANTITY_UNITS_PER_UNIT_TYPES], *unit;
	
	if (!GetQuantityHints(quantity->type, units))
		return -1;	/* already warned */
	unit = units;
	while(unit->suffix != NULL)
	{
		if (quantity->unit == unit->id)
		{
			*proc = unit->convert;
			return unit->float_type;
		}
		unit++;
	}
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Internal parameter error");
	return -1;
}

int32 quantity_to_int(const Quantity *quantity, int32 param)
{
	QuantityConvertProc	proc;
	
	switch (GetQuantityConvertProc(quantity, &proc))
	{
		case 0:
			return (*proc.i)(quantity->value.i, param);
		case 1:
			return (*proc.f)(quantity->value.f, param);
	}
	return 0;
}

FLOAT_T quantity_to_float(const Quantity *quantity, int32 param)
{
	QuantityConvertProc	proc;
	
	switch (GetQuantityConvertProc(quantity, &proc))
	{
		case 0:
			return (*proc.i)(quantity->value.i, param);
		case 1:
			return (*proc.f)(quantity->value.f, param);
	}
	return 0;
}
