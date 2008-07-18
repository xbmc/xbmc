#ifndef PRESET_FRAME_IO_HPP
#define PRESET_FRAME_IO_HPP
#include <vector>
class CustomWave;
class CustomShape;


/// Container class for all preset writeable engine variables. This is the important glue 
/// between the presets and renderer to facilitate smooth preset switching
/// Every preset object needs a reference to one of these.
class PresetOutputs {
public:
    typedef std::vector<CustomWave*> cwave_container;
    typedef std::vector<CustomShape*> cshape_container;

    cwave_container customWaves;
    cshape_container customShapes;

    void Initialize(int gx, int gy);
    PresetOutputs();
    ~PresetOutputs();
    /* PER FRAME VARIABLES BEGIN */
    float zoom;
    float zoomexp;
    float rot;
    float warp;

    float sx;
    float sy;
    float dx;
    float dy;
    float cx;
    float cy;

    float decay;

    float wave_r;
    float wave_g;
    float wave_b;
    float wave_o;
    float wave_x;
    float wave_y;
    float wave_mystery;

    float ob_size;
    float ob_r;
    float ob_g;
    float ob_b;
    float ob_a;

    float ib_size;
    float ib_r;
    float ib_g;
    float ib_b;
    float ib_a;

    float mv_a ;
    float mv_r ;
    float mv_g ;
    float mv_b ;
    float mv_l;
    float mv_x;
    float mv_y;
    float mv_dy;
    float mv_dx;

    int gy,gx;
    /* PER_FRAME VARIABLES END */

    float fRating;
    float fGammaAdj;
    float fVideoEchoZoom;
    float fVideoEchoAlpha;

    int nVideoEchoOrientation;
    int nWaveMode;

    bool bAdditiveWaves;
    bool bWaveDots;
    bool bWaveThick;
    bool bModWaveAlphaByVolume;
    bool bMaximizeWaveColor;
    bool bTexWrap;
    bool bDarkenCenter;
    bool bRedBlueStereo;
    bool bBrighten;
    bool bDarken;
    bool bSolarize;
    bool bInvert;
    bool bMotionVectorsOn;


    float fWaveAlpha ;
    float fWaveScale;
    float fWaveSmoothing;
    float fWaveParam;
    float fModWaveAlphaStart;
    float fModWaveAlphaEnd;
    float fWarpAnimSpeed;
    float fWarpScale;
    float fShader;

    /* Q VARIABLES START */

    float q1;
    float q2;
    float q3;
    float q4;
    float q5;
    float q6;
    float q7;
    float q8;


    /* Q VARIABLES END */

    float **zoom_mesh;
    float **zoomexp_mesh;
    float **rot_mesh;

    float **sx_mesh;
    float **sy_mesh;
    float **dx_mesh;
    float **dy_mesh;
    float **cx_mesh;
    float **cy_mesh;
    float **warp_mesh;


    float **x_mesh;
    float **y_mesh;

  float wavearray[2048][2];
  float wavearray2[2048][2];
 
  int wave_samples;
  bool two_waves;
  bool draw_wave_as_loop;
  double wave_rot;
  double wave_scale;
  

};

/// Container for all *read only* engine variables a preset requires to 
/// evaluate milkdrop equations. Every preset object needs a reference to one of these.
class PresetInputs {

public:
    /* PER_PIXEL VARIBLES BEGIN */

    float x_per_pixel;
    float y_per_pixel;
    float rad_per_pixel;
    float ang_per_pixel;

    /* PER_PIXEL VARIBLES END */

    int fps;


    float time;
    float bass;
    float mid;
    float treb;
    float bass_att;
    float mid_att;
    float treb_att;
    int frame;
    float progress;


    /* variables were added in milkdrop 1.04 */
    int gx,gy;

    float **x_mesh;
    float **y_mesh;
    float **rad_mesh;
    float **theta_mesh;

    float **origtheta;  //grid containing interpolated mesh reference values
    float **origrad;
    float **origx;  //original mesh
    float **origy;

    float mood_r, mood_g, mood_b;

    void ResetMesh();
    ~PresetInputs();
    PresetInputs();
    void Initialize(int gx, int gy);
};

#endif
