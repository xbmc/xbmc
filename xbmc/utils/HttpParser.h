/*
 * This code implements parsing of HTTP requests.
 * This code was written by Steve Hanov in 2009, no copyright is claimed.
 * This code is in the public domain.
 * Code was taken from http://refactormycode.com/codes/778-an-efficient-http-parser
 */

#ifndef HTTPPARSER_H_
#define HTTPPARSER_H_
#include <stdlib.h>
#include <vector>
#include <string>
#include <string.h>

// A class to incrementally parse an HTTP header as it comes in. It 
// lets you know when it has received all required bytes, as specified 
// by the content-length header (if present). If there is no content-length,
// it will stop reading after the final "\n\r".
//
// Example usage:
// 
//    HttpParser parser;
//    HttpParser::status_t status;
//
//    for( ;; ) {
//        // read bytes from socket into buffer, break on error
//        status = parser.addBytes( buffer, length );
//        if ( status != HttpParser::Incomplete ) break;
//    }
//
//    if ( status == HttpParser::Done ) {
//        // parse fully formed http message.
//    }


class HttpParser
{
public:
    HttpParser();
    ~HttpParser();

    enum status_t {
        Done,
        Error,
        Incomplete
    };

    status_t addBytes( const char* bytes, unsigned len );

    const char* getMethod();
    const char* getUri();
    const char* getQueryString();
    const char* getBody();
    // key should be in lower case when looking up.
    const char* getValue( const char* key );
    unsigned getContentLength();

private:
    void parseHeader();
    bool parseRequestLine();

    std::string _data;
    unsigned _headerStart;
    unsigned _bodyStart;
    unsigned _parsedTo;
    int _state;
    unsigned _keyIndex;
    unsigned _valueIndex;
    unsigned _contentLength;
    unsigned _contentStart;
    unsigned _uriIndex;
    
    typedef std::vector<unsigned> IntArray;
    IntArray _keys;

    enum State {
        p_request_line=0,
        p_request_line_cr=1,
        p_request_line_crlf=2,
        p_request_line_crlfcr=3,
        p_key=4,
        p_key_colon=5,
        p_key_colon_sp=6,
        p_value=7,
        p_value_cr=8,
        p_value_crlf=9,
        p_value_crlfcr=10,
        p_content=11, // here we are done parsing the header.
        p_error=12 // here an error has occurred and the parse failed.
    };

    status_t _status;
};
#endif//HTTPPARSER_H_
