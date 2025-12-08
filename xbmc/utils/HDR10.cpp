#include <string>

#include "HevcSei.h"
#include "StringUtils.h"

#include "HDR10.h"


// Logic from MediaInfoLib

std::string CodeToColourPrimaries(uint8_t code)
{
    switch (code)
    {
        case  1 : return "BT.709";
        case  4 : return "BT.470 System M";
        case  5 : return "BT.601 PAL";
        case  6 : return "BT.601 NTSC";
        case  7 : return "SMPTE 240M"; //Same as BT.601 NTSC
        case  8 : return "Generic film";
        case  9 : return "BT.2020";          // Added in HEVC
        case 10 : return "XYZ";              // Added in HEVC 2014
        case 11 : return "DCI P3";           // Added in HEVC 2016
        case 12 : return "Display P3";       // Added in HEVC 2016
        case 22 : return "EBU Tech 3213";    // Added in HEVC 2016
        default : return "";
    }
}

std::string MasteringDisplayColourVolumeText(const MasteringDisplayColourVolume& mdcv)
{
    // Reordering to RGB
    size_t R = 4;
    size_t G = 4;
    size_t B = 4;
    size_t W = 3;

    for (size_t c = 0; c < 3; c++)
    {
      if ((mdcv.displayPrimaries[c].x < 17500) && (mdcv.displayPrimaries[c].y < 17500)) B = c;    // x and y small then blue   
      else if ((mdcv.displayPrimaries[c].y - mdcv.displayPrimaries[c].x) >= 0) G = c;             // y > x then green       
      else R = c;
    }
  
    // Order not automaticly detected, then assume GBR order
    if ((R | B | G) >= 4)
    {
        G=0; B=1; R=2;
    }

    // Attempt to match to Well Known Colour Primaries
    for (uint8_t i = 0; i < 4; i++)
    {
        uint8_t code = knownColourVolumes[i].code;

        // +/- 0.0005 (3 digits after comma)
        if ((mdcv.displayPrimaries[G].x < (knownColourVolumes[i].values[0] - 25)) || (mdcv.displayPrimaries[G].x >= (knownColourVolumes[i].values[0] + 25))) code = 0;
        if ((mdcv.displayPrimaries[G].y < (knownColourVolumes[i].values[1] - 25)) || (mdcv.displayPrimaries[G].y >= (knownColourVolumes[i].values[1] + 25))) code = 0;

        if ((mdcv.displayPrimaries[B].x < (knownColourVolumes[i].values[2] - 25)) || (mdcv.displayPrimaries[B].x >= (knownColourVolumes[i].values[2] + 25))) code = 0;
        if ((mdcv.displayPrimaries[B].y < (knownColourVolumes[i].values[3] - 25)) || (mdcv.displayPrimaries[B].y >= (knownColourVolumes[i].values[3] + 25))) code = 0;

        if ((mdcv.displayPrimaries[R].x < (knownColourVolumes[i].values[4] - 25)) || (mdcv.displayPrimaries[R].x >= (knownColourVolumes[i].values[4] + 25))) code = 0;
        if ((mdcv.displayPrimaries[R].y < (knownColourVolumes[i].values[5] - 25)) || (mdcv.displayPrimaries[R].y >= (knownColourVolumes[i].values[5] + 25))) code = 0;

        // +/- 0.00005 (4 digits after comma)
        if ((mdcv.whitePoint.x < knownColourVolumes[i].values[6] - 2) || (mdcv.whitePoint.x >= knownColourVolumes[i].values[6] + 3)) code = 0;
        if ((mdcv.whitePoint.y < knownColourVolumes[i].values[7] - 2) || (mdcv.whitePoint.y >= knownColourVolumes[i].values[7] + 3)) code = 0;
        
        // if a well known colour primarites return a name
        if (code) return CodeToColourPrimaries(code);        
    }
    
    // Not well known - create a string from values
    return   "R:" + std::to_string(mdcv.displayPrimaries[R].x) + "," + std::to_string(mdcv.displayPrimaries[R].y) + " " +
             "G:" + std::to_string(mdcv.displayPrimaries[G].x) + "," + std::to_string(mdcv.displayPrimaries[G].y) + " " +
             "B:" + std::to_string(mdcv.displayPrimaries[B].x) + "," + std::to_string(mdcv.displayPrimaries[B].y) + " " +
             "W:" + std::to_string(mdcv.whitePoint.x) + "," + std::to_string(mdcv.whitePoint.y);
}