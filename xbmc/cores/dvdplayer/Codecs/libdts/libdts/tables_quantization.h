/*
 * tables_quantization.h
 * Copyright (C) 2004 Gildas Bazin <gbazin@videolan.org>
 *
 * This file is part of dtsdec, a free DTS Coherent Acoustics stream decoder.
 * See http://www.videolan.org/dtsdec.html for updates.
 *
 * dtsdec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dtsdec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

int scale_factor_quant6[] =
{
        1,       2,       2,       3,       3,       4,       6,       7, 
       10,      12,      16,      20,      26,      34,      44,      56, 
       72,      93,     120,     155,     200,     257,     331,     427, 
      550,     708,     912,    1175,    1514,    1950,    2512,    3236, 
     4169,    5370,    6918,    8913,   11482,   14791,   19055,   24547, 
    31623,   40738,   52481,   67608,   87096,  112202,  144544,  186209, 
   239883,  309030,  398107,  512861,  660693,  851138, 1096478, 1412538, 
  1819701, 2344229, 3019952, 3890451, 5011872, 6456542, 8317638,       0
};

int scale_factor_quant7[] =
{
        1,       1,       2,       2,       2,       2,       3,       3, 
        3,       4,       4,       5,       6,       7,       7,       8, 
       10,      11,      12,      14,      16,      18,      20,      23, 
       26,      30,      34,      38,      44,      50,      56,      64, 
       72,      82,      93,     106,     120,     136,     155,     176, 
      200,     226,     257,     292,     331,     376,     427,     484, 
      550,     624,     708,     804,     912,    1035,    1175,    1334, 
     1514,    1718,    1950,    2213,    2512,    2851,    3236,    3673, 
     4169,    4732,    5370,    6095,    6918,    7852,    8913,   10116, 
    11482,   13032,   14791,   16788,   19055,   21627,   24547,   27861, 
    31623,   35892,   40738,   46238,   52481,   59566,   67608,   76736, 
    87096,   98855,  112202,  127350,  144544,  164059,  186209,  211349, 
   239883,  272270,  309030,  350752,  398107,  451856,  512861,  582103, 
   660693,  749894,  851138,  966051, 1096478, 1244515, 1412538, 1603245, 
  1819701, 2065380, 2344229, 2660725, 3019952, 3427678, 3890451, 4415704, 
  5011872, 5688529, 6456542, 7328245, 8317638,       0,       0,       0
};

/* 20bits unsigned fractional binary codes */
int lossy_quant[] =
{
        0, 6710886, 4194304, 3355443, 2474639, 2097152, 1761608, 1426063, 
   796918,  461373,  251658,  146801,   79692,   46137,   27263,   16777, 
    10486,    5872,    3355,    1887,    1258,     713,     336,     168, 
       84,      42,      21,       0,       0,       0,       0,       0
};

double lossy_quant_d[] =
{
          0,     1.6,      1.0,     0.8,    0.59,    0.50,    0.42,    0.34, 
       0.19,    0.11,     0.06,   0.035,   0.019,   0.011,  0.0065,  0.0040, 
     0.0025,  0.0014,   0.0008, 0.00045, 0.00030, 0.00017, 0.00008, 0.00004, 
    0.00002, 0.00001, 0.000005,       0,       0,       0,       0,       0
};

/* 20bits unsigned fractional binary codes */
int lossless_quant[] =
{
        0, 4194304, 2097152, 1384120, 1048576,  696254,  524288,  348127, 
   262144,  131072,   65431,   33026,   16450,    8208,    4100,    2049, 
     1024,     512,     256,     128,      64,      32,      16,       8, 
        4,       2,       1,       0,       0,       0,       0,       0
};

double lossless_quant_d[] =
{
    0,             1.0,      0.5,     0.33,     0.25,    0.166,    0.125,
    0.083,      0.0625,  0.03125,   0.0156, 7.874E-3, 3.922E-3, 1.957E-3,
    9.775E-4, 4.885E-4, 2.442E-4, 1.221E-4, 6.104E-5, 3.052E-5, 1.526E-5,
    7.629E-6, 3.815E-6, 1.907E-6, 9.537E-7, 4.768E-7, 2.384E-7,        0,
           0,        0,        0,        0
};
