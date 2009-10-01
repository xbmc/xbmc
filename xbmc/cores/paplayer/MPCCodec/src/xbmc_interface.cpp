#include "xbmc_interface.h"

extern "C" {
  bool __declspec(dllexport) Open(mpc_decoder **decoder, mpc_reader *reader, mpc_streaminfo *info, double *timeinseconds)
  {
    if (!reader || !decoder || !info) return false;

    // create our decoder
    *decoder = new mpc_decoder;
    if (!(*decoder))
    {
      printf("Unable to create our mpc decoder\n");
      return false;
    }

    // read file's streaminfo data
    mpc_streaminfo_init(info);
    if (mpc_streaminfo_read(info, reader) != ERROR_CODE_OK)
    {
      printf("Not a valid musepack file");
      return false;
    }

    // and read the time in seconds
    if (timeinseconds)
      *timeinseconds = mpc_streaminfo_get_length(info);

    // instantiate a decoder with our file reader
    mpc_decoder_setup(*decoder, reader);
    if (!mpc_decoder_initialize(*decoder, info))
    {
      printf("Error initializing decoder");
      return false;
    }

    return true;
  };

  void __declspec(dllexport) Close(mpc_decoder *decoder)
  {
    // free the memory of our decoder
    if (decoder)
    {
#ifdef USE_SEEK_TABLES
      mpc_decoder_free(decoder);
#endif
      delete decoder;
    }
  };

  bool __declspec(dllexport) Seek(mpc_decoder *decoder, double timeinseconds)
  {
    if (!decoder) return false;
    return mpc_decoder_seek_seconds(decoder, timeinseconds) == 1;
  };

  // returns:
  // -2 EOF occurs
  // -1 ERROR
  //  >0 Success - number of samples read
  int __declspec(dllexport) Read(mpc_decoder *decoder, float *buffer, unsigned int num_samples)
  {
    if (!decoder || !buffer) return -1;
    mpc_uint32_t ret = mpc_decoder_decode(decoder, buffer, NULL, NULL);
    if (ret > 0)
      return ret;   // success :)
    if (ret == 0)
      return -2;  // EOF
    return -1;    // ERROR
  };

};
