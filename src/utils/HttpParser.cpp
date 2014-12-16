/*
 * This code implements parsing of HTTP requests.
 * This code was written by Steve Hanov in 2009, no copyright is claimed.
 * This code is in the public domain.
 * Code was taken from http://refactormycode.com/codes/778-an-efficient-http-parser
 *
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "HttpParser.h"

HttpParser::HttpParser() :
    _headerStart(0),
    _bodyStart(0),
    _parsedTo( 0 ),
    _state( 0 ),
    _keyIndex(0),
    _valueIndex(0),
    _contentLength(0),
    _contentStart(0),
    _uriIndex(0),
    _status( Incomplete )
{

}

HttpParser::~HttpParser()
{

}

void
HttpParser::parseHeader()
{
    // run the fsm.
    const int  CR = 13;
    const int  LF = 10;
    const int  ANY = 256;

    enum Action {
        // make lower case
        LOWER = 0x1,

        // convert current character to null.
        NULLIFY = 0x2,

        // set the header index to the current position
        SET_HEADER_START = 0x4,

        // set the key index to the current position
        SET_KEY = 0x8,

        // set value index to the current position.
        SET_VALUE = 0x10,

        // store current key/value pair.
        STORE_KEY_VALUE = 0x20,

        // sets content start to current position + 1
        SET_CONTENT_START = 0x40
    };

    static const struct FSM {
        State curState;
        int c;
        State nextState;
        unsigned actions;
    } fsm[] = {
        { p_request_line,         CR, p_request_line_cr,     NULLIFY                            },
        { p_request_line,        ANY, p_request_line,        0                                  },
        { p_request_line_cr,      LF, p_request_line_crlf,   0                                  },
        { p_request_line_crlf,    CR, p_request_line_crlfcr, 0                                  },
        { p_request_line_crlf,   ANY, p_key,                 SET_HEADER_START | SET_KEY | LOWER },
        { p_request_line_crlfcr,  LF, p_content,             SET_CONTENT_START                  },
        { p_key,                 ':', p_key_colon,           NULLIFY                            },
        { p_key,                 ANY, p_key,                 LOWER                              },
        { p_key_colon,           ' ', p_key_colon_sp,        0                                  },
        { p_key_colon_sp,        ANY, p_value,               SET_VALUE                          },
        { p_value,                CR, p_value_cr,            NULLIFY | STORE_KEY_VALUE          },
        { p_value,               ANY, p_value,               0                                  },
        { p_value_cr,             LF, p_value_crlf,          0                                  },
        { p_value_crlf,           CR, p_value_crlfcr,        0                                  },
        { p_value_crlf,          ANY, p_key,                 SET_KEY | LOWER                    },
        { p_value_crlfcr,         LF, p_content,             SET_CONTENT_START                  },
        { p_error,               ANY, p_error,               0                                  }
    };

    for( unsigned i = _parsedTo; i < _data.length(); ++i) {
        char c = _data[i];
        State nextState = p_error;

        for ( unsigned d = 0; d < sizeof(fsm) / sizeof(FSM); ++d ) {
            if ( fsm[d].curState == _state && 
                    ( c == fsm[d].c || fsm[d].c == ANY ) ) {

                nextState = fsm[d].nextState;

                if ( fsm[d].actions & LOWER ) {
                    _data[i] = tolower( _data[i] );
                }

                if ( fsm[d].actions & NULLIFY ) {
                    _data[i] = 0;
                }

                if ( fsm[d].actions & SET_HEADER_START ) {
                    _headerStart = i;
                }

                if ( fsm[d].actions & SET_KEY ) {
                    _keyIndex = i;
                }

                if ( fsm[d].actions & SET_VALUE ) {
                    _valueIndex = i;
                }

                if ( fsm[d].actions & SET_CONTENT_START ) {
                    _contentStart = i + 1;
                }

                if ( fsm[d].actions & STORE_KEY_VALUE ) {
                    // store position of first character of key.
                    _keys.push_back( _keyIndex );
                }

                break;
            }
        }

        _state = nextState;

        if ( _state == p_content ) {
            const char* str = getValue("content-length");
            if ( str ) {
                _contentLength = atoi( str );
            }
            break;
        }
    }

    _parsedTo = _data.length();

}

bool
HttpParser::parseRequestLine()
{
    size_t sp1;
    size_t sp2;

    sp1 = _data.find( ' ', 0 );
    if ( sp1 == std::string::npos ) return false;
    sp2 = _data.find( ' ', sp1 + 1 );
    if ( sp2 == std::string::npos ) return false;

    _data[sp1] = 0;
    _data[sp2] = 0;
    _uriIndex = sp1 + 1;
    return true;
}

HttpParser::status_t
HttpParser::addBytes( const char* bytes, unsigned len )
{
    if ( _status != Incomplete ) {
        return _status;
    }

    // append the bytes to data.
    _data.append( bytes, len );

    if ( _state < p_content ) {
        parseHeader();
    }

    if ( _state == p_error ) {
        _status = Error;
    } else if ( _state == p_content ) {
        if ( _contentLength == 0 || _data.length() - _contentStart >= _contentLength ) {
            if ( parseRequestLine() ) {
                _status = Done;
            } else {
                _status = Error;
            }
        }
    }

    return _status;
}

const char*
HttpParser::getMethod() const
{
    return &_data[0];
}

const char*
HttpParser::getUri() const
{
    return &_data[_uriIndex];
}

const char*
HttpParser::getQueryString() const
{
    const char* pos = getUri();
    while( *pos ) {
        if ( *pos == '?' ) {
            pos++;
            break;
        }
        pos++;
    }
    return pos;
}

const char* 
HttpParser::getBody() const
{
    if ( _contentLength > 0 ) {
        return &_data[_contentStart];
    } else  {
        return NULL;
    }
}

// key should be in lower case.
const char* 
HttpParser::getValue( const char* key ) const
{
    for( IntArray::const_iterator iter = _keys.begin();
            iter != _keys.end(); ++iter  )
    {
        unsigned index = *iter;
        if ( strcmp( &_data[index], key ) == 0 ) {
            return &_data[index + strlen(key) + 2];
        }

    }

    return NULL;
}

unsigned
HttpParser::getContentLength() const
{
    return _contentLength;
}

