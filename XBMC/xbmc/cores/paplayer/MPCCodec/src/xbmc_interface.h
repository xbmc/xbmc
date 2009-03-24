
#ifdef _LINUX
#define __declspec(x)
#endif

extern "C" {
#include "mpcdec/mpcdec.h"

bool __declspec(dllexport) Open(mpc_decoder **decoder, mpc_reader *reader,
    mpc_streaminfo *info, double *timeinseconds);
void __declspec(dllexport) Close(mpc_decoder *decoder);
bool __declspec(dllexport) Seek(mpc_decoder *decoder, double timeinseconds);
/* returns:
 * -2 EOF occurs
 * -1 ERROR
 * >0 Success - number of samples read
 */
int __declspec(dllexport) Read(mpc_decoder *decoder, float *buffer,
    unsigned int num_samples);
};
