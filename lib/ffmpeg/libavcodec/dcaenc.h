/*
 * DCA encoder tables
 * Copyright (C) 2008 Alexander E. Patrakov
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_DCAENC_H
#define AVCODEC_DCAENC_H

/* This is a scaled version of the response of the reference decoder to
   this vector of subband samples: ( 1.0 0.0 0.0 ... 0.0 )
   */

static const int32_t UnQMF[512] = {
    7,
    4,
    -961,
    -2844,
    -8024,
    -18978,
    -32081,
    -15635,
    -16582,
    -18359,
    -17180,
    -14868,
    -11664,
    -8051,
    -4477,
    -1327,
    -1670,
    -6019,
    -11590,
    -18030,
    -24762,
    -30965,
    -35947,
    -36145,
    -37223,
    -86311,
    -57024,
    -27215,
    -11274,
    -4684,
    42,
    108,
    188,
    250,
    -1007,
    -596,
    -2289,
    -12218,
    -27191,
    -124367,
    -184256,
    -250538,
    -323499,
    -397784,
    -468855,
    -532072,
    -583000,
    -618041,
    -777916,
    -783868,
    -765968,
    -724740,
    -662468,
    -583058,
    -490548,
    -401623,
    -296090,
    -73154,
    -36711,
    -7766,
    -2363,
    -4905,
    2388,
    2681,
    5651,
    4086,
    71110,
    139742,
    188067,
    151237,
    101355,
    309917,
    343690,
    358839,
    357555,
    334606,
    289625,
    224152,
    142063,
    48725,
    74996,
    238425,
    411666,
    584160,
    744276,
    880730,
    983272,
    1041933,
    1054396,
    789531,
    851022,
    864032,
    675431,
    418134,
    35762,
    66911,
    103502,
    136403,
    -55147,
    -245269,
    -499595,
    -808470,
    -1136858,
    -2010912,
    -2581654,
    -3151901,
    -3696328,
    -4196599,
    -4633761,
    -4993229,
    -5262495,
    -5436311,
    -477650,
    -901314,
    -1308090,
    -1677468,
    -1985525,
    -2212848,
    -2341196,
    -2373915,
    -2269552,
    -2620489,
    -2173858,
    -1629954,
    -946595,
    -193499,
    1119459,
    1138657,
    1335311,
    1126544,
    2765033,
    3139603,
    3414913,
    3599213,
    3676363,
    3448981,
    3328726,
    3111551,
    2810887,
    2428657,
    1973684,
    1457278,
    893848,
    300995,
    -292521,
    -867621,
    -1404936,
    -1871278,
    -2229831,
    -2440932,
    -2462684,
    -2255006,
    -1768898,
    -1079574,
    82115,
    1660302,
    3660715,
    6123610,
    8329598,
    11888744,
    15722147,
    19737089,
    25647773,
    31039399,
    36868007,
    43124253,
    49737161,
    56495958,
    63668945,
    71039511,
    78540240,
    86089058,
    93600041,
    100981151,
    108136061,
    114970055,
    121718321,
    127566038,
    132774642,
    137247294,
    140894737,
    143635018,
    145395599,
    146114032,
    145742999,
    144211606,
    141594341,
    137808404,
    132914122,
    126912246,
    120243281,
    112155281,
    103338368,
    93904953,
    83439152,
    72921548,
    62192990,
    51434918,
    40894003,
    30786726,
    21384955,
    12939112,
    5718193,
    -5790,
    -3959261,
    -5870978,
    -5475538,
    -2517061,
    3247310,
    12042937,
    24076729,
    39531397,
    58562863,
    81297002,
    107826748,
    138209187,
    172464115,
    210569037,
    252468018,
    298045453,
    347168648,
    399634888,
    455137189,
    513586535,
    574537650,
    637645129,
    702597163,
    768856566,
    836022040,
    903618096,
    971159680,
    1038137214,
    1103987353,
    1168195035,
    1230223053,
    1289539180,
    1345620373,
    1397957958,
    1446063657,
    1489474689,
    1527740502,
    1560502307,
    1587383079,
    1608071145,
    1622301248,
    1629859340,
    1630584888,
    1624373875,
    1611178348,
    1591018893,
    1563948667,
    1530105004,
    1489673227,
    1442904075,
    1390107674,
    1331590427,
    1267779478,
    1199115126,
    1126053392,
    1049146257,
    968928307,
    885965976,
    800851610,
    714186243,
    626590147,
    538672486,
    451042824,
    364299927,
    279026812,
    195785029,
    115109565,
    37503924,
    -36564551,
    -106668063,
    -172421668,
    -233487283,
    -289575706,
    -340448569,
    -385919511,
    -425854915,
    -460174578,
    -488840702,
    -511893328,
    -529405118,
    -541489888,
    -548312207,
    -550036471,
    -547005316,
    -539436808,
    -527630488,
    -512084785,
    -492941605,
    -470665204,
    -445668379,
    -418328829,
    -389072810,
    -358293846,
    -326396227,
    -293769619,
    -260792276,
    -227825056,
    -195208961,
    -163262121,
    -132280748,
    -102533727,
    -74230062,
    -47600637,
    -22817785,
    -25786,
    20662895,
    39167253,
    55438413,
    69453741,
    81242430,
    90795329,
    98213465,
    103540643,
    106917392,
    108861938,
    108539682,
    106780704,
    103722568,
    99043289,
    93608686,
    87266209,
    80212203,
    72590022,
    64603428,
    56362402,
    48032218,
    39749162,
    31638971,
    23814664,
    16376190,
    9409836,
    2988017,
    -2822356,
    -7976595,
    -12454837,
    -16241147,
    -19331944,
    -21735011,
    -23468284,
    -24559822,
    -25042936,
    -25035583,
    -24429587,
    -23346408,
    -21860411,
    -20015718,
    -17025330,
    -14968728,
    -12487138,
    -9656319,
    -7846681,
    -5197816,
    -2621904,
    -144953,
    2144746,
    3990570,
    5845884,
    7454650,
    8820394,
    9929891,
    10784445,
    11390921,
    11762056,
    11916017,
    12261189,
    12117604,
    11815303,
    11374622,
    10815301,
    10157241,
    9418799,
    8629399,
    7780776,
    7303680,
    6353499,
    5392738,
    4457895,
    3543062,
    1305978,
    1402521,
    1084092,
    965652,
    -151008,
    -666667,
    -1032157,
    -1231475,
    -1319043,
    -1006023,
    -915720,
    -773426,
    -612377,
    -445864,
    -291068,
    -161337,
    -66484,
    -11725,
    133453,
    388184,
    615856,
    804033,
    942377,
    1022911,
    1041247,
    995854,
    891376,
    572246,
    457992,
    316365,
    172738,
    43037,
    -117662,
    -98542,
    -70279,
    -41458,
    -535790,
    -959038,
    -1364456,
    -1502265,
    -1568530,
    -2378681,
    -2701111,
    -2976407,
    -3182552,
    -3314415,
    -3366600,
    -3337701,
    -3232252,
    -3054999,
    1984841,
    1925903,
    1817377,
    1669153,
    1490069,
    1292040,
    1086223,
    890983,
    699163,
    201358,
    266971,
    296990,
    198419,
    91119,
    4737,
    5936,
    2553,
    2060,
    -3828,
    -1664,
    -4917,
    -20796,
    -36822,
    -131247,
    -154923,
    -162055,
    -161354,
    -148762,
    -125754,
    -94473,
    -57821,
    -19096,
    15172,
    43004,
    65624,
    81354,
    89325,
    89524,
    82766,
    71075,
    55128,
    13686,
    6921,
    1449,
    420,
    785,
    -215,
    -179,
    -113,
    -49,
    6002,
    16007,
    42978,
    100662,
    171472,
    83975,
    93702,
    108813,
    111893,
    110272,
    103914,
    93973,
    81606,
    68041,
    -54058,
    -60695,
    -65277,
    -67224,
    -66213,
    -62082,
    -55574,
    -42988,
    -35272,
    -63735,
    -33501,
    -12671,
    -4038,
    -1232,
    5,
    7
};

#endif /* AVCODEC_DCAENC_H */
