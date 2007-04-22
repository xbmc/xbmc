unit zlibdef;

interface

uses
  Windows;

const
  ZLIB_VERSION = '1.1.3';

type
  voidpf = Pointer;
  int    = Integer;
  uInt   = Cardinal;
  pBytef = PChar;
  uLong  = Cardinal;

  alloc_func = function(opaque: voidpf; items, size: uInt): voidpf;
                    stdcall;
  free_func  = procedure(opaque, address: voidpf);
                    stdcall;

  internal_state = Pointer;

  z_streamp = ^z_stream;
  z_stream = packed record
    next_in: pBytef;          // next input byte
    avail_in: uInt;           // number of bytes available at next_in
    total_in: uLong;          // total nb of input bytes read so far

    next_out: pBytef;         // next output byte should be put there
    avail_out: uInt;          // remaining free space at next_out
    total_out: uLong;         // total nb of bytes output so far

    msg: PChar;               // last error message, NULL if no error
    state: internal_state;    // not visible by applications

    zalloc: alloc_func;       // used to allocate the internal state
    zfree: free_func;         // used to free the internal state
    opaque: voidpf;           // private data object passed to zalloc and zfree

    data_type: int;           // best guess about the data type: ascii or binary
    adler: uLong;             // adler32 value of the uncompressed data
    reserved: uLong;          // reserved for future use
    end;

const
  Z_NO_FLUSH      = 0;
  Z_SYNC_FLUSH    = 2;
  Z_FULL_FLUSH    = 3;
  Z_FINISH        = 4;

  Z_OK            = 0;
  Z_STREAM_END    = 1;

  Z_NO_COMPRESSION         =  0;
  Z_BEST_SPEED             =  1;
  Z_BEST_COMPRESSION       =  9;
  Z_DEFAULT_COMPRESSION    = -1;

  Z_FILTERED            = 1;
  Z_HUFFMAN_ONLY        = 2;
  Z_DEFAULT_STRATEGY    = 0;

  Z_BINARY   = 0;
  Z_ASCII    = 1;
  Z_UNKNOWN  = 2;

  Z_DEFLATED    = 8;

  MAX_MEM_LEVEL = 9;

function adler32(adler: uLong; const buf: pBytef; len: uInt): uLong;
             stdcall;
function crc32(crc: uLong; const buf: pBytef; len: uInt): uLong;
             stdcall;
function deflate(strm: z_streamp; flush: int): int;
             stdcall;
function deflateCopy(dest, source: z_streamp): int;
             stdcall;
function deflateEnd(strm: z_streamp): int;
             stdcall;
function deflateInit2_(strm: z_streamp; level, method,
                       windowBits, memLevel, strategy: int;
                       const version: PChar; stream_size: int): int;
             stdcall;
function deflateInit_(strm: z_streamp; level: int;
                      const version: PChar; stream_size: int): int;
             stdcall;
function deflateParams(strm: z_streamp; level, strategy: int): int;
             stdcall;
function deflateReset(strm: z_streamp): int;
             stdcall;
function deflateSetDictionary(strm: z_streamp;
                              const dictionary: pBytef;
                              dictLength: uInt): int;
             stdcall;
function inflate(strm: z_streamp; flush: int): int;
             stdcall;
function inflateEnd(strm: z_streamp): int;
             stdcall;
function inflateInit2_(strm: z_streamp; windowBits: int;
                       const version: PChar; stream_size: int): int;
             stdcall;
function inflateInit_(strm: z_streamp; const version: PChar;
                      stream_size: int): int;
             stdcall;
function inflateReset(strm: z_streamp): int;
             stdcall;
function inflateSetDictionary(strm: z_streamp;
                              const dictionary: pBytef;
                              dictLength: uInt): int;
             stdcall;
function inflateSync(strm: z_streamp): int;
             stdcall;

function deflateInit(strm: z_streamp; level: int): int;
function deflateInit2(strm: z_streamp; level, method, windowBits,
                      memLevel, strategy: int): int;
function inflateInit(strm: z_streamp): int;
function inflateInit2(strm: z_streamp; windowBits: int): int;

implementation

function deflateInit(strm: z_streamp; level: int): int;
begin
  Result := deflateInit_(strm, level, ZLIB_VERSION, sizeof(z_stream));
end;

function deflateInit2(strm: z_streamp; level, method, windowBits,
                      memLevel, strategy: int): int;
begin
  Result := deflateInit2_(strm, level, method, windowBits, memLevel,
                          strategy, ZLIB_VERSION, sizeof(z_stream));
end;

function inflateInit(strm: z_streamp): int;
begin
  Result := inflateInit_(strm, ZLIB_VERSION, sizeof(z_stream));
end;

function inflateInit2(strm: z_streamp; windowBits: int): int;
begin
  Result := inflateInit2_(strm, windowBits, ZLIB_VERSION,
                          sizeof(z_stream));
end;

const
  zlibDLL = 'png32bd.dll';

function adler32; external zlibDLL;
function crc32; external zlibDLL;
function deflate; external zlibDLL;
function deflateCopy; external zlibDLL;
function deflateEnd; external zlibDLL;
function deflateInit2_; external zlibDLL;
function deflateInit_; external zlibDLL;
function deflateParams; external zlibDLL;
function deflateReset; external zlibDLL;
function deflateSetDictionary; external zlibDLL;
function inflate; external zlibDLL;
function inflateEnd; external zlibDLL;
function inflateInit2_; external zlibDLL;
function inflateInit_; external zlibDLL;
function inflateReset; external zlibDLL;
function inflateSetDictionary; external zlibDLL;
function inflateSync; external zlibDLL;

end.
