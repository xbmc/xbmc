/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */

#include <stdio.h>

#include "Common.hpp"
#include "fatal.h"
#include "KeyHandler.hpp"
#include "event.h"
#include "BeatDetect.hpp"
#include "PresetChooser.hpp"
#include "Renderer.hpp"
#include "projectM.hpp"

#include <iostream>
#include "TimeKeeper.hpp"


class Preset;
interface_t current_interface = DEFAULT_INTERFACE;

void refreshConsole() {

  switch (current_interface) {

  case MENU_INTERFACE:
    // unimplemented
    break;
  case SHELL_INTERFACE:
    // unimplemented
    break;
  case EDITOR_INTERFACE:
    // unimplemented
    break;
  case DEFAULT_INTERFACE:
    break;
  case BROWSER_INTERFACE:
    // unimplemented
    break;
  default:
    break;
  } 
 
}

DLLEXPORT void projectM::key_handler( projectMEvent event,
                            projectMKeycode keycode, projectMModifier modifier ) {

	switch( event ) {


	case PROJECTM_KEYDOWN:

	  //default_key_handler();
	  switch (current_interface)
	    {

	    case MENU_INTERFACE:
//	      menu_key_handler(this, event, keycode);
	      break;
	    case SHELL_INTERFACE:
	      //shell_key_handler();
	      break;
	    case EDITOR_INTERFACE:
//	      editor_key_handler(event,keycode);
	      break;
	    case BROWSER_INTERFACE:
//	      browser_key_handler(event,keycode,modifier);
	      break;
	    case DEFAULT_INTERFACE:
	      default_key_handler(event,keycode);
	      break;
	    default:
	      default_key_handler(event,keycode);
	      break;

	    }
	  break;
	default:
		break;

	}
}

void projectM::default_key_handler( projectMEvent event, projectMKeycode keycode) {

	switch( event ) {

	case PROJECTM_KEYDOWN:
	 
	  switch( keycode )
	    {
	    case PROJECTM_K_UP:
            beatDetect->beat_sensitivity += 0.25;
			if (beatDetect->beat_sensitivity > 5.0) beatDetect->beat_sensitivity = 5.0;
	      break;
	    case PROJECTM_K_DOWN:
            beatDetect->beat_sensitivity -= 0.25;
			if (beatDetect->beat_sensitivity < 0) beatDetect->beat_sensitivity = 0;
	      break;
		case PROJECTM_K_h:
 		  renderer->showhelp = !renderer->showhelp;
	      renderer->showstats= false;
	      renderer->showfps=false;
	    case PROJECTM_K_F1:
	      renderer->showhelp = !renderer->showhelp;
	      renderer->showstats=false;
	      renderer->showfps=false; 
	      break;
	    case PROJECTM_K_y:
		this->setShuffleEnabled(!this->isShuffleEnabled());
		 break;

	    case PROJECTM_K_F5:
	      if (!renderer->showhelp)
		      renderer->showfps = !renderer->showfps;
	      break;
	    case PROJECTM_K_F4:
		if (!renderer->showhelp)
	       		renderer->showstats = !renderer->showstats;
	      break;
	    case PROJECTM_K_F3: {
	      renderer->showpreset = !renderer->showpreset;
	      break;
	     }
	    case PROJECTM_K_F2:
	      renderer->showtitle = !renderer->showtitle;
	      break;
#ifndef MACOS
	    case PROJECTM_K_F9:
#else
        case PROJECTM_K_F8:
#endif
		
	      renderer->studio = !renderer->studio;
	      break;

	    case PROJECTM_K_ESCAPE: {
//	        exit( 1 );
	        break;
	      }
	    case PROJECTM_K_f:
	   
	      break; 
	    case PROJECTM_K_a:
		    renderer->correction = !renderer->correction;
	        break;
	    case PROJECTM_K_b:
	      break;
            case PROJECTM_K_n:
		m_presetChooser->nextPreset(*m_presetPos);
		presetSwitchedEvent(true, **m_presetPos);
		m_activePreset =  m_presetPos->allocate(this->presetInputs, this->presetOutputs);
		renderer->setPresetName(m_activePreset->presetName());
		timeKeeper->StartPreset();
	      break;

	    case PROJECTM_K_r:

		if (m_presetChooser->empty())
			break;

		*m_presetPos = m_presetChooser->weightedRandom();
		presetSwitchedEvent(true, **m_presetPos);
		m_activePreset = m_presetPos->allocate(this->presetInputs, this->presetOutputs);
			
		assert(m_activePreset.get());
			
		renderer->setPresetName(m_activePreset->presetName());
	       
		timeKeeper->StartPreset();
		break;
	    case PROJECTM_K_p:

		if (m_presetChooser->empty())
			break;

		// Case: idle preset currently running, selected last preset of chooser
		else if (*m_presetPos == m_presetChooser->end()) {
			--(*m_presetPos); 
		}

		else if (*m_presetPos != m_presetChooser->begin()) {
			--(*m_presetPos);			
		} 
		
		else {
		   *m_presetPos = m_presetChooser->end();
		   --(*m_presetPos);
		}

		m_activePreset =  m_presetPos->allocate(this->presetInputs, this->presetOutputs);
		renderer->setPresetName(m_activePreset->presetName());
               
	       	timeKeeper->StartPreset();
	      break;
	    case PROJECTM_K_l:
		renderer->noSwitch=!renderer->noSwitch;
	      break;
	    case PROJECTM_K_s:
            	renderer->studio = !renderer->studio;
	    case PROJECTM_K_i:
	        break;
	    case PROJECTM_K_z:
	      break;
	    case PROJECTM_K_0:
//	      nWaveMode=0;
	      break;
	    case PROJECTM_K_6:
//	      nWaveMode=6;
	      break;
	    case PROJECTM_K_7:
//	      nWaveMode=7;
	      break;
	    case PROJECTM_K_m:
	      break;	     
	    case PROJECTM_K_t:
	      break;
	    default:
	      break;
	    }
	default:
		break;

	}
}
