/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#ifdef _WIN32
#include "system.h"
#endif

#include "DllDvdNav.h"
#include "utils/XBMCTinyXML.h"
#include "DVDStateSerializer.h"
#include <sstream>

bool CDVDStateSerializer::test( const dvd_state_t *state  )
{
  dvd_state_t state2;
  std::string buffer;

  memset( &state2, 0, sizeof(dvd_state_t));

  DVDToXMLState(buffer, state);

  XMLToDVDState( &state2, buffer);

  return memcmp( &state2, state, sizeof( dvd_state_t )) == 0;

}

bool CDVDStateSerializer::DVDToXMLState( std::string &xmlstate, const dvd_state_t *state )
{
  char buffer[256];
  CXBMCTinyXML xmlDoc("navstate");

  TiXmlElement eRoot("navstate");
  eRoot.SetAttribute("version", 1);

	
  { TiXmlElement eRegisters("registers");

    for( int i = 0; i < 24; i++ )
    {

      if( state->registers.SPRM[i] )
      { TiXmlElement eReg("sprm");
        eReg.SetAttribute("index", i);

        { TiXmlElement eValue("value");
          sprintf(buffer, "0x%hx", state->registers.SPRM[i]);
          eValue.InsertEndChild( TiXmlText(buffer) );
          eReg.InsertEndChild(eValue);
        }

        eRegisters.InsertEndChild(eReg);
      }
    }

    for( int i = 0; i < 16; i++ )
    {
      if( state->registers.GPRM[i] || state->registers.GPRM_mode[i] || state->registers.GPRM_time[i].tv_sec || state->registers.GPRM_time[i].tv_usec )
      { TiXmlElement eReg("gprm");
        eReg.SetAttribute("index", i);

        { TiXmlElement eValue("value");
          sprintf(buffer, "0x%hx", state->registers.GPRM[i]);
          eValue.InsertEndChild( TiXmlText(buffer) );
          eReg.InsertEndChild(eValue);
        }

        { TiXmlElement eMode("mode");
          sprintf(buffer, "0x%c", state->registers.GPRM_mode[i]);
          eMode.InsertEndChild( TiXmlText(buffer) );
          eReg.InsertEndChild(eMode);
        }

        { TiXmlElement eTime("time");
          { TiXmlElement eValue("tv_sec");
            sprintf(buffer, "%ld", state->registers.GPRM_time[i].tv_sec);
            eValue.InsertEndChild( TiXmlText( buffer ) );
            eTime.InsertEndChild( eValue ) ;
          }

          { TiXmlElement eValue("tv_usec");
            sprintf(buffer, "%ld", (long int)state->registers.GPRM_time[i].tv_usec);
            eValue.InsertEndChild( TiXmlText( buffer ) );
            eTime.InsertEndChild( eValue ) ;
          }
          eReg.InsertEndChild(eTime);
        }
        eRegisters.InsertEndChild(eReg);
      }
    }
    eRoot.InsertEndChild(eRegisters);
  }

  { TiXmlElement element("domain");
    sprintf(buffer, "%d", state->domain);
    element.InsertEndChild( TiXmlText( buffer ) );
    eRoot.InsertEndChild(element);
  }

  { TiXmlElement element("vtsn");
    sprintf(buffer, "%d", state->vtsN);
    element.InsertEndChild( TiXmlText( buffer ) );
    eRoot.InsertEndChild(element);
  }

  { TiXmlElement element("pgcn");
    sprintf(buffer, "%d", state->pgcN);
    element.InsertEndChild( TiXmlText( buffer ) );
    eRoot.InsertEndChild(element);
  }

  { TiXmlElement element("pgn");
    sprintf(buffer, "%d", state->pgN);
    element.InsertEndChild( TiXmlText( buffer ) );
    eRoot.InsertEndChild(element);
  }

  { TiXmlElement element("celln");
    sprintf(buffer, "%d", state->cellN);
    element.InsertEndChild( TiXmlText( buffer ) );
    eRoot.InsertEndChild(element);
  }

  { TiXmlElement element("cell_restart");
    sprintf(buffer, "%d", state->cell_restart);
    element.InsertEndChild( TiXmlText( buffer ) );
    eRoot.InsertEndChild(element);
  }

  { TiXmlElement element("blockn");
    sprintf(buffer, "%d", state->blockN);
    element.InsertEndChild( TiXmlText( buffer ) );
    eRoot.InsertEndChild(element);
  }

  { TiXmlElement rsm("rsm");

    { TiXmlElement element("vtsn");
      sprintf(buffer, "%d", state->rsm_vtsN);
      element.InsertEndChild( TiXmlText( buffer ) );
      rsm.InsertEndChild(element);
    }

    { TiXmlElement element("blockn");
      sprintf(buffer, "%d", state->rsm_blockN);
      element.InsertEndChild( TiXmlText( buffer ) );
      rsm.InsertEndChild(element);
    }

    { TiXmlElement element("pgcn");
      sprintf(buffer, "%d", state->rsm_pgcN);
      element.InsertEndChild( TiXmlText( buffer ) );
      rsm.InsertEndChild(element);
    }

    { TiXmlElement element("celln");
      sprintf(buffer, "%d", state->rsm_cellN);
      element.InsertEndChild( TiXmlText( buffer ) );
      rsm.InsertEndChild(element);
    }

    { TiXmlElement regs("registers");

      for( int i = 0; i < 5; i++ )
      {
        TiXmlElement reg("sprm");
        reg.SetAttribute("index", i);

        { TiXmlElement element("value");
          sprintf(buffer, "0x%hx", state->rsm_regs[i]);
          element.InsertEndChild( TiXmlText(buffer) );
          reg.InsertEndChild(element);
        }

        regs.InsertEndChild(reg);
      }
      rsm.InsertEndChild(regs);
    }
    eRoot.InsertEndChild(rsm);
  }


  xmlDoc.InsertEndChild(eRoot);

  std::stringstream stream;
  stream << xmlDoc;
  xmlstate = stream.str();
  return true;
}

bool CDVDStateSerializer::XMLToDVDState( dvd_state_t *state, const std::string &xmlstate )
{
  CXBMCTinyXML xmlDoc;

  xmlDoc.Parse(xmlstate.c_str());

  if( xmlDoc.Error() )
    return false;

  TiXmlHandle hRoot( xmlDoc.RootElement() );
  if( strcmp( hRoot.Element()->Value(), "navstate" ) != 0 ) return false;

  TiXmlElement *element = NULL;
  TiXmlText *text = NULL;
  int index = 0;

  element = hRoot.FirstChildElement("registers").FirstChildElement("sprm").Element();
  while( element )
  {
    element->Attribute("index", &index);

    text = TiXmlHandle( element ).FirstChildElement("value").FirstChild().Text();
    if( text && index >= 0 && index < 24 )
      sscanf(text->Value(), "0x%hx", &state->registers.SPRM[index]);

    element = element->NextSiblingElement("sprm");
  }

  element = hRoot.FirstChildElement("registers").FirstChildElement("gprm").Element();
  while( element )
  {
    element->Attribute("index", &index);
    if( index >= 0 && index < 16 )
    {
      text = TiXmlHandle( element ).FirstChildElement("value").FirstChild().Text();
      if( text )
        sscanf(text->Value(), "0x%hx", &state->registers.GPRM[index]);

      text = TiXmlHandle( element ).FirstChildElement("mode").FirstChild().Text();
      if( text )
        sscanf(text->Value(), "0x%c", &state->registers.GPRM_mode[index]);

      text = TiXmlHandle( element ).FirstChildElement("time").FirstChildElement("tv_sec").FirstChild().Text();
      if( text )
        sscanf(text->Value(), "%ld", &state->registers.GPRM_time[index].tv_sec);

      text = TiXmlHandle( element ).FirstChildElement("time").FirstChildElement("tv_usec").FirstChild().Text();
      if( text )
        sscanf(text->Value(), "%ld", (long int*)&state->registers.GPRM_time[index].tv_usec);
    }
    element = element->NextSiblingElement("gprm");
  }

  if( (text = hRoot.FirstChildElement("domain").FirstChild().Text()) )
    sscanf(text->Value(), "%d", (int*) &state->domain);

  if( (text = hRoot.FirstChildElement("vtsn").FirstChild().Text()) )
    sscanf(text->Value(), "%d", &state->vtsN);

  if( (text = hRoot.FirstChildElement("pgcn").FirstChild().Text()) )
    sscanf(text->Value(), "%d", &state->pgcN);

  if( (text = hRoot.FirstChildElement("pgn").FirstChild().Text()) )
    sscanf(text->Value(), "%d", &state->pgN);

  if( (text = hRoot.FirstChildElement("celln").FirstChild().Text()) )
    sscanf(text->Value(), "%d", &state->cellN);

  if( (text = hRoot.FirstChildElement("cell_restart").FirstChild().Text()) )
    sscanf(text->Value(), "%d", &state->cell_restart);

  if( (text = hRoot.FirstChildElement("blockn").FirstChild().Text()) )
    sscanf(text->Value(), "%d", &state->blockN);

  { TiXmlHandle hrsm = hRoot.FirstChildElement("rsm");

    if( (text = hrsm.FirstChildElement("vtsn").FirstChild().Text()) )
      sscanf(text->Value(), "%d", &state->rsm_vtsN);

    if( (text = hrsm.FirstChildElement("blockn").FirstChild().Text()) )
      sscanf(text->Value(), "%d", &state->rsm_blockN);

    if( (text = hrsm.FirstChildElement("pgcn").FirstChild().Text()) )
      sscanf(text->Value(), "%d", &state->rsm_pgcN);

    if( (text = hrsm.FirstChildElement("celln").FirstChild().Text()) )
      sscanf(text->Value(), "%d", &state->rsm_cellN);

    element = hrsm.FirstChildElement("registers").FirstChildElement("sprm").Element();
    while( element )
    {
      element->Attribute("index", &index);
      text = TiXmlHandle(element).FirstChildElement("value").FirstChild().Text();
      if( text && index >= 0 && index < 5 )
        sscanf(text->Value(), "0x%hx", &state->rsm_regs[index]);

      element = element->NextSiblingElement("sprm");
    }
  }
  return true;
}

