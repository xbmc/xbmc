// Generated with https://git.fruit.je/src?a=blob;f=dither/dither.c;h=913d06c11f8d7cbd24121ecd772396b7c10f4826;hb=HEAD

#include <stdint.h>

static const int dither_size = 64;
static const int dither_size2 = 4096;
static const uint16_t dither_matrix[] = {
	   3, 2603,  743, 2361,  199, 2711,  806, 3181, 2113, 3820,  988, 3485, 1560, 2650,  327, 3063, 1660, 3340,  356, 1055, 2218, 2796, 3278,  528, 2104, 2909, 1465, 4076,  911, 3358, 1586, 3039,   41, 4038,  690, 3493,  155, 3754,  636, 3403,  271, 1355, 2973, 1816, 1287, 3908, 1095, 2549, 4086,  984, 3452,  357, 3560, 1662,   39, 1362, 2954, 1538,  552, 2823, 1871,  319, 2964,  792, 
	3590, 1799, 4095, 1197, 3302, 2009, 3974, 1602,  475, 1433, 2801, 2059,  551, 4067, 1916,  914, 3887, 1308, 2969, 1817, 3780,  790, 1262, 1828, 3421,  802, 2454,  126, 3006, 2212,  536, 2513, 1029, 2142, 2920, 1641, 3178, 1902, 2848,  966, 2579, 3567, 2252,  142, 2798,  388, 3014, 2005,  216, 2285, 1583, 2994, 1091, 2813, 2002, 3712,  856, 4069, 2292, 1256, 3682, 2405, 1359, 3225, 
	1046, 2855,  404, 2646, 1698,  545, 1138, 2982, 1985, 3610,  133, 3256, 1794,  971, 3167, 2251,  483, 2613,  668, 3468,   75, 3156, 2398, 3838,  201, 2690, 1060, 3599, 1820, 1168, 3890, 1743, 3564, 1487,  372, 2617, 1078,  462, 2193, 3915, 1659,  567, 1123, 3806, 1897, 3623,  849, 1483, 3513, 2857,  777, 2164, 3993,  584, 3338,  461, 2748, 1806,  206, 3264,  729, 1993, 3925,  492, 
	2269, 1508, 3497,  927, 3743, 2109, 3469,  248, 2697, 1041, 2404, 1260, 3741, 2162,   52, 3526, 1214, 3722, 2085, 1449, 2557, 1938,  442, 1418, 2987, 1612, 3232, 2139,  654, 2884,  245, 2754,  621, 3060, 3858, 1341, 3698, 2948, 1436,   64, 3194, 2025, 3092, 1506,  595, 2216, 3252, 2632,  484, 1239, 3625,  122, 1864, 2589, 1475, 2360, 1195, 3588, 2536, 1010, 2890,  110, 2628, 1735, 
	3110,  217, 1939, 2913,  101, 2483, 1274, 3124, 1636, 3791,  810, 3049,  417, 2828, 1416, 2515, 1992,  219, 3088,  844, 3343, 1171, 3632, 2201,  637, 3939,  354, 1330, 3800, 1943, 3284, 1390, 2347,  947, 2047,  200, 2128,  739, 3511, 2707, 1177, 4051,  336, 2501, 3472, 1316,   17, 2087, 3801, 1892, 2466, 1409, 3241, 1031, 3844,  152, 3123,  631, 1485, 3835, 1664, 3396, 1181, 3569, 
	 890, 3790, 2353, 1098, 3229,  820, 3910,  448, 2319,  579, 2618, 1903, 3502,  873, 3988,  585, 3015, 1639, 2261, 4055,  292, 2809,  894, 3168, 1959,  995, 2627, 3128, 2282,  506, 1050, 3986,  125, 3537, 2804, 1533, 3291, 2523, 1860,  539, 2328,  829, 2866, 1965,  941, 2718, 4021, 1444,  877, 3210,  695, 3755,  391, 2980,  839, 3378, 1739, 2217, 3031,  351, 2334,  760, 2468,  413, 
	2833, 1382,  553, 4013, 1886, 2660, 1512, 2092, 3541, 1391, 3950,  202, 1543, 2705, 1734, 3312,  990, 3765,  607, 1317, 2458, 1813, 3906,  134, 2390, 3415, 1671,   38, 1218, 3668, 2535, 1552, 2657, 1883,  518, 4094, 1028,  304, 3884, 1303, 3557, 1680, 3406,  203, 3717, 1854,  422, 2304, 2944,  249, 2651, 1757, 2297, 1576, 2706, 2014,  524, 3970, 1139, 2063, 3639, 1417, 4053, 1763, 
	2150, 3450, 2616, 1247,  334, 3355,  650, 2955,   20, 3187,  885, 2178, 3352,  507, 2351,  162, 2735, 1492, 3254, 2030, 3544,  565, 1575, 2921, 1376,  547, 4037, 2017, 3337, 1730,  287, 3247,  660, 3048, 1201, 2383, 1935, 3138, 1594, 2902,  132, 2758, 1358, 2279,  763, 3203, 1574, 3527, 1157, 3938,  958, 3383, 1130, 4012,  234, 3545, 1335, 2558,   27, 3206,  651, 2949,  209, 3170, 
	 977,   50, 1647, 3087, 2189, 1733, 3768, 1331, 2012, 2470, 1484, 2999,  979, 3863, 1288, 3603, 2147,  447, 2876,    6, 1122, 2673, 3368,  837, 3769, 2529,  965, 2699,  575, 2914, 2149,  954, 3693, 1635, 3441,   11, 3725,  780, 2425,  953, 3797, 2171,  677, 3957, 2587, 1108, 2849,  656, 2129, 1650, 2881,   63, 2612,  735, 2348,  951, 3106, 1614, 3490, 1834, 1223, 2597, 1523, 2306, 
	3851, 1983, 3714,  599, 3524,  179, 2408,  904, 4050,  688, 3427,  369, 2671, 1994, 2931,  645, 1686, 3931, 1221, 2553, 3842, 2071,  305, 2276, 1829,  221, 3227, 1452, 3552, 1096, 3956, 1472, 2438,  335, 2187, 2889, 1428, 2746,  415, 3224, 1851, 1086, 2957,  350, 1764, 3393,  116, 2516, 3662,  398, 2209, 3716, 1310, 3464, 1480, 3762,  381, 2853,  772, 2374, 3943,  487, 3542,  616, 
	1302, 2963,  925, 2731, 1148, 2869, 1414, 3026,  310, 2740, 1744, 3699, 1573,   76, 1126, 3300, 2493,  878, 3438, 1849,  785, 1478, 3653, 2765, 1085, 3605, 2246,  383, 1878, 2479,  106, 2841,  801, 3901, 1842,  610, 3518, 1104, 3996, 2270,  549, 3492, 2105, 3640,  931, 2224, 3916, 1866,  816, 3193, 1024, 1845, 3161,  502, 2990, 1942, 2475, 1035, 3673,  243, 1977, 3097, 1679, 2121, 
	3318,  363, 2287, 1590, 3989,  500, 3453, 1796, 3659, 1240, 2331,  834, 3115, 2537, 4070, 2050,  212, 3079, 2267,  371, 2834, 3223, 1270,  527, 3130, 2052,  938, 2757, 3775,  740, 3471, 1656, 3281, 1062, 3102, 1349, 2512, 2090,  173, 1577, 3100, 1412,   40, 1931, 3152,  520, 1253, 2906, 1467, 2366, 4081,  254, 2089, 2522, 1174,  123, 4040, 2205, 1557, 3196, 1076, 2686,  103, 3920, 
	 850, 2585, 3651,  147, 1909, 2562,  989, 2275,  761, 3235,  186, 3852, 1212, 1888,  603, 1399, 3749, 1561,  711, 3969, 2138,  158, 1997, 4079, 1615,   65, 3913, 1778, 1301, 3072, 2083,  394, 2599, 1990,  154, 3818,  480, 3351, 1827, 3665,  853, 2734, 4066, 1128, 2538, 1619, 3257,  192, 3606,  557, 1625, 2793,  867, 3894, 1791, 3366, 1384,  472, 2779,  843, 3802, 1332, 2434, 1453, 
	2867, 1807, 1198, 2181, 3244, 1311, 3823,   81, 2649, 1381, 2776, 2046,  449, 3574, 2161, 3237, 1067, 2118, 2996, 1700, 1044, 3696, 2654,  759, 2455, 2966,  707, 3157,  256, 2362,  920, 4058,  671, 3601, 2263, 1528, 2976,  922, 2630,  373, 2432, 1971,  580, 3385,  289, 3770, 1960, 2598,  992, 2160, 3069, 1131, 3240,  370, 2745,  899, 3036, 1863, 3575, 2268,  399, 2953,  724, 3270, 
	 314, 3516,  658, 3795,  412, 2997,  809, 3349, 1771, 3999,  702, 3449, 1657, 2621,  280, 2795,  512, 3615,   66, 2531, 3420,  477, 1568, 3357, 1250, 2100, 3508, 1001, 2006, 3727, 1397, 2988, 1812, 1135, 2800,  812, 1922, 3587, 1395, 3935, 1167, 3055, 1554, 2231, 2858, 1203,  773, 3964, 1703, 3433,    5, 3812, 1918, 2393, 1457, 3778,  664, 2569,   53, 1468, 3387, 1727, 4077, 2222, 
	1037, 1966, 3089, 1367, 2762, 1694, 2355, 1087, 2837,  328, 2373, 1286, 3126,  961, 3928, 1746, 2430, 1230, 3177,  857, 1435, 2888, 2198,  264, 3777,  456, 1539, 2578, 3371,  592, 2655,   36, 2173, 3345,  297, 4010, 2397,   73, 2832,  661, 3310,  168, 3830,  691, 1420, 3547, 2305,  387, 2912,  742, 2643, 1544,  614, 3619,  164, 2172, 3316, 1259, 3978, 2042,  613, 2743,  210, 1413, 
	3680, 2637,   13, 2316,  709, 4059,  220, 3670, 1569, 3285,  903, 3786,   31, 2293, 1354, 3342,  738, 4033, 2221, 1877, 3879,  728, 3246, 1926, 1141, 2821, 4006,  156, 1193, 1907, 3231, 1507, 3753,  948, 3083, 1667,  718, 3142, 1780, 2278, 1338, 2747, 1716, 3139, 2136,   94, 3085, 1978, 1356, 3723, 1065, 2101, 3008, 1300, 2831, 1899,  455, 3113,  939, 2898, 3608, 1097, 2126, 3147, 
	 532, 1527, 3911, 1769, 3322, 1296, 2656, 2078,  563, 2000, 2586, 1714, 2926,  782, 2677,  181, 2938, 1535,  396, 2840,  197, 2435, 1032, 3626, 2542,  653, 2235, 1800, 2875, 3872,  353, 2456,  644, 2606, 1351, 2132, 3514, 1100, 3857,  324, 3451,  796, 2526,  419, 3747, 1766,  981, 3362, 2400,  187, 2728, 4023,  416, 3499,  881, 3849, 1521, 2431, 1841,  333, 2299, 1567, 3831,  964, 
	2402, 3411, 1068, 2543,  340, 3109,  907, 3460, 1372, 3942,  259, 3408, 1103, 3631, 1632, 3813, 1049, 2043, 3581, 1344, 3332, 1604, 2983,   21, 1663, 3305, 1272, 3664,  519, 2208, 1048, 3555, 2029, 3949,  465, 2896,  228, 2623, 1579, 2478, 1450, 4045, 1147, 3326,  909, 2573, 4003,  513, 1249, 3532, 1775,  910, 2340, 1717, 2610,  624, 3391,  194, 3912, 2704,  765, 3243,  108, 2919, 
	1689,  291, 2959,  826, 3779, 1626, 2250,   74, 2694,  847, 3013, 1460, 2401,  382, 2864,  627, 2463, 3140,  797, 2666,  515, 3990,  824, 2156, 3846,  341, 2911,  932, 2723, 1638, 3080, 1387,  149, 1748, 3266, 1202, 3711,  942, 3248,  744, 2970,   24, 2344, 1910, 2977,  232, 2107, 2892, 1850, 3047,  343, 2037, 3402,   85, 3220, 1278, 2200, 2854,  905, 1430, 3746, 1975, 2506,  686, 
	3997, 2247, 1255, 3484, 2152,  544, 3034, 3870, 1520, 3528, 2180,  606, 4090, 1781, 2123, 3354, 1591,   80, 3767, 1955, 1245, 2363, 1750, 3165, 1089, 2472, 1566, 3455,   83, 4085,  692, 2685, 3475, 2356,  741, 2769, 1693, 2314,  395, 3637, 2070, 1658, 3467,  481, 1407, 3660, 1531,  753, 3819, 1047, 2532, 3902, 1161, 2491, 1621, 4065,  433, 1676, 3573, 2226,  490, 1189, 3091, 1790, 
	 944, 3309, 1920,  135, 1440, 2799, 1859, 1115, 2427,  443, 1847, 2816,  993, 3041,  238, 1199, 3981, 2254,  949, 2900, 3456,  237, 3658,  548, 1961, 3024,  708, 2313, 1976, 1178, 3301, 2084,  556, 1321, 3880, 2015,   90, 4016, 1914, 2680,  970, 3118, 1194, 3930, 2508,  937, 3282, 2439,   57, 2777, 1578,  510, 3127,  769, 2939,  957, 2041, 3238,   28, 3019, 1890, 3389,  263, 3676, 
	2076,  474, 3067, 2450, 4043,  800, 3556,  250, 3365, 1306, 3841,  107, 3419, 1410, 3570, 2197,  499, 3093, 1831,  425, 1550, 2652,  924, 2414, 4030,  190, 3546, 1007, 3708, 2645,  290, 1607, 3728, 2591,  358, 3205, 1162, 2863, 1346,  633, 3874,  279, 2766,  766, 2082, 3025,  438, 1946, 3629,  808, 3473, 2307, 1343, 3796,  265, 2415, 3729, 1323, 2473,  793, 3836, 1350, 2782, 1070, 
	2624, 3860,  864, 1741,  437, 3098,  985, 2607, 1934, 2933,  828, 2703, 1889,  466, 2572, 1493, 2794, 1136, 3643, 2056, 3914, 1172, 3412, 1826, 1327, 3146, 1651, 2530,  440, 1486, 3073, 2242, 1033, 1815, 2972, 1515, 3463,  489, 2469, 3347, 1454, 2311, 1726, 3197,  124, 1622, 4091, 1156, 2170, 2946, 1835,  167, 2700, 1968, 3320, 1227,  469, 3121, 1018, 2727, 1761,  428, 1969, 3265, 
	  60, 1524, 2301, 3593, 2683, 1269, 2207, 3891,  593, 1564, 3633, 1176, 2375, 3745,  967, 3904,  752, 3313,  180, 2452,  701, 2820,   59, 2923,  467, 2163,  866, 3941, 1898, 3392,  657, 3991,    2, 3592,  831, 2378,  982, 3798, 2099,  140, 3062,  876, 3737, 1279, 3548, 2376,  725, 3344,  301, 1318, 3961, 1107, 3562,  578, 1517, 2861, 2184, 1643, 3922,  193, 3479, 2309, 4088,  840, 
	2487, 3498, 1235,  272, 2031, 3707,   33, 1394, 3148, 2120,  225, 3290,  731, 2044, 3208,    8, 2349, 1691, 2715, 1371, 3162, 1631, 2233, 3821, 1079, 3481, 2806,  112, 2929, 1054, 2420, 1446, 2847, 1933, 3180,  223, 2736, 1879, 1211, 3582, 1773, 2441,  424, 2648,  588, 1927, 2788, 1500, 3117, 2447,  602, 2880, 2190, 1856, 4015,   96, 3628,  604, 1913, 2882, 1319,  628, 1466, 3027, 
	1688,  564, 2732, 3236,  715, 1613, 3306, 2273,  430, 3980, 2548, 1627, 2865,  348, 1722, 2958, 1309, 3690,  860, 4072,  397, 3561,  634, 1438, 2674, 1870,  663, 2377, 1295, 3825,  274, 3487, 1173,  550, 2169, 4060,  756, 3263,  367, 2611,  623, 4032, 2067, 3356, 1064, 3885,  178, 3645,  818, 1809, 3377, 1456,  375, 3137,  859, 2682, 1073, 3400, 2437,  913, 3242, 2604, 3638,  325, 
	2206, 3898, 1982, 1084, 4018, 2521, 1020, 2856, 1832, 1234,  886, 3480, 1277, 4046, 2215, 1045, 3428,  283, 3021, 2027, 1284, 2642, 1912, 3262,  257, 3917, 1601, 3595, 2065,  716, 3212, 1633, 2687, 3703, 1021, 1674, 3029, 1378, 3896, 1609, 3018, 1365,   43, 1708, 2829, 1553, 2514, 1266, 2300, 3829,   14, 2525, 3720, 1292, 2380, 1718, 3050, 1464,  303, 3994, 2055,  114, 1923, 1142, 
	3184, 1353,  145, 3001, 2102,  313, 3517,  679, 3685, 2016, 2915,   95, 2459,  641, 3163,  454, 2600, 1767, 2277,  586, 3444,  163, 3764,  962, 2429, 1140, 3191,  320, 2981, 1753, 2564,  845, 2054,  307, 3304, 2232,   79, 2659,  827, 2294, 1026, 3489, 2560, 3804,  361, 3287,  687, 3053,  408, 1375, 2985, 1774,  778, 3272,  262, 3900,  517, 2103, 3151,  726, 1565, 3759, 2502, 3520, 
	 784, 2324, 3740, 1758,  955, 3131, 1546, 2413,  195, 3211,  786, 3757, 1785, 3535, 1434, 2049, 3882, 1124, 3735, 1459, 2554, 2079, 1505, 3040,  751, 2752, 1419, 2329,  972, 3736,  131, 3104, 3973, 1290, 2770,  672, 3448, 1762, 3694,  231, 3222,  521, 1963,  855, 2326, 1160, 3979, 1697, 3509, 2066,  617, 4052, 2290, 1600, 2810, 1158, 2570, 3549,  998, 2288, 2993, 1237,  503, 1777, 
	2860,  414, 2726,  570, 3649, 2256,  841, 3862, 1322, 2561, 1652, 2280, 1069, 2583,  170, 2886,  673, 2443,   68, 3334,  734, 3953,  364, 2214, 3677,   32, 4063,  573, 3279, 1469, 2407, 1911,  511, 2343, 1504, 3807, 1053, 2381, 1297, 2785, 1653, 2417, 3646, 1445, 3169, 1996,  113, 2771,  999, 2546, 3253, 1188,  153, 3641,  591, 3414, 1900,   46, 1782, 3814,  240, 2196, 3417,  969, 
	4047, 2114, 1481, 3381, 1952,   87, 2952, 2053, 3360,  589, 4022,  337, 3380,  833, 3960, 1603, 3293,  963, 3046, 1981, 2802,  991, 3174, 1824, 1164, 3132, 1620, 2676, 2081,  378, 3883, 1094, 2960, 3580,  174, 2034, 2878,  379, 3315,  612, 3983, 1238,  160, 2842,  562, 3404, 2399,  798, 3731,  269, 1562, 2907, 1973, 2631, 1403, 2320,  926, 3683, 2764, 1133, 3195, 1443, 2667, 1872, 
	   0, 3099, 1027, 2462, 1191, 3982, 1617,  365, 1118, 3077, 1389, 2713, 1811, 2995, 2127,  482, 2011, 3598, 1655,  470, 1369, 3565, 2332,  649, 2850, 1930,  794, 3572, 1120, 3394,  887, 2609, 1587,  733, 1846, 3228,  836, 3888, 2010, 2688,  945, 2956, 2143, 1677, 3704, 1282, 1855, 2968, 1431, 2308, 3919,  453, 3386,  720, 3972,  345, 3221, 2001,  431, 2426,  667, 3893,  377, 3617, 
	2039,  746, 3832,  296, 3286,  615, 2357, 3613, 1940, 2262,   37, 3578,  568, 1326, 3721, 1153, 2789,  270, 2541, 4035, 2203,  146, 1534, 3375,  306, 3871, 2186,  214, 2556, 1893, 3071,   56, 3350, 2074, 4089, 1186, 2626, 1491,   19, 1713, 3470,  390, 3803,  722, 2566,  299, 4075,  497, 3437, 1102, 1802, 2580, 1144, 1862, 2852, 1580, 2485, 1271, 4073, 1729, 2870, 1222, 2339,  872, 
	3297, 2551, 1294, 2897, 1737, 2719, 1313, 3042,  771, 3810, 1490, 2411, 1949, 2664,  130, 3116, 1494, 3787, 1246,  598, 3260, 1848, 3773, 1112, 2480,  946, 2941, 1532, 4009,  611, 1682, 3824, 1226, 2729,  285, 3028,  534, 3531, 2509, 3150, 1265, 2385, 1405, 3329, 1768, 3108, 1190, 1987, 2724,   54, 3577,  842, 3808, 2219,  119, 3751,  700, 2986,  884, 3482,  100, 3215, 1408, 2759, 
	1593,  478, 3558, 2062,  817, 3734,  207, 1797, 2581,  468, 3158,  815, 3945,  980, 3486, 2259,  770, 1788, 3185, 2370, 1030, 2903,  748, 2708, 1805, 3602,  530, 3218, 1248, 2238, 2894,  486, 2396,  851, 3622, 1666, 2354, 1137, 1947,  712, 4031,  902, 3009,   78, 2317,  803, 3533, 2486,  879, 3143, 2115, 3017,  317, 3209, 1244, 2058, 3407,  253, 2266, 1526, 2517,  767, 3657,  196, 
	4011, 1861, 2346,  109, 3086, 1011, 2112, 4061, 1229, 3447, 1683, 2767,  282, 2891, 1843,  494, 3372, 2639,   16, 1951, 3705,  312, 2068, 4002,   70, 1455, 2756, 2003,  157, 3454, 1117, 3687, 1366, 3207, 2144,  674, 3869,  182, 3671, 2188,  277, 2684, 1598, 3881, 1061, 2822,  198, 1616, 3760, 1393,  569, 1673, 2712, 1522, 3591,  861, 2575, 1752, 3288, 1025, 3948, 1814, 2134, 3038, 
	 933, 3239, 1213, 3897, 1582, 2423, 3182,  626, 2681,  136, 2327, 1127, 3612, 2179, 1182, 4093, 2095,  935, 3895,  697, 2528, 1597, 3033, 1152, 2359, 3325, 1016, 3799, 1756, 2593,  342, 2008, 2817,  121, 1497, 3280, 1760, 2786, 1285, 3084, 1732, 3597,  576, 1917, 3426, 1357, 3940, 2091,  376, 3335, 2013, 3877,  819, 2440,  401, 3070, 1345, 3866,  541, 2797,  288, 3333,  574, 1529, 
	2457,  405, 2846,  659, 3395,  346, 1184, 3483, 1595, 3853,  901, 3275, 1755,  680, 3056,  229, 1581, 2975, 1348, 3330, 1233, 3609,  485, 3435,  705, 2048,  322, 2461,  736, 3944, 1545, 3341,  936, 4017, 2614,  402, 2428,  776, 3348,  488, 2584, 1228, 2158, 3061,  318, 2391,  632, 3022, 2453, 1043, 2778,  141, 2961, 1175, 4044, 1919,   25, 2159, 3145, 1624, 2336, 1225, 2629, 3840, 
	1106, 3501, 2148, 1479, 2641, 1830, 2922, 1998,  504, 2096, 3005,  432, 2488, 3826, 1402, 2448, 3692,  366, 2225, 2772,  175, 2140, 2826, 1334, 2608, 3739, 1571, 3495, 2108, 1232, 2678,  559, 2325, 1905, 1145, 3503, 1360, 3955, 2124,  994, 3855,  151, 3370, 1013, 2737, 1709, 3630, 1462,  783, 4020, 1801, 1307, 3689, 2274,  629, 2670, 3538,  787, 1289, 3585,  917, 3742, 1684,   51, 
	3002, 1712,  218, 3642,  749, 3954,   26, 3600, 1000, 2721, 1361, 3656, 1210,   58, 3153,  640, 1974, 3213,  898, 1819, 4026,  928, 1779, 3839,  139, 1056, 2989,  525, 3234,   10, 3553, 1648, 3792,  258, 3101,  699, 2838,   49, 1474, 2945, 1772, 2412, 1547, 4062,  822, 3245,    7, 2035, 3135,  298, 3384, 2192,  498, 1838, 3364, 1038, 1556, 3023, 2444,  177, 2716,  501, 2905, 2061, 
	 883, 4084, 2602, 1200, 2386, 1392, 2812, 2248, 1728, 4027,  242, 1875, 2873, 2166, 1644, 3554, 1080, 2524, 3772,  505, 2369, 3160,  434, 2286, 3192, 1882, 2392, 1363, 2720, 1970, 1003, 2918,  895, 2763, 2106, 3730, 1585, 2379, 3589,  418, 3259,  669, 2773,  352, 2291, 1092, 2691, 3724, 1170, 2661,  952, 2862, 1518, 3043,  222, 2518, 3905,  360, 1692, 4019, 1936, 3432,  973, 3258, 
	2245,  555, 1925, 3074,  444, 3319, 1066,  392, 3204,  813, 2358, 3461,  865, 3976,  458, 2644, 1839,  127, 1514, 2917, 1163, 2028, 3519, 1220,  642, 4068,  278, 3604,  605, 3975, 2503,  406, 3359, 1793, 1264,  471, 3328, 1059, 1921, 2672, 1254, 3794, 1364, 3621, 1789, 3488, 1563,  652, 2337, 1695, 3822,   82, 3465, 1231, 3672, 2045,  940, 2223, 3120,  685, 1320, 2367, 1784,  326, 
	3766, 1519, 3424,  923, 3706, 1867, 2155, 3783, 1451, 2937, 1608,  543, 2596, 1477, 3336,  960, 3899, 2032, 3425,  863, 3661,   44, 1589, 2893, 2135, 1511, 3064, 1759, 2310, 1293, 1833, 3761, 2227,  102, 4078, 2033, 2702,  239, 3984,  732, 2228,   98, 2992,  713, 2590,  233, 2433, 3992,  384, 2965,  755, 2477, 1881,  681, 2775,  529, 1804, 3373, 1155, 2550, 3793,  148, 3507, 2693, 
	1057, 2924,   93, 2714, 1588,  204, 2971,  694, 2544,  104, 3892, 1958, 3250,  189, 2130, 3003,  355, 3122,  648, 2582, 1740, 2213, 3932,  331, 3331,  891, 2533,  781, 3176,  185, 3004,  721, 1180, 3125, 2333,  908, 1654, 3096, 1421, 3431, 2825, 1736, 2125, 3339, 1423, 3058,  805, 1932, 3226, 1386, 3571, 1105, 4092, 2167, 1548, 3771, 2899,   69, 3607,  464, 1724, 3045, 1204, 1964, 
	2389,  706, 3971, 1132, 2449, 3859, 1166, 3369, 1280, 3476,  930, 2422, 1268, 3718,  745, 1705, 2717, 1379, 2271, 3847,  450, 2814, 1022, 2692, 1315, 3647,   89, 3845, 1052, 3536, 1584, 2157, 3652, 1510,  682, 2868, 3624,  538, 2489, 1006,  362, 3876, 1051,  435, 3774, 1146, 3543, 1005, 2237,  191, 2023, 2636,  410, 3200,  183, 2321,  889, 2133, 1488, 2874, 2234,  643, 4041,  389, 
	3594, 1642, 2094, 3295,  523, 2040, 2675,  359, 2295, 1853, 3051,  316, 2824, 1002, 2500, 4071, 1114, 3529,  211, 1304, 3075, 1441, 3382,  693, 2387, 1665, 2901, 1263, 2445, 1915,  581, 3308,  329, 2481, 3834,  188, 2165, 1217, 3750, 1876, 3283, 1470, 2725, 2281, 1702, 2815,   77, 2594, 3929, 1629, 3324,  906, 2940, 1267, 3523, 1388, 4008, 2662,  762, 3702, 1063, 2749, 1525, 3159, 
	 852, 2998,  261, 2739, 1822,  976, 3550, 1672, 4057,  554, 1374, 3828, 1894, 3202,    4, 2220,  572, 1884, 3198, 2119,  723, 3618,  176, 2036, 3968,  427, 2183, 3401,  368, 4036, 2871, 1324, 2709, 1880, 1072, 3149, 1723, 2811,   23, 2394,  757, 2984,  159, 3966,  582, 2093, 3268, 1463,  618, 2859,  463, 3715, 1725, 2283,  655, 3068,  386, 1675, 3011,  266, 1986, 3410,   12, 2073, 
	2474, 1216, 3732,  900, 3924, 2364,   47, 2830,  871, 2605, 3255, 2191,  597, 1610, 3443, 1406, 3655, 2555,  875, 3995, 1570, 2471, 1731, 3000,  804, 3230,  975, 1798, 2620,  897, 2211,   48, 3923,  835, 3446, 2257,  583, 3311, 1549, 4054, 2098, 1273, 3405, 1967, 1340, 3663,  862, 2007, 3748, 1219, 1924, 2416,   30, 3861, 1165, 2467, 1929, 3379, 1121, 3952, 2567,  915, 1787, 3909, 
	 509, 3442, 1501, 2563,  457, 1559, 3353, 1352, 3654, 2060,  150, 1125, 3579, 2574, 1039, 2962,  323, 1628, 3010,   88, 2753,  535, 3713, 1040, 2663, 1891, 3811,  213, 3616, 1429, 3439, 1715, 2117, 2932,  380, 1385, 3889, 1110, 2592,  608, 3094,  411, 2547,  807, 3119,  338, 2315, 3037,  224, 3398, 2710, 1017, 3103, 1516, 3292,  236, 3744,  719, 2382, 1555,  537, 3522, 2647, 1305, 
	2774, 2024,  128, 3190, 1945, 2947,  646, 2510,  421, 1236, 3936, 2698, 1792,  284, 3756,  832, 2322, 3867, 1090, 2241, 3323, 1333, 2137, 3141,   22, 1299, 2803, 2080,  768, 2835,  452, 3066,  639, 1243, 3679, 2635, 2019,  321, 3627, 1312, 1989, 3726, 1489, 3551, 2122, 1605, 4064, 1081, 1770, 2204,  666, 3998, 1858,  540, 2111, 2668, 1427, 2936,   97, 3129, 1895, 2210,  241, 3719, 
	1009, 3307, 1810,  934, 3634, 1119, 3959, 1719, 3445, 2916, 1599,  533, 3179, 2365, 1447, 2844, 1953,  526, 3430, 1887,  789, 3815,  347, 1513, 4082, 2460,  542, 3172, 1670, 2260, 3987, 1368, 3510, 2240, 1808,  115, 3035, 1618, 2146, 3361,  184, 2335,  662, 2783,   42, 2925,  596, 2571, 3596,  956, 2877,  295, 2552, 3494,  987, 3926,  893, 1840, 3667, 1241, 3856, 1083, 3216,  727, 
	2342,  409, 4080, 2436,  300, 2741, 2185,  171, 2026,  683, 2230, 3738, 1339,  747, 4005,  138, 3219, 1502, 2565,  255, 3076, 1699, 2368, 3418,  638, 1747, 3500, 1113, 3763,  120, 1023, 2689,  252, 2792,  774, 4025, 1008, 3217,  791, 2665, 1150, 4004, 1837,  997, 3850, 1437, 3413, 1984,  129, 3171, 1606, 3688, 1206, 2244, 1803,  429, 3112, 2418,  811, 2696,  344, 2496, 1749, 2967, 
	1377, 2839, 1099, 2077, 3271,  838, 1442, 3583, 2679, 1034, 3054,   62, 2069, 3267, 1852, 2519, 1205, 3695,  869, 4024, 2176, 1101, 2974,  918, 2151, 2843,  267, 2406, 1496, 2951, 2174, 1690, 3817, 1143, 3299, 1979, 2495,  476, 3868, 1498, 2979,  493, 3134, 2490, 1937,  445, 2350,  854, 3946, 1169, 2341,  775, 3007,   84, 2805, 3563, 2097,  215, 3294, 1640, 3477,  870, 4014,   71, 
	3678, 1711, 3504,  670, 1623, 3864, 2883,  546, 1645, 4034, 1396, 3457, 2634,  473, 1015, 3559,  423, 2202, 2887, 1611,  558, 3374,  137, 1896, 3644, 1075, 3937,  825, 3458,  460, 3691,  590, 2075, 3081,  439, 1337, 3440, 1742, 2318,   67, 1954, 3491, 1401,  230, 3648, 2751, 1328, 3012, 1687, 2658,  374, 3314, 1704, 4074, 1448,  620, 1329, 3958, 1999,  560, 2088, 2791, 1257, 2284, 
	 821, 2492,  227, 3057, 2540,   29, 1258, 2239, 3016,  273, 2424,  635, 1251, 3684, 2255, 1681, 3105, 1432,   45, 3506, 2619, 1261, 3886, 2484,  436, 3107, 1701, 2625, 1380, 2352, 1551, 3199,  929, 2410, 1865, 3781,  246, 2942, 1185, 3681, 2588,  912, 2264, 3269, 1208,  714, 3752,  268, 3466,  730, 3837, 1242, 2722,  919, 2372, 3416, 2615,  880, 2851, 1476, 3788,  286, 3289, 1668, 
	3399, 1314, 3921, 1795, 1036, 3478, 1948, 3697,  921, 3397, 1707, 3878, 2004, 2943,  166, 2733,  814, 3918, 2086,  978, 1957, 3155,  622, 1461, 2750, 2064,   61, 3346,  676, 3032,  205, 1821, 3977,    1, 2819,  892, 2442,  754, 3233,  566, 1537, 3963,  678, 1696, 2895, 1904, 2465, 1458, 2131, 2904, 1776, 2298,  226, 3710, 1988,  308, 1630, 3636,   34, 3095, 1116, 2464,  703, 2638, 
	 385, 2781,  600, 2323, 3175,  764, 2808,  403, 1825, 2695, 1082, 2845,  339, 1499, 4087, 1179, 3376,  459, 2827, 3758,  294, 2345, 1818, 3409,  959, 3784, 1276, 1857, 4042, 1149, 3534, 2520, 1209, 3363, 1422, 3525, 1634, 4083, 2194, 1823, 3082,  293, 2633, 3530,   99, 3927,  531, 3114, 1058,    9, 3620,  874, 3277,  684, 3052, 1215, 3188, 1908, 2265,  758, 3434, 1885, 3967,  968, 
	3701, 2022, 3459, 1503,  332, 4001, 1473, 2168, 3833,  117, 2338,  779, 3173, 2153,  609, 2576, 1637, 2409,  846, 1542, 3321, 1077, 4056,  169, 2978,  561, 3214, 2253,  400, 2807, 1928,  495, 2991,  689, 2177,  420, 2885, 1088,  172, 2761, 1281, 3422, 1991, 1109, 2388,  943, 3317, 1669, 4029, 2653, 1398, 2497, 1540, 2818, 1836, 3865,  522,  983, 4049, 2601, 1536,  165, 2182, 3164, 
	1572,   92, 1183, 2927, 1873, 2451, 1093, 3201,  888, 3044, 1439, 3789, 1151, 3512, 1783, 3650,  251, 3274, 1868, 2669,  710, 2872, 1370, 2446, 2038, 1646, 2577, 1012, 3635, 2145,  974, 3805, 2272, 1738, 3933, 2527, 1342, 3303, 1944, 3843,  514, 2296,  788, 3709, 1592, 2780, 2020,  281, 2249,  665, 3462,  315, 3985, 1074,  111, 2534, 2018, 2930, 1411,  441, 3576, 2836, 1347,  508, 
	2744, 2243, 3951,  696, 3666,  161, 3521,  516, 2494, 1962, 3436,  491, 2476,   15, 2760,  868, 2258, 1291, 3875,   91, 3700, 2141,  393, 3611,  750, 3903,  275, 3296, 1596,  105, 3144, 1541,  260, 3183, 1042,  118, 3686,  594, 2384,  996, 3539, 1710, 3133,  235, 3030,  587, 3785, 1154, 3020, 1941, 1196, 3154, 1720, 2236, 3474, 1325, 3614,  247, 3249, 1844, 2229, 1014, 3776, 1869, 
	3515,  986, 1956, 2545, 1336, 3078, 2051, 1649, 4048,  309, 1192, 2787, 1721, 3388, 1426, 3965, 2928,  496, 2175, 3111, 1019, 1745, 3065, 1482, 3186, 1252, 2879,  698, 2507, 3947,  896, 2742, 3586, 1400, 2110, 2934, 1706, 2738, 1383, 2950,   35, 2482, 1425, 4039, 1187, 2505, 1509, 3390,  479, 3873, 2395,  848, 2768,  407, 2910,  619, 1754, 2755, 1207, 3816,  717, 3059,  276, 2498, 
	 647, 2908,  311, 3166,  571, 2303,  916, 2935, 1283, 3276, 1874, 3907,  823, 2302,  577, 1995, 1129, 3566, 1415,  601, 2539, 3540,  858, 2595,   18, 2330, 1751, 3505, 1111, 1906, 2403,  625, 2199,  446, 3782,  799, 3367,  349, 4007,  830, 3251, 1071, 2730,  426, 1972, 3568,   72, 2154, 2790, 1404,  144, 3674, 1471, 3854, 1004, 2072, 3934,  882, 2419,   86, 2640, 1685, 3298, 1530, 
	3848, 1373, 3429, 1661, 3809, 1424, 3584,   55, 2622,  675, 2421,  208, 3090, 1224, 3675, 2504,  143, 2057, 2701, 4028, 1558,  244, 1980, 3962, 1159, 3669,  451, 2021, 2784,  302, 3733, 1298, 3261, 1786, 2568, 1275, 2289, 1495, 2499, 1765, 2116, 3827,  704, 3423, 2371,  795, 3189, 1678,  630, 3136, 1901, 2559,  737, 2195, 3273, 2511,  330, 3327, 1950, 3496,  950, 4000, 1134, 2312, 
};
