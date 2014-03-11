//
//  PlexTranscoderClientRPi.h
//  RasPlex
//
//  Created by Lionel Chazallon on 2014-03-07.
//
//

#ifndef PLEXTRANSCODERCLIENTRPI_H
#define PLEXTRANSCODERCLIENTRPI_H

#include <string>
#include "Client/PlexTranscoderClient.h"

class CPlexTranscoderClientRPi : public CPlexTranscoderClient
{
  private:
    int m_maxVideoBitrate;
    int m_maxAudioBitrate;

    std::set<std::string> m_knownVideoCodecs;
    std::set<std::string> m_knownAudioCodecs;

  public:
    CPlexTranscoderClientRPi();
    virtual bool ShouldTranscode(CPlexServerPtr server, const CFileItem &item);
    virtual std::string GetCurrentBitrate(bool local);
    bool CheckCodec(std::string codec);
};

#endif // PLEXTRANSCODERCLIENTRPI_H
