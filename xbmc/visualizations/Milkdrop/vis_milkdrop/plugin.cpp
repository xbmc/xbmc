/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
Order of Function Calls
-----------------------
    The only code that will be called by the plugin framework are the
    12 virtual functions in plugin.h.  But in what order are they called?  
    A breakdown follows.  A function name in { } means that it is only 
    called under certain conditions.

    Order of function calls...
    
    When the PLUGIN launches
    ------------------------
        INITIALIZATION
            OverrideDefaults
            MyPreInitialize
            MyReadConfig
            << DirectX gets initialized at this point >>
            AllocateMyNonDx8Stuff
            AllocateMyDX8Stuff
        RUNNING
            +--> { CleanUpMyDX8Stuff + AllocateMyDX8Stuff }  // called together when user resizes window or toggles fullscreen<->windowed.
            |    MyRenderFn
            |    MyRenderUI
            |    { MyWindowProc }                            // called, between frames, on mouse/keyboard/system events.  100% threadsafe.
            +----<< repeat >>
        CLEANUP        
            CleanUpMyDX8Stuff
            CleanUpMyNonDx8Stuff
            << DirectX gets uninitialized at this point >>

    When the CONFIG PANEL launches
    ------------------------------
        INITIALIZATION
            OverrideDefaults
            MyPreInitialize
            MyReadConfig
            << DirectX gets initialized at this point >>
        RUNNING
            { MyConfigTabProc }                  // called on startup & on keyboard events
        CLEANUP
            [ MyWriteConfig ]                    // only called if user clicked 'OK' to exit
            << DirectX gets uninitialized at this point >>
*/

/*
  NOTES
  -----
  
  To do
  -----
    -VMS VERSION:
        -based on vms 1.05, but the 'fix slow text' option has been added.
            that includes m_lpDDSText, CTextManager (m_text), changes to 
            DrawDarkTranslucentBox, the replacement of all DrawText calls
            (now routed through m_text), and adding the 'fix slow text' cb
            to the config panel.

    -KILLED FEATURES:
        -vj mode
    
    -NEW FEATURES FOR 1.04:
            -added the following variables for per-frame scripting: (all booleans, except 'gamma')
	            wave_usedots, wave_thick, wave_additive, wave_brighten
                gamma, darken_center, wrap, invert, brighten, darken, solarize
                (also, note that echo_zoom, echo_alpha, and echo_orient were already in there,
                 but weren't covered in the documentation!)
        d   -fixed: spectrum w/512 samples + 256 separation -> infinite spike
        d   -reverted dumb changes to aspect ratio stuff
        d   -reverted wave_y fix; now it's backwards, just like it's always been
                (i.e. for wave's y position, 0=bottom and 1=top, which is opposite
                to the convention in the rest of milkdrop.  decided to keep the
                'bug' so presets don't need modified.)
        d   -fixed: Krash: Inconsistency bug - pressing Escape while in the code windows 
                for custom waves completely takes you out of the editing menus, 
                rather than back to the custom wave menu 
        d   -when editing code: fix display of '&' character 
        d   -internal texture size now has a little more bias toward a finer texture, 
                based on the window size, when set to 'Auto'.  (Before, for example,
                to reach 1024x1024, the window had to be 768x768 or greater; now, it
                only has to be 640x640 (25% of the way there).  I adjusted it because
                before, at in-between resolutions like 767x767, it looked very grainy;
                now it will always look nice and crisp, at any window size, but still
                won't cause too much aliasing (due to downsampling for display).
        d   -fixed: rova:
                When creating presets have commented code // in the per_pixel section when cause error in preset.
                Example nothing in per_frame and just comments in the per_pixel. EXamples on repuest I have a few.
        d   -added kill keys:
                -CTRL+K kills all running sprites
                -CTRL+T kills current song title anim
                -CTRL+Y kills current custom message
        d   -notice to sprite users:
                -in milk_img.ini, color key can't be a range anymore; it's
                    now limited to just a single color.  'colorkey_lo' and 
                    'colorkey_hi' have been replaced with just one setting, 
                    'colorkey'.
        d   -song titles + custom messages are working again
        ?   -fixed?: crashes on window resize [out of mem]
                -Rova: BTW the same bug as krash with the window resizing.
                -NOT due to the 'integrate w/winamp' option.
                -> might be fixed now (had forgotten to release m_lpDDSText)
        <AFTER BETA 3..>
        d   -added checkbox to config screen to automatically turn SCROLL LOCK on @ startup
        d   -added alphanumeric seeking to the playlist; while playlist is up,
                you can now press A-Z and 0-9 to seek to the next song in the playlist
                that starts with that character.
        d   -<fixed major bug w/illegal mem access on song title launches;
                could have been causing crashing madness @ startup on many systems>
        d   -<fixed bug w/saving 64x48 mesh size>
        d   -<fixed squashed shapes>
        d   -<fixed 'invert' variable>
        d   -<fixed squashed song titles + custom msgs>
        ?   -<might have fixed scroll lock stuff>  
        ?   -<might have fixed crashing; could have been due to null ptr for failed creation of song title texture.>
        ?   -<also might have solved any remaining resize or exit bugs by callign SetTexture(NULL)
                in DX8 cleanup.>
        d   -<fixed sizing issues with songtitle font.>
        d   -<fixed a potentially bogus call to deallocate memory on exit, when it was cleaning up the menus.>
        d   -<fixed more scroll lock issues>
        d   -<fixed broken Noughts & Crosses presets; max # of per-frame vars was one too few, after the additions of the new built-in variables.>
        d   -<hopefully fixed waveforms>
        <AFTER BETA 4>
            -now when playlist is up, SHIFT+A-Z seeks upward (while lowercase/regular a-z seeks downward).
            -custom shapes:
                -OH MY GOD
                -increased max. # of custom shapes (and waves) from 3 to 4
                -added 'texture' option, which allows you to use the last frame as a texture on the shape
                    -added "tex_ang" and "tex_zoom" params to control the texture coords
                -each frame, custom shapes now draw BEFORE regular waveform + custom waves
                -added init + per-frame vars: "texture", "additive", "thick", "tex_ang", "tex_zoom"
            -fixed valid characters for filenames when importing/exporting custom shapes/waves;
                also, it now gives error messages on error in import/export.
            -cranked max. meshsize up to 96x72
            -Krash, Rova: now the 'q' variables, as modified by the preset per-frame equations, are again 
                readable by the custom waves + custom shapes.  Sorry about that.  Should be the end of the 
                'q' confusion.
            -added 'meshx' and 'meshy' [read-only] variables to the preset init, per-frame, and per-pixel 
                equations (...and inc'd the size of the global variable pool by 2).
            -removed t1-t8 vars for Custom Shapes; they were unnecessary (since there's no per-point code there).
            -protected custom waves from trying to draw when # of sample minus the separation is < 2
                (or 1 if drawing with dots)
            -fixed some [minor] preset-blending bugs in the custom wave code 
            -created a visual map for the flow of values for the q1-q8 and t1-t8 variables:
                q_and_t_vars.gif (or something).
            -fixed clipping of onscreen text in low-video-memory situations.  Now, if there isn't enough
                video memory to create an offscreen texture that is at least 75% of the size of the 
                screen (or to create at least a 256x256 one), it won't bother using one, and will instead draw text directly to the screen.
                Otherwise, if the texture is from 75%-99% of the screen size, text will now at least
                appear in the correct position on the screen so that it will be visible; this will mean
                that the right- and bottom-aligned text will no longer be fully right/bottom-aligned 
                to the edge of the screen.                
            -fixed blurry text 
            -VJ mode is partially restored; the rest will come with beta 7 or the final release.  At the time of beta 6, VJ mode still has some glitches in it, but I'm working on them.  Most notably, it doesn't resize the text image when you resize the window; that's next on my list.
        <AFTER BETA 6:>            
            -now sprites can burn-in on any frame, not just on the last frame.
                set 'burn' to one (in the sprite code) on any frame to make it burn in.
                this will break some older sprites, but it's super easy to fix, and 
                I think it's worth it. =)  thanks to papaw00dy for the suggestion!
            -fixed the asymptotic-value bug with custom waves using spectral data & having < 512 samples (thanks to telek's example!)
            -fixed motion vectors' reversed Y positioning.
            -fixed truncation ("...") of long custom messages
            -fixed that pesky bug w/the last line of code on a page
            -fixed the x-positioning of custom waves & shapes.  Before, if you were 
                saving some coordinates from the preset's per-frame equations (say in q1 and q2)
                and then you set "x = q1; y = q2;" in a custom shape's per-frame code
                (or in a custom wave's per-point code), the x position wouldn't really be
                in the right place, because of aspect ratio multiplications.  Before, you had
                to actually write "x = (q1-0.5)*0.75 + 0.5; y = q2;" to get it to line up; 
                now it's fixed, though, and you can just write "x = q1; y = q2;".
            -fixed some bugs where the plugin start up, in windowed mode, on the wrong window
                (and hence run super slow).
            -fixed some bugs w/a munged window frame when the "integrate with winamp" option
                was checked.
        <AFTER BETA 7:>
            -preset ratings are no longer read in all at once; instead, they are scanned in
                1 per frame until they're all in.  This fixes the long pauses when you switch
                to a directory that has many hundreds of presets.  If you want to switch
                back to the old way (read them all in at once), there is an option for it
                in the config panel.
            -cranked max. mesh size up to 128x96
            -fixed bug in custom shape per-frame code, where t1-t8 vars were not 
                resetting, at the beginning of each frame, to the values that they had 
                @ the end of the custom shape init code's execution.
            -
            -
            -


        beta 2 thread: http://forums.winamp.com/showthread.php?threadid=142635
        beta 3 thread: http://forums.winamp.com/showthread.php?threadid=142760
        beta 4 thread: http://forums.winamp.com/showthread.php?threadid=143500
        beta 6 thread: http://forums.winamp.com/showthread.php?threadid=143974
        (+read about beat det: http://forums.winamp.com/showthread.php?threadid=102205)

@       -code editing: when cursor is on 1st posn. in line, wrong line is highlighted!?
        -requests:
            -random sprites (...they can just write a prog for this, tho)
            -Text-entry mode.
                -Like your favorite online game, hit T or something to enter 'text entry' mode. Type a message, then either hit ESC to clear and cancel text-entry mode, or ENTER to display the text on top of the vis. Easier for custom messages than editing the INI file (and probably stopping or minimizing milkdrop to do it) and reloading.
                -OR SKIP IT; EASY TO JUST EDIT, RELOAD, AND HIT 00.
            -add 'AA' parameter to custom message text file?
        -when mem is low, fonts get kicked out -> white boxes
            -probably happening b/c ID3DXFont can't create a 
             temp surface to use to draw text, since all the
             video memory is gobbled up.
*       -add to installer: q_and_t_vars.gif
*       -presets:
            1. pick final set
                    a. OK-do a pass weeding out slow presets (crank mesh size up)
                    b. OK-do 2nd pass; rate them & delete crappies
                    c. OK-merge w/set from 1.03; check for dupes; delete more suckies
            2. OK-check for cpu-guzzlers
            3. OK-check for big ones (>= 8kb)
            4. check for ultra-spastic-when-audio-quiet ones
            5. update all ratings
            6. zip 'em up for safekeeping
*       -docs: 
                -link to milkdrop.co.uk
                -preset authoring:
                    -there are 11 variable pools:
                        preset:
                            a) preset init & per-frame code
                            b) preset per-pixel code
                        custom wave 1:
                            c) init & per-frame code
                            d) per-point code
                        custom wave 2:
                            e) init & per-frame code
                            f) per-point code
                        custom wave 3:
                            g) init & per-frame code
                            h) per-point code
                        i) custom shape 1: init & per-frame code
                        j) custom shape 2: init & per-frame code
                        k) custom shape 3: init & per-frame code

                    -all of these have predefined variables, the values of many of which 
                        trickle down from init code, to per-frame code, to per-pixel code, 
                        when the same variable is defined for each of these.
                    -however, variables that you define ("my_var = 5;") do NOT trickle down.
                        To allow you to pass custom values from, say, your per-frame code
                        to your per-pixel code, the variables q1 through q8 were created.
                        For custom waves and custom shapes, t1 through t8 work similarly.
                    -q1-q8:
                        -purpose: to allow [custom] values to carry from {the preset init
                            and/or per-frame equations}, TO: {the per-pixel equations},
                            {custom waves}, and {custom shapes}.
                        -are first set in preset init code.
                        -are reset, at the beginning of each frame, to the values that 
                            they had at the end of the preset init code. 
                        -can be modified in per-frame code...
                            -changes WILL be passed on to the per-pixel code 
                            -changes WILL pass on to the q1-q8 vars in the custom waves
                                & custom shapes code
                            -changes will NOT pass on to the next frame, though;
                                use your own (custom) variables for that.
                        -can be modified in per-pixel code...
                            -changes will pass on to the next *pixel*, but no further
                            -changes will NOT pass on to the q1-q8 vars in the custom
                                waves or custom shapes code.
                            -changes will NOT pass on to the next frame, after the
                                last pixel, though.
                        -CUSTOM SHAPES: q1-q8...
                            -are readable in both the custom shape init & per-frame code
                            -start with the same values as q1-q8 had at the end of the *preset*
                                per-frame code, this frame
                            -can be modified in the init code, but only for a one-time
                                pass-on to the per-frame code.  For all subsequent frames
                                (after the first), the per-frame code will get the q1-q8
                                values as described above.
                            -can be modified in the custom shape per-frame code, but only 
                                as temporary variables; the changes will not pass on anywhere.
                        -CUSTOM WAVES: q1-q8...
                            -are readable in the custom wave init, per-frame, and per-point code
                            -start with the same values as q1-q8 had at the end of the *preset*
                                per-frame code, this frame
                            -can be modified in the init code, but only for a one-time
                                pass-on to the per-frame code.  For all subsequent frames
                                (after the first), the per-frame code will get the q1-q8
                                values as described above.
                            -can be modified in the custom wave per-frame code; changes will
                                pass on to the per-point code, but that's it.
                            -can be modified in the per-point code, and the modified values
                                will pass on from point to point, but won't carry beyond that.
                        -CUSTOM WAVES: t1-t8...
                            -allow you to generate & save values in the custom wave init code,
                                that can pass on to the per-frame and, more sigificantly,
                                per-point code (since it's in a different variable pool).
                            -...                                
                        


                        !-whatever the values of q1-q8 were at the end of the per-frame and per-pixel
                            code, these are copied to the q1-q8 variables in the custom wave & custom 
                            shape code, for that frame.  However, those values are separate.
                            For example, if you modify q1-q8 in the custom wave #1 code, those changes 
                            will not be visible anywhere else; if you modify q1-q8 in the custom shape
                            #2 code, same thing.  However, if you modify q1-q8 in the per-frame custom
                            wave code, those modified values WILL be visible to the per-point custom
                            wave code, and can be modified within it; but after the last point,
                            the values q1-q8 will be discarded; on the next frame, in custom wave #1
                            per-frame code, the values will be freshly copied from the values of the 
                            main q1-q8 after the preset's per-frame and per-point code have both been
                            executed.                          
                    -monitor: 
                        -can be read/written in preset init code & preset per-frame code.
                        -not accessible from per-pixel code.
                        -if you write it on one frame, then that value persists to the next frame.
                    -t1-t8:
                        -
                        -
                        -
                -regular docs:
                    -put in the stuff recommended by VMS (vidcap, etc.)
                    -add to troubleshooting:
                        1) desktop mode icons not appearing?  or
                           onscreen text looking like colored boxes?
                             -> try freeing up some video memory.  lower your res; drop to 16 bit;
                                etc.  TURN OFF AUTO SONGTITLES.
                        1) slow desktop/fullscr mode?  -> try disabling auto songtitles + desktop icons.
                            also try reducing texsize to 256x256, since that eats memory that the text surface could claim.
                        2) 
                        3) 
        *   -presets:
                -add new 
                -fix 3d presets (bring gammas back down to ~1.0)
                -check old ones, make sure they're ok
                    -"Rovastar - Bytes"
                    -check wave_y
        *   -document custom waves & shapes
        *   -slow text is mostly fixed... =(
                -desktop icons + playlist both have begin/end around them now, but in desktop mode,
                 if you bring up playlist or Load menu, fps drops in half; press Esc, and fps doesn't go back up.
            -
            -
            -
        -DONE / v1.04:
            -updated to VMS 1.05
                -[list benefits...]
                -
                -
            -3d mode: 
                a) SWAPPED DEFAULT L/R LENS COLORS!  All images on the web are left=red, right=blue!                    
                b) fixed image display when viewing a 3D preset in a non-4:3 aspect ratio window
                c) gamma now works for 3d presets!  (note: you might have to update your old presets.
                        if they were 3D presets, the gamma was ignored and 1.0 was used; now,
                        if gamma was >1.0 in the old preset, it will probably appear extremely bright.)
                d) added SHIFT+F9 and CTRL+C9 to inc/dec stereo separation
                e) added default stereo separation to config panel
            -cranked up the max. mesh size (was 48x36, now 64x48) and the default mesh size
                (was 24x18, now 32x24)
            -fixed aspect ratio for final display
            -auto-texsize is now computed slightly differently; for vertically or horizontally-stretched
                windows, the texsize is now biased more toward the larger dimension (vs. just the
                average).
            -added anisotropic filtering (for machines that support it)
            -fixed bug where the values of many variables in the preset init code were not set prior 
                to execution of the init code (e.g. time, bass, etc. were all broken!)
            -added various preset blend effects.  In addition to the old uniform fade, there is
                now a directional wipe, radial wipe, and plasma fade.
            -FIXED SLOW TEXT for DX8 (at least, on the geforce 4).  
                Not using DT_RIGHT or DT_BOTTOM was the key.

        
        -why does text kill it in desktop mode?
        -text is SLOOW
        -to do: add (and use) song title font + tooltip font
        -re-test: menus, text, boxes, etc.
        -re-test: TIME        
        -testing:
            -make sure sound works perfectly.  (had to repro old pre-vms sound analysis!)
            -autogamma: currently assumes/requires that GetFrame() resets to 0 on a mode change
                (i.e. windowed -> fullscreen)... is that the case?
            -restore motion vectors
            -
            -
        -restore lost surfaces
        -test bRedraw flag (desktop mode/paused)
        -search for //? in milkdropfs.cpp and fix things
            
        problem: no good soln for VJ mode
        problem: D3DX won't give you solid background for your text.
            soln: (for later-) create wrapper fn that draws w/solid bkg.

        SOLN?: use D3DX to draw all text (plugin.cpp stuff AND playlist); 
        then, for VJ mode, create a 2nd DxContext 
        w/its own window + windowproc + fonts.  (YUCK)
    1) config panel: test, and add WM_HELP's (copy from tooltips)
    2) keyboard input: test; and...
        -need to reset UI_MODE when playlist is turned on, and
        -need to reset m_show_playlist when UI_MODE is changed.  (?)
        -(otherwise they can both show @ same time and will fight 
            for keys and draw over each other)
    3) comment out most of D3D stuff in milkdropfs.cpp, and then 
        get it running w/o any milkdrop, but with text, etc.
    4) sound

  Issues / To Do Later
  --------------------
    1) sprites: color keying stuff probably won't work any more...
    2) scroll lock: pull code from Monkey
    3) m_nGridY should not always be m_nGridX*3/4!
    4) get solid backgrounds for menus, waitstring code, etc.
        (make a wrapper function!)

    99) at end: update help screen

  Things that will be different
  -----------------------------
    1) font sizes are no longer relative to size of window; they are absolute.
    2) 'don't clear screen at startup' option is gone
    3) 'always on top' option is gone
    4) text will not be black-on-white when an inverted-color preset is showing

                -VJ mode:
                    -notes
                        1. (remember window size/pos, and save it from session to session?  nah.)
                        2. (kiv: scroll lock)
                        3. (VJ window + desktop mode:)
                                -ok w/o VJ mode
                                -w/VJ mode, regardless of 'fix slow text' option, probs w/focus;
                                    click on vj window, and plugin window flashes to top of Z order!
                                -goes away if you comment out 1st call to PushWindowToJustBeforeDesktop()...
                                -when you disable PushWindowToJustBeforeDesktop:
                                    -..and click on EITHER window, milkdrop jumps in front of the taskbar.
                                    -..and click on a non-MD window, nothing happens.
                                d-FIXED somehow, magically, while fixing bugs w/true fullscreen mode!
                        4. (VJ window + true fullscreen mode:)
                                d-make sure VJ window gets placed on the right monitor, at startup,
                                    and respects taskbar posn.
                                d-bug - start in windowed mode, then dbl-clk to go [true] fullscreen 
                                    on 2nd monitor, all with VJ mode on, and it excepts somewhere 
                                    in m_text.DrawNow() in a call to DrawPrimitive()!
                                    FIXED - had to check m_vjd3d8_device->TestCooperativeLevel
                                    each frame, and destroy/reinit if device needed reset.
                                d-can't resize VJ window when grfx window is running true fullscreen!
                                    -FIXED, by dropping the Sleep(30)/return when m_lost_focus
                                        was true, and by not consuming WM_NCACTIVATE in true fullscreen
                                        mode when m_hTextWnd was present, since DX8 doesn't do its
                                        auto-minimize thing in that case.



*/

#include "plugin.h"
#include "utility.h"
#include "support.h"
//#include "resource.h"
#include "defines.h"
#include "shell_defines.h"
#include <stdio.h>
#include <io.h>
#include <time.h>      // for time()
//#include <commctrl.h>  // for sliders
#include <assert.h>
//#include "../XmlDocument.h"

#define FRAND ((rand() % 7381)/7380.0f)
#define strnicmp _strnicmp
#define strcmpi  _strcmpi

//extern CSoundData*   pg_sound;	// declared in main.cpp
extern CPlugin* g_plugin;		// declared in MilkDropXBMC.cpp
extern char g_visName[];			// declared in MilkDropXBMC.cpp

// from support.cpp:
extern bool g_bDebugOutput;
extern bool g_bDumpFileCleared;

// these callback functions are called by menu.cpp whenever the user finishes editing an eval_ expression.
void OnUserEditedPerFrame(LPARAM param1, LPARAM param2)
{
	g_plugin->m_pState->RecompileExpressions(RECOMPILE_PRESET_CODE, 0);
}
void OnUserEditedPerPixel(LPARAM param1, LPARAM param2)
{
	g_plugin->m_pState->RecompileExpressions(RECOMPILE_PRESET_CODE, 0);
}
void OnUserEditedPresetInit(LPARAM param1, LPARAM param2)
{
	g_plugin->m_pState->RecompileExpressions(RECOMPILE_PRESET_CODE, 1);
}
void OnUserEditedWavecode(LPARAM param1, LPARAM param2)
{
	g_plugin->m_pState->RecompileExpressions(RECOMPILE_WAVE_CODE, 0);
}
void OnUserEditedWavecodeInit(LPARAM param1, LPARAM param2)
{
	g_plugin->m_pState->RecompileExpressions(RECOMPILE_WAVE_CODE, 1);
}
void OnUserEditedShapecode(LPARAM param1, LPARAM param2)
{
	g_plugin->m_pState->RecompileExpressions(RECOMPILE_SHAPE_CODE, 0);
}
void OnUserEditedShapecodeInit(LPARAM param1, LPARAM param2)
{
	g_plugin->m_pState->RecompileExpressions(RECOMPILE_SHAPE_CODE, 1);
}

// Modify the help screen text here.
// Watch the # of lines, though; if there are too many, they will get cut off;
//   and watch the length of the lines, since there is no wordwrap.  
// A good guideline: your entire help screen should be visible when fullscreen 
//   @ 640x480 and using the default help screen font.
char g_szHelp[] = 
{
    "ESC: exit help/menus/fullscreen\r"                    // part of framework
    "ALT+D: toggle desktop mode\r"          // part of framework
    "ALT+ENTER: toggle fullscreen\r"        // part of framework
    "\r"
	"SCROLL LOCK: [un]lock current preset\r"
	"L: load specific preset\r"
	"R: toggle random/sequential preset order\r"
	"H: instant Hard cut (to next preset)\r"
	"spacebar: transition to next preset\r"
	"+/-: rate current preset\r"
    "\r"
    "F1: toggle help display\r"                  // part of framework
    "F2: toggle song display\r"            // <- specific to this example plugin
    "F3: toggle song display\r"           // <- specific to this example plugin
	"F4: toggle preset name display\r"
    "F5: toggle fps display\r"                   // <- specific to this example plugin
	"F6: toggle preset rating display\r"
	"F7: reload milk_msg.ini\r"
	"F8: change directory/drive\r"
	"F9: toggle stereo 3D mode\r"
    "     + ctrl/shift = inc/dec depth\r"
    "\r"
    "PLAYBACK:\r"                           // part of framework
    "   ZXCVB: prev play pause stop next\r"   // part of framework
    "   P: show/hide playlist\r"              // part of framework
    "   U: toggle shuffle\r"         // part of framework
    "   up/down arrows: adjust vol.\r"        // part of framework
    "   left/right arrows: seek 5 sec.\r"            // part of framework
    "              +SHIFT: seek 30 sec.\r"       // part of framework
    "\r"
    "PRESET EDITING AND SAVING\r"
    "   M: show/hide preset-editing Menu\r"
    "   S: save new preset\r"
    "   N: show per-frame variable moNitor\r"
    "\r"
    "SPRITES, CUSTOM MESSAGES...\r"
    "   T: launch song title animation\r"
    "   Y: enter custom message mode\r"
    "   K: enter sprite mode\r"
    "   SHIFT + K: enter sprite kill mode\r"
    "   ** see milkdrop.html for the rest! **"
};

#define IPC_CB_VISRANDOM 628 // param is status of random

//----------------------------------------------------------------------

void CPlugin::OverrideDefaults()
{
    // Here, you have the option of overriding the "default defaults"
    //   for the stuff on tab 1 of the config panel, replacing them
    //   with custom defaults for your plugin.
    // To override any of the defaults, just uncomment the line 
    //   and change the value.
    // DO NOT modify these values from any function but this one!

    // This example plugin only changes the default width/height
    //   for fullscreen mode; the "default defaults" are just
    //   640 x 480.
    // If your plugin is very dependent on smooth animation and you
    //   wanted it plugin to have the 'save cpu' option OFF by default,
    //   for example, you could set 'm_save_cpu' to 0 here.

    // m_start_fullscreen      = 0;       // 0 or 1
    // m_start_desktop         = 0;       // 0 or 1
    // m_fake_fullscreen_mode  = 0;       // 0 or 1
     m_max_fps_fs            = 60;      // 1-120, or 0 for 'unlimited'
     m_max_fps_dm            = 60;      // 1-120, or 0 for 'unlimited'
     m_max_fps_w             = 60;      // 1-120, or 0 for 'unlimited'
    // m_show_press_f1_msg     = 1;       // 0 or 1
       m_allow_page_tearing_w  = 0;       // 0 or 1
    // m_allow_page_tearing_fs = 0;       // 0 or 1
    // m_allow_page_tearing_dm = 1;       // 0 or 1
    // m_minimize_winamp       = 1;       // 0 or 1
    // m_desktop_textlabel_boxes = 1;     // 0 or 1
    // m_save_cpu              = 0;       // 0 or 1

    // strcpy(m_fontinfo[0].szFace, "Trebuchet MS"); // system font
    // m_fontinfo[0].nSize     = 18;
    // m_fontinfo[0].bBold     = 0;
    // m_fontinfo[0].bItalic   = 0;
    // strcpy(m_fontinfo[1].szFace, "Times New Roman"); // decorative font
    // m_fontinfo[1].nSize     = 24;
    // m_fontinfo[1].bBold     = 0;
    // m_fontinfo[1].bItalic   = 1;

    m_disp_mode_fs.Width    = 1024;             // normally 640
    m_disp_mode_fs.Height   = 768;              // normally 480
    m_disp_mode_fs.Format   = D3DFMT_X8R8G8B8;  // use either D3DFMT_X8R8G8B8 or D3DFMT_R5G6B5.  The former will match to any 32-bit color format available, and the latter will match to any 16-bit color available, if that exact format can't be found.
    // m_disp_mode_fs.RefreshRate = 60;
}

//----------------------------------------------------------------------

void CPlugin::MyPreInitialize()
{
    // Initialize EVERY data member you've added to CPlugin here;
    //   these will be the default values.
    // If you want to initialize any of your variables with random values
    //   (using rand()), be sure to seed the random number generator first!
    // (If you want to change the default values for settings that are part of
    //   the plugin shell (framework), do so from OverrideDefaults() above.)

    // seed the system's random number generator w/the current system time:
    srand((unsigned)time(NULL));

    // CONFIG PANEL SETTINGS THAT WE'VE ADDED (TAB #2)
	m_bFirstRun		            = true;
	m_fBlendTimeUser			= 1.1f;
	m_fBlendTimeAuto			= 8.0f;
	m_fTimeBetweenPresets		= 8.0f;
	m_fTimeBetweenPresetsRand	= 5.0f;
	m_bSequentialPresetOrder    = false;
  m_bHoldPreset            = false;
	m_bHardCutsDisabled			= false;
	m_fHardCutLoudnessThresh	= 2.5f;
	m_fHardCutHalflife			= 60.0f;
	//m_nWidth			= 1024;
	//m_nHeight			= 768;
	//m_nDispBits		= 16;
	m_nTexSize			= 1024;	// -1 means "auto"
	m_nGridX			= 32;
	m_nGridY			= 24;

	m_bShowPressF1ForHelp = true;
	//strcpy(m_szMonitorName, "[don't use multimon]");
	m_bShowMenuToolTips = true;	// NOTE: THIS IS CURRENTLY HARDWIRED TO TRUE - NO OPTION TO CHANGE
	m_n16BitGamma	= 2;
	m_bAutoGamma    = true;
	//m_nFpsLimit			= -1;
	m_cLeftEye3DColor[0]	= 255;
	m_cLeftEye3DColor[1]	= 0;
	m_cLeftEye3DColor[2]	= 0;
	m_cRightEye3DColor[0]	= 0;
	m_cRightEye3DColor[1]	= 255;
	m_cRightEye3DColor[2]	= 255;
	m_bEnableRating			= false;
    m_bInstaScan            = false;
	m_bSongTitleAnims		= true;
	m_fSongTitleAnimDuration = 1.7f;
	m_fTimeBetweenRandomSongTitles = -1.0f;
	m_fTimeBetweenRandomCustomMsgs = -1.0f;
	m_nSongTitlesSpawned = 0;
	m_nCustMsgsSpawned = 0;

    m_bAlways3D		  	    = false;
    m_fStereoSep            = 1.0f;
    //m_bAlwaysOnTop		= false;
    //m_bFixSlowText          = true;
    m_bWarningsDisabled     = false;
    m_bWarningsDisabled2    = false;
    m_bAnisotropicFiltering = true;
    m_bPresetLockOnAtStartup = false;

//    m_gdi_title_font_doublesize  = NULL;
//    m_d3dx_title_font_doublesize = NULL;

    // RUNTIME SETTINGS THAT WE'VE ADDED
    m_prev_time = GetTime() - 0.0333f; // note: this will be updated each frame, at bottom of MyRenderFn.
	m_bTexSizeWasAuto	= false;
	//m_bPresetLockedByUser = false;  NOW SET IN DERIVED SETTINGS
	m_bPresetLockedByCode = false;
	m_fStartTime	= 0.0f;
	m_fPresetStartTime = 0.0f;
	m_fNextPresetTime = -1.0f;	// negative value means no time set (...it will be auto-set on first call to UpdateTime)
	m_pState    = &m_state_DO_NOT_USE[0];
	m_pOldState = &m_state_DO_NOT_USE[1];
	m_UI_mode			= UI_REGULAR;
    //m_nTrackPlaying	= 0;
	//m_nSongPosMS      = 0;
	//m_nSongLenMS      = 0;
    m_bUserPagedUp      = false;
    m_bUserPagedDown    = false;
	m_fMotionVectorsTempDx = 0.0f;
	m_fMotionVectorsTempDy = 0.0f;
	
    m_waitstring.bActive		= false;
	m_waitstring.bOvertypeMode  = false;
	m_waitstring.szClipboard[0] = 0;

	m_nPresets		= 0;
	m_nDirs			= 0;
    m_nPresetListCurPos = 0;
	m_nCurrentPreset = 0;
	m_szCurrentPresetFile[0] = 0;
	m_pPresetAddr	= NULL;
	m_pfPresetRating = NULL;
	m_szpresets		= new char[16384];
	m_nSizeOfPresetList =      16384;
	m_szPresetDir[0] = 0; // will be set @ end of this function
    m_nRatingReadProgress = -1;

    myfft.Init(576, MY_FFT_SAMPLES, -1);
	memset(&mysound, 0, sizeof(mysound));

	//m_nTextHeightPixels = -1;
	//m_nTextHeightPixels_Fancy = -1;
	m_bShowFPS			= false;
	m_bShowRating		= false;
	m_bShowPresetInfo	= false;
	m_bShowDebugInfo	= false;
	m_bShowSongTitle	= false;
	m_bShowSongTime		= false;
	m_bShowSongLen		= false;
	m_fShowUserMessageUntilThisTime = -1.0f;
	m_fShowRatingUntilThisTime = -1.0f;
	m_szUserMessage[0]	= 0;
	m_szDebugMessage[0]	= 0;
    m_szSongTitle[0]    = 0;
    m_szSongTitlePrev[0] = 0;

	m_lpVS[0]				= NULL;
	m_lpVS[1]				= NULL;
//	m_lpDDSTitle			= NULL;
    m_nTitleTexSizeX        = 0;
    m_nTitleTexSizeY        = 0;
	m_verts					= NULL;
	m_verts_temp            = NULL;
	m_vertinfo				= NULL;
	m_indices				= NULL;

	m_bMMX			        = false;
    m_bHasFocus             = true;
    m_bHadFocus             = false;
    m_bOrigScrollLockState  = false;//GetKeyState(VK_SCROLL) & 1;
    // m_bMilkdropScrollLockState is derived at end of MyReadConfig()

	m_nNumericInputMode   = NUMERIC_INPUT_MODE_CUST_MSG;
	m_nNumericInputNum    = 0;
	m_nNumericInputDigits = 0;
	//td_custom_msg_font   m_CustomMessageFont[MAX_CUSTOM_MESSAGE_FONTS];
	//td_custom_msg        m_CustomMessage[MAX_CUSTOM_MESSAGES];

    //texmgr      m_texmgr;		// for user sprites

	m_supertext.bRedrawSuperText = false;
	m_supertext.fStartTime = -1.0f;

	// --------------------other init--------------------

    g_bDebugOutput		= false;
	g_bDumpFileCleared	= false;

	strcpy(m_szWinampPluginsPath,  GetConfigIniFile());
    char *p = strrchr(m_szWinampPluginsPath, '/');
    if (p) *(p+1) = 0;
	strcpy(m_szPresetDir,  m_szWinampPluginsPath);
	strcpy(m_szMsgIniFile, m_szWinampPluginsPath);
	strcpy(m_szImgIniFile, m_szWinampPluginsPath);
	strcat(m_szPresetDir,  "milkdrop/");
	strcat(m_szMsgIniFile, MSG_INIFILE);
	strcat(m_szImgIniFile, IMG_INIFILE);
	
	/*
    char buf[MAX_PATH];
    sprintf(buf, "Program version is %3.2f%c", INT_VERSION*0.01f, (INT_SUBVERSION==0) ? (' ') : ('a'+INT_SUBVERSION-1));
	dumpmsg(buf);
	sprintf(buf, "Config.ini file is %s", GetConfigIniFile());
	dumpmsg(buf);
	sprintf(buf, "Preset directory is %s", m_szPresetDir);
	dumpmsg(buf);
	sprintf(buf, "Winamp window handle is %08x", GetWinampWindow());
	dumpmsg(buf);
	sprintf(buf, "MMX %s detected.", m_bMMX ? "successfully " : "was NOT ");
	dumpmsg(buf);
	//sprintf(buf, "SSE %s detected.", m_bSSE ? "successfully " : "was NOT ");
	//dumpmsg(buf);
    */
}

//----------------------------------------------------------------------

extern void LoadSettings();

void CPlugin::MyReadConfig()
{
/*


    // Read the user's settings from the .INI file.
    // If you've added any controls to the config panel, read their value in
    //   from the .INI file here.

    // use this function         declared in   to read a value of this type:
    // -----------------         -----------   ----------------------------
    // InternalGetPrivateProfileInt      Win32 API     int
    // GetPrivateProfileBool     utility.h     bool
    // GetPrivateProfileBOOL     utility.h     BOOL
    // InternalGetPrivateProfileFloat    utility.h     float
    // InternalGetPrivateProfileString   Win32 API     string

    //ex: m_fog_enabled = InternalGetPrivateProfileInt("settings","fog_enabled"       ,m_fog_enabled       ,GetConfigIniFile());

	int n=0;
    char *pIni = GetConfigIniFile();

	m_bFirstRun		= !GetPrivateProfileBool("settings","bConfigured" ,false,pIni);
	m_bEnableRating = GetPrivateProfileBool("settings","bEnableRating",m_bEnableRating,pIni);
    m_bInstaScan    = GetPrivateProfileBool("settings","bInstaScan",m_bInstaScan,pIni);
	m_bHardCutsDisabled = GetPrivateProfileBool("settings","bHardCutsDisabled",m_bHardCutsDisabled,pIni);
	g_bDebugOutput	= GetPrivateProfileBool("settings","bDebugOutput",g_bDebugOutput,pIni);
	//m_bShowSongInfo = GetPrivateProfileBool("settings","bShowSongInfo",m_bShowSongInfo,pIni);
	//m_bShowPresetInfo=GetPrivateProfileBool("settings","bShowPresetInfo",m_bShowPresetInfo,pIni);
	m_bShowPressF1ForHelp = GetPrivateProfileBool("settings","bShowPressF1ForHelp",m_bShowPressF1ForHelp,pIni);
	//m_bShowMenuToolTips = GetPrivateProfileBool("settings","bShowMenuToolTips",m_bShowMenuToolTips,pIni);
	m_bSongTitleAnims   = GetPrivateProfileBool("settings","bSongTitleAnims",m_bSongTitleAnims,pIni);

	m_bShowFPS			= GetPrivateProfileBool("settings","bShowFPS",       m_bShowFPS			,pIni);
	m_bShowRating		= GetPrivateProfileBool("settings","bShowRating",    m_bShowRating		,pIni);
	m_bShowPresetInfo	= GetPrivateProfileBool("settings","bShowPresetInfo",m_bShowPresetInfo	,pIni);
	//m_bShowDebugInfo	= GetPrivateProfileBool("settings","bShowDebugInfo", m_bShowDebugInfo	,pIni);
	m_bShowSongTitle	= GetPrivateProfileBool("settings","bShowSongTitle", m_bShowSongTitle	,pIni);
	m_bShowSongTime		= GetPrivateProfileBool("settings","bShowSongTime",  m_bShowSongTime	,pIni);
	m_bShowSongLen		= GetPrivateProfileBool("settings","bShowSongLen",   m_bShowSongLen		,pIni);

	//m_bFixPinkBug		= GetPrivateProfileBool("settings","bFixPinkBug",m_bFixPinkBug,pIni);
	  int nTemp = GetPrivateProfileBool("settings","bFixPinkBug",-1,pIni);
	  if (nTemp == 0)
		  m_n16BitGamma = 0;
	  else if (nTemp == 1)
		  m_n16BitGamma = 2;
	m_n16BitGamma		= InternalGetPrivateProfileInt("settings","n16BitGamma",m_n16BitGamma,pIni);
	m_bAutoGamma        = GetPrivateProfileBool("settings","bAutoGamma",m_bAutoGamma,pIni);
	m_bAlways3D				= GetPrivateProfileBool("settings","bAlways3D",m_bAlways3D,pIni);
    m_fStereoSep            = InternalGetPrivateProfileFloat("settings","fStereoSep",m_fStereoSep,pIni);
	//m_bFixSlowText          = GetPrivateProfileBool("settings","bFixSlowText",m_bFixSlowText,pIni);
	//m_bAlwaysOnTop		= GetPrivateProfileBool("settings","bAlwaysOnTop",m_bAlwaysOnTop,pIni);
	m_bWarningsDisabled		= GetPrivateProfileBool("settings","bWarningsDisabled",m_bWarningsDisabled,pIni);
	m_bWarningsDisabled2    = GetPrivateProfileBool("settings","bWarningsDisabled2",m_bWarningsDisabled2,pIni);
    m_bAnisotropicFiltering = GetPrivateProfileBool("settings","bAnisotropicFiltering",m_bAnisotropicFiltering,pIni);
    m_bPresetLockOnAtStartup = GetPrivateProfileBool("settings","bPresetLockOnAtStartup",m_bPresetLockOnAtStartup,pIni);

	m_cLeftEye3DColor[0] = InternalGetPrivateProfileInt("settings","nLeftEye3DColorR",m_cLeftEye3DColor[0],pIni);
	m_cLeftEye3DColor[1] = InternalGetPrivateProfileInt("settings","nLeftEye3DColorG",m_cLeftEye3DColor[1],pIni);
	m_cLeftEye3DColor[2] = InternalGetPrivateProfileInt("settings","nLeftEye3DColorB",m_cLeftEye3DColor[2],pIni);
	m_cRightEye3DColor[0] = InternalGetPrivateProfileInt("settings","nRightEye3DColorR",m_cRightEye3DColor[0],pIni);
	m_cRightEye3DColor[1] = InternalGetPrivateProfileInt("settings","nRightEye3DColorG",m_cRightEye3DColor[1],pIni);
	m_cRightEye3DColor[2] = InternalGetPrivateProfileInt("settings","nRightEye3DColorB",m_cRightEye3DColor[2],pIni);

	m_nTexSize		= InternalGetPrivateProfileInt("settings","nTexSize"    ,m_nTexSize    ,pIni);
		m_bTexSizeWasAuto = (m_nTexSize == -1);
	m_nGridX		= InternalGetPrivateProfileInt("settings","nMeshSize"   ,m_nGridX      ,pIni);
	m_nGridY        = m_nGridX*3/4;
	
	m_fBlendTimeUser			= InternalGetPrivateProfileFloat("settings","fBlendTimeUser"         ,m_fBlendTimeUser         ,pIni);
	m_fBlendTimeAuto			= InternalGetPrivateProfileFloat("settings","fBlendTimeAuto"         ,m_fBlendTimeAuto         ,pIni);
	m_fTimeBetweenPresets		= InternalGetPrivateProfileFloat("settings","fTimeBetweenPresets"    ,m_fTimeBetweenPresets    ,pIni);
	m_fTimeBetweenPresetsRand	= InternalGetPrivateProfileFloat("settings","fTimeBetweenPresetsRand",m_fTimeBetweenPresetsRand,pIni);
	m_fHardCutLoudnessThresh	= InternalGetPrivateProfileFloat("settings","fHardCutLoudnessThresh" ,m_fHardCutLoudnessThresh ,pIni);
	m_fHardCutHalflife			= InternalGetPrivateProfileFloat("settings","fHardCutHalflife"       ,m_fHardCutHalflife       ,pIni);
	m_fSongTitleAnimDuration	= InternalGetPrivateProfileFloat("settings","fSongTitleAnimDuration" ,m_fSongTitleAnimDuration ,pIni);
	m_fTimeBetweenRandomSongTitles = InternalGetPrivateProfileFloat("settings","fTimeBetweenRandomSongTitles" ,m_fTimeBetweenRandomSongTitles,pIni);
	m_fTimeBetweenRandomCustomMsgs = InternalGetPrivateProfileFloat("settings","fTimeBetweenRandomCustomMsgs" ,m_fTimeBetweenRandomCustomMsgs,pIni);

    // --------

	InternalGetPrivateProfileString("settings","szPresetDir",m_szPresetDir,m_szPresetDir,sizeof(m_szPresetDir),pIni);

	ReadCustomMessages();

	// bounds-checking:
	if (m_nGridX > MAX_GRID_X)
		m_nGridX = MAX_GRID_X;
	if (m_nGridY > MAX_GRID_Y)
		m_nGridY = MAX_GRID_Y;
	if (m_fTimeBetweenPresetsRand < 0)
		m_fTimeBetweenPresetsRand = 0;
	if (m_fTimeBetweenPresets < 0.1f)
		m_fTimeBetweenPresets = 0.1f;

    // DERIVED SETTINGS
    m_bPresetLockedByUser      = m_bPresetLockOnAtStartup;
    m_bMilkdropScrollLockState = m_bPresetLockOnAtStartup;
*/

	m_bPresetLockedByUser = false;

	//-------------------------------------------------------------------------
	// XML version

  LoadSettings();
}

//----------------------------------------------------------------------

void CPlugin::MyWriteConfig()
{
/*

    // Write the user's settings to the .INI file.
    // This gets called only when the user runs the config panel and hits OK.
    // If you've added any controls to the config panel, write their value out 
    //   to the .INI file here.

    // use this function         declared in   to write a value of this type:
    // -----------------         -----------   ----------------------------
    // WritePrivateProfileInt    Win32 API     int
    // WritePrivateProfileInt    utility.h     bool
    // WritePrivateProfileInt    utility.h     BOOL
    // WritePrivateProfileFloat  utility.h     float
    // WritePrivateProfileString Win32 API     string

    // ex: WritePrivateProfileInt(m_fog_enabled       ,"fog_enabled"       ,GetConfigIniFile(),"settings");

    char *pIni = GetConfigIniFile();

	// constants:
	WritePrivateProfileString("settings","bConfigured","1",pIni);
	
	//note: m_szPresetDir is not written here; it is written manually, whenever it changes.
	
	char szSectionName[] = "settings";
	
	WritePrivateProfileInt(m_bSongTitleAnims,		"bSongTitleAnims",		pIni, "settings");
	WritePrivateProfileInt(m_bHardCutsDisabled,	    "bHardCutsDisabled",	pIni, "settings");
	WritePrivateProfileInt(m_bEnableRating,		    "bEnableRating",		pIni, "settings");
	WritePrivateProfileInt(m_bInstaScan,            "bInstaScan",		    pIni, "settings");
	WritePrivateProfileInt(g_bDebugOutput,		    "bDebugOutput",			pIni, "settings");

	//itePrivateProfileInt(m_bShowPresetInfo, 	    "bShowPresetInfo",		pIni, "settings");
	//itePrivateProfileInt(m_bShowSongInfo, 		"bShowSongInfo",        pIni, "settings");
	//itePrivateProfileInt(m_bFixPinkBug, 		    "bFixPinkBug",			pIni, "settings");

	WritePrivateProfileInt(m_bShowPressF1ForHelp,   "bShowPressF1ForHelp",	pIni, "settings");
	//itePrivateProfileInt(m_bShowMenuToolTips, 	"bShowMenuToolTips",    pIni, "settings");
	WritePrivateProfileInt(m_n16BitGamma, 		    "n16BitGamma",			pIni, "settings");
	WritePrivateProfileInt(m_bAutoGamma,  		    "bAutoGamma",			pIni, "settings");

	WritePrivateProfileInt(m_bAlways3D, 			"bAlways3D",			pIni, "settings");
    WritePrivateProfileFloat(m_fStereoSep,          "fStereoSep",           pIni, "settings");
	//WritePrivateProfileInt(m_bFixSlowText,		    "bFixSlowText",			pIni, "settings");
	//itePrivateProfileInt(m_bAlwaysOnTop,		    "bAlwaysOnTop",			pIni, "settings");
	WritePrivateProfileInt(m_bWarningsDisabled,	    "bWarningsDisabled",	pIni, "settings");
	WritePrivateProfileInt(m_bWarningsDisabled2,	"bWarningsDisabled2",	pIni, "settings");
	WritePrivateProfileInt(m_bAnisotropicFiltering,	"bAnisotropicFiltering",pIni, "settings");
    WritePrivateProfileInt(m_bPresetLockOnAtStartup,"bPresetLockOnAtStartup",pIni,"settings");
		
	WritePrivateProfileInt(m_cLeftEye3DColor[0], 	"nLeftEye3DColorR",		pIni, "settings");
	WritePrivateProfileInt(m_cLeftEye3DColor[1], 	"nLeftEye3DColorG",		pIni, "settings");
	WritePrivateProfileInt(m_cLeftEye3DColor[2], 	"nLeftEye3DColorB",		pIni, "settings");
	WritePrivateProfileInt(m_cRightEye3DColor[0],   "nRightEye3DColorR",	pIni, "settings");
	WritePrivateProfileInt(m_cRightEye3DColor[1],   "nRightEye3DColorG",	pIni, "settings");
	WritePrivateProfileInt(m_cRightEye3DColor[2],   "nRightEye3DColorB",	pIni, "settings");
											
	WritePrivateProfileInt(m_nTexSize, 			    "nTexSize",				pIni, "settings");
	WritePrivateProfileInt(m_nGridX, 				"nMeshSize",			pIni, "settings");
	
	WritePrivateProfileFloat(m_fBlendTimeAuto,          "fBlendTimeAuto",           pIni, "settings");
	WritePrivateProfileFloat(m_fBlendTimeUser,          "fBlendTimeUser",           pIni, "settings");
	WritePrivateProfileFloat(m_fTimeBetweenPresets,     "fTimeBetweenPresets",      pIni, "settings");
	WritePrivateProfileFloat(m_fTimeBetweenPresetsRand, "fTimeBetweenPresetsRand",  pIni, "settings");
	WritePrivateProfileFloat(m_fHardCutLoudnessThresh,  "fHardCutLoudnessThresh",   pIni, "settings");
	WritePrivateProfileFloat(m_fHardCutHalflife,        "fHardCutHalflife",         pIni, "settings");
	WritePrivateProfileFloat(m_fSongTitleAnimDuration,  "fSongTitleAnimDuration",   pIni, "settings");
	WritePrivateProfileFloat(m_fTimeBetweenRandomSongTitles,"fTimeBetweenRandomSongTitles",pIni, "settings");
	WritePrivateProfileFloat(m_fTimeBetweenRandomCustomMsgs,"fTimeBetweenRandomCustomMsgs",pIni, "settings");
*/
}

//----------------------------------------------------------------------

int CPlugin::AllocateMyNonDx8Stuff()
{
    // This gets called only once, when your plugin is actually launched.
    // If only the config panel is launched, this does NOT get called.
    //   (whereas MyPreInitialize() still does).
    // If anything fails here, return FALSE to safely exit the plugin,
    //   but only after displaying a messagebox giving the user some information
    //   about what went wrong.

    /*
    if (!m_hBlackBrush)
		m_hBlackBrush = CreateSolidBrush(RGB(0,0,0));
    */
	
	BuildMenus();

	m_bMMX = CheckForMMX();
	//m_bSSE = CheckForSSE();

	m_pState->Default();
	m_pOldState->Default();

  // JM
  if (m_bHoldPreset && m_nPresets >= 0)
  {
    printf("Preset is held, loading preset %i", m_nCurrentPreset);
    if (m_nCurrentPreset > 0)
    {
      m_nCurrentPreset--;
      LoadNextPreset(0.0f);
    }
    else
    {
      m_nCurrentPreset++;
      LoadPreviousPreset(0.0f);
    }
  }
  else
  {
    printf("Preset is not held, loading a random preset. Current is %i", m_nCurrentPreset);
	  LoadRandomPreset(0.0f);
  }
  // JM
    return true;
}

//----------------------------------------------------------------------

void CPlugin::CleanUpMyNonDx8Stuff()
{
    // This gets called only once, when your plugin exits.
    // Be sure to clean up any objects here that were 
    //   created/initialized in AllocateMyNonDx8Stuff.
    
    //sound.Finish();

	if (m_pPresetAddr) 
	{
		delete m_pPresetAddr;
		m_pPresetAddr = NULL;
	}

	if (m_pfPresetRating)
	{
		delete m_pfPresetRating;
		m_pfPresetRating = NULL;
	}

	if (m_szpresets)
	{
		delete m_szpresets;
		m_szpresets = NULL;
	}

    // NOTE: DO NOT DELETE m_gdi_titlefont_doublesize HERE!!!

/*
	m_menuPreset  .Finish();
	m_menuWave    .Finish();
	m_menuAugment .Finish();
    m_menuCustomWave.Finish();
    m_menuCustomShape.Finish();
	m_menuMotion  .Finish();
	m_menuPost    .Finish();
    for (int i=0; i<MAX_CUSTOM_WAVES; i++)
	    m_menuWavecode[i].Finish();
    for (i=0; i<MAX_CUSTOM_SHAPES; i++)
	    m_menuShapecode[i].Finish();
*/

    SetScrollLock(m_bOrigScrollLockState);

    //dumpmsg("Finish: cleanup complete.");
}

//----------------------------------------------------------------------

int CPlugin::AllocateMyDX8Stuff() 
{
    // (...aka OnUserResizeWindow) 
    // (...aka OnToggleFullscreen)
    
    // Allocate and initialize all your DX8 and D3DX stuff here: textures, 
    //   surfaces, vertex/index buffers, D3DX fonts, and so on.  
    // If anything fails here, return FALSE to safely exit the plugin,
    //   but only after displaying a messagebox giving the user some information
    //   about what went wrong.  If the error is NON-CRITICAL, you don't *have*
    //   to return; just make sure that the rest of the code will be still safely 
    //   run (albeit with degraded features).  
    // If you run out of video memory, you might want to show a short messagebox
    //   saying what failed to allocate and that the reason is a lack of video
    //   memory, and then call SuggestHowToFreeSomeMem(), which will show them
    //   a *second* messagebox that (intelligently) suggests how they can free up 
    //   some video memory.
    // Don't forget to account for each object you create/allocate here by cleaning
    //   it up in CleanUpMyDX8Stuff!
    // IMPORTANT:
    //   Note that the code here isn't just run at program startup!
    //   When the user toggles between fullscreen and windowed modes
    //   or resizes the window, the base class calls this function before 
    //   destroying & recreating the plugin window and DirectX object, and then
    //   calls AllocateMyDX8Stuff afterwards, to get your plugin running again.


    char buf[2048];


    /*char fname[512];
    sprintf(fname, "%s%s", GetPluginsDirPath(), TEXTURE_NAME);
    if (D3DXCreateTextureFromFile(GetDevice(), fname, &m_object_tex) != S_OK)
    {
        // just give a warning, and move on
        m_object_tex = NULL;    // (make sure pointer wasn't mangled by some bad driver)

        char msg[1024];
        sprintf(msg, "Unable to load texture:\r%s", fname);
        MessageBox(GetPluginWindow(), msg, "WARNING", MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
        //return false;
    }*/

    // Note: this code used to be in OnResizeGraphicsWindow().

	// create m_lpVS[2] 
    {
	    // auto-guess texsize
/*	    if (m_bTexSizeWasAuto)
	    {
		    float fExp = logf( max(GetWidth(),GetHeight())*0.75f + 0.25f*min(GetWidth(),GetHeight()) ) / logf(2.0f);
            float bias = 0.667f;
            if (fExp + bias >= 11.0f)   // ..don't jump to 2048x2048 quite as readily
                bias = 0.5f;
		    int   nExp = (int)(fExp + bias);
		    m_nTexSize = (int)powf(2.0f, (float)nExp);
	    }
*/	    

		// clip texsize by max. from caps
	    if (m_nTexSize > GetCaps()->MaxTextureWidth && GetCaps()->MaxTextureWidth>0)
		    m_nTexSize = GetCaps()->MaxTextureWidth;
	    if (m_nTexSize > GetCaps()->MaxTextureHeight && GetCaps()->MaxTextureHeight>0)
		    m_nTexSize = GetCaps()->MaxTextureHeight;

	    // reallocate
	    bool bSuccess = false;
	    int nOrigTexSize = m_nTexSize;

	    do
	    {
		    SafeRelease(m_lpVS[0]);
		    SafeRelease(m_lpVS[1]);

        LPDIRECT3DSURFACE9 pBackBuffer, pZBuffer, tmpSurface;
        D3DSURFACE_DESC tmpDesc;
        D3DVIEWPORT9 pVP;
        GetDevice()->GetRenderTarget(0, &pBackBuffer );
        GetDevice()->GetDepthStencilSurface(&tmpSurface);
        tmpSurface->GetDesc(&tmpDesc);
        GetDevice()->GetViewport(&pVP);
        UINT uiwidth=(pVP.Width>m_nTexSize) ? pVP.Width:m_nTexSize;
        UINT uiheight=(pVP.Height>m_nTexSize) ? pVP.Height:m_nTexSize;
        
        printf("CreateDepthStencilSurface with %u x %u", uiwidth, uiheight);
        if(GetDevice()->CreateDepthStencilSurface(uiwidth, uiheight, tmpDesc.Format, D3DMULTISAMPLE_NONE, 0, TRUE, &pZBuffer, NULL) != D3D_OK)
          printf("Can't create DepthStencilSurface");

        if(GetDevice()->SetDepthStencilSurface(pZBuffer) != D3D_OK)
          printf("failed to set DepthStencilSurface");
		    // create VS1 and VS2
        bSuccess = (GetDevice()->CreateTexture(m_nTexSize, m_nTexSize, 1, D3DUSAGE_RENDERTARGET, GetBackBufFormat(), D3DPOOL_DEFAULT, &m_lpVS[0], NULL) == D3D_OK);
        if (bSuccess)
			  {
				  IDirect3DSurface9* pNewTarget = NULL;
				  if (m_lpVS[0]->GetSurfaceLevel(0, &pNewTarget) == D3D_OK) 
				  {
					  GetDevice()->SetRenderTarget(0, pNewTarget);
					  GetDevice()->Clear(0, 0, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
					  pNewTarget->Release();
				  }

          bSuccess = (GetDevice()->CreateTexture(m_nTexSize, m_nTexSize, 1, D3DUSAGE_RENDERTARGET, GetBackBufFormat(), D3DPOOL_DEFAULT, &m_lpVS[1], NULL) == D3D_OK);
          if (bSuccess)
				  {
					  if (m_lpVS[1]->GetSurfaceLevel(0, &pNewTarget) == D3D_OK) 
					  {
						  GetDevice()->SetRenderTarget(0, pNewTarget);
						  GetDevice()->Clear(0, 0, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
						  pNewTarget->Release();
					  }
				  }
          else
            printf("failed to create texture %d x %d", m_nTexSize, m_nTexSize);
			  }
        else
          printf("failed to create texture %d x %d", m_nTexSize, m_nTexSize);

        GetDevice()->SetRenderTarget(0, pBackBuffer);
        SafeRelease(pBackBuffer);
        SafeRelease(pZBuffer);
        SafeRelease(tmpSurface);


		    if (!bSuccess && m_bTexSizeWasAuto)
			    m_nTexSize /= 2;
	    }
	    while (!bSuccess && m_nTexSize >= 256 && m_bTexSizeWasAuto);

	    if (!bSuccess)
	    {
            char buf[2048];
		    if (m_bTexSizeWasAuto || m_nTexSize==256)
			    sprintf(buf, "couldn't create offscreen surfaces! (probably not enough video memory left)\rtry selecting a smaller video mode or changing the color depth.");
		    else
			    sprintf(buf, "couldn't create offscreen surfaces! (probably not enough video memory left)\r\r\rRECOMMENDATION: SET THE TEXTURE SIZE BACK TO 'AUTO' AND TRY AGAIN");
		    dumpmsg(buf);
//		    MessageBox(GetPluginWindow(), buf, "MILKDROP ERROR", MB_OK|MB_SETFOREGROUND|MB_TOPMOST );
		    return false;
	    }
      printf("Textures created!");

	    if (m_nTexSize != nOrigTexSize)
	    {
            char buf[2048];
		    sprintf(buf, "Init: -WARNING-: low memory; ideal auto texture size would be %d, but it had to be lowered to %d!", nOrigTexSize, m_nTexSize);
		    dumpmsg(buf);
		    if (!m_bWarningsDisabled)
		    {
			    sprintf(buf, "WARNING: low video memory; auto texture had to be reduced\rfrom %dx%d to %dx%d.\r\rThe image may look chunky or blocky.\r", nOrigTexSize, nOrigTexSize, m_nTexSize, m_nTexSize);
			    if (GetScreenMode() != FULLSCREEN)
			    {
                    switch(GetBackBufFormat())
                    {
//                    case D3DFMT_R8G8B8  :
                    case D3DFMT_A8R8G8B8:
                    case D3DFMT_X8R8G8B8:
					    strcat(buf, "\rRECOMMENDATION: **reduce your Windows color depth to 16 bits**\rto free up some video memory, or lower your screen resolution.\r");
                        break;                    
                    default:
                        strcat(buf, "\rRECOMMENDATION: lower your screen resolution, to free up some video memory.\r");
                        break;
                    }
			    }
				dumpmsg(buf);
//			    MessageBox(GetPluginWindow(), buf, "WARNING", MB_OK|MB_SETFOREGROUND|MB_TOPMOST );
		    }
	    }
    }

    // -----------------

	/*if (m_bFixSlowText && !m_bSeparateTextWindow)
	{
        if (D3DXCreateTexture(GetDevice(), GetWidth(), GetHeight(), 1, D3DUSAGE_RENDERTARGET, GetBackBufFormat(), D3DPOOL_DEFAULT, &m_lpDDSText) != D3D_OK)
		{
            char buf[2048];
			dumpmsg("Init: -WARNING-:"); 
			sprintf(buf, "WARNING: Not enough video memory to make a dedicated text surface; \rtext will still be drawn directly to the back buffer.\r\rTo avoid seeing this error again, uncheck the 'fix slow text' option.");
			dumpmsg(buf); 
			if (!m_bWarningsDisabled)
				MessageBox(GetPluginWindow(), buf, "WARNING", MB_OK|MB_SETFOREGROUND|MB_TOPMOST );
			m_lpDDSText = NULL;
		}
	}*/

    // -----------------

#if 0
	// reallocate the texture for font titles + custom msgs (m_lpDDSTitle)
	{
		m_nTitleTexSizeX = m_nTexSize;
		m_nTitleTexSizeY = m_nTexSize/2;

		//dumpmsg("Init: [re]allocating title surface");

        // [DEPRECATED as of transition to dx8:] 
		// We could just create one title surface, but this is a problem because many
		// systems can only call DrawText on DDSCAPS_OFFSCREENPLAIN surfaces, and can NOT
		// draw text on a DDSCAPS_TEXTURE surface (it comes out garbled).  
		// So, we create one of each; we draw the text to the DDSCAPS_OFFSCREENPLAIN surface 
		// (m_lpDDSTitle[1]), then we blit that (once) to the DDSCAPS_TEXTURE surface 
		// (m_lpDDSTitle[0]), which can then be drawn onto the screen on polys.

		int square = 0;
        HRESULT hr;

		do
		{
			hr = D3DXCreateTexture(GetDevice(), m_nTitleTexSizeX, m_nTitleTexSizeY, 1, D3DUSAGE_RENDERTARGET, GetBackBufFormat(), D3DPOOL_DEFAULT, &m_lpDDSTitle);
			if (hr != D3D_OK)
			{
				if (square==0)
				{
					square = 1;
					m_nTitleTexSizeY *= 2;
				}
				else
				{
					m_nTitleTexSizeX /= 2;
					m_nTitleTexSizeY /= 2;
				}
			}
		}
		while (hr != D3D_OK && m_nTitleTexSizeX > 16);

		if (hr != D3D_OK)
		{
			//dumpmsg("Init: -WARNING-: Title texture could not be created!");
            m_lpDDSTitle = NULL;
            //SafeRelease(m_lpDDSTitle);
			//return true;
		}
		else
		{
			//sprintf(buf, "Init: title texture size is %dx%d (ideal size was %dx%d)", m_nTitleTexSizeX, m_nTitleTexSizeY, m_nTexSize, m_nTexSize/4);
			//dumpmsg(buf);
			m_supertext.bRedrawSuperText = true;
		}
	}
#endif
    // -----------------

#if 0
    // create 'm_gdi_title_font_doublesize'
    int songtitle_font_size = m_fontinfo[SONGTITLE_FONT].nSize * m_nTitleTexSizeX/256;
    if (songtitle_font_size<6) songtitle_font_size=6;
    if (!(m_gdi_title_font_doublesize = CreateFont(songtitle_font_size, 0, 0, 0, m_fontinfo[SONGTITLE_FONT].bBold ? 900 : 400, m_fontinfo[SONGTITLE_FONT].bItalic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, m_fontinfo[SONGTITLE_FONT].bAntiAliased ? ANTIALIASED_QUALITY : DEFAULT_QUALITY, DEFAULT_PITCH, m_fontinfo[SONGTITLE_FONT].szFace)))
    {
        MessageBox(NULL, "Error creating double-sized GDI title font", "ERROR", MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
        return false;
    }

    if (D3DXCreateFont(GetDevice(), m_gdi_title_font_doublesize, &m_d3dx_title_font_doublesize) != D3D_OK)
    {
        MessageBox(GetPluginWindow(), "Error creating double-sized d3dx title font", "ERROR", MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
        return false;
    }
#endif
    // -----------------

//    m_texmgr.Init(GetDevice());

	//dumpmsg("Init: mesh allocation");
	m_verts      = new SPRITEVERTEX[(m_nGridX+1)*(m_nGridY+1)];
	m_verts_temp = new SPRITEVERTEX[(m_nGridX+2)];
	m_vertinfo   = new td_vertinfo[(m_nGridX+1)*(m_nGridY+1)];
	m_indices    = new WORD[(m_nGridX+2)*(m_nGridY*2)];
	if (!m_verts || !m_vertinfo)
	{
		sprintf(buf, "couldn't allocate mesh - out of memory");
		dumpmsg(buf); 
//		MessageBox(GetPluginWindow(), buf, "MILKDROP ERROR", MB_OK|MB_SETFOREGROUND|MB_TOPMOST );
		return false;
	}

	int nVert = 0;
	for (int y=0; y<=m_nGridY; y++)
	{
		for (int x=0; x<=m_nGridX; x++)
		{
			// precompute x,y,z
			m_verts[nVert].x = x/(float)m_nGridX*2.0f - 1.0f;
			m_verts[nVert].y = y/(float)m_nGridY*2.0f - 1.0f;
			m_verts[nVert].z = 0.0f;

			// precompute rad, ang, being conscious of aspect ratio
			m_vertinfo[nVert].rad = sqrtf(m_verts[nVert].x*m_verts[nVert].x + m_verts[nVert].y*m_verts[nVert].y*ASPECT*ASPECT);
			if (y==m_nGridY/2 && x==m_nGridX/2)
				m_vertinfo[nVert].ang = 0.0f;
			else
				m_vertinfo[nVert].ang = atan2f(m_verts[nVert].y*ASPECT, m_verts[nVert].x);
            m_vertinfo[nVert].a = 1;
            m_vertinfo[nVert].c = 0;

			nVert++;
		}
	}
	
	int xref, yref;
	nVert = 0;
	for (int quadrant=0; quadrant<4; quadrant++)
	{
		for (int slice=0; slice < m_nGridY/2; slice++)
		{
			for (int i=0; i < m_nGridX + 2; i++)
			{
				// quadrants:	2 3
				//				0 1

				xref = i/2;
				yref = (i%2) + slice;

				if (quadrant & 1)
					xref = m_nGridX - xref;

				if (quadrant & 2)
					yref = m_nGridY - yref;

				m_indices[nVert++] = xref + (yref)*(m_nGridX+1);
			}
		}
	}

    return true;
}

//----------------------------------------------------------------------

void CPlugin::CleanUpMyDX8Stuff(int final_cleanup)
{
    // Clean up all your DX8 and D3DX textures, fonts, buffers, etc. here.
    // EVERYTHING CREATED IN ALLOCATEMYDX8STUFF() SHOULD BE CLEANED UP HERE.
    // The input parameter, 'final_cleanup', will be 0 if this is 
    //   a routine cleanup (part of a window resize or switch between
    //   fullscr/windowed modes), or 1 if this is the final cleanup
    //   and the plugin is exiting.  Note that even if it is a routine
    //   cleanup, *you still have to release ALL your DirectX stuff,
    //   because the DirectX device is being destroyed and recreated!*
    // Also set all the pointers back to NULL;
    //   this is important because if we go to reallocate the DX8
    //   stuff later, and something fails, then CleanUp will get called,
    //   but it will then be trying to clean up invalid pointers.)
    // The SafeRelease() and SafeDelete() macros make your code prettier;
    //   they are defined here in utility.h as follows:
    //       #define SafeRelease(x) if (x) {x->Release(); x=NULL;}
    //       #define SafeDelete(x)  if (x) {delete x; x=NULL;} 
    // IMPORTANT:
    //   This function ISN'T only called when the plugin exits!
    //   It is also called whenever the user toggles between fullscreen and 
    //   windowed modes, or resizes the window.  Basically, on these events, 
    //   the base class calls CleanUpMyDX8Stuff before Reset()ing the DirectX 
    //   device, and then calls AllocateMyDX8Stuff afterwards.


    // NOTE: not necessary; shell does this for us.
    /*if (GetDevice())
    {
        GetDevice()->SetTexture(0, NULL);
        GetDevice()->SetTexture(1, NULL);
    }*/

    // 2. release stuff
    SafeRelease(m_lpVS[0]);
    SafeRelease(m_lpVS[1]);
//    SafeRelease(m_lpDDSTitle);
//    SafeRelease(m_d3dx_title_font_doublesize);

    // NOTE: THIS CODE IS IN THE RIGHT PLACE.
/*    if (m_gdi_title_font_doublesize)
    {
        DeleteObject(m_gdi_title_font_doublesize);
        m_gdi_title_font_doublesize = NULL;
    }
*/
//    m_texmgr.Finish();

	if (m_verts != NULL)
	{
		delete m_verts;
		m_verts = NULL;
	}

	if (m_verts_temp != NULL)
	{
		delete m_verts_temp;
		m_verts_temp = NULL;
	}

	if (m_vertinfo != NULL)
	{
		delete m_vertinfo;
		m_vertinfo = NULL;
	}

	if (m_indices != NULL)
	{
		delete m_indices;
		m_indices = NULL;
	}
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

void CPlugin::MyRenderFn(int redraw)
{
    // Render a frame of animation here.  
    // This function is called each frame just AFTER BeginScene().
    // For timing information, call 'GetTime()' and 'GetFps()'.
    // The usual formula is like this (but doesn't have to be):
    //   1. take care of timing/other paperwork/etc. for new frame
    //   2. clear the background
    //   3. get ready for 3D drawing
    //   4. draw your 3D stuff
    //   5. call PrepareFor2DDrawing()
    //   6. draw your 2D stuff (overtop of your 3D scene)
    // If the 'redraw' flag is 1, you should try to redraw
    //   the last frame; GetTime, GetFps, and GetFrame should
    //   all return the same values as they did on the last 
    //   call to MyRenderFn().  Otherwise, the redraw flag will
    //   be zero, and you can draw a new frame.  The flag is
    //   used to force the desktop to repaint itself when 
    //   running in desktop mode and Winamp is paused or stopped.

    //   1. take care of timing/other paperwork/etc. for new frame
    {
        float dt = GetTime() - m_prev_time;
        m_prev_time = GetTime(); // note: m_prev_time is not for general use!
        if (m_bPresetLockedByUser || m_bPresetLockedByCode)
        {
		    m_fPresetStartTime += dt;
		    m_fNextPresetTime += dt;
        }

        UpdatePresetRatings(); // read in a few each frame, til they're all in
    }

    // 2. check for lost or gained kb focus:
    // (note: can't use wm_setfocus or wm_killfocus because they don't work w/embedwnd)
/*
	if (GetFrame()==0)
    {
        SetScrollLock(m_bPresetLockOnAtStartup);
    }
    else
    {
        m_bHadFocus = m_bHasFocus;
        if (GetParent(GetPluginWindow()) == GetWinampWindow())
            m_bHasFocus = (GetFocus() == GetPluginWindow());
        else
            m_bHasFocus = (GetFocus() == GetParent(GetPluginWindow())) || 
                          (GetFocus() == GetPluginWindow());
        
        if (m_hTextWnd && GetFocus()==m_hTextWnd)
            m_bHasFocus = 1;

        if (GetFocus()==NULL)
            m_bHasFocus = 0;
                          ;
        //HWND t1 = GetFocus();
        //HWND t2 = GetPluginWindow();
        //HWND t3 = GetParent(t2);
        
        if (m_bHadFocus==1 && m_bHasFocus==0)
        {
            m_bMilkdropScrollLockState = GetKeyState(VK_SCROLL) & 1;
            SetScrollLock(m_bOrigScrollLockState);
        }
        else if (m_bHadFocus==0 && m_bHasFocus==1)
        {
            m_bOrigScrollLockState = GetKeyState(VK_SCROLL) & 1;
            SetScrollLock(m_bMilkdropScrollLockState);
        }
    }

    GetWinampSongTitle(GetWinampWindow(), m_szSongTitle, sizeof(m_szSongTitle)-1);
    if (strcmp(m_szSongTitle, m_szSongTitlePrev))
    {
        strcpy(m_szSongTitlePrev, m_szSongTitle);
        if (m_bSongTitleAnims)
            LaunchSongTitleAnim();
    }
*/
    // 2. Clear the background:
    //DWORD clear_color = (m_fog_enabled) ? FOG_COLOR : 0xFF000000;
    //GetDevice()->Clear(0, 0, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, clear_color, 1.0f, 0);

    // 5. switch to 2D drawing mode.  2D coord system:
    //         +--------+ Y=-1
    //         |        |
    //         | screen |             Z=0: front of scene
    //         |        |             Z=1: back of scene
    //         +--------+ Y=1
    //       X=-1      X=1
    PrepareFor2DDrawing(GetDevice());

    DoCustomSoundAnalysis();    // emulates old pre-vms milkdrop sound analysis

    RenderFrame(redraw);  // see milkdropfs.cpp

    /*
    for (int i=0; i<10; i++)
    {
        RECT r;
        r.top = GetHeight()*i/10;
        r.left = 0;
        r.right = GetWidth();
        r.bottom = r.top + GetFontHeight(DECORATIVE_FONT);
        char buf[256];
        switch(i)
        {
        case 0: strcpy(buf, "this is a test"); break;
        case 1: strcpy(buf, "argh"); break;
        case 2: strcpy(buf, "!!"); break;
        case 3: strcpy(buf, "TESTING FONTS"); break;
        case 4: strcpy(buf, "rancid bear grease"); break;
        case 5: strcpy(buf, "whoppers and ding dongs"); break;
        case 6: strcpy(buf, "billy & joey"); break;
        case 7: strcpy(buf, "."); break;
        case 8: strcpy(buf, "---"); break;
        case 9: strcpy(buf, "test"); break;
        }
        int t = (int)( 54 + 18*sin(i/10.0f*53.7f + 1) - 28*sin(i/10.0f*39.4f + 3) );
        if (((GetFrame() + i*107) % t) < t*8/9)
            m_text.QueueText(GetFont(DECORATIVE_FONT), buf, r, 0, 0xFFFF00FF);
    }
    /**/
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

void CPlugin::DrawTooltip(char* str, int xR, int yB)
{
#if 0
	/*
    // draws a string in the lower-right corner of the screen.
    // note: ID3DXFont handles DT_RIGHT and DT_BOTTOM *very poorly*.
    //       it is best to calculate the size of the text first, 
    //       then place it in the right spot.
    // note: use DT_WORDBREAK instead of DT_WORD_ELLIPSES, otherwise certain fonts'
    //       calcrect (for the dark box) will be wrong.

    RECT r, r2;
    SetRect(&r, 0, 0, xR-TEXT_MARGIN*2, 2048);
    m_text.DrawText(GetFont(TOOLTIP_FONT), str, -1, &r, DT_NOPREFIX | /*DT_SINGLELINE |*/ DT_WORDBREAK  | DT_CALCRECT, 0xFFFFFFFF);
    r2.bottom = yB - TEXT_MARGIN;
    r2.right  = xR - TEXT_MARGIN;
    r2.left   = r2.right - (r.right-r.left);
    r2.top    = r2.bottom - (r.bottom-r.top);
    //m_text.DrawText(GetFont(TOOLTIP_FONT), str, -1, &r2, DT_NOPREFIX | /*DT_SINGLELINE |*/ DT_WORD_ELLIPSIS, 0xFF000000);
    RECT r3 = r2; r3.left -= 4; r3.top -= 2; r3.right += 2; r3.bottom += 2;
    DrawDarkTranslucentBox(&r3);
    m_text.DrawText(GetFont(TOOLTIP_FONT), str, -1, &r2, DT_NOPREFIX | /*DT_SINGLELINE |*/ DT_WORDBREAK , 0xFFFFFFFF);
#endif
}

#define MTO_UPPER_RIGHT 0
#define MTO_UPPER_LEFT  1
#define MTO_LOWER_RIGHT 2
#define MTO_LOWER_LEFT  3

#define SelectFont(n) { \
    pFont = GetFont(n); \
    h = GetFontHeight(n); \
}
#define MyTextOut(str, corner) { \
    SetRect(&r, 0, 0, xR-xL, 2048); \
    m_text.DrawText(pFont, str, -1, &r, DT_NOPREFIX | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_CALCRECT, 0xFFFFFFFF); \
    int w = r.right - r.left; \
    if      (corner == MTO_UPPER_LEFT ) SetRect(&r, xL, *upper_left_corner_y, xL+w, *upper_left_corner_y + h); \
    else if (corner == MTO_UPPER_RIGHT) SetRect(&r, xR-w, *upper_right_corner_y, xR, *upper_right_corner_y + h); \
    else if (corner == MTO_LOWER_LEFT ) SetRect(&r, xL, *lower_left_corner_y - h, xL+w, *lower_left_corner_y); \
    else if (corner == MTO_LOWER_RIGHT) SetRect(&r, xR-w, *lower_right_corner_y - h, xR, *lower_right_corner_y); \
    m_text.DrawText(pFont, str, -1, &r, DT_NOPREFIX | DT_SINGLELINE | DT_WORD_ELLIPSIS, 0xFFFFFFFF); \
    if      (corner == MTO_UPPER_LEFT ) *upper_left_corner_y  += h; \
    else if (corner == MTO_UPPER_RIGHT) *upper_right_corner_y += h; \
    else if (corner == MTO_LOWER_LEFT ) *lower_left_corner_y  -= h; \
    else if (corner == MTO_LOWER_RIGHT) *lower_right_corner_y -= h; \
}

void CPlugin::MyRenderUI(
                         int *upper_left_corner_y,  // increment me!
                         int *upper_right_corner_y, // increment me!
                         int *lower_left_corner_y,  // decrement me!
                         int *lower_right_corner_y, // decrement me!
                         int xL, 
                         int xR
                        )
{

#if 0
    // draw text messages directly to the back buffer.
    // when you draw text into one of the four corners,
    //   draw the text at the current 'y' value for that corner
    //   (one of the first 4 params to this function),
    //   and then adjust that y value so that the next time
    //   text is drawn in that corner, it gets drawn above/below
    //   the previous text (instead of overtop of it).
    // when drawing into the upper or lower LEFT corners,
    //   left-align your text to 'xL'.
    // when drawing into the upper or lower RIGHT corners,
    //   right-align your text to 'xR'.

    // note: try to keep the bounding rectangles on the text small; 
    //   the smaller the area that has to be locked (to draw the text),
    //   the faster it will be.  (on some cards, drawing text is 
    //   ferociously slow, so even if it works okay on yours, it might
    //   not work on another video card.)
    // note: if you want some text to be on the screen often, and the text
    //   won't be changing every frame, please consider the poor folks 
    //   whose video cards hate that; in that case you should probably
    //   draw the text just once, to a texture, and then display the
    //   texture each frame.  This is how the help screen is done; see
    //   pluginshell.cpp for example code.

    RECT r;
    char buf[512];
    LPD3DXFONT pFont = GetFont(DECORATIVE_FONT);
    int h = GetFontHeight(DECORATIVE_FONT);

    if (!pFont)
        return;

    if (!GetFont(DECORATIVE_FONT))
        return;

    // 1. render text in upper-right corner
    {
        // a) preset name
		if (m_bShowPresetInfo)
		{
            SelectFont(DECORATIVE_FONT);
			sprintf(buf, "%s ", m_pState->m_szDesc);
            MyTextOut(buf, MTO_UPPER_RIGHT);
		}

        // b) preset rating
		if (m_bShowRating || GetTime() < m_fShowRatingUntilThisTime)
		{
			// see also: SetCurrentPresetRating() in milkdrop.cpp
            SelectFont(DECORATIVE_FONT);
			sprintf(buf, " Rating: %d ", (int)m_pState->m_fRating);	
			if (!m_bEnableRating)
				strcat(buf, "[disabled] ");
            MyTextOut(buf, MTO_UPPER_RIGHT);
		}

        // c) fps display
        if (m_bShowFPS)
        {
            SelectFont(DECORATIVE_FONT);
            sprintf(buf, "fps: %4.2f ", GetFps()); // leave extra space @ end, so italicized fonts don't get clipped
            MyTextOut(buf, MTO_UPPER_RIGHT);
        }

		// d) custom timed message:
		if (!m_bWarningsDisabled2 && GetTime() < m_fShowUserMessageUntilThisTime)
		{
            SelectFont(SIMPLE_FONT);
			sprintf(buf, "%s ", m_szUserMessage);
            MyTextOut(buf, MTO_UPPER_RIGHT);
		}

        // e) debug information
		if (m_bShowDebugInfo)
		{
            SelectFont(SIMPLE_FONT);
			sprintf(buf, " pf monitor: %6.4f ", (float)(*m_pState->var_pf_monitor));
            MyTextOut(buf, MTO_UPPER_RIGHT);
		}
    }

    // 2. render text in lower-right corner
    {
        // waitstring tooltip:
		if (m_waitstring.bActive && m_bShowMenuToolTips && m_waitstring.szToolTip[0])
		{
            DrawTooltip(m_waitstring.szToolTip, xR, *lower_right_corner_y);
		}
    }

    // 3. render text in lower-left corner
    {
        char buf2[512];
        char buf3[512+1]; // add two extra spaces to end, so italicized fonts don't get clipped

        // render song title in lower-left corner:
        if (m_bShowSongTitle)
        {
            SelectFont(DECORATIVE_FONT);
            GetWinampSongTitle(GetWinampWindow(), buf, sizeof(buf)); // defined in utility.h/cpp
            sprintf(buf3, "%s ", buf); 
            MyTextOut(buf3, MTO_LOWER_LEFT);
        }

        // render song time & len above that:
        if (m_bShowSongTime || m_bShowSongLen)
        {
            GetWinampSongPosAsText(GetWinampWindow(), buf); // defined in utility.h/cpp
            GetWinampSongLenAsText(GetWinampWindow(), buf2); // defined in utility.h/cpp
            if (m_bShowSongTime && m_bShowSongLen)
                sprintf(buf3, "%s / %s ", buf, buf2);
            else if (m_bShowSongTime)
                strcpy(buf3, buf);
            else
                strcpy(buf3, buf2);

            SelectFont(DECORATIVE_FONT);
            MyTextOut(buf3, MTO_LOWER_LEFT);
        }
    }

    // 4. render text in upper-left corner
    {
		char buf[8192*3];  // leave extra space for &->&&, and [,[,& insertion

        SelectFont(SIMPLE_FONT);

		// stuff for loading presets, menus, etc:

		if (m_waitstring.bActive)
		{
			// 1. draw the prompt string
            MyTextOut(m_waitstring.szPrompt, MTO_UPPER_LEFT);

            // 2. reformat the waitstring text for display
            int bBrackets = m_waitstring.nSelAnchorPos != -1 && m_waitstring.nSelAnchorPos != m_waitstring.nCursorPos;
            int bCursorBlink = bBrackets ? 0 : ((int)(GetTime()*3.0f) % 2);
			strcpy(buf, m_waitstring.szText);

            int temp_cursor_pos = m_waitstring.nCursorPos;
            int temp_anchor_pos = m_waitstring.nSelAnchorPos;

            // replace &'s in the code with '&&' so they actually show up
            {
                char buf2[sizeof(buf)];
                int len = strlen(buf), y=0;
                for (int x=0; x<=len; x++)
                {
                    buf2[y++] = buf[x];
                    if (buf[x]=='&'  &&  (x != m_waitstring.nCursorPos || !bCursorBlink))
                    {
                        buf2[y++] = '&';
                        if (x<m_waitstring.nCursorPos)
                            temp_cursor_pos++;
                        if (x<m_waitstring.nSelAnchorPos)
                            temp_anchor_pos++;
                    }
                }
                strcpy(buf, buf2);
            }

			if (bBrackets)
			{
				// insert [] around the selection
				int start = (temp_cursor_pos < temp_anchor_pos) ? temp_cursor_pos : temp_anchor_pos;
				int end   = (temp_cursor_pos > temp_anchor_pos) ? temp_cursor_pos - 1 : temp_anchor_pos - 1;
				int len   = strlen(buf);
				int i;

				for (i=len; i>end; i--)
					buf[i+1] = buf[i];
				buf[end+1] = ']';
				len++;

				for (i=len; i>=start; i--)
					buf[i+1] = buf[i];
				buf[start] = '[';
				len++;
			}
			else
			{
				// underline the current cursor position by inserting an '&'
				if (temp_cursor_pos == (int)strlen(buf))
				{
					if (bCursorBlink)
						strcat(buf, "_");
					else
						strcat(buf, " ");
				}
				else if (bCursorBlink)
				{
					for (int i=strlen(buf); i>=temp_cursor_pos; i--)
						buf[i+1] = buf[i];
					buf[temp_cursor_pos] = '&';
				}
			}

            RECT rect;
            SetRect(&rect, xL, *upper_left_corner_y, xR, *lower_left_corner_y);
            rect.top += PLAYLIST_INNER_MARGIN;
            rect.left += PLAYLIST_INNER_MARGIN;
            rect.right -= PLAYLIST_INNER_MARGIN;
            rect.bottom -= PLAYLIST_INNER_MARGIN;

			// then draw the edit string
			if (m_waitstring.bDisplayAsCode)
			{
				char buf2[8192];
				int top_of_page_pos = 0;

				// compute top_of_page_pos so that the line the cursor is on will show.
                // also compute dims of the black rectangle while we're at it.
				{
					int start = 0;
					int pos   = 0;
					int ypixels = 0;
					int page  = 1;
                    int exit_on_next_page = 0;

                    RECT box = rect;
                    box.right = box.left;
                    box.bottom = box.top;

					while (buf[pos] != 0)  // for each line of text... (note that it might wrap)
					{
						start = pos;
						while (buf[pos] != LINEFEED_CONTROL_CHAR && buf[pos] != 0) 
							pos++;

						char ch = buf[pos];
						buf[pos] = 0;
						sprintf(buf2, "   %s ", &buf[start]);
						RECT r2 = rect;
                        r2.bottom = 4096;
						m_text.DrawText(GetFont(SIMPLE_FONT), buf2, -1, &r2, DT_CALCRECT | DT_WORDBREAK, 0xFFFFFFFF);
                        int h = r2.bottom-r2.top;
						ypixels += h;
						buf[pos] = ch;
						
                        if (start > m_waitstring.nCursorPos) // make sure 'box' gets updated for each line on this page
                            exit_on_next_page = 1;

						if (ypixels > rect.bottom-rect.top) // this line belongs on the next page
						{
                            if (exit_on_next_page)
                            {
                                buf[start] = 0; // so text stops where the box stops, when we draw the text
                                break;
                            }

							ypixels = h;
							top_of_page_pos = start;
							page++;

                            box = rect;
                            box.right = box.left;
                            box.bottom = box.top;
						}
                        box.bottom += h;
                        box.right = max(box.right, box.left + r2.right-r2.left);
                        
                        if (buf[pos]==0)
                            break;
                        pos++;
					}

                    // use r2 to draw a dark box:
                    box.top -= PLAYLIST_INNER_MARGIN;
                    box.left -= PLAYLIST_INNER_MARGIN;
                    box.right += PLAYLIST_INNER_MARGIN;
                    box.bottom += PLAYLIST_INNER_MARGIN;
                    DrawDarkTranslucentBox(&box);
                    *upper_left_corner_y += box.bottom - box.top + PLAYLIST_INNER_MARGIN*3;
                    
					sprintf(m_waitstring.szToolTip, " Page %d ", page);
				}

				// display multiline (replace all character 13's with a CR)
				{
					int start = top_of_page_pos;
					int pos   = top_of_page_pos;
					
					while (buf[pos] != 0)
					{
						while (buf[pos] != LINEFEED_CONTROL_CHAR && buf[pos] != 0) 
							pos++;

						char ch = buf[pos];
						buf[pos] = 0;
						sprintf(buf2, "   %s ", &buf[start]);
                        DWORD color = MENU_COLOR;
						if (m_waitstring.nCursorPos >= start && m_waitstring.nCursorPos <= pos)
							color = MENU_HILITE_COLOR;
						rect.top += m_text.DrawText(GetFont(SIMPLE_FONT), buf2, -1, &rect, DT_WORDBREAK, color);
						buf[pos] = ch;

                        if (rect.top > rect.bottom)
                            break;
						
						if (buf[pos] != 0) pos++;
						start = pos;
					} 
				}
                // note: *upper_left_corner_y is updated above, when the dark box is drawn.
			}
			else
			{
				// display on one line
				char buf2[8192];
				sprintf(buf2, "    %s ", buf);
                RECT box = rect;
                box.bottom = 4096;
				m_text.DrawText(GetFont(SIMPLE_FONT), buf2, -1, &box, DT_WORDBREAK | DT_CALCRECT, MENU_COLOR );

                // use r2 to draw a dark box:
                box.top -= PLAYLIST_INNER_MARGIN;
                box.left -= PLAYLIST_INNER_MARGIN;
                box.right += PLAYLIST_INNER_MARGIN;
                box.bottom += PLAYLIST_INNER_MARGIN;
                DrawDarkTranslucentBox(&box);
                *upper_left_corner_y += box.bottom - box.top + PLAYLIST_INNER_MARGIN*3;

                m_text.DrawText(GetFont(SIMPLE_FONT), buf2, -1, &rect, DT_WORDBREAK, MENU_COLOR );
			}
		}
		else if (m_UI_mode == UI_MENU)
		{
			assert(m_pCurMenu);
            SetRect(&r, xL, *upper_left_corner_y, xR, *lower_left_corner_y);
            
            RECT darkbox;
			m_pCurMenu->DrawMenu(r, xR, *lower_right_corner_y, 1, &darkbox);
            *upper_left_corner_y += darkbox.bottom - darkbox.top + PLAYLIST_INNER_MARGIN*3;

            darkbox.right += PLAYLIST_INNER_MARGIN*2;
            darkbox.bottom += PLAYLIST_INNER_MARGIN*2;
            DrawDarkTranslucentBox(&darkbox);

            r.top += PLAYLIST_INNER_MARGIN;
            r.left += PLAYLIST_INNER_MARGIN;
            r.right += PLAYLIST_INNER_MARGIN;
            r.bottom += PLAYLIST_INNER_MARGIN;
			m_pCurMenu->DrawMenu(r, xR, *lower_right_corner_y);
		}
		else if (m_UI_mode == UI_LOAD_DEL)
		{
            RECT rect;
            SetRect(&rect, xL, *upper_left_corner_y, xR, *lower_left_corner_y);
			rect.top += m_text.DrawText(GetFont(SIMPLE_FONT),  "Are you SURE you want to delete this preset? [y/N]", -1, &rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR);
			sprintf(buf, "(preset to delete: %s)", m_pPresetAddr[m_nPresetListCurPos]);
			rect.top += m_text.DrawText(GetFont(SIMPLE_FONT),  buf, -1, &rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR);
            *upper_left_corner_y = rect.top;
		}
		else if (m_UI_mode == UI_SAVE_OVERWRITE)
		{
            RECT rect;
            SetRect(&rect, xL, *upper_left_corner_y, xR, *lower_left_corner_y);
			rect.top += m_text.DrawText(GetFont(SIMPLE_FONT),  "This file already exists.  Overwrite it?  [y/N]", -1, &rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR);
			sprintf(buf, "(file in question: %s.milk)", m_waitstring.szText);
			rect.top += m_text.DrawText(GetFont(SIMPLE_FONT),  buf, -1, &rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR);
            *upper_left_corner_y = rect.top;
		}
		else if (m_UI_mode == UI_LOAD)
		{
			if (m_nPresets == 0)
			{
				// note: this error message is repeated in milkdrop.cpp in LoadRandomPreset()
				sprintf(m_szUserMessage, "ERROR: No preset files found in %s*.milk", m_szPresetDir);
				m_fShowUserMessageUntilThisTime = GetTime() + 6.0f;
				m_UI_mode = UI_REGULAR;
			}
			else
			{
                MyTextOut("Load which preset?  (arrow keys to scroll; Esc/close, Enter/select, INS/rename; DEL/delete)", MTO_UPPER_LEFT);

                RECT rect;
                SetRect(&rect, xL, *upper_left_corner_y, xR, *lower_left_corner_y);
                rect.top += PLAYLIST_INNER_MARGIN;
                rect.left += PLAYLIST_INNER_MARGIN;
                rect.right -= PLAYLIST_INNER_MARGIN;
                rect.bottom -= PLAYLIST_INNER_MARGIN;

				int lines_available = (rect.bottom - rect.top - PLAYLIST_INNER_MARGIN*2) / GetFontHeight(SIMPLE_FONT);

				if (lines_available < 1)
				{
					// force it
					rect.bottom = rect.top + GetFontHeight(SIMPLE_FONT) + 1;
					lines_available = 1;
				}
                if (lines_available > MAX_PRESETS_PER_PAGE)
                    lines_available = MAX_PRESETS_PER_PAGE;

				if (m_bUserPagedDown)
				{
					m_nPresetListCurPos += lines_available;
					if (m_nPresetListCurPos >= m_nPresets)
						m_nPresetListCurPos = m_nPresets - 1;
					
					// remember this preset's name so the next time they hit 'L' it jumps straight to it
					//strcpy(m_szLastPresetSelected, m_pPresetAddr[m_nPresetListCurPos]);
					
					m_bUserPagedDown = false;
				}

				if (m_bUserPagedUp)
				{
					m_nPresetListCurPos -= lines_available;
					if (m_nPresetListCurPos < 0)
						m_nPresetListCurPos = 0;
					
					// remember this preset's name so the next time they hit 'L' it jumps straight to it
					//strcpy(m_szLastPresetSelected, m_pPresetAddr[m_nPresetListCurPos]);
					
					m_bUserPagedUp = false;
				}

				int i;
				int first_line = m_nPresetListCurPos - (m_nPresetListCurPos % lines_available);
				int last_line  = first_line + lines_available;
				char str[512], str2[512];

				if (last_line > m_nPresets) 
					last_line = m_nPresets;

				// tooltip:
				if (m_bShowMenuToolTips)
				{
					char buf[256];
					sprintf(buf, " (page %d of %d) ", m_nPresetListCurPos/lines_available+1, (m_nPresets+lines_available-1)/lines_available);
                    DrawTooltip(buf, xR, *lower_right_corner_y);
				}

                RECT orig_rect = rect;

                RECT box;
                box.top = rect.top;
                box.left = rect.left;
                box.right = rect.left;
                box.bottom = rect.top;

                for (int pass=0; pass<2; pass++)
                {   
                    //if (pass==1)
                    //    GetFont(SIMPLE_FONT)->Begin();

                    rect = orig_rect;
				    for (i=first_line; i<last_line; i++)
				    {
					    // remove the extension before displaying the filename.  also pad w/spaces.
					    //strcpy(str, m_pPresetAddr[i]);
					    bool bIsDir = (m_pPresetAddr[i][0] == '*');
					    bool bIsRunning = false;
					    bool bIsSelected = (i == m_nPresetListCurPos);
					    
					    if (bIsDir)
					    {
						    // directory
						    if (strcmp(m_pPresetAddr[i]+1, "..")==0)
							    sprintf(str2, " [ %s ] (parent directory) ", m_pPresetAddr[i]+1);
						    else
							    sprintf(str2, " [ %s ] ", m_pPresetAddr[i]+1);
					    }
					    else
					    {
						    // preset file
						    strcpy(str, m_pPresetAddr[i]);
						    RemoveExtension(str);
						    sprintf(str2, " %s ", str);

						    if (strcmp(m_pState->m_szDesc, str)==0)
							    bIsRunning = true;
					    }
					    
					    if (bIsRunning && m_bPresetLockedByUser)
						    strcat(str2, "<locked> ");

                        DWORD color = bIsDir ? DIR_COLOR : PLAYLIST_COLOR_NORMAL;
                        if (bIsRunning)
                            color = bIsSelected ? PLAYLIST_COLOR_BOTH : PLAYLIST_COLOR_PLAYING_TRACK;
                        else if (bIsSelected)
                            color = PLAYLIST_COLOR_HILITE_TRACK;

                        RECT r2 = rect;
                        rect.top += m_text.DrawText(GetFont(SIMPLE_FONT),  str2, -1, &r2, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | (pass==0 ? DT_CALCRECT : 0), color);

                        if (pass==0)  // calculating dark box 
                        {
                            box.right = max(box.right, box.left + r2.right-r2.left);
                            box.bottom += r2.bottom-r2.top;
                        }
				    }

                    //if (pass==1)
                    //    GetFont(SIMPLE_FONT)->End();

                    if (pass==0)  // calculating dark box 
                    {
                        box.top -= PLAYLIST_INNER_MARGIN;
                        box.left -= PLAYLIST_INNER_MARGIN;
                        box.right += PLAYLIST_INNER_MARGIN;
                        box.bottom += PLAYLIST_INNER_MARGIN;
                        DrawDarkTranslucentBox(&box);
                        *upper_left_corner_y = box.bottom + PLAYLIST_INNER_MARGIN;
                    }
                }
			}
		}
    }

#endif
}

//----------------------------------------------------------------------

LRESULT CPlugin::MyWindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    // You can handle Windows messages here while the plugin is running, 
    //   such as mouse events (WM_MOUSEMOVE/WM_LBUTTONDOWN), keypresses 
    //   (WK_KEYDOWN/WM_CHAR), and so on.
    // This function is threadsafe (thanks to Winamp's architecture), 
    //   so you don't have to worry about using semaphores or critical 
    //   sections to read/write your class member variables.
    // If you don't handle a message, let it continue on the usual path 
    //   (to Winamp) by returning DefWindowProc(hWnd,uMsg,wParam,lParam).
    // If you do handle a message, prevent it from being handled again
    //   (by Winamp) by returning 0.

    // IMPORTANT: For the WM_KEYDOWN, WM_KEYUP, and WM_CHAR messages,
    //   you must return 0 if you process the message (key),
    //   and 1 if you do not.  DO NOT call DefWindowProc() 
    //   for these particular messages!

    SHORT mask = 1 << (sizeof(SHORT)*8 - 1);
    bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0;
    bool bCtrlHeldDown  = (GetKeyState(VK_CONTROL) & mask) != 0;

    switch (uMsg)
    {

    case WM_COMMAND:
      {
      switch (LOWORD(wParam)) {
        case ID_VIS_NEXT: 
            LoadNextPreset(m_fBlendTimeUser);
            return 0;
        case ID_VIS_PREV: 
            LoadPreviousPreset(m_fBlendTimeUser);
            return 0;
        case ID_VIS_RANDOM: {
            USHORT v = HIWORD(wParam) ? 1 : 0; 
            if (wParam >> 16 == 0xFFFF) {
         		  SendMessage(GetWinampWindow(),WM_WA_IPC,!m_bHoldPreset,IPC_CB_VISRANDOM);
              break;
            }
            if (!v) {
              m_bHoldPreset = 1;
              m_bSequentialPresetOrder = 1;
            } else {
              m_bHoldPreset = 0;
              m_bSequentialPresetOrder = 0;
            }
            if (v) LoadRandomPreset(m_fBlendTimeUser);
		        sprintf(m_szUserMessage, "random presets %s", v ? "on" : "off");
		        m_fShowUserMessageUntilThisTime = GetTime() + 4.0f;
            return 0;
          }
        case ID_VIS_FS:
            if (GetFrame() > 0) ToggleFullScreen();
            return 0;
        case ID_VIS_CFG:
            ToggleHelp();
            return 0;
        case ID_VIS_MENU:
            POINT pt;
            GetCursorPos(&pt);
            SendMessage(hWnd, WM_CONTEXTMENU, (int)hWnd, (pt.y << 16) | pt.x);
            return 0;
        }
      }

    /*
    case WM_SETFOCUS:
        m_bOrigScrollLockState = GetKeyState(VK_SCROLL) & 1;
        SetScrollLock(m_bMilkdropScrollLockState);
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    case WM_KILLFOCUS:
        m_bMilkdropScrollLockState = GetKeyState(VK_SCROLL) & 1;
        SetScrollLock(m_bOrigScrollLockState);
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    */

    case WM_CHAR:   // plain & simple alphanumeric keys
		if (m_waitstring.bActive)	// if user is in the middle of editing a string
		{
			if (wParam >= ' ' && wParam <= 'z')
			{
				int len = strlen(m_waitstring.szText);

				if (m_waitstring.bFilterBadChars &&
					(wParam == '\"' ||
					wParam == '\\' ||
					wParam == '/' ||
					wParam == ':' ||
					wParam == '*' ||
					wParam == '?' ||
					wParam == '|' ||
					wParam == '<' ||
					wParam == '>' ||
					wParam == '&'))	// NOTE: '&' is legal in filenames, but we try to avoid it since during GDI display it acts as a control code (it will not show up, but instead, underline the character following it).
				{
					// illegal char
					strcpy(m_szUserMessage, "(illegal character)");
					m_fShowUserMessageUntilThisTime = GetTime() + 2.5f;
				}
				else if (len+1 >= m_waitstring.nMaxLen)
				{
					// m_waitstring.szText has reached its limit
					strcpy(m_szUserMessage, "(string too long)");
					m_fShowUserMessageUntilThisTime = GetTime() + 2.5f;
				}
				else
				{
					m_fShowUserMessageUntilThisTime = GetTime();	// if there was an error message already, clear it

					char buf[16];
					sprintf(buf, "%c", wParam);

					if (m_waitstring.nSelAnchorPos != -1)
						WaitString_NukeSelection();

					if (m_waitstring.bOvertypeMode)
					{
						// overtype mode
						if (m_waitstring.nCursorPos == len)
							strcat(m_waitstring.szText, buf);
						else
							m_waitstring.szText[m_waitstring.nCursorPos] = buf[0];
						m_waitstring.nCursorPos++;
					}
					else
					{
						// insert mode:
						for (int i=len; i>=m_waitstring.nCursorPos; i--)
							m_waitstring.szText[i+1] = m_waitstring.szText[i];
						m_waitstring.szText[m_waitstring.nCursorPos] = buf[0];
						m_waitstring.nCursorPos++;
					}
				}
			}
            return 0; // we processed (or absorbed) the key
		}
		else if (m_UI_mode == UI_LOAD_DEL)	// waiting to confirm file delete
		{
			if (wParam == 'y' || wParam == 'Y')
			{
				// first add pathname to filename
				char szDelFile[512];
				sprintf(szDelFile, "%s%s", GetPresetDir(), m_pPresetAddr[m_nPresetListCurPos]);

				DeletePresetFile(szDelFile);
			}

			m_UI_mode = UI_LOAD;

            return 0; // we processed (or absorbed) the key
		}
		else if (m_UI_mode == UI_SAVE_OVERWRITE)	// waiting to confirm overwrite file on save
		{
			if (wParam == 'y' || wParam == 'Y')
			{
				// first add pathname + extension to filename
				char szNewFile[512];
				sprintf(szNewFile, "%s%s.milk", GetPresetDir(), m_waitstring.szText);

				SavePresetAs(szNewFile);

				// exit waitstring mode
				m_UI_mode = UI_REGULAR;
				m_waitstring.bActive = false;
				m_bPresetLockedByCode = false;
			}
			else if ((wParam >= ' ' && wParam <= 'z') || wParam == 27)		// 27 is the ESCAPE key
			{
				// go back to SAVE AS mode
				m_UI_mode = UI_SAVEAS;
				m_waitstring.bActive = true;
			}

            return 0; // we processed (or absorbed) the key
		}
		else	// normal handling of a simple key (all non-virtual-key hotkeys end up here)
		{
			if (HandleRegularKey(wParam)==0)
                return 0;
		}
        return 1; // end case WM_CHAR

    case WM_KEYDOWN:    // virtual-key codes
        // Note that some keys will never reach this point, since they are 
        //   intercepted by the plugin shell (see PluginShellWindowProc(),
        //   at the end of pluginshell.cpp for which ones).
        // For a complete list of virtual-key codes, look up the keyphrase
        //   "virtual-key codes [win32]" in the msdn help.
		switch(wParam)
		{
		case VK_F2:		m_bShowSongTitle = !m_bShowSongTitle;   return 0; // we processed (or absorbed) the key
		case VK_F3:
			if (m_bShowSongTime && m_bShowSongLen)
			{
				m_bShowSongTime = false;
				m_bShowSongLen  = false;
			}
			else if (m_bShowSongTime && !m_bShowSongLen)
			{
				m_bShowSongLen  = true;
			}
			else 
			{
				m_bShowSongTime = true;
				m_bShowSongLen  = false;
			}
			return 0; // we processed (or absorbed) the key
		case VK_F4:		m_bShowPresetInfo = !m_bShowPresetInfo;	return 0; // we processed (or absorbed) the key
		case VK_F5:		m_bShowFPS = !m_bShowFPS;				return 0; // we processed (or absorbed) the key
		case VK_F6:		m_bShowRating = !m_bShowRating;			return 0; // we processed (or absorbed) the key
		case VK_F7:	
			if (m_nNumericInputMode == NUMERIC_INPUT_MODE_CUST_MSG)
				ReadCustomMessages();		// re-read custom messages
			return 0; // we processed (or absorbed) the key
		case VK_F8:		
			{
				m_UI_mode = UI_CHANGEDIR;

				// enter WaitString mode
				m_waitstring.bActive = true;
				m_waitstring.bFilterBadChars = false;
				m_waitstring.bDisplayAsCode = false;
				m_waitstring.nSelAnchorPos = -1;
				m_waitstring.nMaxLen = min(sizeof(m_waitstring.szText)-1, MAX_PATH - 1);
				strcpy(m_waitstring.szText, GetPresetDir());
				{
					// for subtle beauty - remove the trailing '\' from the directory name (if it's not just "x:\")
					int len = strlen(m_waitstring.szText);
					if (len > 3 && m_waitstring.szText[len-1] == '\\')
						m_waitstring.szText[len-1] = 0;
				}
				strcpy(m_waitstring.szPrompt, "Directory to jump to:");
				m_waitstring.szToolTip[0] = 0;
				m_waitstring.nCursorPos = strlen(m_waitstring.szText);	// set the starting edit position
			}
			return 0; // we processed (or absorbed) the key
		case VK_F9:
            if (bShiftHeldDown)
                m_fStereoSep /= 1.08f;
            else if (bCtrlHeldDown)
                m_fStereoSep *= 1.08f;
            else
                m_pState->m_bRedBlueStereo = !m_pState->m_bRedBlueStereo;	
            if (m_fStereoSep < 0.1f) m_fStereoSep = 0.1f;
            if (m_fStereoSep > 10) m_fStereoSep = 10;
            return FALSE;
		case VK_SCROLL:	
			//BYTE keys[256];
			//GetKeyboardState(keys);
			//m_bPresetLockedByUser = (keys[VK_SCROLL] & 1) ? true : false;
			m_bPresetLockedByUser = GetKeyState(VK_SCROLL) & 1;
			return 0; // we processed (or absorbed) the key
		//case VK_F6:	break;
		//case VK_F7: conflict
		//case VK_F8:	break;
		//case VK_F9: conflict
		}

		// next handle the waitstring case (for string-editing),
		//	then the menu navigation case,
		//  then handle normal case (handle the message normally or pass on to winamp)

		// case 1: waitstring mode
		if (m_waitstring.bActive) 
		{
			// handle arrow keys, home, end, etc. 

			SHORT mask = 1 << (sizeof(SHORT)*8 - 1);	// we want the highest-order bit
			bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0;
			bool bCtrlHeldDown = (GetKeyState(VK_CONTROL) & mask) != 0;

			if (wParam == VK_LEFT || wParam == VK_RIGHT || 
				wParam == VK_HOME || wParam == VK_END ||
				wParam == VK_UP || wParam == VK_DOWN)
			{
				if (bShiftHeldDown)
				{
					if (m_waitstring.nSelAnchorPos == -1)
						m_waitstring.nSelAnchorPos = m_waitstring.nCursorPos;
				}
				else
				{
					m_waitstring.nSelAnchorPos = -1;
				}
			}

			if (bCtrlHeldDown)		// copy/cut/paste
			{
				switch(wParam)
				{
				case 'c':		
				case 'C':		
				case VK_INSERT:	
					WaitString_Copy();		
					return 0; // we processed (or absorbed) the key
				case 'x':		
				case 'X':		
					WaitString_Cut();		
					return 0; // we processed (or absorbed) the key
				case 'v':		
				case 'V':		
					WaitString_Paste();		
					return 0; // we processed (or absorbed) the key
				case VK_LEFT:	WaitString_SeekLeftWord();	return 0; // we processed (or absorbed) the key
				case VK_RIGHT:	WaitString_SeekRightWord();	return 0; // we processed (or absorbed) the key
				case VK_HOME:	m_waitstring.nCursorPos = 0;	return 0; // we processed (or absorbed) the key
				case VK_END:	m_waitstring.nCursorPos = strlen(m_waitstring.szText);	return 0; // we processed (or absorbed) the key
				case VK_RETURN:
					if (m_waitstring.bDisplayAsCode)
					{
						// CTRL+ENTER accepts the string -> finished editing
						//assert(m_pCurMenu);
						m_pCurMenu->OnWaitStringAccept(m_waitstring.szText);
							// OnWaitStringAccept calls the callback function.  See the
							// calls to CMenu::AddItem from milkdrop.cpp to find the
							// callback functions for different "waitstrings".
						m_waitstring.bActive = false;
						m_UI_mode = UI_MENU;
					}
					return 0; // we processed (or absorbed) the key
				}
			}
			else	// waitstring mode key pressed, and ctrl NOT held down
			{
				switch(wParam)
				{
				case VK_INSERT:
					m_waitstring.bOvertypeMode = !m_waitstring.bOvertypeMode;
					return 0; // we processed (or absorbed) the key

				case VK_LEFT:
					if (m_waitstring.nCursorPos > 0) 
						m_waitstring.nCursorPos--;
					return 0; // we processed (or absorbed) the key

				case VK_RIGHT:
					if (m_waitstring.nCursorPos < (int)strlen(m_waitstring.szText))
						m_waitstring.nCursorPos++;
					return 0; // we processed (or absorbed) the key

				case VK_HOME:	
					m_waitstring.nCursorPos -= WaitString_GetCursorColumn();
					return 0; // we processed (or absorbed) the key

				case VK_END:	
					m_waitstring.nCursorPos += WaitString_GetLineLength() - WaitString_GetCursorColumn();
					return 0; // we processed (or absorbed) the key

				case VK_UP:
					WaitString_SeekUpOneLine();
					return 0; // we processed (or absorbed) the key

				case VK_DOWN:
					WaitString_SeekDownOneLine();
					return 0; // we processed (or absorbed) the key

				case VK_BACK:
					if (m_waitstring.nSelAnchorPos != -1)
					{
						WaitString_NukeSelection();
					}
					else if (m_waitstring.nCursorPos > 0)
					{
						int len = strlen(m_waitstring.szText);
						for (int i=m_waitstring.nCursorPos-1; i<len; i++)
							m_waitstring.szText[i] = m_waitstring.szText[i+1];
						m_waitstring.nCursorPos--;
					}
					return 0; // we processed (or absorbed) the key
				
				case VK_DELETE:
					if (m_waitstring.nSelAnchorPos != -1)
					{
						WaitString_NukeSelection();
					}
					else
					{
						int len = strlen(m_waitstring.szText);
						if (m_waitstring.nCursorPos < len)
						{
							for (int i=m_waitstring.nCursorPos; i<len; i++)
								m_waitstring.szText[i] = m_waitstring.szText[i+1];
						}
					}
					return 0; // we processed (or absorbed) the key

				case VK_RETURN:
					if (m_UI_mode == UI_LOAD_RENAME)	// rename (move) the file
					{
						// first add pathnames to filenames
						char szOldFile[512];
						char szNewFile[512];
						strcpy(szOldFile, GetPresetDir());
						strcpy(szNewFile, GetPresetDir());
						strcat(szOldFile, m_pPresetAddr[m_nPresetListCurPos]);
						strcat(szNewFile, m_waitstring.szText);
						strcat(szNewFile, ".milk");

						RenamePresetFile(szOldFile, szNewFile);
					}
                    else if (m_UI_mode == UI_IMPORT_WAVE ||
                             m_UI_mode == UI_EXPORT_WAVE ||
                             m_UI_mode == UI_IMPORT_SHAPE ||
                             m_UI_mode == UI_EXPORT_SHAPE)
                    {
                        int bWave   = (m_UI_mode == UI_IMPORT_WAVE || m_UI_mode == UI_EXPORT_WAVE);
                        int bImport = (m_UI_mode == UI_IMPORT_WAVE || m_UI_mode == UI_IMPORT_SHAPE);

                        int i = m_pCurMenu->GetCurItem()->m_lParam;
                        int ret;
                        switch(m_UI_mode)
                        {
                        case UI_IMPORT_WAVE : ret = m_pState->m_wave[i].Import("custom wave", m_waitstring.szText, 0); break;
                        case UI_EXPORT_WAVE : ret = m_pState->m_wave[i].Export("custom wave", m_waitstring.szText, 0); break;
                        case UI_IMPORT_SHAPE: ret = m_pState->m_shape[i].Import("custom shape", m_waitstring.szText, 0); break;
                        case UI_EXPORT_SHAPE: ret = m_pState->m_shape[i].Export("custom shape", m_waitstring.szText, 0); break;
                        }

                        if (bImport)
                            m_pState->RecompileExpressions(1);

						m_fShowUserMessageUntilThisTime = GetTime() - 1.0f;	// if there was an error message already, clear it
                        if (!ret)
                        {
                            if (m_UI_mode==UI_IMPORT_WAVE || m_UI_mode==UI_IMPORT_SHAPE)
							    strcpy(m_szUserMessage, "(error importing - bad filename, or file does not exist)");
                            else
							    strcpy(m_szUserMessage, "(error exporting - bad filename, or file can not be overwritten)");
							m_fShowUserMessageUntilThisTime = GetTime() + 2.5f;
                        }

						m_waitstring.bActive = false;
                        m_UI_mode = UI_MENU;
						m_bPresetLockedByCode = false;
                    }
					else if (m_UI_mode == UI_SAVEAS)
					{
						// first add pathname + extension to filename
						char szNewFile[512];
						sprintf(szNewFile, "%s%s.milk", GetPresetDir(), m_waitstring.szText);

						if (GetFileAttributes(szNewFile) != -1)		// check if file already exists
						{
							// file already exists -> overwrite it?
							m_waitstring.bActive = false;
							m_UI_mode = UI_SAVE_OVERWRITE;
						}
						else
						{
							SavePresetAs(szNewFile);

							// exit waitstring mode
							m_UI_mode = UI_REGULAR;
							m_waitstring.bActive = false;
							m_bPresetLockedByCode = false;
						}
					}
					else if (m_UI_mode == UI_EDIT_MENU_STRING)
					{
						if (m_waitstring.bDisplayAsCode)
						{
							if (m_waitstring.nSelAnchorPos != -1)
								WaitString_NukeSelection();

							int len = strlen(m_waitstring.szText);
							if (len + 1 < m_waitstring.nMaxLen)
							{
								// insert a linefeed.  Use CTRL+return to accept changes in this case.
								for (int pos=len+1; pos > m_waitstring.nCursorPos; pos--)
									m_waitstring.szText[pos] = m_waitstring.szText[pos - 1];
								m_waitstring.szText[m_waitstring.nCursorPos++] = LINEFEED_CONTROL_CHAR;

								m_fShowUserMessageUntilThisTime = GetTime() - 1.0f;	// if there was an error message already, clear it
							}
							else
							{
								// m_waitstring.szText has reached its limit
								strcpy(m_szUserMessage, "(string too long)");
								m_fShowUserMessageUntilThisTime = GetTime() + 2.5f;
							}
						}
						else
						{
							// finished editing
							//assert(m_pCurMenu);
							m_pCurMenu->OnWaitStringAccept(m_waitstring.szText);
								// OnWaitStringAccept calls the callback function.  See the
								// calls to CMenu::AddItem from milkdrop.cpp to find the
								// callback functions for different "waitstrings".
							m_waitstring.bActive = false;
							m_UI_mode = UI_MENU;
						}
					}
					else if (m_UI_mode == UI_CHANGEDIR)
					{
						m_fShowUserMessageUntilThisTime = GetTime();	// if there was an error message already, clear it

						// change dir
						char szOldDir[512];
						char szNewDir[512];
						strcpy(szOldDir, GetPresetDir());
						strcpy(szNewDir, m_waitstring.szText);

						int len = strlen(szNewDir);
						if (len > 0 && szNewDir[len-1] != '\\')
							strcat(szNewDir, "\\");

						strcpy(GetPresetDir(), szNewDir);
						UpdatePresetList();
						if (m_nPresets == 0)
						{
							// new dir. was invalid -> allow them to try again
							strcpy(GetPresetDir(), szOldDir);
							UpdatePresetList();

							// give them a warning
							strcpy(m_szUserMessage, "(invalid path)");
							m_fShowUserMessageUntilThisTime = GetTime() + 3.5f;
						}
						else
						{
							// success
							strcpy(GetPresetDir(), szNewDir);

							// save new path to registry
							WritePrivateProfileString("settings","szPresetDir",GetPresetDir(),GetConfigIniFile());

							// set current preset index to -1 because current preset is no longer in the list
							m_nCurrentPreset = -1;

							// go to file load menu
							m_waitstring.bActive = false;
							m_UI_mode = UI_LOAD;
						}
					}
					return 0; // we processed (or absorbed) the key

				case VK_ESCAPE:
					if (m_UI_mode == UI_LOAD_RENAME)
					{
						m_waitstring.bActive = false;
						m_UI_mode = UI_LOAD;
					}
					else if (
                        m_UI_mode == UI_SAVEAS || 
                        m_UI_mode == UI_SAVE_OVERWRITE ||
                        m_UI_mode == UI_EXPORT_SHAPE || 
                        m_UI_mode == UI_IMPORT_SHAPE || 
                        m_UI_mode == UI_EXPORT_WAVE || 
                        m_UI_mode == UI_IMPORT_WAVE)
					{
						m_bPresetLockedByCode = false;
						m_waitstring.bActive = false;
						m_UI_mode = UI_REGULAR;
					}
                    else if (m_UI_mode == UI_EDIT_MENU_STRING)
                    {
						m_waitstring.bActive = false;
                        if (m_waitstring.bDisplayAsCode)    // if were editing code...
    						m_UI_mode = UI_MENU;    // return to menu
                        else
    						m_UI_mode = UI_REGULAR; // otherwise don't (we might have been editing a filename, for example)
                    }
                    else /*if (m_UI_mode == UI_EDIT_MENU_STRING || m_UI_mode == UI_CHANGEDIR || 1)*/
					{
						m_waitstring.bActive = false;
						m_UI_mode = UI_REGULAR;
					}
					return 0; // we processed (or absorbed) the key
				}
			}

			// don't let keys go anywhere else
			return 0; // we processed (or absorbed) the key
		} 

		// case 2: menu is up & gets the keyboard input
		if (m_UI_mode == UI_MENU)	
		{
			//assert(m_pCurMenu);
			if (m_pCurMenu->HandleKeydown(hWnd, uMsg, wParam, lParam) == 0) 
				return 0; // we processed (or absorbed) the key
		}

		// case 3: handle non-character keys (virtual keys) and return 0.
        //         if we don't handle them, return 1, and the shell will
        //         (passing some to the shell's key bindings, some to Winamp,
        //          and some to DefWindowProc)
		//		note: regular hotkeys should be handled in HandleRegularKey.
		switch(wParam)
		{
		case VK_LEFT:
		case VK_RIGHT:
			if (m_UI_mode == UI_LOAD)
			{	
				// it's annoying when the music skips if you hit the left arrow from the Load menu, so instead, we exit the menu
				if (wParam == VK_LEFT) m_UI_mode = UI_REGULAR;
				return 0; // we processed (or absorbed) the key
			}
            break;

		case VK_ESCAPE:
			if (m_UI_mode == UI_LOAD || 
				m_UI_mode == UI_MENU)
			{
				m_UI_mode = UI_REGULAR;
				return 0; // we processed (or absorbed) the key
			}
			else if (m_UI_mode == UI_LOAD_DEL)
			{
				m_UI_mode = UI_LOAD;
				return 0; // we processed (or absorbed) the key
			}
			else if (m_UI_mode == UI_SAVE_OVERWRITE)
			{
				m_UI_mode = UI_SAVEAS;
				// return to waitstring mode, leaving all the parameters as they were before:
				m_waitstring.bActive = true;
				return 0; // we processed (or absorbed) the key
			}
			/*else if (hwnd == GetPluginWindow())		// (don't close on ESC for text window)
			{
				dumpmsg("User pressed ESCAPE");
				//m_bExiting = true;
				PostMessage(hwnd, WM_CLOSE, 0, 0);	
				return 0; // we processed (or absorbed) the key
			}*/
			break;

		case VK_UP:
			if (m_UI_mode == UI_LOAD)
			{
				if (m_nPresetListCurPos > 0)
					m_nPresetListCurPos--;
				return 0; // we processed (or absorbed) the key

				// remember this preset's name so the next time they hit 'L' it jumps straight to it
				//strcpy(m_szLastPresetSelected, m_pPresetAddr[m_nPresetListCurPos]);
			}
			break;

		case VK_DOWN:
			if (m_UI_mode == UI_LOAD)
			{
				if (m_nPresetListCurPos < m_nPresets - 1) 
					m_nPresetListCurPos++;
				return 0; // we processed (or absorbed) the key

				// remember this preset's name so the next time they hit 'L' it jumps straight to it
				//strcpy(m_szLastPresetSelected, m_pPresetAddr[m_nPresetListCurPos]);
			}
			break;

		case VK_SPACE:
			if (!m_bPresetLockedByCode)
			{
				LoadRandomPreset(m_fBlendTimeUser);
				return 0; // we processed (or absorbed) the key
			}
			break;

		case VK_PRIOR:	
			if (m_UI_mode == UI_LOAD)
            {
				m_bUserPagedUp = true;
				return 0; // we processed (or absorbed) the key
            }
			break;
		case VK_NEXT:	
			if (m_UI_mode == UI_LOAD)
            {
				m_bUserPagedDown = true;
				return 0; // we processed (or absorbed) the key
            }
			break;
		case VK_HOME:	
			if (m_UI_mode == UI_LOAD)
            {
				m_nPresetListCurPos = 0;
				return 0; // we processed (or absorbed) the key
            }
			break;
		case VK_END:	
			if (m_UI_mode == UI_LOAD)
            {
				m_nPresetListCurPos = m_nPresets - 1;
				return 0; // we processed (or absorbed) the key
            }
			break;
		
		case VK_DELETE:
			if (m_UI_mode == UI_LOAD)
			{
				if (m_pPresetAddr[m_nPresetListCurPos][0] != '*')	// can't delete directories
					m_UI_mode = UI_LOAD_DEL;
				return 0; // we processed (or absorbed) the key
			}
			else //if (m_nNumericInputDigits == 0)
			{
				if (m_nNumericInputMode == NUMERIC_INPUT_MODE_CUST_MSG)
				{
				    m_nNumericInputDigits = 0;
				    m_nNumericInputNum = 0;

					// stop display of text message.
					m_supertext.fStartTime = -1.0f;
    				return 0; // we processed (or absorbed) the key
				}
				else if (m_nNumericInputMode == NUMERIC_INPUT_MODE_SPRITE)
				{
					// kill newest sprite (regular DELETE key)
					// oldest sprite (SHIFT + DELETE),
					// or all sprites (CTRL + SHIFT + DELETE).

                    m_nNumericInputDigits = 0;
				    m_nNumericInputNum = 0;

					SHORT mask = 1 << (sizeof(SHORT)*8 - 1);	// we want the highest-order bit
					bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0;
					bool bCtrlHeldDown = (GetKeyState(VK_CONTROL) & mask) != 0;

					if (bShiftHeldDown && bCtrlHeldDown)
					{
						for (int x=0; x<NUM_TEX; x++)
							m_texmgr.KillTex(x);
					}
					else
					{
						int newest = -1;
						int frame;
						for (int x=0; x<NUM_TEX; x++)
						{
							if (m_texmgr.m_tex[x].pSurface)
							{
								if ((newest == -1) ||
									(!bShiftHeldDown && m_texmgr.m_tex[x].nStartFrame > frame) ||
									(bShiftHeldDown && m_texmgr.m_tex[x].nStartFrame < frame))
								{
									newest = x;
									frame = m_texmgr.m_tex[x].nStartFrame;
								}
							}
						}

						if (newest != -1)
							m_texmgr.KillTex(newest);
					}
					return 0; // we processed (or absorbed) the key
    			}
			}
			break;

		case VK_INSERT:		// RENAME
			if (m_UI_mode == UI_LOAD)
			{
				if (m_pPresetAddr[m_nPresetListCurPos][0] != '*')	// can't rename directories
				{
					// go into RENAME mode
					m_UI_mode = UI_LOAD_RENAME;
					m_waitstring.bActive = true;
					m_waitstring.bFilterBadChars = true;
					m_waitstring.bDisplayAsCode = false;
					m_waitstring.nSelAnchorPos = -1;
					m_waitstring.nMaxLen = min(sizeof(m_waitstring.szText)-1, MAX_PATH - strlen(GetPresetDir()) - 6);	// 6 for the extension + null char.  We set this because win32 LoadFile, MoveFile, etc. barf if the path+filename+ext are > MAX_PATH chars.

					// initial string is the filename, minus the extension
					strcpy(m_waitstring.szText, m_pPresetAddr[m_nPresetListCurPos]);
					RemoveExtension(m_waitstring.szText);

					// set the prompt & 'tooltip'
					sprintf(m_waitstring.szPrompt, "Enter the new name for \"%s\":", m_waitstring.szText);
					m_waitstring.szToolTip[0] = 0;

					// set the starting edit position
					m_waitstring.nCursorPos = strlen(m_waitstring.szText);
				}
				return 0; // we processed (or absorbed) the key
			}
			break;

		case VK_RETURN:

			if (m_UI_mode == UI_LOAD)
			{
				if (m_pPresetAddr[m_nPresetListCurPos][0] == '*')
				{
					// CHANGE DIRECTORY
					char *p = GetPresetDir();

					if (strcmp(m_pPresetAddr[m_nPresetListCurPos], "*..") == 0)
					{
						// back up one dir
						char *p2 = strrchr(p, '\\');
						if (p2) 
						{
							*p2 = 0;
							p2 = strrchr(p, '\\');
							if (p2)	*(p2+1) = 0;
						}
					}
					else
					{
						// open subdir
						strcat(p, &m_pPresetAddr[m_nPresetListCurPos][1]);
						strcat(p, "\\");
					}

					WritePrivateProfileString("settings","szPresetDir",GetPresetDir(),GetConfigIniFile());

					UpdatePresetList();	
					
					// set current preset index to -1 because current preset is no longer in the list
					m_nCurrentPreset = -1;
				}
				else
				{
					// LOAD NEW PRESET
					m_nCurrentPreset = m_nPresetListCurPos;

					// first take the filename and prepend the path.  (already has extension)
					char s[MAX_PATH];
					strcpy(s, GetPresetDir());	// note: m_szPresetDir always ends with '\'
					strcat(s, m_pPresetAddr[m_nCurrentPreset]);

					// now load (and blend to) the new preset
					LoadPreset(s, m_fBlendTimeUser);
				}
				return 0; // we processed (or absorbed) the key
			}
			break;

		case VK_BACK:
			// pass on to parent
			//PostMessage(m_hWndParent,message,wParam,lParam);
			m_nNumericInputDigits = 0;
			m_nNumericInputNum = 0;
			return 0;

        case 'T':
            if (bCtrlHeldDown)
            {
    			// stop display of custom message or song title.
			    m_supertext.fStartTime = -1.0f;
                return 0;
            }
            break;
        case 'K':
            if (bCtrlHeldDown)      // kill all sprites
            {
		        for (int x=0; x<NUM_TEX; x++)
			        if (m_texmgr.m_tex[x].pSurface)
                        m_texmgr.KillTex(x);
                return 0;
            }
            break;
        case 'Y':
            if (bCtrlHeldDown)      // stop display of custom message or song title.
            {
			    m_supertext.fStartTime = -1.0f;
                return 0;
            }
            break;
		}
        return 1; // end case WM_KEYDOWN
	
	case WM_KEYUP:
    	return 1;
		break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;
    }

#endif

    return 0;
};

//----------------------------------------------------------------------

int CPlugin::HandleRegularKey(WPARAM wParam)
{
#if 0
	// here we handle all the normal keys for milkdrop-
	// these are the hotkeys that are used when you're not
	// in the middle of editing a string, navigating a menu, etc.
	
	// do not make references to virtual keys here; only
	// straight WM_CHAR messages should be sent in.  

    // return 0 if you process/absorb the key; otherwise return 1.

	if (m_UI_mode == UI_LOAD && ((wParam >= 'A' && wParam <= 'Z') || (wParam >= 'a' && wParam <= 'z')))
	{
		SeekToPreset((char)wParam);
		return 0; // we processed (or absorbed) the key
	}
	else switch(wParam)
	{
	case '0':	
	case '1':	
	case '2':	
	case '3':	
	case '4':	
	case '5':	
	case '6':	
	case '7':	
	case '8':	
	case '9':	
		{
			int digit = wParam - '0';
			m_nNumericInputNum		= (m_nNumericInputNum*10) + digit;
			m_nNumericInputDigits++;

			if (m_nNumericInputDigits >= 2)
			{
				if (m_nNumericInputMode == NUMERIC_INPUT_MODE_CUST_MSG)
					LaunchCustomMessage(m_nNumericInputNum); 
				else if (m_nNumericInputMode == NUMERIC_INPUT_MODE_SPRITE)
					LaunchSprite(m_nNumericInputNum, -1); 
				else if (m_nNumericInputMode == NUMERIC_INPUT_MODE_SPRITE_KILL)
				{
					for (int x=0; x<NUM_TEX; x++)
						if (m_texmgr.m_tex[x].nUserData == m_nNumericInputNum)
							m_texmgr.KillTex(x);
				}

				m_nNumericInputDigits = 0;
				m_nNumericInputNum = 0;
			}
		}
		return 0; // we processed (or absorbed) the key

    // row 1 keys
	case 'q':
		m_pState->m_fVideoEchoZoom /= 1.05f;
		return 0; // we processed (or absorbed) the key
	case 'Q':
		m_pState->m_fVideoEchoZoom *= 1.05f;
		return 0; // we processed (or absorbed) the key
	case 'w':
		m_pState->m_nWaveMode++;
		if (m_pState->m_nWaveMode >= NUM_WAVES) m_pState->m_nWaveMode = 0;
		return 0; // we processed (or absorbed) the key
	case 'W':
		m_pState->m_nWaveMode--;
		if (m_pState->m_nWaveMode < 0) m_pState->m_nWaveMode = NUM_WAVES - 1;
		return 0; // we processed (or absorbed) the key
	case 'e':
		m_pState->m_fWaveAlpha -= 0.1f; 
		if (m_pState->m_fWaveAlpha.eval(-1) < 0.0f) m_pState->m_fWaveAlpha = 0.0f;
		return 0; // we processed (or absorbed) the key
	case 'E':
		m_pState->m_fWaveAlpha += 0.1f; 
		//if (m_pState->m_fWaveAlpha.eval(-1) > 1.0f) m_pState->m_fWaveAlpha = 1.0f;
		return 0; // we processed (or absorbed) the key

	case 'I':	m_pState->m_fZoom -= 0.01f;		return 0; // we processed (or absorbed) the key
	case 'i':	m_pState->m_fZoom += 0.01f;		return 0; // we processed (or absorbed) the key

	case 'n':
	case 'N':
		m_bShowDebugInfo = !m_bShowDebugInfo;
		return 0; // we processed (or absorbed) the key

	case 'r':
	case 'R':
    m_bHoldPreset = 0;
    m_bSequentialPresetOrder = !m_bSequentialPresetOrder;
		sprintf(m_szUserMessage, "preset order is now %s", (m_bSequentialPresetOrder) ? "SEQUENTIAL" : "RANDOM");
		m_fShowUserMessageUntilThisTime = GetTime() + 4.0f;
		SendMessage(GetWinampWindow(),WM_WA_IPC,!m_bHoldPreset,IPC_CB_VISRANDOM);
		return 0; // we processed (or absorbed) the key
		
	case 'x':
	case 'X':
    m_bHoldPreset = !m_bHoldPreset;
		m_fShowUserMessageUntilThisTime = GetTime() + 4.0f;
		if (m_bHoldPreset)
      strcpy(m_szUserMessage, "locking preset");
    else
      strcpy(m_szUserMessage, "unlocking preset");
		SendMessage(GetWinampWindow(),WM_WA_IPC,!m_bHoldPreset,IPC_CB_VISRANDOM);
    return 0;
  
	case 'u':
	case 'U':
		if (SendMessage(GetWinampWindow(),WM_USER,0,250))
			sprintf(m_szUserMessage, "shuffle is now OFF");	// shuffle was on
		else
			sprintf(m_szUserMessage, "shuffle is now ON");	// shuffle was off

		m_fShowUserMessageUntilThisTime = GetTime() + 4.0f;

		// toggle shuffle
		PostMessage(GetWinampWindow(),WM_COMMAND,40023,0);

		return 0; // we processed (or absorbed) the key


	/*
	case 'u':	m_pState->m_fWarpScale /= 1.1f;			break;
	case 'U':	m_pState->m_fWarpScale *= 1.1f;			break;
	case 'i':	m_pState->m_fWarpAnimSpeed /= 1.1f;		break;
	case 'I':	m_pState->m_fWarpAnimSpeed *= 1.1f;		break;
	*/
	case 't':	
	case 'T':	
		LaunchSongTitleAnim();
		return 0; // we processed (or absorbed) the key
	case 'o':	m_pState->m_fWarpAmount /= 1.1f;	return 0; // we processed (or absorbed) the key
	case 'O':	m_pState->m_fWarpAmount *= 1.1f;	return 0; // we processed (or absorbed) the key
	
	// row 2 keys
	case 'a':
		m_pState->m_fVideoEchoAlpha -= 0.1f;
		if (m_pState->m_fVideoEchoAlpha.eval(-1) < 0) m_pState->m_fVideoEchoAlpha = 0;
		return 0; // we processed (or absorbed) the key
	case 'A':
		m_pState->m_fVideoEchoAlpha += 0.1f;
		if (m_pState->m_fVideoEchoAlpha.eval(-1) > 1.0f) m_pState->m_fVideoEchoAlpha = 1.0f;
		return 0; // we processed (or absorbed) the key
	case 'd':
		m_pState->m_fDecay += 0.01f;
		if (m_pState->m_fDecay.eval(-1) > 1.0f) m_pState->m_fDecay = 1.0f;
		return 0; // we processed (or absorbed) the key
	case 'D':
		m_pState->m_fDecay -= 0.01f;
		if (m_pState->m_fDecay.eval(-1) < 0.9f) m_pState->m_fDecay = 0.9f;
		return 0; // we processed (or absorbed) the key
	case 'h':
	case 'H':
		// instant hard cut
		LoadRandomPreset(0.0f);
		m_fHardCutThresh *= 2.0f;
		return 0; // we processed (or absorbed) the key
	case 'f':
	case 'F':
		m_pState->m_nVideoEchoOrientation = (m_pState->m_nVideoEchoOrientation + 1) % 4;
		return 0; // we processed (or absorbed) the key
	case 'g':
		m_pState->m_fGammaAdj -= 0.1f;
		if (m_pState->m_fGammaAdj.eval(-1) < 0.0f) m_pState->m_fGammaAdj = 0.0f;
		return 0; // we processed (or absorbed) the key
	case 'G':
		m_pState->m_fGammaAdj += 0.1f;
		//if (m_pState->m_fGammaAdj > 1.0f) m_pState->m_fGammaAdj = 1.0f;
		return 0; // we processed (or absorbed) the key
    case 'j':
		m_pState->m_fWaveScale *= 0.9f;
		return 0; // we processed (or absorbed) the key
	case 'J':
		m_pState->m_fWaveScale /= 0.9f;
		return 0; // we processed (or absorbed) the key

	case 'y':
	case 'Y':
		m_nNumericInputMode   = NUMERIC_INPUT_MODE_CUST_MSG;
		m_nNumericInputNum    = 0;
		m_nNumericInputDigits = 0;
		return 0; // we processed (or absorbed) the key
	case 'k':
	case 'K':
		{
			SHORT mask = 1 << (sizeof(SHORT)*8 - 1);	// we want the highest-order bit
			bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0;

			if (bShiftHeldDown)
				m_nNumericInputMode   = NUMERIC_INPUT_MODE_SPRITE_KILL;
			else
				m_nNumericInputMode   = NUMERIC_INPUT_MODE_SPRITE;
			m_nNumericInputNum    = 0;
			m_nNumericInputDigits = 0;
		}
		return 0; // we processed (or absorbed) the key

	// row 3/misc. keys

	case '[':
		m_pState->m_fXPush -= 0.005f;
		return 0; // we processed (or absorbed) the key
	case ']':
		m_pState->m_fXPush += 0.005f;
		return 0; // we processed (or absorbed) the key
	case '{':
		m_pState->m_fYPush -= 0.005f;
		return 0; // we processed (or absorbed) the key
	case '}':
		m_pState->m_fYPush += 0.005f;
		return 0; // we processed (or absorbed) the key
	case '<':
		m_pState->m_fRot += 0.02f;
		return 0; // we processed (or absorbed) the key
	case '>':
		m_pState->m_fRot -= 0.02f;
		return 0; // we processed (or absorbed) the key

	case 's':				// SAVE PRESET
	case 'S':
		if (m_UI_mode == UI_REGULAR)
		{
			m_bPresetLockedByCode = true;
			m_UI_mode = UI_SAVEAS;

			// enter WaitString mode
			m_waitstring.bActive = true;
			m_waitstring.bFilterBadChars = true;
			m_waitstring.bDisplayAsCode = false;
			m_waitstring.nSelAnchorPos = -1;
			m_waitstring.nMaxLen = min(sizeof(m_waitstring.szText)-1, MAX_PATH - strlen(GetPresetDir()) - 6);	// 6 for the extension + null char.    We set this because win32 LoadFile, MoveFile, etc. barf if the path+filename+ext are > MAX_PATH chars.
			strcpy(m_waitstring.szText, m_pState->m_szDesc);			// initial string is the filename, minus the extension
			strcpy(m_waitstring.szPrompt, "Save as:");
			m_waitstring.szToolTip[0] = 0;
			m_waitstring.nCursorPos = strlen(m_waitstring.szText);	// set the starting edit position
            return 0;
		}
		break;

	case 'l':				// LOAD PRESET
	case 'L':
		if (m_UI_mode == UI_LOAD)
		{
			m_UI_mode = UI_REGULAR;
            return 0; // we processed (or absorbed) the key

		}
		else if (
			m_UI_mode == UI_REGULAR || 
			m_UI_mode == UI_MENU)
		{
			m_UI_mode = UI_LOAD;
			m_bUserPagedUp = false;
			m_bUserPagedDown = false;
            return 0; // we processed (or absorbed) the key

		}
        break;

	case 'm':
	case 'M':
		
		if (m_UI_mode == UI_MENU)
			m_UI_mode = UI_REGULAR;
		else if (m_UI_mode == UI_REGULAR || m_UI_mode == UI_LOAD)
			m_UI_mode = UI_MENU;

		return 0; // we processed (or absorbed) the key

	case '-':		
		SetCurrentPresetRating(m_pState->m_fRating - 1.0f);
		return 0; // we processed (or absorbed) the key
	case '+':
		SetCurrentPresetRating(m_pState->m_fRating + 1.0f);
		return 0; // we processed (or absorbed) the key

	}
#endif
    return 1;
}

//----------------------------------------------------------------------

void CPlugin::RefreshTab2(HWND hwnd)
{
/*
	ShowWindow(GetDlgItem(hwnd, IDC_BRIGHT_SLIDER), !m_bAutoGamma);
	ShowWindow(GetDlgItem(hwnd, IDC_T1), !m_bAutoGamma);
	ShowWindow(GetDlgItem(hwnd, IDC_T2), !m_bAutoGamma);
	ShowWindow(GetDlgItem(hwnd, IDC_T3), !m_bAutoGamma);
	ShowWindow(GetDlgItem(hwnd, IDC_T4), !m_bAutoGamma);
	ShowWindow(GetDlgItem(hwnd, IDC_T5), !m_bAutoGamma);

    EnableWindow(GetDlgItem(hwnd, IDC_CB_INSTASCAN), m_bEnableRating);
*/
}
/*
int CALLBACK MyEnumFontsProc(
  CONST LOGFONT *lplf,     // logical-font data
  CONST TEXTMETRIC *lptm,  // physical-font data
  DWORD dwType,            // font type
  LPARAM lpData            // application-defined data
)
{
	SendMessage( GetDlgItem( (HWND)lpData, IDC_FONT3), CB_ADDSTRING, 0, (LPARAM)(lplf->lfFaceName));
	return 1;
}
*/

/*
void DoColors(HWND hwnd, int *r, int *g, int *b)
{
	static COLORREF acrCustClr[16]; 

	CHOOSECOLOR cc;
	ZeroMemory(&cc, sizeof(CHOOSECOLOR));
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hwnd;//NULL;//hSaverMainWindow;
	cc.Flags = CC_RGBINIT | CC_FULLOPEN;
	cc.rgbResult = RGB(*r,*g,*b);
	cc.lpCustColors = (LPDWORD)acrCustClr;
	if (ChooseColor(&cc))
	{
		*r = GetRValue(cc.rgbResult);
		*g = GetGValue(cc.rgbResult);
		*b = GetBValue(cc.rgbResult);
	}
}
*/
//----------------------------------------------------------------------

BOOL CPlugin::MyConfigTabProc(int nPage, HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
#if 0

    // This is the only function you need to worry about for programming
    //   tabs 2 through 8 on the config panel.  (Tab 1 contains settings
    //   that are common to all plugins, and the code is located in pluginshell.cpp).
    // By default, only tab 2 is enabled; to enable tabes 3+, see 
    //  'Enabling Additional Tabs (pages) on the Config Panel' in DOCUMENTATION.TXT.
    // You should always return 0 for this function.
    // Note that you don't generally have to use critical sections or semaphores
    //   here; Winamp controls the plugin's message queue, and only gives it message
    //   in between frames.
    // 
    // Incoming parameters:
    //   'nPage' indicates which tab (aka 'property page') is currently showing: 2 through 5.
    //   'hwnd' is the window handle of the property page (which is a dialog of its own,
    //         embedded in the config dialog).
    //   'msg' is the windows message being sent.  The main ones are:
    //
    //      1) WM_INITDIALOG: This means the page is being initialized, because the
    //          user clicked on it.  When you get this message, you should initialize
    //          all the controls on the page, and set them to reflect the settings
    //          that are stored in member variables.
    //
    //      2) WM_DESTROY: This is sent when a tab disappears, either because another 
    //          tab is about to be displayed, or because the user clicked OK or Cancel.
    //          In any case, you should read the current settings of all the controls
    //          on the page, and store them in member variables.  (If the user clicked
    //          CANCEL, these values will not get saved to disk, but for simplicity,
    //          we always poll the controls here.)
    //
    //      3) WM_HELP: This is sent when the user clicks the '?' icon (in the
    //          titlebar of the config panel) and then clicks on a control.  When you
    //          get this message, you should display a MessageBox telling the user
    //          a little bit about that control/setting.  
    //
    //      4) WM_COMMAND: Advanced.  This notifies you when the user clicks on a 
    //          control.  Use this if you need to do certain things when the user 
    //          changes a setting.  (For example, one control might only be enabled
    //          when a certain checkbox is enabled; you would use EnableWindow() for
    //          this.)
    // 
    // For complete details on adding your own controls to one of the pages, please see 
    // 'Adding Controls to the Config Panel' in DOCUMENTATION.TXT.
    
    int t;
    float val;
	
    if (nPage == 2)
    {
        switch(msg)
        {
        case WM_INITDIALOG: // initialize controls here
            {
			    //-------------- texture size combo box ---------------------
                char buf[2048];

			    for (int i=0; i<4; i++)
			    {
				    int size = (int)pow(2, i+8);
				    sprintf(buf, " %4d x %4d ", size, size);
				    int nPos = SendMessage( GetDlgItem( hwnd, IDC_TEXSIZECOMBO ), CB_ADDSTRING, 0, (LPARAM)buf);
				    SendMessage( GetDlgItem( hwnd, IDC_TEXSIZECOMBO ), CB_SETITEMDATA, nPos, size);
			    }

			    // throw the "Auto" option in there
			    int nPos = SendMessage( GetDlgItem( hwnd, IDC_TEXSIZECOMBO ), CB_ADDSTRING, 0, (LPARAM)" Auto ");
			    SendMessage( GetDlgItem( hwnd, IDC_TEXSIZECOMBO ), CB_SETITEMDATA, nPos, -1);
			    
			    for (i=0; i<4+1; i++)
			    {
				    int size = SendMessage( GetDlgItem( hwnd, IDC_TEXSIZECOMBO ), CB_GETITEMDATA, i, 0);
				    if (size == m_nTexSize)
				    {
					    SendMessage( GetDlgItem( hwnd, IDC_TEXSIZECOMBO ), CB_SETCURSEL, i, 0);
				    }
			    }

			    //-------------- mesh size combo box ---------------------
			    HWND meshcombo = GetDlgItem( hwnd, IDC_MESHSIZECOMBO );
			    nPos = SendMessage(meshcombo , CB_ADDSTRING, 0, (LPARAM)"  8 x  6 FAST ");
			    SendMessage( meshcombo, CB_SETITEMDATA, nPos, 8);
			    nPos = SendMessage( meshcombo, CB_ADDSTRING, 0, (LPARAM)" 16 x 12 fast ");
			    SendMessage( meshcombo, CB_SETITEMDATA, nPos, 16);
			    nPos = SendMessage( meshcombo, CB_ADDSTRING, 0, (LPARAM)" 24 x 18 ");
			    SendMessage( meshcombo, CB_SETITEMDATA, nPos, 24);
			    nPos = SendMessage( meshcombo, CB_ADDSTRING, 0, (LPARAM)" 32 x 24 (default)");
			    SendMessage( meshcombo, CB_SETITEMDATA, nPos, 32);
			    nPos = SendMessage( meshcombo, CB_ADDSTRING, 0, (LPARAM)" 40 x 30 ");
			    SendMessage( meshcombo, CB_SETITEMDATA, nPos, 40);
			    nPos = SendMessage( meshcombo, CB_ADDSTRING, 0, (LPARAM)" 48 x 36 slow ");
			    SendMessage( meshcombo, CB_SETITEMDATA, nPos, 48);
			    nPos = SendMessage( meshcombo, CB_ADDSTRING, 0, (LPARAM)" 64 x 48 SLOW ");
			    SendMessage( meshcombo, CB_SETITEMDATA, nPos, 64);
			    nPos = SendMessage( meshcombo, CB_ADDSTRING, 0, (LPARAM)" 80 x 60 SLOW ");
			    SendMessage( meshcombo, CB_SETITEMDATA, nPos, 80);
			    nPos = SendMessage( meshcombo, CB_ADDSTRING, 0, (LPARAM)" 96 x 72 SLOW ");
			    SendMessage( meshcombo, CB_SETITEMDATA, nPos, 96);
			    nPos = SendMessage( meshcombo, CB_ADDSTRING, 0, (LPARAM)"128 x 96 SLOW ");
			    SendMessage( meshcombo, CB_SETITEMDATA, nPos, 128);
                // note: if you expand max. size, be sure to also raise MAX_GRID_X and MAX_GRID_Y
                // in md_defines.h!

			    int nCount = SendMessage( GetDlgItem( hwnd, IDC_MESHSIZECOMBO ), CB_GETCOUNT, 0, 0);
			    for (i=0; i<nCount; i++)
			    {
				    int size = SendMessage( GetDlgItem( hwnd, IDC_MESHSIZECOMBO ), CB_GETITEMDATA, i, 0);
				    if (size == m_nGridX)
				    {
					    SendMessage( GetDlgItem( hwnd, IDC_MESHSIZECOMBO ), CB_SETCURSEL, i, 0);
				    }
			    }

			    //---------16-bit brightness slider--------------

			    SendMessage( GetDlgItem( hwnd, IDC_BRIGHT_SLIDER), TBM_SETRANGEMIN,
				    FALSE, (LPARAM)(0) );
			    SendMessage( GetDlgItem( hwnd, IDC_BRIGHT_SLIDER), TBM_SETRANGEMAX,
				    FALSE, (LPARAM)(4) );
			    SendMessage( GetDlgItem( hwnd, IDC_BRIGHT_SLIDER), TBM_SETPOS,
				    TRUE, (LPARAM)(m_n16BitGamma) );
			    for (i=0; i<5; i++)
				    SendMessage( GetDlgItem( hwnd, IDC_BRIGHT_SLIDER), TBM_SETTIC, 0, i);

			    // append debug output filename to the checkbox's text
			    GetWindowText( GetDlgItem(hwnd, IDC_CB_DEBUGOUTPUT), buf, 256);
			    strcat(buf, DEBUGFILE);
			    SetWindowText( GetDlgItem(hwnd, IDC_CB_DEBUGOUTPUT), buf);

			    // set checkboxes
			    CheckDlgButton(hwnd, IDC_CB_DEBUGOUTPUT, g_bDebugOutput);
			    //CheckDlgButton(hwnd, IDC_CB_PRESSF1, (!m_bShowPressF1ForHelp));
			    //CheckDlgButton(hwnd, IDC_CB_TOOLTIPS, m_bShowMenuToolTips);
			    CheckDlgButton(hwnd, IDC_CB_ALWAYS3D, m_bAlways3D);
			    //CheckDlgButton(hwnd, IDC_CB_FIXSLOWTEXT, m_bFixSlowText);
			    //CheckDlgButton(hwnd, IDC_CB_TOP, m_bAlwaysOnTop);
			    //CheckDlgButton(hwnd, IDC_CB_CLS, !m_bClearScreenAtStartup);
			    CheckDlgButton(hwnd, IDC_CB_NOWARN, m_bWarningsDisabled);
			    CheckDlgButton(hwnd, IDC_CB_NOWARN2, m_bWarningsDisabled2);
                CheckDlgButton(hwnd, IDC_CB_ANISO, m_bAnisotropicFiltering);
                CheckDlgButton(hwnd, IDC_CB_SCROLLON, m_bPresetLockOnAtStartup);
			    //CheckDlgButton(hwnd, IDC_CB_PINKFIX, m_bFixPinkBug);
			    CheckDlgButton(hwnd, IDC_CB_NORATING, !m_bEnableRating);
                CheckDlgButton(hwnd, IDC_CB_INSTASCAN, m_bInstaScan);
			    CheckDlgButton(hwnd, IDC_CB_AUTOGAMMA, m_bAutoGamma);

                RefreshTab2(hwnd);
            }
            break;  // case WM_INITDIALOG

        case WM_COMMAND:
            {
                int id = LOWORD(wParam);
                //g_ignore_tab2_clicks = 1;
                switch (id)
                {
                case IDC_CB_NORATING:
				    m_bEnableRating = !DlgItemIsChecked(hwnd, IDC_CB_NORATING);
				    RefreshTab2(hwnd);
				    break;				

                case IDC_CB_AUTOGAMMA:
				    m_bAutoGamma = DlgItemIsChecked(hwnd, IDC_CB_AUTOGAMMA);
				    RefreshTab2(hwnd);
				    break;				
                    
                case ID_SPRITE:
				    {
					    char szPath[512], szFile[512];
					    strcpy(szPath, GetConfigIniFile());
					    char *p = strrchr(szPath, '\\');
					    if (p != NULL)
					    {
						    *(p+1) = 0;
						    strcpy(szFile, szPath);
						    strcat(szFile, "milk_img.ini");
						    //dumpmsg("opening: (file, path)");
						    //dumpmsg(szFile);
						    //dumpmsg(szPath);
						    int ret = (int)ShellExecute(NULL, "open", szFile, NULL, szPath, SW_SHOWNORMAL);
						    if (ret <= 32)
						    {
							    MessageBox(hwnd, "error in ShellExecute", "error in ShellExecute", MB_OK|MB_SETFOREGROUND|MB_TOPMOST|MB_TASKMODAL);
						    }
					    }
				    }
				    break;

			    case ID_MSG:
				    {
					    char szPath[512], szFile[512];
					    strcpy(szPath, GetConfigIniFile());
					    char *p = strrchr(szPath, '\\');
					    if (p != NULL)
					    {
						    *(p+1) = 0;
						    strcpy(szFile, szPath);
						    strcat(szFile, "milk_msg.ini");
						    //dumpmsg("opening: (file, path)");
						    //dumpmsg(szFile);
						    //dumpmsg(szPath);
						    int ret = (int)ShellExecute(NULL, "open", szFile, NULL, szPath, SW_SHOWNORMAL);
						    if (ret <= 32)
						    {
							    MessageBox(hwnd, "error in ShellExecute", "error in ShellExecute", MB_OK|MB_SETFOREGROUND|MB_TOPMOST|MB_TASKMODAL);
						    }
					    }
				    }
				    break;
                }
                //g_ignore_tab2_clicks = 0;
            } // end WM_COMMAND case
            break;

        case WM_DESTROY:    // read controls here
            {
				// texsize
				t = SendMessage( GetDlgItem( hwnd, IDC_TEXSIZECOMBO ), CB_GETCURSEL, 0, 0);
				if (t == CB_ERR) t = 0;
				m_nTexSize	= SendMessage( GetDlgItem( hwnd, IDC_TEXSIZECOMBO ), CB_GETITEMDATA, t, 0);

				// mesh(grid)size
				t = SendMessage( GetDlgItem( hwnd, IDC_MESHSIZECOMBO ), CB_GETCURSEL, 0, 0);
				if (t == CB_ERR) t = 0;
				m_nGridX		= SendMessage( GetDlgItem( hwnd, IDC_MESHSIZECOMBO ), CB_GETITEMDATA, t, 0);

				// 16-bit-brightness slider
				t = SendMessage( GetDlgItem( hwnd, IDC_BRIGHT_SLIDER ), TBM_GETPOS, 0, 0);
				if (t != CB_ERR) m_n16BitGamma = t;

				// checkboxes
				g_bDebugOutput = DlgItemIsChecked(hwnd, IDC_CB_DEBUGOUTPUT);
				//m_bShowPressF1ForHelp = (!DlgItemIsChecked(hwnd, IDC_CB_PRESSF1));
				//m_bShowMenuToolTips = DlgItemIsChecked(hwnd, IDC_CB_TOOLTIPS);
				//m_bClearScreenAtStartup = !DlgItemIsChecked(hwnd, IDC_CB_CLS);
				m_bAlways3D = DlgItemIsChecked(hwnd, IDC_CB_ALWAYS3D);
				//m_bFixSlowText = DlgItemIsChecked(hwnd, IDC_CB_FIXSLOWTEXT);
				//m_bAlwaysOnTop = DlgItemIsChecked(hwnd, IDC_CB_TOP);
				m_bWarningsDisabled = DlgItemIsChecked(hwnd, IDC_CB_NOWARN);
				m_bWarningsDisabled2 = DlgItemIsChecked(hwnd, IDC_CB_NOWARN2);
                m_bAnisotropicFiltering = DlgItemIsChecked(hwnd, IDC_CB_ANISO);
                m_bPresetLockOnAtStartup = DlgItemIsChecked(hwnd, IDC_CB_SCROLLON);
				
				//m_bFixPinkBug = DlgItemIsChecked(hwnd, IDC_CB_PINKFIX);
				m_bEnableRating = !DlgItemIsChecked(hwnd, IDC_CB_NORATING);
                m_bInstaScan = DlgItemIsChecked(hwnd, IDC_CB_INSTASCAN);
				m_bAutoGamma = DlgItemIsChecked(hwnd, IDC_CB_AUTOGAMMA);
                
            }
            break;  // case WM_DESTROY

        case WM_HELP:   // give help box for controls here
            if (lParam)
            {
                HELPINFO *ph = (HELPINFO*)lParam;
                char title[256], buf[2048], ctrl_name[256];
                GetWindowText(GetDlgItem(hwnd, ph->iCtrlId), ctrl_name, sizeof(ctrl_name)-1);
                RemoveSingleAmpersands(ctrl_name);
                buf[0] = 0;

                strcpy(title, ctrl_name);
            
                switch(ph->iCtrlId)
                {
                case IDC_TEXSIZECOMBO:
                case IDC_TEXSIZECOMBO_CAPTION:
                    strcpy(title, "Texture Size");
                    strcpy(buf, 
                        "This sets the size of the image that milkdrop uses, internally,\r"
                        "to drive the visuals.  The bigger the value here, the crisper\r"
                        "the image you see.  It's highly recommended that you set this\r"
                        "to 'auto', which will determine the ideal image (texture) size\r"
                        "automatically.  However, if you experience visual problems (such\r"
                        "as black streaks or missing chunks in the image) due to low\r"
                        "video memory, you might want to set this to a low value (like\r"
                        "256x256 or 512x512)."
                    ); 
                    break;

                case IDC_MESHSIZECOMBO:
                case IDC_MESHSIZECOMBO_CAPTION:
                    strcpy(title, "Mesh Size");
                    strcpy(buf, 
                        "MilkDrop uses a mesh of polygons to warp the image each frame.\r"
                        "This setting determines how finely subdivided that mesh is.\r"
                        "A larger mesh size will mean finer resolution 'movement' in the\r"
                        "image; basically, it will look better.  Watch out, though - \r"
                        "only crank this way up if you have a fast CPU."
                    );
                    break;

                case IDC_CB_ALWAYS3D:
                    strcpy(buf, 
                        "Enable this to force all presets to be displayed in 3D mode.\r"
                        "(Note that you need glasses with differently-colored lenses\r"
                        " to see the effect.)"
                    );
                    break;
                case IDC_CB_INSTASCAN:
                    strcpy(title, "Scan presets instantly");
                    strcpy(buf, 
                        "Check this to scan all preset ratings instantly, at startup or when\r"
                        "the directory changes.  Normally, they are scanned in the background,\r"
                        "one per frame, until all of them are read in.  If you'd like to read\r"
                        "them all in instantly, to alleviate those few seconds of disk grinding,\r"
                        "check this box; but be warned that it will create a short pause at\r"
                        "startup or when you change directories.\r"
                        "\r"
                        "NOTE: this only applies if the preset rating system is enabled."
                    );
                    break;
                case IDC_CB_NORATING:
                    strcpy(title, "Disable preset rating");
                    strcpy(buf, 
                        "Check this to turn off the preset rating system.  This means that\r"
                        "at startup, or whenever you change the preset directory, there is\r"
                        "less likely to be a slowdown (usually a few seconds of disk activity)\r"
                        "while milkdrop scans all the presets to extract their ratings (if\r"
                        "instant scanning is off); or a short pause while it does this (if\r"
                        "instant scanning is on)."
                    );
                    break;
                case IDC_CB_NOWARN:
                    strcpy(buf, 
                        "Check this to disable warning messageboxes at startup."
                    );
                    break;
                case IDC_CB_NOWARN2:
                    strcpy(buf, 
                        "Check this to disable any & all warning messages that appear in the\r"
                        "upper-right corner of the screen."
                    );
                    break;
                case IDC_CB_ANISO:
                    strcpy(title, "Use anisotropic filtering");
                    strcpy(buf, 
                        "Check this to try to use an advanced form \r"
                        "of interpolation whenwarping the image each frame."
                    );
                    break;
                case IDC_CB_SCROLLON:
                    strcpy(title, "Start with preset lock ON");
                    strcpy(buf, 
                        "Check this to make MilkDrop automatically start in 'preset lock' mode,\r"
                        "meaning that the preset will not change until the user changes it\r"
                        "manually (either by pressing SPACE, hitting H for a hard cut, or by\r"
                        "selecting a new preset from the 'L'oad menu).\r"
                        "\r"
                        "Use the SCROLL LOCK key while MilkDrop is running to toggle the preset\r"
                        "lock on or off.  When the SCROLL LOCK light is on, that means that the\r"
                        "preset lock is also on, and vice versa."
                    );
                    break;

                case IDC_BRIGHT_SLIDER:
                case IDC_BRIGHT_SLIDER_BOX:
                case IDC_T1:
                case IDC_T2:
                case IDC_T3:
                case IDC_T4:
                case IDC_T5:
                    strcpy(buf, 
                        "The brightness slider lets you control the overall brightness\r"
                        "of the image.  If the image is continually washed out to bright\r"
                        "purple or white, you'll want to crank this down to (probably) zero.\r"
                        "If the image is chronically dark, crank this up.\r"
                        "\r"
                        "Note that the slider is not visible when the nearby 'guess'\r"
                        "checkbox is checked.  Uncheck it to manually set the brightness.\r"
                        "\r"
                        "Also note that this brightness adjustment is only a concern in\r"
                        "16-bit color modes.  (32-bit doesn't have this problem.)  So,\r"
                        "if you're running Windows in 16-bit color, this slider will affect\r"
                        "windowed, desktop, and 'fake' fullscreen modes.  And if you've\r"
                        "selected a 16-bit fullscreen display mode, it will affect that\r"
                        "too."
                    );
                    break;

                case IDC_CB_AUTOGAMMA:
                    strcpy(buf, 
                        "Check this option to ask milkdrop to make an educated guess\r"
                        "for the 'brightness control for 16-bit color' setting, based\r"
                        "on the vendor of your video card.  This usually gets it, but\r"
                        "not always.\r"
                        "\r"
                        "The slider is only visible when this option is unchecked.\r"
                        "\r"
                        "See the help for the slider for more information."
                    );
                    break;

                case ID_SPRITE:
                    strcpy(buf, 
                        "Click this button to edit milk_img.ini, the file that defines\r"
                        "all of the custom sprites you can invoke for display while\r"
                        "milkdrop is running.  A sprite is an image that you can fade\r"
                        "in or our, move around, and so on."
                    );
                    break;
                case ID_MSG:
                    strcpy(buf, 
                        "Click this button to edit milk_msg.ini, the file that you can\r"
                        "configure to set up custom overlaid text messages that you can\r"
                        "display while milkdrop is running."
                    );
                    break;
                }

                if (buf[0])
                    MessageBox(hwnd, buf, title, MB_OK|MB_SETFOREGROUND|MB_TOPMOST|MB_TASKMODAL);
            }
            break;  // case WM_HELP
        }
    }
    else if (nPage==3)
    {
        switch(msg)
        {
        case WM_INITDIALOG:
            {
                char buf[2048];

			    sprintf(buf, " %3.2f", m_fStereoSep);
			    SetWindowText( GetDlgItem( hwnd, IDC_3DSEP ), buf );

			    sprintf(buf, " %2.1f", m_fSongTitleAnimDuration);
			    SetWindowText( GetDlgItem( hwnd, IDC_SONGTITLEANIM_DURATION ), buf );
			    sprintf(buf, " %2.1f", m_fTimeBetweenRandomSongTitles);
			    SetWindowText( GetDlgItem( hwnd, IDC_RAND_TITLE             ), buf );
			    sprintf(buf, " %2.1f", m_fTimeBetweenRandomCustomMsgs);
			    SetWindowText( GetDlgItem( hwnd, IDC_RAND_MSG               ), buf );

			    CheckDlgButton(hwnd, IDC_CB_TITLE_ANIMS, m_bSongTitleAnims);
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam)==IDLEFT)
                DoColors(hwnd, &m_cLeftEye3DColor[0], &m_cLeftEye3DColor[1], &m_cLeftEye3DColor[2]);
            if (LOWORD(wParam)==IDRIGHT)
                DoColors(hwnd, &m_cRightEye3DColor[0], &m_cRightEye3DColor[1], &m_cRightEye3DColor[2]);
            break;
        case WM_DESTROY:
            {
                char buf[2048];

				GetWindowText( GetDlgItem( hwnd, IDC_3DSEP ), buf, sizeof(buf));
				if (sscanf(buf, "%f", &val) == 1)
					m_fStereoSep = val;

				GetWindowText( GetDlgItem( hwnd, IDC_SONGTITLEANIM_DURATION ), buf, sizeof(buf));
				if (sscanf(buf, "%f", &val) == 1)
					m_fSongTitleAnimDuration = val;
				GetWindowText( GetDlgItem( hwnd, IDC_RAND_TITLE             ), buf, sizeof(buf));
				if (sscanf(buf, "%f", &val) == 1)
					m_fTimeBetweenRandomSongTitles = val;
				GetWindowText( GetDlgItem( hwnd, IDC_RAND_MSG               ), buf, sizeof(buf));
				if (sscanf(buf, "%f", &val) == 1)
					m_fTimeBetweenRandomCustomMsgs = val;

				m_bSongTitleAnims = DlgItemIsChecked(hwnd, IDC_CB_TITLE_ANIMS);
            }
            break;
        case WM_HELP:   // give help box for controls here
            if (lParam)
            {
                HELPINFO *ph = (HELPINFO*)lParam;
                char title[256], buf[2048], ctrl_name[256];
                GetWindowText(GetDlgItem(hwnd, ph->iCtrlId), ctrl_name, sizeof(ctrl_name)-1);
                RemoveSingleAmpersands(ctrl_name);
                buf[0] = 0;

                strcpy(title, ctrl_name);
            
                switch(ph->iCtrlId)
                {
                case IDLEFT:
                    sprintf(buf, 
                        "Click this button to tell milkdrop the color of the %s-eye lens\r"
                        "of your colored 3D glasses.  If the color selected here doesn't match\r"
                        "the color of the lens, you won't be able to perceive depth when\r"
                        "running presets in 3D.", "left"
                    );
                    break;

                case IDRIGHT:
                    sprintf(buf, 
                        "Click this button to tell milkdrop the color of the %s-eye lens\r"
                        "of your colored 3D glasses.  If the color selected here doesn't match\r"
                        "the color of the lens, you won't be able to perceive depth when\r"
                        "running presets in 3D.", "right"
                    );
                    break;

                case IDC_3DSEP:
                case IDC_3DSEP_LABEL:
                    GetWindowText(GetDlgItem(hwnd, IDC_3DSEP_LABEL), title, sizeof(title)-1);
                    strcpy(buf, 
                        "Change this value to adjust the degree of depth perceived when\r"
                        "running milkdrop presets in 3D mode.  1.0 is the default; higher\r"
                        "values will exaggerate the perception of depth , but go too far\r"
                        "and the illusion of depth will be shattered.  Values between 0\r"
                        "and 1 will decrease the depth perception.  The ideal value depends\r"
                        "on the distance between you and your display (be it a monitor or\r"
                        "a projector screen) and the size of the display.\r"
                        "\r"
                        "The value of 1.0 is normalized to work well for most desktop\r"
                        "displays; if you're projecting milkdrop onto a wall or screen\r"
                        "for a crowd, you should DEFINITELY experiment with this value.\r"
                        "\r"
                        "Note that you can also change this value while milkdrop is running;\r"
                        "hit F1 (at runtime) for help on which keys will accomplish this."
                    );
                    break;

                case IDC_SONGTITLEANIM_DURATION:
                case IDC_SONGTITLEANIM_DURATION_LABEL:
                    GetWindowText(GetDlgItem(hwnd, IDC_SONGTITLEANIM_DURATION_LABEL), title, sizeof(title)-1);
                    strcpy(buf, 
                        "The duration, in seconds, of song title animations."
                    );
                    break;

                case IDC_RAND_TITLE:
                case IDC_RAND_TITLE_LABEL:
                    GetWindowText(GetDlgItem(hwnd, IDC_RAND_TITLE_LABEL), title, sizeof(title)-1);
                    strcpy(buf, 
                        "The mean (average) time, in seconds, between randomly-launched\r"
                        "song title animations.  Set to a negative value to disable random\r"
                        "launching."
                    );
                    break;

                case IDC_RAND_MSG:
                case IDC_RAND_MSG_LABEL:
                    GetWindowText(GetDlgItem(hwnd, IDC_RAND_MSG_LABEL), title, sizeof(title)-1);
                    strcpy(buf, 
                        "The mean (average) time, in seconds, between randomly-launched\r"
                        "custom messages (from milk_msg.ini).  Set to a negative value\r"
                        "to disable random launching."
                    );
                    break;

                case IDC_CB_TITLE_ANIMS:
                    strcpy(buf, 
                        "Check this to automatically launch song title animations whenever\r"
                        "the track changes."
                    );
                    break;
                }

                if (buf[0])
                     MessageBox(hwnd, buf, title, MB_OK|MB_SETFOREGROUND|MB_TOPMOST|MB_TASKMODAL);
            }
            break;  // case WM_HELP
        }
    }
    else if (nPage==4)
    {
        switch(msg)
        {
        case WM_INITDIALOG:
            {
                char buf[2048];

			    // soft cuts
			    sprintf(buf, " %2.1f", m_fTimeBetweenPresets);
			    SetWindowText( GetDlgItem( hwnd, IDC_BETWEEN_TIME ), buf );
			    sprintf(buf, " %2.1f", m_fTimeBetweenPresetsRand);
			    SetWindowText( GetDlgItem( hwnd, IDC_BETWEEN_TIME_RANDOM ), buf );
			    sprintf(buf, " %2.1f", m_fBlendTimeUser);
			    SetWindowText( GetDlgItem( hwnd, IDC_BLEND_USER ), buf );
			    sprintf(buf, " %2.1f", m_fBlendTimeAuto);
			    SetWindowText( GetDlgItem( hwnd, IDC_BLEND_AUTO ), buf );

			    // hard cuts
			    sprintf(buf, " %2.1f", m_fHardCutHalflife);
			    SetWindowText( GetDlgItem( hwnd, IDC_HARDCUT_BETWEEN_TIME ), buf );

			    int n = (int)((m_fHardCutLoudnessThresh - 1.25f) * 10.0f);
			    if (n<0) n = 0;
			    if (n>20) n = 20;
			    SendMessage( GetDlgItem( hwnd, IDC_HARDCUT_LOUDNESS), TBM_SETRANGEMIN, FALSE, (LPARAM)(0) );
			    SendMessage( GetDlgItem( hwnd, IDC_HARDCUT_LOUDNESS), TBM_SETRANGEMAX, FALSE, (LPARAM)(20) );
			    SendMessage( GetDlgItem( hwnd, IDC_HARDCUT_LOUDNESS), TBM_SETPOS,      TRUE,  (LPARAM)(n) );

			    CheckDlgButton(hwnd, IDC_CB_HARDCUTS, m_bHardCutsDisabled);
            }
            break;
        case WM_DESTROY:
            {
                char buf[2048];

				// soft cuts
				GetWindowText( GetDlgItem( hwnd, IDC_BETWEEN_TIME ), buf, sizeof(buf));
				if (sscanf(buf, "%f", &val) == 1)
					m_fTimeBetweenPresets = val;
				GetWindowText( GetDlgItem( hwnd, IDC_BETWEEN_TIME_RANDOM ), buf, sizeof(buf));
				if (sscanf(buf, "%f", &val) == 1)
					m_fTimeBetweenPresetsRand = val;
				GetWindowText( GetDlgItem( hwnd, IDC_BLEND_AUTO ), buf, sizeof(buf));
				if (sscanf(buf, "%f", &val) == 1)
					m_fBlendTimeAuto = val;
				GetWindowText( GetDlgItem( hwnd, IDC_BLEND_USER ), buf, sizeof(buf));
				if (sscanf(buf, "%f", &val) == 1)
					m_fBlendTimeUser = val;

				// hard cuts
				GetWindowText( GetDlgItem( hwnd, IDC_HARDCUT_BETWEEN_TIME ), buf, sizeof(buf));
				if (sscanf(buf, "%f", &val) == 1)
					m_fHardCutHalflife = val;

				t = SendMessage( GetDlgItem( hwnd, IDC_HARDCUT_LOUDNESS ), TBM_GETPOS, 0, 0);
				if (t != CB_ERR) m_fHardCutLoudnessThresh = 1.25f + t/10.0f;

				m_bHardCutsDisabled = DlgItemIsChecked(hwnd, IDC_CB_HARDCUTS);
            }
            break;
        case WM_HELP:
            if (lParam)
            {
                HELPINFO *ph = (HELPINFO*)lParam;
                char title[256], buf[2048], ctrl_name[256];
                GetWindowText(GetDlgItem(hwnd, ph->iCtrlId), ctrl_name, sizeof(ctrl_name)-1);
                RemoveSingleAmpersands(ctrl_name);
                buf[0] = 0;

                strcpy(title, ctrl_name);
            
                switch(ph->iCtrlId)
                {
                case IDC_BETWEEN_TIME:
                case IDC_BETWEEN_TIME_LABEL:
                    GetWindowText(GetDlgItem(hwnd, IDC_BETWEEN_TIME_LABEL), title, sizeof(title)-1);
                    strcpy(buf, 
                        "The minimum amount of time that elapses between preset changes\r"
                        "(excluding hard cuts, which take priority).  The old preset will\r"
                        "begin to blend or fade into a new preset after this amount of time,\r"
                        "plus some random amount of time as specified below in the\r"
                        "'additional random time' box.  Add these two values together to\r"
                        "get the maximum amount of time that will elapse between preset\r"
                        "changes."
                    );
                    break;

                case IDC_BETWEEN_TIME_RANDOM:
                case IDC_BETWEEN_TIME_RANDOM_LABEL:
                    GetWindowText(GetDlgItem(hwnd, IDC_BETWEEN_TIME_RANDOM_LABEL), title, sizeof(title)-1);
                    strcpy(buf, 
                        "The additional random maximum # of seconds between preset fades\r"
                        "(aka preset changes) (aka soft cuts)."
                    );
                    break;

                case IDC_BLEND_AUTO:
                case IDC_BLEND_AUTO_LABEL:
                    GetWindowText(GetDlgItem(hwnd, IDC_BLEND_AUTO_LABEL), title, sizeof(title)-1);
                    strcpy(buf, 
                        "The duration, in seconds, of a soft cut (a normal fade from one preset\r"
                        "to another) that is initiated because some amount of time has passed.\r"
                        "A value less than 1 will make for a very quick transition, while a value\r"
                        "around 3 or 4 will allow you to see some interesting behavior during\r"
                        "the blend."
                    );
                    break;

                case IDC_BLEND_USER:
                case IDC_BLEND_USER_LABEL:
                    GetWindowText(GetDlgItem(hwnd, IDC_BLEND_USER_LABEL), title, sizeof(title)-1);
                    strcpy(buf, 
                        "The duration, in seconds, of a soft cut (a normal fade from one preset\r"
                        "to another) that is initiated by you, when you press the 'H' key (for\r"
                        "a Hard cut).  A value less than 1 will make for a very quick transition,\r"
                        "while a value around 3 or 4 will allow you to see some interesting behavior\r"
                        "during the blend."
                    );
                    break;

                case IDC_HARDCUT_BETWEEN_TIME:
                case IDC_HARDCUT_BETWEEN_TIME_LABEL:
                    GetWindowText(GetDlgItem(hwnd, IDC_HARDCUT_BETWEEN_TIME_LABEL), title, sizeof(title)-1);
                    strcpy(buf, 
                        "The amount of time, in seconds, between hard cuts.  Hard cuts are\r"
                        "set off by loud beats in the music, with (ideally) about this much\r"
                        "time in between them."
                    );
                    break;

                case IDC_HARDCUT_LOUDNESS:
                case IDC_HARDCUT_LOUDNESS_LABEL:
                case IDC_HARDCUT_LOUDNESS_MIN:
                case IDC_HARDCUT_LOUDNESS_MAX:
                    GetWindowText(GetDlgItem(hwnd, IDC_HARDCUT_LOUDNESS_LABEL), title, sizeof(title)-1);
                    strcpy(buf, 
                        "Use this slider to adjust the sensitivity of the beat detection\r"
                        "algorithm used to detect the beats that cause hard cuts.  A value\r"
                        "close to 'min' will cause the algorithm to be very sensitive (so\r"
                        "even small beats will trigger it); a value close to 'max' will\r"
                        "cause only the largest beats to trigger it."
                    );
                    break;

                case IDC_CB_HARDCUTS:
                    strcpy(buf, 
                        "Check this to disable hard cuts; a loud beat\r"
                        "will never cause the preset to change."
                    );
                    break;

                }

                if (buf[0])
                     MessageBox(hwnd, buf, title, MB_OK|MB_SETFOREGROUND|MB_TOPMOST|MB_TASKMODAL);
            }
            break;
        }
    }
#endif 0

    return false;
}

//----------------------------------------------------------------------

void CPlugin::Randomize()
{
	srand((int)(GetTime()*100));
	//m_fAnimTime		= (rand() % 51234L)*0.01f;
	m_fRandStart[0]		= (rand() % 64841L)*0.01f;
	m_fRandStart[1]		= (rand() % 53751L)*0.01f;
	m_fRandStart[2]		= (rand() % 42661L)*0.01f;
	m_fRandStart[3]		= (rand() % 31571L)*0.01f;

	//CState temp;
	//temp.Randomize(rand() % NUM_MODES);
	//m_pState->StartBlend(&temp, m_fAnimTime, m_fBlendTimeUser);
}

//----------------------------------------------------------------------

void CPlugin::BuildMenus()
{
#if 0
    char buf[1024];

	m_pCurMenu = &m_menuPreset;//&m_menuMain;
	
	m_menuPreset     .Init("--edit current preset");
	m_menuMotion     .Init("--MOTION");
    m_menuCustomShape.Init("--drawing: custom shapes");
    m_menuCustomWave .Init("--drawing: custom waves");
	m_menuWave       .Init("--drawing: simple waveform");
	m_menuAugment    .Init("--drawing: borders, motion vectors");
	m_menuPost       .Init("--post-processing, misc.");
    for (int i=0; i<MAX_CUSTOM_WAVES; i++)
    {
        sprintf(buf, "--custom wave %d", i+1);
	    m_menuWavecode[i].Init(buf);
    }
    for (i=0; i<MAX_CUSTOM_SHAPES; i++)
    {
        sprintf(buf, "--custom shape %d", i+1);
	    m_menuShapecode[i].Init(buf);
    }
	
    //-------------------------------------------

    // MAIN MENU / menu hierarchy

    m_menuPreset.AddChildMenu(&m_menuMotion);
    m_menuPreset.AddChildMenu(&m_menuCustomShape);
    m_menuPreset.AddChildMenu(&m_menuCustomWave);
    m_menuPreset.AddChildMenu(&m_menuWave);
    m_menuPreset.AddChildMenu(&m_menuAugment);
    m_menuPreset.AddChildMenu(&m_menuPost);

    for (i=0; i<MAX_CUSTOM_SHAPES; i++)
	    m_menuCustomShape.AddChildMenu(&m_menuShapecode[i]);
    for (i=0; i<MAX_CUSTOM_WAVES; i++)
	    m_menuCustomWave.AddChildMenu(&m_menuWavecode[i]);
    
	// NOTE: all of the eval menuitems use a CALLBACK function to register the user's changes (see last param)
	m_menuPreset.AddItem("[ edit preset initialization code ]", &m_pState->m_szPerFrameInit, MENUITEMTYPE_STRING, 
        "read-only: zoom, rot, warp, cx, cy, dx, dy, sx, sy; decay, gamma;\r"
        "           echo_zoom, echo_scale, echo_orient;\r"
        "           ib_{size|r|g|b|a}, ob_{size|r|g|b|a}, mv_{x|y|dx|dy|l|r|g|b|a};\r"
        "           wave_{r|g|b|a|x|y|mode|mystery|usedots|thick|additive|brighten};\r"
        "           darken_center, wrap; invert, brighten, darken, solarize\r"
        "           time, fps, frame, progress; {bass|mid|treb}[_att]\r"
        "write:     q1-q8, monitor"
    , 256, 0, &OnUserEditedPresetInit);
	m_menuPreset.AddItem("[ edit per_frame equations ]", &m_pState->m_szPerFrameExpr, MENUITEMTYPE_STRING,        
        "read-only:  time, fps, frame, progress; {bass|mid|treb}[_att]\r"
        "read/write: zoom, rot, warp, cx, cy, dx, dy, sx, sy; q1-q8; monitor\r"
        "            mv_{x|y|dx|dy|l|r|g|b|a}, ib_{size|r|g|b|a}, ob_{size|r|g|b|a};\r"
        "            wave_{r|g|b|a|x|y|mode|mystery|usedots|thick|additive|brighten};\r"
        "            darken_center, wrap; invert, brighten, darken, solarize\r"
        "            decay, gamma, echo_zoom, echo_alpha, echo_orient"
    , 256, 0, &OnUserEditedPerFrame);
	m_menuPreset.AddItem("[ edit per_pixel equations ]", &m_pState->m_szPerPixelExpr, MENUITEMTYPE_STRING,        
        "read-only:  x, y, rad, ang; time, fps, frame, progress; {bass|mid|treb}[_att]\r"
        "read/write: dx, dy, zoom, rot, warp, cx, cy, sx, sy, q1-q8"
    , 256, 0, &OnUserEditedPerPixel);


    //-------------------------------------------

	// menu items
	m_menuWave.AddItem("wave type",         &m_pState->m_nWaveMode,     MENUITEMTYPE_INT, "each value represents a different way of drawing the waveform", 0, NUM_WAVES-1);
	m_menuWave.AddItem("size",              &m_pState->m_fWaveScale,    MENUITEMTYPE_LOGBLENDABLE, "relative size of the waveform");
	m_menuWave.AddItem("smoothing",         &m_pState->m_fWaveSmoothing,MENUITEMTYPE_BLENDABLE, "controls the smoothness of the waveform; 0=natural sound data (no smoothing), 0.9=max. smoothing", 0.0f, 0.9f);
	m_menuWave.AddItem("mystery parameter", &m_pState->m_fWaveParam,    MENUITEMTYPE_BLENDABLE, "what this one does is a secret (actually, its effect depends on the 'wave type'", -1.0f, 1.0f);
	m_menuWave.AddItem("position (X)",      &m_pState->m_fWaveX,        MENUITEMTYPE_BLENDABLE, "position of the waveform: 0 = far left edge of screen, 0.5 = center, 1 = far right", 0, 1);
	m_menuWave.AddItem("position (Y)",      &m_pState->m_fWaveY,        MENUITEMTYPE_BLENDABLE, "position of the waveform: 0 = very bottom of screen, 0.5 = center, 1 = top", 0, 1);
	m_menuWave.AddItem("color (red)",       &m_pState->m_fWaveR,        MENUITEMTYPE_BLENDABLE, "amount of red color in the wave (0..1)", 0, 1);
	m_menuWave.AddItem("color (green)",     &m_pState->m_fWaveG,        MENUITEMTYPE_BLENDABLE, "amount of green color in the wave (0..1)", 0, 1);
	m_menuWave.AddItem("color (blue)",      &m_pState->m_fWaveB,        MENUITEMTYPE_BLENDABLE, "amount of blue color in the wave (0..1)", 0, 1);
	m_menuWave.AddItem("opacity",           &m_pState->m_fWaveAlpha,    MENUITEMTYPE_LOGBLENDABLE, "opacity of the waveform; lower numbers = more transparent", 0.001f, 100.0f);
	m_menuWave.AddItem("use dots",          &m_pState->m_bWaveDots,     MENUITEMTYPE_BOOL, "if true, the waveform is drawn as dots (instead of lines)");
	m_menuWave.AddItem("draw thick",        &m_pState->m_bWaveThick,    MENUITEMTYPE_BOOL, "if true, the waveform's lines (or dots) are drawn with double thickness");
	m_menuWave.AddItem("modulate opacity by volume", &m_pState->m_bModWaveAlphaByVolume,  MENUITEMTYPE_BOOL,      "if true, the waveform opacity is affected by the music's volume");
	m_menuWave.AddItem("modulation: transparent volume", &m_pState->m_fModWaveAlphaStart, MENUITEMTYPE_BLENDABLE, "when the relative volume hits this level, the wave becomes transparent.  1 = normal loudness, 0.5 = extremely quiet, 1.5 = extremely loud", 0.0f, 2.0f);
	m_menuWave.AddItem("modulation: opaque volume",   &m_pState->m_fModWaveAlphaEnd,      MENUITEMTYPE_BLENDABLE, "when the relative volume hits this level, the wave becomes opaque.  1 = normal loudness, 0.5 = extremely quiet, 1.5 = extremely loud", 0.0f, 2.0f);
	m_menuWave.AddItem("additive drawing",  &m_pState->m_bAdditiveWaves, MENUITEMTYPE_BOOL, "if true, the wave is drawn additively, saturating the image at white");
	m_menuWave.AddItem("color brightening", &m_pState->m_bMaximizeWaveColor, MENUITEMTYPE_BOOL, "if true, the red, green, and blue color components will be scaled up until at least one of them reaches 1.0");

	m_menuAugment.AddItem("outer border thickness" ,&m_pState->m_fOuterBorderSize, MENUITEMTYPE_BLENDABLE, "thickness of the outer border drawn at the edges of the screen", 0, 0.5f);
	m_menuAugment.AddItem(" color (red)"           ,&m_pState->m_fOuterBorderR   , MENUITEMTYPE_BLENDABLE, "amount of red color in the outer border", 0, 1);
	m_menuAugment.AddItem(" color (green)"         ,&m_pState->m_fOuterBorderG   , MENUITEMTYPE_BLENDABLE, "amount of green color in the outer border", 0, 1);
	m_menuAugment.AddItem(" color (blue)"          ,&m_pState->m_fOuterBorderB   , MENUITEMTYPE_BLENDABLE, "amount of blue color in the outer border", 0, 1);
	m_menuAugment.AddItem(" opacity"               ,&m_pState->m_fOuterBorderA   , MENUITEMTYPE_BLENDABLE, "opacity of the outer border (0=transparent, 1=opaque)", 0, 1);
	m_menuAugment.AddItem("inner border thickness" ,&m_pState->m_fInnerBorderSize, MENUITEMTYPE_BLENDABLE, "thickness of the inner border drawn at the edges of the screen", 0, 0.5f);
	m_menuAugment.AddItem(" color (red)"           ,&m_pState->m_fInnerBorderR   , MENUITEMTYPE_BLENDABLE, "amount of red color in the inner border", 0, 1);
	m_menuAugment.AddItem(" color (green)"         ,&m_pState->m_fInnerBorderG   , MENUITEMTYPE_BLENDABLE, "amount of green color in the inner border", 0, 1);
	m_menuAugment.AddItem(" color (blue)"          ,&m_pState->m_fInnerBorderB   , MENUITEMTYPE_BLENDABLE, "amount of blue color in the inner border", 0, 1);
	m_menuAugment.AddItem(" opacity"               ,&m_pState->m_fInnerBorderA   , MENUITEMTYPE_BLENDABLE, "opacity of the inner border (0=transparent, 1=opaque)", 0, 1);
	m_menuAugment.AddItem("motion vector opacity"  ,&m_pState->m_fMvA            , MENUITEMTYPE_BLENDABLE, "opacity of the motion vectors (0=transparent, 1=opaque)", 0, 1);
	m_menuAugment.AddItem(" num. mot. vectors (X)" ,&m_pState->m_fMvX            , MENUITEMTYPE_BLENDABLE, "the number of motion vectors on the x-axis", 0, 64);
	m_menuAugment.AddItem(" num. mot. vectors (Y)" ,&m_pState->m_fMvY            , MENUITEMTYPE_BLENDABLE, "the number of motion vectors on the y-axis", 0, 48);
	m_menuAugment.AddItem(" offset (X)"            ,&m_pState->m_fMvDX           , MENUITEMTYPE_BLENDABLE, "horizontal placement offset of the motion vectors", -1, 1);
	m_menuAugment.AddItem(" offset (Y)"            ,&m_pState->m_fMvDY           , MENUITEMTYPE_BLENDABLE, "vertical placement offset of the motion vectors", -1, 1);
	m_menuAugment.AddItem(" trail length"          ,&m_pState->m_fMvL            , MENUITEMTYPE_BLENDABLE, "the length of the motion vectors (1=normal)", 0, 5);
	m_menuAugment.AddItem(" color (red)"           ,&m_pState->m_fMvR            , MENUITEMTYPE_BLENDABLE, "amount of red color in the motion vectors", 0, 1);
	m_menuAugment.AddItem(" color (green)"         ,&m_pState->m_fMvG            , MENUITEMTYPE_BLENDABLE, "amount of green color in the motion vectors", 0, 1);
	m_menuAugment.AddItem(" color (blue)"          ,&m_pState->m_fMvB            , MENUITEMTYPE_BLENDABLE, "amount of blue color in the motion vectors", 0, 1);

	m_menuMotion.AddItem("zoom amount",     &m_pState->m_fZoom,         MENUITEMTYPE_LOGBLENDABLE, "controls inward/outward motion.  0.9=zoom out, 1.0=no zoom, 1.1=zoom in");
	m_menuMotion.AddItem(" zoom exponent",  &m_pState->m_fZoomExponent, MENUITEMTYPE_LOGBLENDABLE, "controls the curvature of the zoom; 1=normal");
	m_menuMotion.AddItem("warp amount",     &m_pState->m_fWarpAmount,   MENUITEMTYPE_LOGBLENDABLE, "controls the magnitude of the warping; 0=none, 1=normal, 2=major warping...");
	m_menuMotion.AddItem(" warp scale",     &m_pState->m_fWarpScale,    MENUITEMTYPE_LOGBLENDABLE, "controls the wavelength of the warp; 1=normal, less=turbulent, more=smoother");
	m_menuMotion.AddItem(" warp speed",     &m_pState->m_fWarpAnimSpeed,MENUITEMTYPE_LOGFLOAT,     "controls the speed of the warp; 1=normal, less=slower, more=faster");
	m_menuMotion.AddItem("rotation amount",     &m_pState->m_fRot,      MENUITEMTYPE_BLENDABLE,    "controls the amount of rotation.  0=none, 0.1=slightly right, -0.1=slightly clockwise, 0.1=CCW", -1.00f, 1.00f);
	m_menuMotion.AddItem(" rot., center of (X)",&m_pState->m_fRotCX,    MENUITEMTYPE_BLENDABLE,    "controls where the center of rotation is, horizontally.  0=left, 0.5=center, 1=right", -1.0f, 2.0f);
	m_menuMotion.AddItem(" rot., center of (Y)",&m_pState->m_fRotCY,    MENUITEMTYPE_BLENDABLE,    "controls where the center of rotation is, vertically.  0=top, 0.5=center, 1=bottom", -1.0f, 2.0f);
	m_menuMotion.AddItem("translation (X)",     &m_pState->m_fXPush,    MENUITEMTYPE_BLENDABLE,    "controls amount of constant horizontal motion; -0.01 = slight shift right, 0=none, 0.01 = to left", -1.0f, 1.0f);
	m_menuMotion.AddItem("translation (Y)",     &m_pState->m_fYPush,    MENUITEMTYPE_BLENDABLE,    "controls amount of constant vertical motion; -0.01 = slight shift downward, 0=none, 0.01 = upward", -1.0f, 1.0f);
	m_menuMotion.AddItem("scaling (X)",         &m_pState->m_fStretchX, MENUITEMTYPE_LOGBLENDABLE, "controls amount of constant horizontal stretching; 0.99=shrink, 1=normal, 1.01=stretch");
	m_menuMotion.AddItem("scaling (Y)",         &m_pState->m_fStretchY, MENUITEMTYPE_LOGBLENDABLE, "controls amount of constant vertical stretching; 0.99=shrink, 1=normal, 1.01=stretch");

	m_menuPost.AddItem("sustain level",           &m_pState->m_fDecay,                MENUITEMTYPE_BLENDABLE, "controls the eventual fade to black; 1=no fade, 0.9=strong fade; 0.98=recommended.", 0.50f, 1.0f);
	m_menuPost.AddItem("darken center",           &m_pState->m_bDarkenCenter,         MENUITEMTYPE_BOOL,      "when ON, help keeps the image from getting too bright by continually dimming the center point");
	m_menuPost.AddItem("gamma adjustment",        &m_pState->m_fGammaAdj,             MENUITEMTYPE_BLENDABLE, "controls brightness; 1=normal, 2=double, 3=triple, etc.", 1.0f, 8.0f);
	m_menuPost.AddItem("hue shader",              &m_pState->m_fShader,               MENUITEMTYPE_BLENDABLE, "adds subtle color variations to the image.  0=off, 1=fully on", 0.0f, 1.0f);
	m_menuPost.AddItem("video echo: alpha",       &m_pState->m_fVideoEchoAlpha,       MENUITEMTYPE_BLENDABLE, "controls the opacity of the second graphics layer; 0=transparent (off), 0.5=half-mix, 1=opaque", 0.0f, 1.0f);
	m_menuPost.AddItem(" video echo: zoom",       &m_pState->m_fVideoEchoZoom,        MENUITEMTYPE_LOGBLENDABLE, "controls the size of the second graphics layer");
	m_menuPost.AddItem(" video echo: orientation",&m_pState->m_nVideoEchoOrientation, MENUITEMTYPE_INT, "selects an orientation for the second graphics layer.  0=normal, 1=flip on x, 2=flip on y, 3=flip on both", 0.0f, 3.0f);
	m_menuPost.AddItem("texture wrap",            &m_pState->m_bTexWrap,              MENUITEMTYPE_BOOL, "sets whether or not screen elements can drift off of one side and onto the other");
	m_menuPost.AddItem("stereo 3D",               &m_pState->m_bRedBlueStereo,        MENUITEMTYPE_BOOL, "displays the image in stereo 3D; you need 3D glasses (with red and blue lenses) for this.");
	m_menuPost.AddItem("filter: invert",          &m_pState->m_bInvert,        MENUITEMTYPE_BOOL, "inverts the colors in the image");
	m_menuPost.AddItem("filter: brighten",        &m_pState->m_bBrighten,      MENUITEMTYPE_BOOL, "brightens the darker parts of the image (nonlinear; square root filter)");
	m_menuPost.AddItem("filter: darken",          &m_pState->m_bDarken,        MENUITEMTYPE_BOOL, "darkens the brighter parts of the image (nonlinear; squaring filter)");
	m_menuPost.AddItem("filter: solarize",        &m_pState->m_bSolarize,      MENUITEMTYPE_BOOL, "emphasizes mid-range colors");

    for (i=0; i<MAX_CUSTOM_WAVES; i++)
    {
        // blending: do both; fade opacities in/out (w/exagerrated weighting)
        m_menuWavecode[i].AddItem("enabled"            ,&m_pState->m_wave[i].enabled,    MENUITEMTYPE_BOOL, "enables or disables this custom waveform/spectrum"); // bool
        m_menuWavecode[i].AddItem("number of samples"  ,&m_pState->m_wave[i].samples,    MENUITEMTYPE_INT, "the number of samples (points) that makes up the waveform", 2, 512);        // 0-512
        m_menuWavecode[i].AddItem("L/R separation"     ,&m_pState->m_wave[i].sep,        MENUITEMTYPE_INT, "the offset between the left & right channels; useful for doing phase plots. Keep low (<32) when using w/spectrum.", 0, 256);        // 0-512
        m_menuWavecode[i].AddItem("scaling"            ,&m_pState->m_wave[i].scaling,    MENUITEMTYPE_LOGFLOAT, "the size of the wave (1=normal)");
        m_menuWavecode[i].AddItem("smoothing"          ,&m_pState->m_wave[i].smoothing,  MENUITEMTYPE_FLOAT, "0=the raw wave; 1=a highly damped (smoothed) wave", 0, 1);
	    m_menuWavecode[i].AddItem("color (red)"        ,&m_pState->m_wave[i].r,          MENUITEMTYPE_FLOAT, "amount of red color in the wave (0..1)", 0, 1);
	    m_menuWavecode[i].AddItem("color (green)"      ,&m_pState->m_wave[i].g,          MENUITEMTYPE_FLOAT, "amount of green color in the wave (0..1)", 0, 1);
	    m_menuWavecode[i].AddItem("color (blue)"       ,&m_pState->m_wave[i].b,          MENUITEMTYPE_FLOAT, "amount of blue color in the wave (0..1)", 0, 1);
	    m_menuWavecode[i].AddItem("opacity"            ,&m_pState->m_wave[i].a,          MENUITEMTYPE_FLOAT, "opacity of the waveform; 0=transparent, 1=opaque", 0, 1);
        m_menuWavecode[i].AddItem("use spectrum"       ,&m_pState->m_wave[i].bSpectrum,  MENUITEMTYPE_BOOL, "if ON, the data in value1 and value2 will constitute a frequency spectrum (instead of waveform values)");        // 0-5 [0=wave left, 1=wave center, 2=wave right; 3=spectrum left, 4=spec center, 5=spec right]
        m_menuWavecode[i].AddItem("use dots"           ,&m_pState->m_wave[i].bUseDots,   MENUITEMTYPE_BOOL, "if ON, the samples will be drawn with dots, instead of connected lines"); // bool
        m_menuWavecode[i].AddItem("draw thick"         ,&m_pState->m_wave[i].bDrawThick, MENUITEMTYPE_BOOL, "if ON, the samples will be overdrawn 4X to make them thicker, bolder, and more visible"); // bool
        m_menuWavecode[i].AddItem("additive drawing"   ,&m_pState->m_wave[i].bAdditive,  MENUITEMTYPE_BOOL, "if ON, the samples will add color to sature the image toward white; otherwise, they replace what's there."); // bool
        m_menuWavecode[i].AddItem("--export to file"   ,(void*)UI_EXPORT_WAVE, MENUITEMTYPE_UIMODE, "export the settings for this custom waveform to a file on disk", 0, 0, NULL, UI_EXPORT_WAVE, i);              
        m_menuWavecode[i].AddItem("--import from file" ,(void*)UI_IMPORT_WAVE, MENUITEMTYPE_UIMODE, "import settings for a custom waveform from a file on disk"     , 0, 0, NULL, UI_IMPORT_WAVE, i);       
        m_menuWavecode[i].AddItem("[ edit initialization code ]",&m_pState->m_wave[i].m_szInit, MENUITEMTYPE_STRING, "IN: time, frame, fps, progress; q1-q8 (from preset init) / OUT: t1-t8", 256, 0, &OnUserEditedWavecodeInit);   
        m_menuWavecode[i].AddItem("[ edit per-frame code ]",&m_pState->m_wave[i].m_szPerFrame,  MENUITEMTYPE_STRING, "IN: time, frame, fps, progress; q1-q8, t1-t8; r,g,b,a; {bass|mid|treb}[_att] / OUT: r,g,b,a; t1-t8", 256, 0, &OnUserEditedWavecode);   
        m_menuWavecode[i].AddItem("[ edit per-point code ]",&m_pState->m_wave[i].m_szPerPoint,  MENUITEMTYPE_STRING, "IN: sample [0..1]; value1 [left ch], value2 [right ch], plus all vars for per-frame code / OUT: x,y; r,g,b,a; t1-t8", 256, 0, &OnUserEditedWavecode);       
    }

    for (i=0; i<MAX_CUSTOM_SHAPES; i++)
    {
        // blending: do both; fade opacities in/out (w/exagerrated weighting)
        m_menuShapecode[i].AddItem("enabled"             ,&m_pState->m_shape[i].enabled,    MENUITEMTYPE_BOOL, "enables or disables this shape"); // bool
        m_menuShapecode[i].AddItem("number of sides"     ,&m_pState->m_shape[i].sides,      MENUITEMTYPE_INT, "the default number of sides that make up the polygonal shape", 3, 100);        
        m_menuShapecode[i].AddItem("draw thick"          ,&m_pState->m_shape[i].thickOutline,MENUITEMTYPE_BOOL, "if ON, the border will be overdrawn 4X to make it thicker, bolder, and more visible"); // bool
        m_menuShapecode[i].AddItem("additive drawing"    ,&m_pState->m_shape[i].additive,   MENUITEMTYPE_BOOL, "if ON, the shape will add color to sature the image toward white; otherwise, it will replace what's there."); // bool
	    m_menuShapecode[i].AddItem("x position"          ,&m_pState->m_shape[i].x,          MENUITEMTYPE_FLOAT, "default x position of the shape (0..1; 0=left side, 1=right side)", 0, 1);
	    m_menuShapecode[i].AddItem("y position"          ,&m_pState->m_shape[i].y,          MENUITEMTYPE_FLOAT, "default y position of the shape (0..1; 0=bottom, 1=top of screen)", 0, 1);
	    m_menuShapecode[i].AddItem("radius"              ,&m_pState->m_shape[i].rad,        MENUITEMTYPE_LOGFLOAT, "default radius of the shape (0+)");
	    m_menuShapecode[i].AddItem("angle"               ,&m_pState->m_shape[i].ang,        MENUITEMTYPE_FLOAT,    "default rotation angle of the shape (0...3.14*2)", 0, 3.1415927f*2.0f);
        m_menuShapecode[i].AddItem("textured"            ,&m_pState->m_shape[i].textured,   MENUITEMTYPE_BOOL, "if ON, the shape will be textured with the image from the previous frame"); // bool
        m_menuShapecode[i].AddItem("texture zoom"        ,&m_pState->m_shape[i].tex_zoom,   MENUITEMTYPE_LOGFLOAT, "the portion of the previous frame's image to use with the shape"); // bool
        m_menuShapecode[i].AddItem("texture angle"       ,&m_pState->m_shape[i].tex_ang,    MENUITEMTYPE_FLOAT   , "the angle at which to rotate the previous frame's image before applying it to the shape", 0, 3.1415927f*2.0f); // bool
	    m_menuShapecode[i].AddItem("inner color (red)"   ,&m_pState->m_shape[i].r,          MENUITEMTYPE_FLOAT, "default amount of red color toward the center of the shape (0..1)", 0, 1);
	    m_menuShapecode[i].AddItem("inner color (green)" ,&m_pState->m_shape[i].g,          MENUITEMTYPE_FLOAT, "default amount of green color toward the center of the shape (0..1)", 0, 1);
	    m_menuShapecode[i].AddItem("inner color (blue)"  ,&m_pState->m_shape[i].b,          MENUITEMTYPE_FLOAT, "default amount of blue color toward the center of the shape (0..1)", 0, 1);
	    m_menuShapecode[i].AddItem("inner opacity"       ,&m_pState->m_shape[i].a,          MENUITEMTYPE_FLOAT, "default opacity of the center of the shape; 0=transparent, 1=opaque", 0, 1);
	    m_menuShapecode[i].AddItem("outer color (red)"   ,&m_pState->m_shape[i].r2,         MENUITEMTYPE_FLOAT, "default amount of red color toward the outer edge of the shape (0..1)", 0, 1);
	    m_menuShapecode[i].AddItem("outer color (green)" ,&m_pState->m_shape[i].g2,         MENUITEMTYPE_FLOAT, "default amount of green color toward the outer edge of the shape (0..1)", 0, 1);
	    m_menuShapecode[i].AddItem("outer color (blue)"  ,&m_pState->m_shape[i].b2,         MENUITEMTYPE_FLOAT, "default amount of blue color toward the outer edge of the shape (0..1)", 0, 1);
	    m_menuShapecode[i].AddItem("outer opacity"       ,&m_pState->m_shape[i].a2,         MENUITEMTYPE_FLOAT, "default opacity of the outer edge of the shape; 0=transparent, 1=opaque", 0, 1);
	    m_menuShapecode[i].AddItem("border color (red)"  ,&m_pState->m_shape[i].border_r,   MENUITEMTYPE_FLOAT, "default amount of red color in the shape's border (0..1)", 0, 1);
	    m_menuShapecode[i].AddItem("border color (green)",&m_pState->m_shape[i].border_g,   MENUITEMTYPE_FLOAT, "default amount of green color in the shape's border (0..1)", 0, 1);
	    m_menuShapecode[i].AddItem("border color (blue)" ,&m_pState->m_shape[i].border_b,   MENUITEMTYPE_FLOAT, "default amount of blue color in the shape's border (0..1)", 0, 1);
	    m_menuShapecode[i].AddItem("border opacity"      ,&m_pState->m_shape[i].border_a,   MENUITEMTYPE_FLOAT, "default opacity of the shape's border; 0=transparent, 1=opaque", 0, 1);
        m_menuShapecode[i].AddItem("--export to file"   ,NULL, MENUITEMTYPE_UIMODE, "export the settings for this custom shape to a file on disk", 0, 0, NULL, UI_EXPORT_SHAPE, i);       
        m_menuShapecode[i].AddItem("--import from file" ,NULL, MENUITEMTYPE_UIMODE, "import settings for a custom shape from a file on disk"     , 0, 0, NULL, UI_IMPORT_SHAPE, i);
        m_menuShapecode[i].AddItem("[ edit initialization code ]",&m_pState->m_shape[i].m_szInit, MENUITEMTYPE_STRING, "IN: time, frame, fps, progress; q1-q8 (from preset init); x,y,rad,ang; r,g,b,a; r2,g2,b2,a2; border_{r|g|b|a}; sides, thick, additive, textured\rOUT: t1-t8; x,y,rad,ang; r,g,b,a; r2,g2,b2,a2; border_{r|g|b|a}; sides, thick, additive, textured", 256, 0, &OnUserEditedShapecodeInit);   
        m_menuShapecode[i].AddItem("[ edit per-frame code ]",&m_pState->m_shape[i].m_szPerFrame,  MENUITEMTYPE_STRING, "IN: time, frame, fps, progress; q1-q8 (from preset init); x,y,rad,ang; r,g,b,a; r2,g2,b2,a2; border_{r|g|b|a}; sides, thick, additive, textured\rOUT: t1-t8; x,y,rad,ang; r,g,b,a; r2,g2,b2,a2; border_{r|g|b|a}; sides, thick, additive, textured", 256, 0, &OnUserEditedShapecode);   
        //m_menuShapecode[i].AddItem("[ edit per-point code ]",&m_pState->m_shape[i].m_szPerPoint,  MENUITEMTYPE_STRING, "IN: sample [0..1]; value1 [left ch], value2 [right ch], plus all vars for per-frame code / OUT: x,y; r,g,b,a; t1-t8", 256, 0, &OnUserEditedWavecode);       
    }
#endif 0
}

void CPlugin::WriteRealtimeConfig()
{
#if 0
	/*if (m_bSeparateTextWindow && m_hTextWnd)
	{
		RECT rect;
		if (GetWindowRect(m_hTextWnd, &rect))
		{
			WritePrivateProfileInt(rect.left,   "nTextWndLeft",   GetConfigIniFile(), "settings");
			WritePrivateProfileInt(rect.top,    "nTextWndTop",    GetConfigIniFile(), "settings");
			WritePrivateProfileInt(rect.right,  "nTextWndRight",  GetConfigIniFile(), "settings");
			WritePrivateProfileInt(rect.bottom, "nTextWndBottom", GetConfigIniFile(), "settings");
		}
	}*/

	WritePrivateProfileInt(m_bShowFPS			, "bShowFPS",        GetConfigIniFile(), "settings");
	WritePrivateProfileInt(m_bShowRating		, "bShowRating",     GetConfigIniFile(), "settings");
	WritePrivateProfileInt(m_bShowPresetInfo	, "bShowPresetInfo", GetConfigIniFile(), "settings");
	//WritePrivateProfileInt(m_bShowDebugInfo	, "bShowDebugInfo",  GetConfigIniFile(), "settings");
	WritePrivateProfileInt(m_bShowSongTitle	, "bShowSongTitle",  GetConfigIniFile(), "settings");
	WritePrivateProfileInt(m_bShowSongTime	, "bShowSongTime",   GetConfigIniFile(), "settings");
	WritePrivateProfileInt(m_bShowSongLen		, "bShowSongLen",    GetConfigIniFile(), "settings");
#endif
}

void CPlugin::dumpmsg(char *s)
{
  printf(s);
}

void CPlugin::LoadPreviousPreset(float fBlendTime) 
{
	// make sure file list is ok
	if (m_nPresets - m_nDirs == 0)
	{
		UpdatePresetList();
		if (m_nPresets - m_nDirs == 0)
		{
			// note: this error message is repeated in milkdropfs.cpp in DrawText()
			sprintf(m_szUserMessage, "ERROR: No preset files found in %s*.milk", m_szPresetDir);
			m_fShowUserMessageUntilThisTime = GetTime() + 6.0f;
			return;
		}
	}

	m_nCurrentPreset--;
	if (m_nCurrentPreset < m_nDirs || m_nCurrentPreset >= m_nPresets)
		m_nCurrentPreset = m_nPresets-1;

  strcpy(m_szCurrentPresetFile, m_szPresetDir);	// note: m_szPresetDir always ends with '\'
	strcat(m_szCurrentPresetFile, m_pPresetAddr[m_nCurrentPreset]);

  LoadPreset(m_szCurrentPresetFile, fBlendTime);
}

void CPlugin::LoadNextPreset(float fBlendTime) 
{
	// make sure file list is ok
	if (m_nPresets - m_nDirs == 0)
	{
		UpdatePresetList();
		if (m_nPresets - m_nDirs == 0)
		{
			// note: this error message is repeated in milkdropfs.cpp in DrawText()
			sprintf(m_szUserMessage, "ERROR: No preset files found in %s*.milk", m_szPresetDir);
			m_fShowUserMessageUntilThisTime = GetTime() + 6.0f;
			return;
		}
	}

	m_nCurrentPreset++;
	if (m_nCurrentPreset < m_nDirs || m_nCurrentPreset >= m_nPresets)
		m_nCurrentPreset = m_nDirs;

  strcpy(m_szCurrentPresetFile, m_szPresetDir);	// note: m_szPresetDir always ends with '\'
	strcat(m_szCurrentPresetFile, m_pPresetAddr[m_nCurrentPreset]);

  LoadPreset(m_szCurrentPresetFile, fBlendTime);
}

void CPlugin::LoadRandomPreset(float fBlendTime)
{
  if (m_bHoldPreset) return;
	// make sure file list is ok
	if (m_nPresets - m_nDirs == 0)
	{
		UpdatePresetList();
		if (m_nPresets - m_nDirs == 0)
		{
			// note: this error message is repeated in milkdropfs.cpp in DrawText()
			sprintf(m_szUserMessage, "ERROR: No preset files found in %s*.milk", m_szPresetDir);
			m_fShowUserMessageUntilThisTime = GetTime() + 6.0f;
			return;
		}
	}
	
	// --TEMPORARY--
	// this comes in handy if you want to mass-modify a batch of presets;
	// just automatically tweak values in Import, then they immediately get exported to a .MILK in a new dir.
	/*
	for (int i=0; i<m_nPresets; i++)
	{
		char szPresetFile[512];
		strcpy(szPresetFile, m_szPresetDir);	// note: m_szPresetDir always ends with '\'
		strcat(szPresetFile, m_pPresetAddr[i]);
		//CState newstate;
		m_state2.Import("preset00", szPresetFile);

		strcpy(szPresetFile, "c:\\t7\\");
		strcat(szPresetFile, m_pPresetAddr[i]);
		m_state2.Export("preset00", szPresetFile);
	}
	*/
	// --[END]TEMPORARY--

	if (m_bSequentialPresetOrder)
	{
		m_nCurrentPreset++;
		if (m_nCurrentPreset < m_nDirs || m_nCurrentPreset >= m_nPresets)
			m_nCurrentPreset = m_nDirs;
	}
	else
	{
		// pick a random file
		if (!m_bEnableRating || (m_pfPresetRating[m_nPresets - 1] < 0.1f) || (m_nRatingReadProgress < m_nPresets))
		{
			m_nCurrentPreset = m_nDirs + (rand() % (m_nPresets - m_nDirs));
		}
		else
		{
			float cdf_pos = (rand() % 14345)/14345.0f*m_pfPresetRating[m_nPresets - 1];

			/*
			char buf[512];
			sprintf(buf, "max = %f, rand = %f, \tvalues: ", m_pfPresetRating[m_nPresets - 1], cdf_pos);
			for (int i=m_nDirs; i<m_nPresets; i++)
			{
				char buf2[32];
				sprintf(buf2, "%3.1f ", m_pfPresetRating[i]);
				strcat(buf, buf2);
			}
			dumpmsg(buf);
			*/

			if (cdf_pos < m_pfPresetRating[m_nDirs])
			{
				m_nCurrentPreset = m_nDirs;
			}
			else
			{
				int lo = m_nDirs;
				int hi = m_nPresets;
				while (lo + 1 < hi)
				{
					int mid = (lo+hi)/2;
					if (m_pfPresetRating[mid] > cdf_pos)
						hi = mid;
					else
						lo = mid;
				}
				m_nCurrentPreset = hi;
			}
		}
	}

	// m_pPresetAddr[m_nCurrentPreset] points to the preset file to load (w/o the path);
	// first prepend the path, then load section [preset00] within that file
	strcpy(m_szCurrentPresetFile, m_szPresetDir);	// note: m_szPresetDir always ends with '\'
	strcat(m_szCurrentPresetFile, m_pPresetAddr[m_nCurrentPreset]);

	LoadPreset(m_szCurrentPresetFile, fBlendTime);
}

void CPlugin::RandomizeBlendPattern()
{
    if (!m_vertinfo)
        return;

    int mixtype = rand()%4;

    if (mixtype==0)
    {
        // constant, uniform blend
        int nVert = 0;
	    for (int y=0; y<=m_nGridY; y++)
	    {
		    for (int x=0; x<=m_nGridX; x++)
		    {
                m_vertinfo[nVert].a = 1;
                m_vertinfo[nVert].c = 0;
			    nVert++;
            }
        }
    }
    else if (mixtype==1)
    {
        // directional wipe
        float ang = FRAND*6.28f;
        float vx = cosf(ang);
        float vy = sinf(ang);
        float band = 0.1f + 0.2f*FRAND; // 0.2 is good
        float inv_band = 1.0f/band;
    
        int nVert = 0;
	    for (int y=0; y<=m_nGridY; y++)
	    {
            float fy = y/(float)m_nGridY;
		    for (int x=0; x<=m_nGridX; x++)
		    {
                float fx = x/(float)m_nGridX;

                // at t==0, mix rangse from -10..0
                // at t==1, mix ranges from   1..11

                float t = (fx-0.5f)*vx + (fy-0.5f)*vy + 0.5f;
                t = (t-0.5f)/sqrtf(2.0f) + 0.5f;

                m_vertinfo[nVert].a = inv_band * (1 + band);
                m_vertinfo[nVert].c = -inv_band + inv_band*t;//(x/(float)m_nGridX - 0.5f)/band;
			    nVert++;
		    }
	    }
    }
    else if (mixtype==2)
    {
        // plasma transition
        float band = 0.02f + 0.18f*FRAND;
        float inv_band = 1.0f/band;

        // first generate plasma array of height values
        m_vertinfo[                               0].c = FRAND;
        m_vertinfo[                        m_nGridX].c = FRAND;
        m_vertinfo[m_nGridY*(m_nGridX+1)           ].c = FRAND;
        m_vertinfo[m_nGridY*(m_nGridX+1) + m_nGridX].c = FRAND;
        GenPlasma(0, m_nGridX, 0, m_nGridY, 0.25f);

        // then find min,max so we can normalize to [0..1] range and then to the proper 'constant offset' range.
        float minc = m_vertinfo[0].c;
        float maxc = m_vertinfo[0].c;
        int x,y,nVert;
    
        nVert = 0;
	    for (y=0; y<=m_nGridY; y++)
	    {
		    for (x=0; x<=m_nGridX; x++)
            {
                if (minc > m_vertinfo[nVert].c)
                    minc = m_vertinfo[nVert].c;
                if (maxc < m_vertinfo[nVert].c)
                    maxc = m_vertinfo[nVert].c;
			    nVert++;
		    }
	    }

        float mult = 1.0f/(maxc-minc);
        nVert = 0;
	    for (y=0; y<=m_nGridY; y++)
	    {
		    for (x=0; x<=m_nGridX; x++)
            {
                float t = (m_vertinfo[nVert].c - minc)*mult;
                m_vertinfo[nVert].a = inv_band * (1 + band);
                m_vertinfo[nVert].c = -inv_band + inv_band*t;
                nVert++;
            }
        }
    }
    else if (mixtype==3)
    {
        // radial blend
        float band = 0.02f + 0.14f*FRAND + 0.34f*FRAND;
        float inv_band = 1.0f/band;
        float dir = (rand()%2)*2 - 1;

        int nVert = 0;
	    for (int y=0; y<=m_nGridY; y++)
	    {
            float dy = y/(float)m_nGridY - 0.5f;
		    for (int x=0; x<=m_nGridX; x++)
		    {
                float dx = x/(float)m_nGridX - 0.5f;
                float t = sqrtf(dx*dx + dy*dy)*1.41421f;
                if (dir==-1)
                    t = 1-t;

                m_vertinfo[nVert].a = inv_band * (1 + band);
                m_vertinfo[nVert].c = -inv_band + inv_band*t;
			    nVert++;
            }
        }
    }
}

void CPlugin::GenPlasma(int x0, int x1, int y0, int y1, float dt)
{
    int midx = (x0+x1)/2;
    int midy = (y0+y1)/2;
    float t00 = m_vertinfo[y0*(m_nGridX+1) + x0].c;
    float t01 = m_vertinfo[y0*(m_nGridX+1) + x1].c;
    float t10 = m_vertinfo[y1*(m_nGridX+1) + x0].c;
    float t11 = m_vertinfo[y1*(m_nGridX+1) + x1].c;

    if (y1-y0 >= 2)
    {
        if (x0==0)
            m_vertinfo[midy*(m_nGridX+1) + x0].c = 0.5f*(t00 + t10) + (FRAND*2-1)*dt;
        m_vertinfo[midy*(m_nGridX+1) + x1].c = 0.5f*(t01 + t11) + (FRAND*2-1)*dt;
    }
    if (x1-x0 >= 2)
    {
        if (y0==0)
            m_vertinfo[y0*(m_nGridX+1) + midx].c = 0.5f*(t00 + t01) + (FRAND*2-1)*dt;
        m_vertinfo[y1*(m_nGridX+1) + midx].c = 0.5f*(t10 + t11) + (FRAND*2-1)*dt;
    }

    if (y1-y0 >= 2 && x1-x0 >= 2)
    {
        // do midpoint & recurse:
        t00 = m_vertinfo[midy*(m_nGridX+1) + x0].c;
        t01 = m_vertinfo[midy*(m_nGridX+1) + x1].c;
        t10 = m_vertinfo[y0*(m_nGridX+1) + midx].c;
        t11 = m_vertinfo[y1*(m_nGridX+1) + midx].c;
        m_vertinfo[midy*(m_nGridX+1) + midx].c = 0.25f*(t10 + t11 + t00 + t01) + (FRAND*2-1)*dt;

        GenPlasma(x0, midx, y0, midy, dt*0.5f);
        GenPlasma(midx, x1, y0, midy, dt*0.5f);
        GenPlasma(x0, midx, midy, y1, dt*0.5f);
        GenPlasma(midx, x1, midy, y1, dt*0.5f);
    }
}

void CPlugin::LoadPreset(char *szPresetFilename, float fBlendTime)
{
	OutputDebugString("Milkdrop: Loading preset ");
	OutputDebugString(szPresetFilename);
	OutputDebugString("\n");


	if (szPresetFilename != m_szCurrentPresetFile)
		strcpy(m_szCurrentPresetFile, szPresetFilename);
	
	CState *temp = m_pState;
	m_pState = m_pOldState;
	m_pOldState = temp;

	m_pState->Import("preset00", m_szCurrentPresetFile);

    RandomizeBlendPattern();

	if (fBlendTime >= 0.001f)
		m_pState->StartBlendFrom(m_pOldState, GetTime(), fBlendTime);

	m_fPresetStartTime = GetTime();
	m_fNextPresetTime = -1.0f;		// flags UpdateTime() to recompute this

}

void CPlugin::SeekToPreset(char cStartChar)
{
	if (cStartChar >= 'a' && cStartChar <= 'z')
		cStartChar -= 'a' - 'A';

	for (int i = m_nDirs; i < m_nPresets; i++)
	{
		char ch = m_pPresetAddr[i][0];
		if (ch >= 'a' && ch <= 'z')
			ch -= 'a' - 'A';
		if (ch == cStartChar)
		{
			m_nPresetListCurPos = i;
			return;
		}
	}	
}


void CPlugin::UpdatePresetList()
{
	struct _finddata_t c_file;
	long hFile;
	//HANDLE hFindFile;

	char szMask[512];
	char szPath[512];
	char szLastPresetSelected[512];

	if (m_nPresetListCurPos >= m_nDirs && m_nPresetListCurPos < m_nPresets)
		strcpy(szLastPresetSelected, m_pPresetAddr[m_nPresetListCurPos]);
	else
		strcpy(szLastPresetSelected, "");

	// make sure the path exists; if not, go to winamp plugins dir
/*
	if (GetFileAttributes(m_szPresetDir) == -1)
	{
		strcpy(m_szPresetDir, m_szWinampPluginsPath);
		if (GetFileAttributes(m_szPresetDir) == -1)
		{
			strcpy(m_szPresetDir, "c:\\");
		}
	}
*/
	strcpy(szPath, m_szPresetDir);
	int len = strlen(szPath);
	if (len>0 && szPath[len-1] != '/') 
	{
		strcat(szPath, "/");
	}
	strcpy(szMask, szPath);
	strcat(szMask, "*.*");


	WIN32_FIND_DATA ffd;
	ZeroMemory(&ffd, sizeof(ffd));

    m_nRatingReadProgress = 0;
    
	for (int i=0; i<2; i++)		// usually RETURNs at end of first loop
	{
		m_nPresets = 0;
		m_nDirs    = 0;

		// find first .MILK file
		if( (hFile = _findfirst(szMask, &c_file )) != -1L )		// note: returns filename -without- path
		//if( (hFindFile = FindFirstFile(szMask, &ffd )) != INVALID_HANDLE_VALUE )		// note: returns filename -without- path
		{
			char *p = m_szpresets;
			int  bytes_left = m_nSizeOfPresetList - 1;		// save space for extra null-termination of last string
			
			/*
			len = strlen(ffd.cFileName);
			bytes_left -= len+1;
			strcpy(p, ffd.cFileName);
			p += len+1;
			m_nPresets++;
			*/
			//dumpmsg(ffd.cFileName);

			// find the rest
			//while (_findnext( hFile, &c_file ) == 0)

			do
			{
				bool bSkip = false;

				char szFilename[512];
				strcpy(szFilename, c_file.name);

				/*if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
				// skip "." directory
				if (strcmp(ffd.cFileName, ".")==0)// || strlen(ffd.cFileName) < 1)
				bSkip = true;
				else
				sprintf(szFilename, "*%s", ffd.cFileName);
				}
				else*/
				{
					// skip normal files not ending in ".milk"
					int len = strlen(c_file.name);
					if (len < 5 || strcmpi(c_file.name + len - 5, ".milk") != 0)
						bSkip = true;					
				}

				if (!bSkip)
				{
					//dumpmsg(szFilename);
					len = strlen(szFilename);
					bytes_left -= len+1;

					m_nPresets++;
					/*if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
					m_nDirs++;*/

					if (bytes_left >= 0)
					{
						strcpy(p, szFilename);
						p += len+1;
					}
				}
			}
			while(_findnext(hFile,&c_file) == 0);

			_findclose( hFile );

			if (bytes_left >= 0) 
			{
				// success!  double-null-terminate the last string.
				*p = 0;

				// also fill each slot of m_pPresetAddr with the address of each string
				// but do it in ALPHABETICAL ORDER
				if (m_pPresetAddr) delete m_pPresetAddr;
				m_pPresetAddr = new CHARPTR[m_nPresets];
				
				//the unsorted version:
				p = m_szpresets;
				for (int k=0; k<m_nPresets; k++)
				{
					m_pPresetAddr[k] = p;
					while (*p) p++;
					p++;
				}
				
				MergeSortPresets(0, m_nPresets-1);

				// finally, try to re-select the most recently-used preset in the list
				m_nPresetListCurPos = 0;
				if (szLastPresetSelected[0])
				{
					// try to automatically seek to the last preset loaded
					for (int i=m_nDirs; i<m_nPresets; i++)
						if (strcmp(szLastPresetSelected, m_pPresetAddr[i])==0)
							m_nPresetListCurPos = i; 
				}

                // call once, at least, to reallocate the ratings array:
				UpdatePresetRatings();

				// RETURN HERE - SUCCESS
				// RETURN HERE - SUCCESS
				// RETURN HERE - SUCCESS
				return;
			}
			else
			{
				// reallocate and do it again
				//dumpmsg("too many presets -> reallocating list...");
				int new_size = (-bytes_left + m_nSizeOfPresetList)*11/10;	// overallocate a bit
				delete m_szpresets;
				m_szpresets = new char[new_size];
				m_nSizeOfPresetList = new_size;
			}
		}
	}

	// should never get here
	sprintf(m_szUserMessage, "Unfathomable error getting preset file list!");
	m_fShowUserMessageUntilThisTime = GetTime() + 6.0f;
	
	m_nPresets = 0;
	m_nDirs = 0;
}


void CPlugin::MergeSortPresets(int left, int right)
{
	// note: left..right range is inclusive
	int nItems = right-left+1;

	if (nItems > 2)
	{
		// recurse to sort 2 halves (but don't actually recurse on a half if it only has 1 element)
		int mid = (left+right)/2;
		/*if (mid   != left) */ MergeSortPresets(left, mid);
		/*if (mid+1 != right)*/ MergeSortPresets(mid+1, right);
				
		// then merge results
		int a = left;
		int b = mid + 1;
		while (a <= mid && b <= right)
		{
			bool bSwap;

			// merge the sorted arrays; give preference to strings that start with a '*' character
			int nSpecial = 0;
			if (m_pPresetAddr[a][0] == '*') nSpecial++;
			if (m_pPresetAddr[b][0] == '*') nSpecial++;

			if (nSpecial == 1)
			{
				bSwap = (m_pPresetAddr[b][0] == '*');
			}
			else
			{
				bSwap = (mystrcmpi(m_pPresetAddr[a], m_pPresetAddr[b]) > 0);
			}

			if (bSwap)
			{
				CHARPTR temp = m_pPresetAddr[b];
				for (int k=b; k>a; k--)
					m_pPresetAddr[k] = m_pPresetAddr[k-1];
				m_pPresetAddr[a] = temp;
				mid++;
				b++;
			}
			a++;
		}
	}
	else if (nItems == 2)
	{
		// sort 2 items; give preference to 'special' strings that start with a '*' character
		int nSpecial = 0;
		if (m_pPresetAddr[left][0] == '*') nSpecial++;
		if (m_pPresetAddr[right][0] == '*') nSpecial++;

		if (nSpecial == 1)
		{
			if (m_pPresetAddr[right][0] == '*')
			{
				CHARPTR temp = m_pPresetAddr[left];
				m_pPresetAddr[left] = m_pPresetAddr[right];
				m_pPresetAddr[right] = temp;
			}
		}
		else if (mystrcmpi(m_pPresetAddr[left], m_pPresetAddr[right]) > 0)
		{
			CHARPTR temp = m_pPresetAddr[left];
			m_pPresetAddr[left] = m_pPresetAddr[right];
			m_pPresetAddr[right] = temp;
		}
	}
}


void CPlugin::WaitString_NukeSelection()
{
	if (m_waitstring.bActive &&
		m_waitstring.nSelAnchorPos != -1)
	{
		// nuke selection.  note: start & end are INCLUSIVE.
		int start = (m_waitstring.nCursorPos < m_waitstring.nSelAnchorPos) ? m_waitstring.nCursorPos : m_waitstring.nSelAnchorPos;
		int end   = (m_waitstring.nCursorPos > m_waitstring.nSelAnchorPos) ? m_waitstring.nCursorPos - 1 : m_waitstring.nSelAnchorPos - 1;
		int len   = strlen(m_waitstring.szText);
		int how_far_to_shift   = end - start + 1;
		int num_chars_to_shift = len - end;		// includes NULL char
		
		for (int i=0; i<num_chars_to_shift; i++)
			m_waitstring.szText[start + i] = m_waitstring.szText[start + i + how_far_to_shift];
		
		// clear selection
		m_waitstring.nCursorPos = start;
		m_waitstring.nSelAnchorPos = -1;
	}
}

void CPlugin::WaitString_Cut()
{
	if (m_waitstring.bActive &&
		m_waitstring.nSelAnchorPos != -1)
	{
		WaitString_Copy();
		WaitString_NukeSelection();
	}
}

void CPlugin::WaitString_Copy()
{
	if (m_waitstring.bActive &&
		m_waitstring.nSelAnchorPos != -1)
	{
		// note: start & end are INCLUSIVE.
		int start = (m_waitstring.nCursorPos < m_waitstring.nSelAnchorPos) ? m_waitstring.nCursorPos : m_waitstring.nSelAnchorPos;
		int end   = (m_waitstring.nCursorPos > m_waitstring.nSelAnchorPos) ? m_waitstring.nCursorPos - 1 : m_waitstring.nSelAnchorPos - 1;
		int chars_to_copy = end - start + 1;
		
		for (int i=0; i<chars_to_copy; i++)
			m_waitstring.szClipboard[i] = m_waitstring.szText[start + i];
		m_waitstring.szClipboard[chars_to_copy] = 0;
	}
}

void CPlugin::WaitString_Paste()
{
	// NOTE: if there is a selection, it is wiped out, and replaced with the clipboard contents.
	
	if (m_waitstring.bActive)
	{
		WaitString_NukeSelection();

		int len = strlen(m_waitstring.szText);
		int chars_to_insert = strlen(m_waitstring.szClipboard);
		int i;

		if (len + chars_to_insert + 1 >= m_waitstring.nMaxLen)
		{
			chars_to_insert = m_waitstring.nMaxLen - len - 1;
			
			// inform user
			strcpy(m_szUserMessage, "(string too long)");
			m_fShowUserMessageUntilThisTime = GetTime() + 2.5f;
		}
		else
		{
			m_fShowUserMessageUntilThisTime = GetTime();	// if there was an error message already, clear it
		}

		for (i=len; i >= m_waitstring.nCursorPos; i--)
			m_waitstring.szText[i + chars_to_insert] = m_waitstring.szText[i];
		for (i=0; i < chars_to_insert; i++)
			m_waitstring.szText[i + m_waitstring.nCursorPos] = m_waitstring.szClipboard[i];
		m_waitstring.nCursorPos += chars_to_insert;
	}
}

void CPlugin::WaitString_SeekLeftWord()
{
	// move to beginning of prior word 

	while (m_waitstring.nCursorPos > 0 && 
		   !IsAlphanumericChar(m_waitstring.szText[m_waitstring.nCursorPos-1]))
	{
		m_waitstring.nCursorPos--;
	}

	while (m_waitstring.nCursorPos > 0 &&
		   IsAlphanumericChar(m_waitstring.szText[m_waitstring.nCursorPos-1]))
	{
		m_waitstring.nCursorPos--;
	}
}

void CPlugin::WaitString_SeekRightWord()
{
	// move to beginning of next word
	
	//testing  lotsa   stuff  

	int len = strlen(m_waitstring.szText);

	while (m_waitstring.nCursorPos < len &&
		   IsAlphanumericChar(m_waitstring.szText[m_waitstring.nCursorPos]))
	{
		m_waitstring.nCursorPos++;
	}

	while (m_waitstring.nCursorPos < len &&
		   !IsAlphanumericChar(m_waitstring.szText[m_waitstring.nCursorPos]))
	{
		m_waitstring.nCursorPos++;
	}
}

int CPlugin::WaitString_GetCursorColumn()
{
	if (m_waitstring.bDisplayAsCode)
	{
		int column = 0;
		while (m_waitstring.nCursorPos - column - 1 >= 0 &&
			   m_waitstring.szText[m_waitstring.nCursorPos - column - 1] != LINEFEED_CONTROL_CHAR)
		{
			column++;
		}
		return column;
	}
	else 
	{
		return m_waitstring.nCursorPos;
	}
}

int	CPlugin::WaitString_GetLineLength()
{
	int line_start = m_waitstring.nCursorPos - WaitString_GetCursorColumn();
	int line_length = 0;

	while (m_waitstring.szText[line_start + line_length] != 0 &&
		   m_waitstring.szText[line_start + line_length] != LINEFEED_CONTROL_CHAR)
	{
		line_length++;
	}

	return line_length;
}

void CPlugin::WaitString_SeekUpOneLine()
{
	int column = g_plugin->WaitString_GetCursorColumn();

	if (column != m_waitstring.nCursorPos)
	{
		// seek to very end of previous line (cursor will be at the semicolon)
		m_waitstring.nCursorPos -= column + 1;
		
		int new_column = g_plugin->WaitString_GetCursorColumn();

		if (new_column > column)
			m_waitstring.nCursorPos -= (new_column - column);
	}
}

void CPlugin::WaitString_SeekDownOneLine()
{
	int column = g_plugin->WaitString_GetCursorColumn();
	int newpos = m_waitstring.nCursorPos;

	while (m_waitstring.szText[newpos] != 0 && m_waitstring.szText[newpos] != LINEFEED_CONTROL_CHAR)
		newpos++;

	if (m_waitstring.szText[newpos] != 0)
	{
		m_waitstring.nCursorPos = newpos + 1;

		while (	column > 0 && 
				m_waitstring.szText[m_waitstring.nCursorPos] != LINEFEED_CONTROL_CHAR &&
				m_waitstring.szText[m_waitstring.nCursorPos] != 0)
		{
			m_waitstring.nCursorPos++;
			column--;
		}
	}
}


void CPlugin::SavePresetAs(char *szNewFile)
{
	// overwrites the file if it was already there,
	// so you should check if the file exists first & prompt user to overwrite,
	//   before calling this function

	if (!m_pState->Export("preset00", szNewFile))
	{
		// error
		strcpy(m_szUserMessage, "ERROR: unable to save the file");
		m_fShowUserMessageUntilThisTime = GetTime() + 6.0f;
	}
	else
	{
		// pop up confirmation
		strcpy(m_szUserMessage, "[save successful]");
		m_fShowUserMessageUntilThisTime = GetTime() + 4.0f;

		// update m_pState->m_szDesc with the new name
		strcpy(m_pState->m_szDesc, m_waitstring.szText);

		// refresh file listing
		UpdatePresetList();
	}
}

void CPlugin::DeletePresetFile(char *szDelFile)
{
	// NOTE: this function additionally assumes that m_nPresetListCurPos indicates 
	//		 the slot that the to-be-deleted preset occupies!

	// delete file
	if (!DeleteFile(szDelFile))
	{
		// error
		strcpy(m_szUserMessage, "ERROR: unable to delete delete the file");
		m_fShowUserMessageUntilThisTime = GetTime() + 6.0f;
	}
	else
	{
		// pop up confirmation
		sprintf(m_szUserMessage, "[preset \"%s\" deleted]", m_pPresetAddr[m_nPresetListCurPos]);
		m_fShowUserMessageUntilThisTime = GetTime() + 4.0f;

		// refresh file listing & re-select the next file after the one deleted

		if (m_nPresetListCurPos < m_nPresets - 1)
			m_nPresetListCurPos++;
		else if (m_nPresetListCurPos > 0)
			m_nPresetListCurPos--;
		else 
			m_nPresetListCurPos = 0;

		UpdatePresetList();
	}
}

void CPlugin::RenamePresetFile(char *szOldFile, char *szNewFile)
{
	// NOTE: this function additionally assumes that m_nPresetListCurPos indicates 
	//		 the slot that the to-be-renamed preset occupies!

	if (GetFileAttributes(szNewFile) != -1)		// check if file already exists
	{
		// error
		strcpy(m_szUserMessage, "ERROR: a file already exists with that filename");
		m_fShowUserMessageUntilThisTime = GetTime() + 6.0f;
		
		// (user remains in UI_LOAD_RENAME mode to try another filename)
	}
	else
	{
		// rename 
		if (!MoveFile(szOldFile, szNewFile))
		{
			// error
			strcpy(m_szUserMessage, "ERROR: unable to rename the file");
			m_fShowUserMessageUntilThisTime = GetTime() + 6.0f;
		}
		else
		{
			// pop up confirmation
			strcpy(m_szUserMessage, "[rename successful]");
			m_fShowUserMessageUntilThisTime = GetTime() + 4.0f;

			// if this preset was the active one, update m_pState->m_szDesc with the new name
			char buf[512];
			sprintf(buf, "%s.milk", m_pState->m_szDesc);
			if (strcmp(m_pPresetAddr[m_nPresetListCurPos], buf) == 0)
			{
				strcpy(m_pState->m_szDesc, m_waitstring.szText);
			}

			// refresh file listing & do a trick to make it re-select the renamed file
			strcpy(m_pPresetAddr[m_nPresetListCurPos], m_waitstring.szText);
			strcat(m_pPresetAddr[m_nPresetListCurPos], ".milk");
			UpdatePresetList();
		}

		// exit waitstring mode (return to load menu)
		m_UI_mode = UI_LOAD;
		m_waitstring.bActive = false;
	}
}


void CPlugin::UpdatePresetRatings()
{
	if (!m_bEnableRating) 
		return;

    if (m_nRatingReadProgress==-1 || m_nRatingReadProgress==m_nPresets)
        return;
    
	int k;

	// read in ratings for each preset:
	//if (m_pfPresetRating) delete m_pfPresetRating;
	//m_pfPresetRating = new float[m_nPresets];
	if (m_pfPresetRating == NULL)
		m_pfPresetRating = new float[m_nPresets];

    if (m_nRatingReadProgress==0 && m_nDirs>0)
    {
	    for (k=0; k<m_nDirs; k++)
	    {
		    m_pfPresetRating[m_nRatingReadProgress] = 0.0f;
            m_nRatingReadProgress++;
	    }

        if (!m_bInstaScan)
            return;
    }

    int presets_per_frame = m_bInstaScan ? 4096 : 1;
    int k1 = m_nRatingReadProgress;
    int k2 = min(m_nRatingReadProgress + presets_per_frame, m_nPresets);
	for (k=k1; k<k2; k++)
	{
		char szFullPath[512];
		sprintf(szFullPath, "%s%s", m_szPresetDir, m_pPresetAddr[k]);
		float f = InternalGetPrivateProfileFloat("preset00", "fRating", 3.0f, szFullPath);
		if (f < 0) f = 0;
		if (f > 5) f = 5;

		if (k==0)
			m_pfPresetRating[k] = f;
		else
			m_pfPresetRating[k] = m_pfPresetRating[k-1] + f;

        m_nRatingReadProgress++;
	}
}


void CPlugin::SetCurrentPresetRating(float fNewRating)
{
	if (!m_bEnableRating)
		return;

	if (fNewRating < 0) fNewRating = 0;
	if (fNewRating > 5) fNewRating = 5;
	float change = (fNewRating - m_pState->m_fRating);

	// update the file on disk:
	//char szPresetFileNoPath[512];
	//char szPresetFileWithPath[512];
	//sprintf(szPresetFileNoPath,   "%s.milk", m_pState->m_szDesc);
	//sprintf(szPresetFileWithPath, "%s%s.milk", GetPresetDir(), m_pState->m_szDesc);
	WritePrivateProfileFloat(fNewRating, "fRating", m_szCurrentPresetFile, "preset00");

	// update the copy of the preset in memory
	m_pState->m_fRating = fNewRating;

	// update the cumulative internal listing:
	if (m_nCurrentPreset != -1 && m_nRatingReadProgress >= m_nCurrentPreset)		// (can be -1 if dir. changed but no new preset was loaded yet)
		for (int i=m_nCurrentPreset; i<m_nPresets; i++)
			m_pfPresetRating[i] += change;

	/* keep in view:
		-test switching dirs w/o loading a preset, and trying to change the rating
			->m_nCurrentPreset is out of range!
		-soln: when adjusting rating: 
			1. file to modify is m_szCurrentPresetFile
			2. only update CDF if m_nCurrentPreset is not -1
		-> set m_nCurrentPreset to -1 whenever dir. changes
		-> set m_szCurrentPresetFile whenever you load a preset
	*/

	// show a message
	if (!m_bShowRating)
	{
		// see also: DrawText() in milkdropfs.cpp
		m_fShowRatingUntilThisTime = GetTime() + 2.0f;
	}
}



void CPlugin::ReadCustomMessages()
{
#if 0

	int n;

	// First, clear all old data
	for (n=0; n<MAX_CUSTOM_MESSAGE_FONTS; n++)
	{
		strcpy(m_CustomMessageFont[n].szFace, "arial");
		m_CustomMessageFont[n].bBold = false;
		m_CustomMessageFont[n].bItal = false;
		m_CustomMessageFont[n].nColorR = 255;
		m_CustomMessageFont[n].nColorG = 255;
		m_CustomMessageFont[n].nColorB = 255;
	}

	for (n=0; n<MAX_CUSTOM_MESSAGES; n++)
	{
		m_CustomMessage[n].szText[0] = 0;
		m_CustomMessage[n].nFont = 0;
		m_CustomMessage[n].fSize = 50.0f;  // [0..100]  note that size is not absolute, but relative to the size of the window
		m_CustomMessage[n].x = 0.5f;
		m_CustomMessage[n].y = 0.5f;
		m_CustomMessage[n].randx = 0;
		m_CustomMessage[n].randy = 0;
		m_CustomMessage[n].growth = 1.0f;
		m_CustomMessage[n].fTime = 1.5f;
		m_CustomMessage[n].fFade = 0.2f;

		m_CustomMessage[n].bOverrideBold = false;
		m_CustomMessage[n].bOverrideItal = false;
		m_CustomMessage[n].bOverrideFace = false;
		m_CustomMessage[n].bOverrideColorR = false;
		m_CustomMessage[n].bOverrideColorG = false;
		m_CustomMessage[n].bOverrideColorB = false;
		m_CustomMessage[n].bBold = false;
		m_CustomMessage[n].bItal = false;
		strcpy(m_CustomMessage[n].szFace, "arial");
		m_CustomMessage[n].nColorR = 255;
		m_CustomMessage[n].nColorG = 255;
		m_CustomMessage[n].nColorB = 255;
		m_CustomMessage[n].nRandR = 0;
		m_CustomMessage[n].nRandG = 0;
		m_CustomMessage[n].nRandB = 0;
	}

	// Then read in the new file
	for (n=0; n<MAX_CUSTOM_MESSAGE_FONTS; n++)
	{
		char szSectionName[32];
		sprintf(szSectionName, "font%02d", n);

		// get face, bold, italic, x, y for this custom message FONT
		InternalGetPrivateProfileString(szSectionName,"face","arial",m_CustomMessageFont[n].szFace,sizeof(m_CustomMessageFont[n].szFace), m_szMsgIniFile);
		m_CustomMessageFont[n].bBold	= GetPrivateProfileBool(szSectionName,"bold",m_CustomMessageFont[n].bBold,  m_szMsgIniFile);
		m_CustomMessageFont[n].bItal	= GetPrivateProfileBool(szSectionName,"ital",m_CustomMessageFont[n].bItal,  m_szMsgIniFile);
		m_CustomMessageFont[n].nColorR  = InternalGetPrivateProfileInt (szSectionName,"r"     ,m_CustomMessageFont[n].nColorR, m_szMsgIniFile);
		m_CustomMessageFont[n].nColorG  = InternalGetPrivateProfileInt (szSectionName,"g"     ,m_CustomMessageFont[n].nColorG, m_szMsgIniFile);
		m_CustomMessageFont[n].nColorB  = InternalGetPrivateProfileInt (szSectionName,"b"     ,m_CustomMessageFont[n].nColorB, m_szMsgIniFile);
	}

	for (n=0; n<MAX_CUSTOM_MESSAGES; n++)
	{
		char szSectionName[64];
		sprintf(szSectionName, "message%02d", n);

		// get fontID, size, text, etc. for this custom message:
		InternalGetPrivateProfileString(szSectionName,"text","",m_CustomMessage[n].szText,sizeof(m_CustomMessage[n].szText), m_szMsgIniFile);
        if (m_CustomMessage[n].szText[0])
        {
		    m_CustomMessage[n].nFont	= InternalGetPrivateProfileInt  (szSectionName,"font"  ,m_CustomMessage[n].nFont,   m_szMsgIniFile);
		    m_CustomMessage[n].fSize	= InternalGetPrivateProfileFloat(szSectionName,"size"  ,m_CustomMessage[n].fSize,   m_szMsgIniFile);
		    m_CustomMessage[n].x		= InternalGetPrivateProfileFloat(szSectionName,"x"     ,m_CustomMessage[n].x,       m_szMsgIniFile);
		    m_CustomMessage[n].y		= InternalGetPrivateProfileFloat(szSectionName,"y"     ,m_CustomMessage[n].y,       m_szMsgIniFile);
		    m_CustomMessage[n].randx    = InternalGetPrivateProfileFloat(szSectionName,"randx" ,m_CustomMessage[n].randx,   m_szMsgIniFile);
		    m_CustomMessage[n].randy    = InternalGetPrivateProfileFloat(szSectionName,"randy" ,m_CustomMessage[n].randy,   m_szMsgIniFile);

		    m_CustomMessage[n].growth   = InternalGetPrivateProfileFloat(szSectionName,"growth",m_CustomMessage[n].growth,  m_szMsgIniFile);
		    m_CustomMessage[n].fTime    = InternalGetPrivateProfileFloat(szSectionName,"time"  ,m_CustomMessage[n].fTime,   m_szMsgIniFile);
		    m_CustomMessage[n].fFade    = InternalGetPrivateProfileFloat(szSectionName,"fade"  ,m_CustomMessage[n].fFade,   m_szMsgIniFile);
		    m_CustomMessage[n].nColorR  = InternalGetPrivateProfileInt  (szSectionName,"r"     ,m_CustomMessage[n].nColorR, m_szMsgIniFile);
		    m_CustomMessage[n].nColorG  = InternalGetPrivateProfileInt  (szSectionName,"g"     ,m_CustomMessage[n].nColorG, m_szMsgIniFile);
		    m_CustomMessage[n].nColorB  = InternalGetPrivateProfileInt  (szSectionName,"b"     ,m_CustomMessage[n].nColorB, m_szMsgIniFile);
		    m_CustomMessage[n].nRandR   = InternalGetPrivateProfileInt  (szSectionName,"randr" ,m_CustomMessage[n].nRandR,  m_szMsgIniFile);
		    m_CustomMessage[n].nRandG   = InternalGetPrivateProfileInt  (szSectionName,"randg" ,m_CustomMessage[n].nRandG,  m_szMsgIniFile);
		    m_CustomMessage[n].nRandB   = InternalGetPrivateProfileInt  (szSectionName,"randb" ,m_CustomMessage[n].nRandB,  m_szMsgIniFile);

		    // overrides: r,g,b,face,bold,ital
		    InternalGetPrivateProfileString(szSectionName,"face","",m_CustomMessage[n].szFace,sizeof(m_CustomMessage[n].szFace), m_szMsgIniFile);
		    m_CustomMessage[n].bBold	= InternalGetPrivateProfileInt (szSectionName, "bold", -1, m_szMsgIniFile);
		    m_CustomMessage[n].bItal  	= InternalGetPrivateProfileInt (szSectionName, "ital", -1, m_szMsgIniFile);
		    m_CustomMessage[n].nColorR  = InternalGetPrivateProfileInt (szSectionName, "r"   , -1, m_szMsgIniFile);
		    m_CustomMessage[n].nColorG  = InternalGetPrivateProfileInt (szSectionName, "g"   , -1, m_szMsgIniFile);
		    m_CustomMessage[n].nColorB  = InternalGetPrivateProfileInt (szSectionName, "b"   , -1, m_szMsgIniFile);

		    m_CustomMessage[n].bOverrideFace   = (m_CustomMessage[n].szFace[0] != 0);
		    m_CustomMessage[n].bOverrideBold   = (m_CustomMessage[n].bBold != -1);
		    m_CustomMessage[n].bOverrideItal   = (m_CustomMessage[n].bItal != -1);
		    m_CustomMessage[n].bOverrideColorR = (m_CustomMessage[n].nColorR != -1);
		    m_CustomMessage[n].bOverrideColorG = (m_CustomMessage[n].nColorG != -1);
		    m_CustomMessage[n].bOverrideColorB = (m_CustomMessage[n].nColorB != -1);
        }
	}
#endif

}

void CPlugin::LaunchCustomMessage(int nMsgNum)
{
	if (nMsgNum > 99)
		nMsgNum = 99;

	if (nMsgNum < 0)
	{
		int count=0;
		// choose randomly
		for (nMsgNum=0; nMsgNum<100; nMsgNum++)
			if (m_CustomMessage[nMsgNum].szText[0])
				count++;

		int sel = (rand()%count)+1;
		count = 0;
		for (nMsgNum=0; nMsgNum<100; nMsgNum++)
		{
			if (m_CustomMessage[nMsgNum].szText[0])
				count++;
			if (count==sel)
				break;
		}
	}

	
	if (nMsgNum < 0 || 
		nMsgNum >= MAX_CUSTOM_MESSAGES || 
		m_CustomMessage[nMsgNum].szText[0]==0)
	{
		return;
	}

	int fontID = m_CustomMessage[nMsgNum].nFont;

	m_supertext.bRedrawSuperText = true;
	m_supertext.bIsSongTitle = false;
	strcpy(m_supertext.szText, m_CustomMessage[nMsgNum].szText);

	// regular properties:
	m_supertext.fFontSize   = m_CustomMessage[nMsgNum].fSize;
	m_supertext.fX          = m_CustomMessage[nMsgNum].x + m_CustomMessage[nMsgNum].randx * ((rand()%1037)/1037.0f*2.0f - 1.0f);
	m_supertext.fY          = m_CustomMessage[nMsgNum].y + m_CustomMessage[nMsgNum].randy * ((rand()%1037)/1037.0f*2.0f - 1.0f);
	m_supertext.fGrowth     = m_CustomMessage[nMsgNum].growth;
	m_supertext.fDuration   = m_CustomMessage[nMsgNum].fTime;
	m_supertext.fFadeTime   = m_CustomMessage[nMsgNum].fFade;

	// overrideables:
	if (m_CustomMessage[nMsgNum].bOverrideFace)
		strcpy(m_supertext.nFontFace, m_CustomMessage[nMsgNum].szFace);
	else
		strcpy(m_supertext.nFontFace, m_CustomMessageFont[fontID].szFace);
	m_supertext.bItal   = (m_CustomMessage[nMsgNum].bOverrideItal) ? (m_CustomMessage[nMsgNum].bItal != 0) : (m_CustomMessageFont[fontID].bItal != 0);
	m_supertext.bBold   = (m_CustomMessage[nMsgNum].bOverrideBold) ? (m_CustomMessage[nMsgNum].bBold != 0) : (m_CustomMessageFont[fontID].bBold != 0);
	m_supertext.nColorR = (m_CustomMessage[nMsgNum].bOverrideColorR) ? m_CustomMessage[nMsgNum].nColorR : m_CustomMessageFont[fontID].nColorR;
	m_supertext.nColorG = (m_CustomMessage[nMsgNum].bOverrideColorG) ? m_CustomMessage[nMsgNum].nColorG : m_CustomMessageFont[fontID].nColorG;
	m_supertext.nColorB = (m_CustomMessage[nMsgNum].bOverrideColorB) ? m_CustomMessage[nMsgNum].nColorB : m_CustomMessageFont[fontID].nColorB;

	// randomize color
	m_supertext.nColorR += (int)(m_CustomMessage[nMsgNum].nRandR * ((rand()%1037)/1037.0f*2.0f - 1.0f));
	m_supertext.nColorG += (int)(m_CustomMessage[nMsgNum].nRandG * ((rand()%1037)/1037.0f*2.0f - 1.0f));
	m_supertext.nColorB += (int)(m_CustomMessage[nMsgNum].nRandB * ((rand()%1037)/1037.0f*2.0f - 1.0f));
	if (m_supertext.nColorR < 0) m_supertext.nColorR = 0;
	if (m_supertext.nColorG < 0) m_supertext.nColorG = 0;
	if (m_supertext.nColorB < 0) m_supertext.nColorB = 0;
	if (m_supertext.nColorR > 255) m_supertext.nColorR = 255;
	if (m_supertext.nColorG > 255) m_supertext.nColorG = 255;
	if (m_supertext.nColorB > 255) m_supertext.nColorB = 255;

	// fix &'s for display:
	/*
	{	
		int pos = 0;
		int len = strlen(m_supertext.szText);
		while (m_supertext.szText[pos] && pos<255)
		{
			if (m_supertext.szText[pos] == '&')
			{
				for (int x=len; x>=pos; x--)
					m_supertext.szText[x+1] = m_supertext.szText[x];
				len++;
				pos++;
			}
			pos++;
		}
	}*/

	m_supertext.fStartTime = GetTime(); 
}

void CPlugin::LaunchSongTitleAnim()
{
	m_supertext.bRedrawSuperText = true;
	m_supertext.bIsSongTitle = true;
	strcpy(m_supertext.szText, m_szSongTitle);
	strcpy(m_supertext.nFontFace, m_fontinfo[SONGTITLE_FONT].szFace);
	m_supertext.fFontSize   = (float)m_fontinfo[SONGTITLE_FONT].nSize;
	m_supertext.bBold       = m_fontinfo[SONGTITLE_FONT].bBold;
	m_supertext.bItal       = m_fontinfo[SONGTITLE_FONT].bItalic;
	m_supertext.fX          = 0.5f;
	m_supertext.fY          = 0.5f;
	m_supertext.fGrowth     = 1.0f;
	m_supertext.fDuration   = m_fSongTitleAnimDuration;
	m_supertext.nColorR     = 255;
	m_supertext.nColorG     = 255;
	m_supertext.nColorB     = 255;

	m_supertext.fStartTime = GetTime(); 
}

bool CPlugin::LaunchSprite(int nSpriteNum, int nSlot)
{
#if 0
	char initcode[8192], code[8192], img[512], section[64];
	char szTemp[8192];

	initcode[0] = 0;
	code[0] = 0;
	img[0] = 0;
	sprintf(section, "img%02d", nSpriteNum);

	// 1. read in image filename
	InternalGetPrivateProfileString(section, "img", "", img, sizeof(img)-1, m_szImgIniFile);
	if (img[0] == 0)
	{
		sprintf(m_szUserMessage, "sprite #%d error: couldn't find 'img=' setting, or sprite is not defined", nSpriteNum); 
		m_fShowUserMessageUntilThisTime = GetTime() + 7.0f;
		return false;
	}
	
	if (img[1] != ':' || img[2] != '\\')
	{
		// it's not in the form "x:\blah\billy.jpg" so prepend plugin dir path.
		char temp[512];
		strcpy(temp, img);
		sprintf(img, "%s%s", m_szWinampPluginsPath, temp);
	}

	// 2. get color key
	//unsigned int ck_lo = (unsigned int)InternalGetPrivateProfileInt(section, "colorkey_lo", 0x00000000, m_szImgIniFile);
	//unsigned int ck_hi = (unsigned int)InternalGetPrivateProfileInt(section, "colorkey_hi", 0x00202020, m_szImgIniFile);
    // FIRST try 'colorkey_lo' (for backwards compatibility) and then try 'colorkey'
    unsigned int ck = (unsigned int)InternalGetPrivateProfileInt(section, "colorkey_lo", 0x00000000, m_szImgIniFile);
    ck = (unsigned int)InternalGetPrivateProfileInt(section, "colorkey", ck, m_szImgIniFile);

	// 3. read in init code & per-frame code
	for (int n=0; n<2; n++)
	{
		char *pStr = (n==0) ? initcode : code;
		char szLineName[32];
		int len;

		int line = 1;
		int char_pos = 0;
		bool bDone = false;
		
		while (!bDone)
		{
			if (n==0)
				sprintf(szLineName, "init_%d", line);
			else
				sprintf(szLineName, "code_%d", line);


			InternalGetPrivateProfileString(section, szLineName, "~!@#$", szTemp, 8192, m_szImgIniFile);	// fixme
			len = strlen(szTemp);

			if ((strcmp(szTemp, "~!@#$")==0) ||		// if the key was missing,
				(len >= 8191-char_pos-1))			// or if we're out of space
			{
				bDone = true;
			}
			else 
			{
				sprintf(&pStr[char_pos], "%s%c", szTemp, LINEFEED_CONTROL_CHAR);
			}
		
			char_pos += len + 1;
			line++;
		}
		pStr[char_pos++] = 0;	// null-terminate
	}

	if (nSlot == -1)
	{
		// find first empty slot; if none, chuck the oldest sprite & take its slot.
		int oldest_index = 0;
		int oldest_frame = m_texmgr.m_tex[0].nStartFrame;
		for (int x=0; x<NUM_TEX; x++)
		{
			if (!m_texmgr.m_tex[x].pSurface)
			{
				nSlot = x;
				break;
			}
			else if (m_texmgr.m_tex[x].nStartFrame < oldest_frame)
			{
				oldest_index = x;
				oldest_frame = m_texmgr.m_tex[x].nStartFrame;
			}
		}

		if (nSlot == -1)
		{
			nSlot = oldest_index;
			m_texmgr.KillTex(nSlot);
		}
	}

	int ret = m_texmgr.LoadTex(img, nSlot, initcode, code, GetTime(), GetFrame(), ck);
	m_texmgr.m_tex[nSlot].nUserData = nSpriteNum;

	m_fShowUserMessageUntilThisTime = GetTime() + 6.0f;

	switch(ret & TEXMGR_ERROR_MASK)
	{
	case TEXMGR_ERR_SUCCESS:
		switch(ret & TEXMGR_WARNING_MASK)
		{
		case TEXMGR_WARN_ERROR_IN_INIT_CODE: sprintf(m_szUserMessage, "sprite #%d warning: error in initialization code ", nSpriteNum); break;
		case TEXMGR_WARN_ERROR_IN_REG_CODE:  sprintf(m_szUserMessage, "sprite #%d warning: error in per-frame code", nSpriteNum); break;
		default: 
			// success; no errors OR warnings.
			m_fShowUserMessageUntilThisTime = GetTime() - 1.0f; 
			break;
		}
		break;
	case TEXMGR_ERR_BAD_INDEX:              sprintf(m_szUserMessage, "sprite #%d error: bad slot index", nSpriteNum); break;
	/*
    case TEXMGR_ERR_OPENING:                sprintf(m_szUserMessage, "sprite #%d error: unable to open imagefile", nSpriteNum); break;
	case TEXMGR_ERR_FORMAT:                 sprintf(m_szUserMessage, "sprite #%d error: file is corrupt or non-jpeg image", nSpriteNum); break;
	case TEXMGR_ERR_IMAGE_NOT_24_BIT:       sprintf(m_szUserMessage, "sprite #%d error: image does not have 3 color channels", nSpriteNum); break;
	case TEXMGR_ERR_IMAGE_TOO_LARGE:        sprintf(m_szUserMessage, "sprite #%d error: image is too large", nSpriteNum); break;
	case TEXMGR_ERR_CREATESURFACE_FAILED:   sprintf(m_szUserMessage, "sprite #%d error: createsurface() failed", nSpriteNum); break;
	case TEXMGR_ERR_LOCKSURFACE_FAILED:     sprintf(m_szUserMessage, "sprite #%d error: lock() failed", nSpriteNum); break;
	case TEXMGR_ERR_CORRUPT_JPEG:           sprintf(m_szUserMessage, "sprite #%d error: jpeg is corrupt", nSpriteNum); break;
    */
    case TEXMGR_ERR_BADFILE:                sprintf(m_szUserMessage, "sprite #%d error: image file missing or corrupt", nSpriteNum); break;
    case TEXMGR_ERR_OUTOFMEM:               sprintf(m_szUserMessage, "sprite #%d error: out of memory, unable to load image", nSpriteNum); break;
	}
	return (ret & TEXMGR_ERROR_MASK) ? false : true;
#endif
	return false;
}

void CPlugin::KillSprite(int iSlot)
{
//	m_texmgr.KillTex(iSlot);
}

void CPlugin::DoCustomSoundAnalysis()
{
    memcpy(mysound.fWave[0], m_sound.fWaveform[0], sizeof(float)*576);
    memcpy(mysound.fWave[1], m_sound.fWaveform[1], sizeof(float)*576);

    // do our own [UN-NORMALIZED] fft
	float fWaveLeft[576];
	for (int i=0; i<576; i++) 
        fWaveLeft[i] = m_sound.fWaveform[0][i];

	memset(mysound.fSpecLeft, 0, sizeof(float)*MY_FFT_SAMPLES);

	myfft.time_to_frequency_domain(fWaveLeft, mysound.fSpecLeft);
	//for (i=0; i<MY_FFT_SAMPLES; i++) fSpecLeft[i] = sqrtf(fSpecLeft[i]*fSpecLeft[i] + fSpecTemp[i]*fSpecTemp[i]);

	// sum spectrum up into 3 bands
	for (int i=0; i<3; i++)
	{
		// note: only look at bottom half of spectrum!  (hence divide by 6 instead of 3)
		int start = MY_FFT_SAMPLES*i/6;
		int end   = MY_FFT_SAMPLES*(i+1)/6;
		int j;

		mysound.imm[i] = 0;

		for (j=start; j<end; j++)
			mysound.imm[i] += mysound.fSpecLeft[j];
	}

	// do temporal blending to create attenuated and super-attenuated versions
	for (int i=0; i<3; i++)
	{
        float rate;

		if (mysound.imm[i] > mysound.avg[i])
			rate = 0.2f;
		else
			rate = 0.5f;
        rate = AdjustRateToFPS(rate, 30.0f, GetFps());
        mysound.avg[i] = mysound.avg[i]*rate + mysound.imm[i]*(1-rate);

		if (GetFrame() < 50)
			rate = 0.9f;
		else
			rate = 0.992f;
        rate = AdjustRateToFPS(rate, 30.0f, GetFps());
        mysound.long_avg[i] = mysound.long_avg[i]*rate + mysound.imm[i]*(1-rate);


		// also get bass/mid/treble levels *relative to the past*
		if (fabsf(mysound.long_avg[i]) < 0.001f)
			mysound.imm_rel[i] = 1.0f;
		else
			mysound.imm_rel[i]  = mysound.imm[i] / mysound.long_avg[i];

		if (fabsf(mysound.long_avg[i]) < 0.001f)
			mysound.avg_rel[i]  = 1.0f;
		else
			mysound.avg_rel[i]  = mysound.avg[i] / mysound.long_avg[i];
	}
}
