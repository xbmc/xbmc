#pragma once

class CDVDStateSerializer
{ 
public:
  static bool DVDToXMLState( std::string &xmlstate, const dvd_state_t *state );
  static bool XMLToDVDState( dvd_state_t *state, const std::string &xmlstate );

  static bool test( const dvd_state_t *state );
};

