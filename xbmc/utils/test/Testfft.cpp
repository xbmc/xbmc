/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/fft.h"
#include "utils/StdString.h"

#include "gtest/gtest.h"

/* refdata[] below was generated using the following Python script.

import math
import wave
import struct
import sys

if __name__=='__main__':
    # http://stackoverflow.com/questions/3637350/how-to-write-stereo-wav-files-in-python
    # http://www.sonicspot.com/guide/wavefiles.html
    freq=440.0
    data_size=128
    fname="test.wav"
    frate=11025.0
    amp=64000.0
    nchannels=1
    sampwidth=2
    framerate=int(frate)
    nframes=data_size
    comptype="NONE"
    compname="not compressed"
    data=[math.sin(2*math.pi*freq*(x/frate))
          for x in range(data_size)]
    count = 0
    sys.stdout.write("static const float refdata[] = {\n")
    for v in data:
      sys.stdout.write(str(v) + "f,")
      count += 1
      if count % 4 == 0:
        sys.stdout.write("\n")
      else:
        sys.stdout.write(" ")
    sys.stdout.write("};\n")
*/

static const float refdata[] = {
0.0f, 0.248137847944f, 0.480754541017f, 0.683299780871f,
0.843104254616f, 0.950172107096f, 0.997806186976f, 0.983026957126f,
0.906758866265f, 0.773772524197f, 0.592386297618f, 0.373945991846f,
0.132115164631f, -0.117979536667f, -0.360694554958f, -0.580847936259f,
-0.764668969029f, -0.900659549526f, -0.980313394334f, -0.998648112931f,
-0.954516858881f, -0.850680065687f, -0.693632780202f, -0.493198393981f,
-0.261914184895f, -0.0142471037071f, 0.234311141425f, 0.46821309956f,
0.672828078337f, 0.835357301588f, 0.945634479622f, 0.996761716077f,
0.985540975016f, 0.912674119782f, 0.782719011074f, 0.603804410325f,
0.387121521337f, 0.146223974503f, -0.103819960006f, -0.347369900587f,
-0.569191668423f, -0.755410193505f, -0.894377407666f, -0.977400837465f,
-0.999287323041f, -0.958667853037f, -0.858083196987f, -0.703824978831f,
-0.505542132473f, -0.275637355817f, -0.0284913153908f, 0.220436872025f,
0.455576615403f, 0.662219798253f, 0.827440779159f, 0.940904897555f,
0.995514912254f, 0.987854937681f, 0.918404109336f, 0.791506613611f,
0.615099956615f, 0.400218468928f, 0.160303102331f, -0.0896393089034f,
-0.333974733507f, -0.557419860219f, -0.745998077066f, -0.887913715899f,
-0.974289877741f, -0.999723687553f, -0.96262424695f, -0.865312145751f,
-0.71387430784f, -0.517783250833f, -0.289304575039f, -0.0427297436149f,
0.206517856086f, 0.442847653627f, 0.651477093995f, 0.819356294308f,
0.935984320952f, 0.994066028594f, 0.989968375411f, 0.923947671797f,
0.800133548011f, 0.626270643602f, 0.413234176067f, 0.17434969019f,
-0.0754404618937f, -0.320511772808f, -0.545534901211f, -0.736434530278f,
-0.881269786291f, -0.970981146657f, -0.999957117888f, -0.966385237512f,
-0.872365444572f, -0.723778727314f, -0.529919264233f, -0.302913068248f,
-0.056959498117f, 0.192556919032f, 0.430028798089f, 0.64060214623f,
0.811105488104f, 0.930873748644f, 0.992415359208f, 0.991880859198f,
0.929303681875f, 0.80859806309f, 0.637314203745f, 0.42616600069f,
0.188360886759f, -0.0612263012046f, -0.306983751339f, -0.533539203927f,
-0.72672149445f, -0.874446967495f, -0.967475315852f, -0.999987566662f,
-0.969950061278f, -0.879241661701f, -0.733536226752f, -0.541947709182f,
-0.316460073053f, -0.0711776903954f, 0.178556894799f, 0.417122650891f
};

#define REFDATA_NUMELEMENTS 128 /*(sizeof(refdata)/sizeof(float))*/

/* All reference data below were generated by using the following C++ code.

  fprintf(stdout, "static const float reffftdata[] = {\n");
  for (i = 0; i < REFDATA_NUMELEMENTS; i++)
  {
    fprintf(stdout, "%.6ff,", vardata[i]);
    if ((i + 1) % 4 == 0)
      fprintf(stdout, "\n");
    else
      fprintf(stdout, " ");
  }
  fprintf(stdout, "};\n");

*/

static const float reffftdata[] = {
0.000000f, 0.449505f, 0.120648f, 0.233138f,
0.392939f, 0.043552f, 0.833054f, -0.148657f,
1.675773f, -0.424927f, 3.999525f, -3.602860f,
48.666092f, 0.123507f, -6.779173f, -0.179830f,
-3.556417f, -0.323241f, -2.528245f, -0.424007f,
-1.999831f, -0.503748f, -1.662579f, -0.569499f,
-1.417722f, -0.624251f, -1.224108f, -0.669501f,
-1.061725f, -0.706093f, -0.919814f, -0.734565f,
-0.792209f, -0.755297f, -0.675225f, -0.768604f,
-0.566617f, -0.774772f, -0.465016f, -0.774090f,
-0.369611f, -0.766866f, -0.279953f, -0.753430f,
-0.195832f, -0.734147f, -0.117191f, -0.709420f,
-0.044083f, -0.679685f, 0.023383f, -0.645416f,
0.085060f, -0.607121f, 0.140793f, -0.565342f,
0.190426f, -0.520648f, 0.233821f, -0.473637f,
0.270868f, -0.424926f, 0.301492f, -0.375151f,
0.325655f, -0.324962f, 0.343369f, -0.275019f,
0.354691f, -0.225984f, 0.359731f, -0.178525f,
0.358652f, -0.133307f, 0.351671f, -0.090984f,
0.339060f, -0.052212f, 0.321141f, -0.017629f,
0.298293f, 0.012125f, 0.270944f, 0.036423f,
0.239573f, 0.054633f, 0.204705f, 0.066113f,
0.166914f, 0.070200f, 0.126814f, 0.066174f,
0.085068f, 0.053236f, 0.042377f, 0.030439f,
-0.000509f, -0.003392f, -0.042792f, -0.049815f,
-0.083613f, -0.111011f, -0.122037f, -0.190212f,
-0.157020f, -0.292521f, -0.187344f, -0.426526f,
-0.211503f, -0.607937f, -0.227426f, -0.868800f,
-0.231823f, -1.285804f, -0.218194f, -2.098434f,
-0.168627f, -4.622009f, 0.009162f, 38.381283f,
-2.935104f, 3.676884f, -0.513415f, 1.829207f,
-0.334315f, 1.124193f, -0.207913f, 0.725023f,
};

static const float reffftinversedata[] = {
0.000000f, 0.449505f, 0.120648f, 0.725023f,
-0.066419f, 1.124193f, -0.207913f, 1.829207f,
-0.334315f, 3.676884f, -0.513415f, 38.381283f,
-2.935104f, -4.622009f, 0.009162f, -2.098434f,
-0.168627f, -1.285804f, -0.218194f, -0.868800f,
-0.231823f, -0.607937f, -0.227426f, -0.426526f,
-0.211503f, -0.292521f, -0.187344f, -0.190212f,
-0.157020f, -0.111011f, -0.122037f, -0.049815f,
-0.083613f, -0.003392f, -0.042792f, 0.030439f,
-0.000509f, 0.053236f, 0.042377f, 0.066174f,
0.085068f, 0.070200f, 0.126814f, 0.066113f,
0.166914f, 0.054633f, 0.204705f, 0.036423f,
0.239573f, 0.012125f, 0.270944f, -0.017629f,
0.298293f, -0.052212f, 0.321141f, -0.090984f,
0.339060f, -0.133307f, 0.351671f, -0.178525f,
0.358652f, -0.225984f, 0.359731f, -0.275019f,
0.354691f, -0.324962f, 0.343369f, -0.375151f,
0.325655f, -0.424926f, 0.301492f, -0.473637f,
0.270868f, -0.520648f, 0.233821f, -0.565342f,
0.190426f, -0.607121f, 0.140793f, -0.645416f,
0.085060f, -0.679685f, 0.023383f, -0.709420f,
-0.044083f, -0.734147f, -0.117191f, -0.753430f,
-0.195832f, -0.766866f, -0.279953f, -0.774090f,
-0.369611f, -0.774772f, -0.465016f, -0.768604f,
-0.566617f, -0.755297f, -0.675225f, -0.734565f,
-0.792209f, -0.706093f, -0.919814f, -0.669501f,
-1.061725f, -0.624251f, -1.224108f, -0.569499f,
-1.417722f, -0.503748f, -1.662579f, -0.424007f,
-1.999831f, -0.323241f, -2.528245f, -0.179830f,
-3.556417f, 0.123507f, -6.779173f, -3.602860f,
48.666092f, -0.424927f, 3.999525f, -0.148657f,
1.675773f, 0.043552f, 0.833054f, 0.233138f,
};

static const float reftwochannelrfftdata[] = {
0.014556f, 0.202055f, 0.174283f, 0.564540f,
0.779294f, 1.223621f, 2.855724f, 3.432353f,
14.488905f, 15.470927f, 1926.995483f, 1936.110718f,
34.176479f, 33.159004f, 8.778496f, 8.333804f,
4.234725f, 3.962682f, 2.589059f, 2.398601f,
1.791486f, 1.647755f, 1.337403f, 1.223514f,
1.051121f, 0.957675f, 0.857526f, 0.778798f,
0.719788f, 0.652054f, 0.617972f, 0.558680f,
0.540455f, 0.487790f, 0.480051f, 0.432682f,
0.432111f, 0.389031f, 0.393506f, 0.353940f,
0.362065f, 0.325402f, 0.336244f, 0.301994f,
0.314917f, 0.282679f, 0.297250f, 0.266693f,
0.282615f, 0.253461f, 0.270538f, 0.242548f,
0.260654f, 0.233622f, 0.252682f, 0.226430f,
0.246418f, 0.220773f, 0.241694f, 0.216511f,
0.238397f, 0.213538f, 0.236449f, 0.211782f,
0.117902f, 0.105600f, -0.357960f, -0.308602f,
-0.372784f, -0.292395f, -0.387933f, -0.276250f,
-0.403502f, -0.260078f, -0.419604f, -0.243792f,
-0.436362f, -0.227290f, -0.453916f, -0.210478f,
-0.472433f, -0.193249f, -0.492110f, -0.175478f,
-0.513182f, -0.157028f, -0.535943f, -0.137741f,
-0.560756f, -0.117426f, -0.588082f, -0.095855f,
-0.618519f, -0.072741f, -0.652857f, -0.047725f,
-0.692169f, -0.020336f, -0.737947f, 0.010054f,
-0.792340f, 0.044317f, -0.858569f, 0.083711f,
-0.941691f, 0.130146f, -1.050159f, 0.186711f,
-1.199277f, 0.258840f, -1.419963f, 0.357229f,
-1.785721f, 0.506807f, -2.525070f, 0.783990f,
-4.890463f, 1.604234f, 36.070564f, -11.902861f,
3.081995f, -0.935849f, 1.359818f, -0.300774f,
0.721637f, -0.007045f, 0.368046f, 0.218320f,
};

static const float reftwochanwithwindowdata[] = {
0.000078f, 0.000219f, 0.000856f, 0.001174f,
0.007080f, 0.007566f, 0.107324f, 0.108530f,
90.137039f, 90.161346f, 504.375732f, 504.333801f,
173.107437f, 173.125870f, 0.244316f, 0.243784f,
0.012957f, 0.012861f, 0.001967f, 0.001938f,
0.000483f, 0.000471f, 0.000157f, 0.000152f,
0.000062f, 0.000059f, 0.000028f, 0.000026f,
0.000014f, 0.000013f, 0.000007f, 0.000007f,
0.000004f, 0.000004f, 0.000003f, 0.000002f,
0.000002f, 0.000001f, 0.000001f, 0.000001f,
0.000001f, 0.000001f, 0.000000f, 0.000000f,
0.000000f, 0.000000f, 0.000000f, 0.000000f,
0.000000f, 0.000000f, 0.000000f, 0.000000f,
0.000000f, 0.000000f, 0.000000f, 0.000000f,
0.000000f, 0.000000f, 0.000000f, 0.000000f,
0.000000f, 0.000000f, 0.000000f, 0.000000f,
0.000000f, 0.000000f, 0.000058f, 0.000038f,
0.000081f, 0.000016f, 0.000106f, -0.000007f,
0.000133f, -0.000029f, 0.000164f, -0.000052f,
0.000199f, -0.000077f, 0.000241f, -0.000105f,
0.000290f, -0.000135f, 0.000349f, -0.000169f,
0.000422f, -0.000210f, 0.000513f, -0.000257f,
0.000628f, -0.000314f, 0.000778f, -0.000385f,
0.000975f, -0.000475f, 0.001243f, -0.000593f,
0.001616f, -0.000750f, 0.002154f, -0.000969f,
0.002959f, -0.001283f, 0.004224f, -0.001760f,
0.006336f, -0.002532f, 0.010161f, -0.003890f,
0.017893f, -0.006565f, 0.036268f, -0.012797f,
0.093398f, -0.031901f, 0.406511f, -0.135766f,
-10.831606f, 3.581835f, 18.487400f, -6.118526f,
-7.816598f, 2.582984f, -0.270999f, 0.085336f,
-0.071148f, 0.017091f, -0.026548f, -0.001455f,
};

TEST(Testfft, fft)
{
  int i;
  float vardata[REFDATA_NUMELEMENTS];
  CStdString refstr, varstr;

  memcpy(vardata, refdata, sizeof(refdata));
  fft(vardata, REFDATA_NUMELEMENTS/2, 1);
  for (i = 0; i < REFDATA_NUMELEMENTS; i++)
  {
    /* To more consistently test the resulting floating point numbers, they
     * are converted to strings and the strings are tested for equality.
     */
    refstr.Format("%.6f", reffftdata[i]);
    varstr.Format("%.6f", vardata[i]);
    EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  }
}

TEST(Testfft, fft_inverse)
{
  int i;
  float vardata[REFDATA_NUMELEMENTS];
  CStdString refstr, varstr;

  memcpy(vardata, refdata, sizeof(refdata));
  fft(vardata, REFDATA_NUMELEMENTS/2, -1);
  for (i = 0; i < REFDATA_NUMELEMENTS; i++)
  {
    refstr.Format("%.6f", reffftinversedata[i]);
    varstr.Format("%.6f", vardata[i]);
    EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  }
}

TEST(Testfft, twochannelrfft)
{
  int i;
  float vardata[REFDATA_NUMELEMENTS];
  CStdString refstr, varstr;

  memcpy(vardata, refdata, sizeof(refdata));
  twochannelrfft(vardata, REFDATA_NUMELEMENTS/2);
  for (i = 0; i < REFDATA_NUMELEMENTS; i++)
  {
    refstr.Format("%.6f", reftwochannelrfftdata[i]);
    varstr.Format("%.6f", vardata[i]);
    EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  }
}

TEST(Testfft, twochanwithwindow)
{
  int i;
  float vardata[REFDATA_NUMELEMENTS];
  CStdString refstr, varstr;

  memcpy(vardata, refdata, sizeof(refdata));
  twochanwithwindow(vardata, REFDATA_NUMELEMENTS/2);
  for (i = 0; i < REFDATA_NUMELEMENTS; i++)
  {
    refstr.Format("%.6f", reftwochanwithwindowdata[i]);
    varstr.Format("%.6f", vardata[i]);
    EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  }
}
