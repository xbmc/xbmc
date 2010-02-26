/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef _decimal_h
#define _decimal_h

typedef enum
{TRUNCATE=0, HALF_EVEN, HALF_UP, CEILING, FLOOR}
  decimal_round_mode;
typedef int32 decimal_digit_t;

typedef struct st_decimal_t {
  int    intg, frac, len;
  my_bool sign;
  decimal_digit_t *buf;
} decimal_t;

int internal_str2dec(const char *from, decimal_t *to, char **end,
                     my_bool fixed);
int decimal2string(decimal_t *from, char *to, int *to_len,
                   int fixed_precision, int fixed_decimals,
                   char filler);
int decimal2ulonglong(decimal_t *from, ulonglong *to);
int ulonglong2decimal(ulonglong from, decimal_t *to);
int decimal2longlong(decimal_t *from, longlong *to);
int longlong2decimal(longlong from, decimal_t *to);
int decimal2double(decimal_t *from, double *to);
int double2decimal(double from, decimal_t *to);
int decimal_actual_fraction(decimal_t *from);
int decimal2bin(decimal_t *from, uchar *to, int precision, int scale);
int bin2decimal(const uchar *from, decimal_t *to, int precision, int scale);

int decimal_size(int precision, int scale);
int decimal_bin_size(int precision, int scale);
int decimal_result_size(decimal_t *from1, decimal_t *from2, char op,
                        int param);

int decimal_intg(decimal_t *from);
int decimal_add(decimal_t *from1, decimal_t *from2, decimal_t *to);
int decimal_sub(decimal_t *from1, decimal_t *from2, decimal_t *to);
int decimal_cmp(decimal_t *from1, decimal_t *from2);
int decimal_mul(decimal_t *from1, decimal_t *from2, decimal_t *to);
int decimal_div(decimal_t *from1, decimal_t *from2, decimal_t *to,
                int scale_incr);
int decimal_mod(decimal_t *from1, decimal_t *from2, decimal_t *to);
int decimal_round(decimal_t *from, decimal_t *to, int new_scale,
                  decimal_round_mode mode);
int decimal_is_zero(decimal_t *from);
void max_decimal(int precision, int frac, decimal_t *to);

#define string2decimal(A,B,C) internal_str2dec((A), (B), (C), 0)
#define string2decimal_fixed(A,B,C) internal_str2dec((A), (B), (C), 1)

/* set a decimal_t to zero */

#define decimal_make_zero(dec)        do {                \
                                        (dec)->buf[0]=0;    \
                                        (dec)->intg=1;      \
                                        (dec)->frac=0;      \
                                        (dec)->sign=0;      \
                                      } while(0)

/*
  returns the length of the buffer to hold string representation
  of the decimal (including decimal dot, possible sign and \0)
*/

#define decimal_string_size(dec) (((dec)->intg ? (dec)->intg : 1) + \
				  (dec)->frac + ((dec)->frac > 0) + 2)

/* negate a decimal */
#define decimal_neg(dec) do { (dec)->sign^=1; } while(0)

/*
  conventions:

    decimal_smth() == 0     -- everything's ok
    decimal_smth() <= 1     -- result is usable, but precision loss is possible
    decimal_smth() <= 2     -- result can be unusable, most significant digits
                               could've been lost
    decimal_smth() >  2     -- no result was generated
*/

#define E_DEC_OK                0
#define E_DEC_TRUNCATED         1
#define E_DEC_OVERFLOW          2
#define E_DEC_DIV_ZERO          4
#define E_DEC_BAD_NUM           8
#define E_DEC_OOM              16

#define E_DEC_ERROR            31
#define E_DEC_FATAL_ERROR      30

#endif

