#include "IdlePreset.hpp"
#include <sstream>
#include <string>

const std::string IdlePreset::IDLE_PRESET_NAME
	("Geiss & Sperl - Feedback (projectM idle HDR mix)");

std::string IdlePreset::presetText() {

std::ostringstream out;

out << "[preset00]\n" <<
"fRating=2.000000\n" <<
"fGammaAdj=1.700000\n" <<
"fDecay=0.940000\n" <<
"fVideoEchoZoom=1.000000\n" <<
"fVideoEchoAlpha=0.000000\n" <<
"nVideoEchoOrientation=0\n" <<
"nWaveMode=0\n" <<
"bAdditiveWaves=1\n" <<
"bWaveDots=0\n" <<
"bWaveThick=0\n" <<
"bModWaveAlphaByVolume=0\n" <<
"bMaximizeWaveColor=0\n" <<
"bTexWrap=1\n" <<
"bDarkenCenter=0\n" <<
"bRedBlueStereo=0\n" <<
"bBrighten=0\n" <<
"bDarken=0\n" <<
"bSolarize=0\n" <<
"bInvert=0\n" <<
"fWaveAlpha=0.001000\n" <<
"fWaveScale=0.010000\n" <<
"fWaveSmoothing=0.630000\n" <<
"fWaveParam=-1.000000\n" <<
"fModWaveAlphaStart=0.710000\n" <<
"fModWaveAlphaEnd=1.300000\n" <<
"fWarpAnimSpeed=1.000000\n" <<
"fWarpScale=1.331000\n" <<
"fZoomExponent=1.000000\n" <<
"fShader=0.000000\n" <<
"zoom=13.290894\n" <<
"rot=-0.020000\n" <<
"cx=0.500000\n" <<
"cy=0.500000\n" <<
"dx=-0.280000\n" <<
"dy=-0.320000\n" <<
"warp=0.010000\n" <<
"sx=1.000000\n" <<
"sy=1.000000\n" <<
"wave_r=0.650000\n" <<
"wave_g=0.650000\n" <<
"wave_b=0.650000\n" <<
"wave_x=0.500000\n" <<
"wave_y=0.500000\n" <<
"ob_size=0.000000\n" <<
"ob_r=0.010000\n" <<
"ob_g=0.000000\n" <<
"ob_b=0.000000\n" <<
"ob_a=1.000000\n" <<
"ib_size=0.000000\n" <<
"ib_r=0.950000\n" <<
"ib_g=0.850000\n" <<
"ib_b=0.650000\n" <<
"ib_a=1.000000\n" <<
"nMotionVectorsX=64.000000\n" <<
"nMotionVectorsY=0.000000\n" <<
"mv_dx=0.000000\n" <<
"mv_dy=0.000000\n" <<
"mv_l=0.900000\n" <<
"mv_r=1.000000\n" <<
"mv_g=1.000000\n" <<
"mv_b=1.000000\n" <<
"mv_a=0.000000\n" <<
"shapecode_3_enabled=1\n" <<
"shapecode_3_sides=20\n" <<
"shapecode_3_additive=0\n" <<
"shapecode_3_thickOutline=0\n" <<
"shapecode_3_textured=1\n" <<
"shapecode_3_ImageURL=M.tga\n" <<
"shapecode_3_x=0.68\n" <<
"shapecode_3_y=0.5\n" <<
"shapecode_3_rad=0.41222\n" <<
"shapecode_3_ang=0\n" <<
"shapecode_3_tex_ang=0\n" <<
"shapecode_3_tex_zoom=0.71\n" <<
"shapecode_3_r=1\n" <<
"shapecode_3_g=1\n" <<
"shapecode_3_b=1\n" <<
"shapecode_3_a=1\n" <<
"shapecode_3_r2=1\n" <<
"shapecode_3_g2=1\n" <<
"shapecode_3_b2=1\n" <<
"shapecode_3_a2=1\n" <<
"shapecode_3_border_r=0\n" <<
"shapecode_3_border_g=0\n" <<
"shapecode_3_border_b=0\n" <<
"shapecode_3_border_a=0\n" <<
"shape_3_per_frame1=x = x + q1;\n" <<
"shape_3_per_frame2=y = y + q2;\n" <<
"shape_3_per_frame3=r =0.5 + 0.5*sin(q8*0.613 + 1);\n" <<
"shape_3_per_frame4=g = 0.5 + 0.5*sin(q8*0.763 + 2);\n" <<
"shape_3_per_frame5=b = 0.5 + 0.5*sin(q8*0.771 + 5);\n" <<
"shape_3_per_frame6=r2 = 0.5 + 0.5*sin(q8*0.635 + 4);\n" <<
"shape_3_per_frame7=g2 = 0.5 + 0.5*sin(q8*0.616+ 1);\n" <<
"shape_3_per_frame8=b2 = 0.5 + 0.5*sin(q8*0.538 + 3);\n" <<
"shapecode_4_enabled=1\n" <<
"shapecode_4_sides=4\n" <<
"shapecode_4_additive=0\n" <<
"shapecode_4_thickOutline=0\n" <<
"shapecode_4_textured=1\n" <<
"shapecode_4_ImageURL=headphones.tga\n" <<
"shapecode_4_x=0.68\n" <<
"shapecode_4_y=0.58\n" <<
"shapecode_4_rad=0.6\n" <<
"shapecode_4_ang=0\n" <<
"shapecode_4_tex_ang=0\n" <<
"shapecode_4_tex_zoom=0.71\n" <<
"shapecode_4_r=1\n" <<
"shapecode_4_g=1\n" <<
"shapecode_4_b=1\n" <<
"shapecode_4_a=1\n" <<
"shapecode_4_r2=1\n" <<
"shapecode_4_g2=1\n" <<
"shapecode_4_b2=1\n" <<
"shapecode_4_a2=1\n" <<
"shapecode_4_border_r=0\n" <<
"shapecode_4_border_g=0\n" <<
"shapecode_4_border_b=0\n" <<
"shapecode_4_border_a=0\n" <<
"shape_4_per_frame1=x = x + q1;\n" <<
"shape_4_per_frame2=y = y + q2;\n" <<
"shape_4_per_frame3=rad = rad + bass * 0.1;\n" <<
"shape_4_per_frame4=a = q3;\n" <<
"shape_4_per_frame5=a2 = q3;\n" <<
"shapecode_6_enabled=1\n" <<
"shapecode_6_sides=4\n" <<
"shapecode_6_additive=0\n" <<
"shapecode_6_thickOutline=0\n" <<
"shapecode_6_textured=1\n" <<
"shapecode_6_ImageURL=project.tga\n" <<
"shapecode_6_x=0.38\n" <<
"shapecode_6_y=0.435\n" <<
"shapecode_6_rad=0.8\n" <<
"shapecode_6_ang=0\n" <<
"shapecode_6_tex_ang=0\n" <<
"shapecode_6_tex_zoom=0.71\n" <<
"shapecode_6_r=1\n" <<
"shapecode_6_g=1\n" <<
"shapecode_6_b=1\n" <<
"shapecode_6_a=1\n" <<
"shapecode_6_r2=1\n" <<
"shapecode_6_g2=1\n" <<
"shapecode_6_b2=1\n" <<
"shapecode_6_a2=1\n" <<
"shapecode_6_border_r=0\n" <<
"shapecode_6_border_g=0\n" <<
"shapecode_6_border_b=0\n" <<
"shapecode_6_border_a=0\n" <<
"shape_6_per_frame1=x = x + q1;\n" <<
"shape_6_per_frame2=y = y + q2;\n" <<
"shape_6_per_frame3=a = q3;\n" <<
"shape_6_per_frame4=a2 = q3;\n" <<
"per_frame_1=ob_r = 0.5 + 0.4*sin(time*1.324);\n" <<
"per_frame_2=ob_g = 0.5 + 0.4*cos(time*1.371);\n" <<
"per_frame_3=ob_b = 0.5+0.4*sin(2.332*time);\n" <<
"per_frame_4=ib_r = 0.5 + 0.25*sin(time*1.424);\n" <<
"per_frame_5=ib_g = 0.25 + 0.25*cos(time*1.871);\n" <<
"per_frame_6=ib_b = 1-ob_b;\n" <<
"per_frame_7=volume = 0.15*(bass+bass_att+treb+treb_att+mid+mid_att);\n" <<
"per_frame_8=xamptarg = if(equal(frame%15,0),min(0.5*volume*bass_att,0.5),xamptarg);\n" <<
"per_frame_9=xamp = xamp + 0.5*(xamptarg-xamp);\n" <<
"per_frame_10=xdir = if(above(abs(xpos),xamp),-sign(xpos),if(below(abs(xspeed),0.1),2*above(xpos,0)-1,xdir));\n" <<
"per_frame_11=xaccel = xdir*xamp - xpos - xspeed*0.055*below(abs(xpos),xamp);\n" <<
"per_frame_12=xspeed = xspeed + xdir*xamp - xpos - xspeed*0.055*below(abs(xpos),xamp);\n" <<
"per_frame_13=xpos = xpos + 0.001*xspeed;\n" <<
"per_frame_14=dx = xpos*0.05;\n" <<
"per_frame_15=yamptarg = if(equal(frame%15,0),min(0.3*volume*treb_att,0.5),yamptarg);\n" <<
"per_frame_16=yamp = yamp + 0.5*(yamptarg-yamp);\n" <<
"per_frame_17=ydir = if(above(abs(ypos),yamp),-sign(ypos),if(below(abs(yspeed),0.1),2*above(ypos,0)-1,ydir));\n" <<
"per_frame_18=yaccel = ydir*yamp - ypos - yspeed*0.055*below(abs(ypos),yamp);\n" <<
"per_frame_19=yspeed = yspeed + ydir*yamp - ypos - yspeed*0.055*below(abs(ypos),yamp);\n" <<
"per_frame_20=ypos = ypos + 0.001*yspeed;\n" <<
"per_frame_21=dy = ypos*0.05;\n" <<
"per_frame_22=wave_a = 0;\n" <<
"per_frame_23=q8 = oldq8 + 0.0003*(pow(1+1.2*bass+0.4*bass_att+0.1*treb+0.1*treb_att+0.1*mid+0.1*mid_att,6)/fps);\n" <<
"per_frame_24=oldq8 = q8;\n" <<
"per_frame_25=q7 = 0.003*(pow(1+1.2*bass+0.4*bass_att+0.1*treb+0.1*treb_att+0.1*mid+0.1*mid_att,6)/fps);\n" <<
"per_frame_26=rot = 0.4 + 1.5*sin(time*0.273) + 0.4*sin(time*0.379+3);\n" <<
"per_frame_27=q1 = 0.05*sin(time*1.14);\n" <<
"per_frame_28=q2 = 0.03*sin(time*0.93+2);\n" <<
"per_frame_29=q3 = if(above(frame,60),1, frame/60.0);\n" <<
"per_frame_30=oldq8 = if(above(oldq8,1000),0,oldq8);\n" <<
"per_pixel_1=zoom =( log(sqrt(2)-rad) -0.24)*1;\n";

return out.str();

}

std::unique_ptr<Preset> IdlePreset::allocate( PresetInputs & presetInputs, PresetOutputs & presetOutputs)
{

  std::istringstream in(presetText());
  return std::unique_ptr<Preset>(new Preset(in, IDLE_PRESET_NAME, presetInputs, presetOutputs));
}

