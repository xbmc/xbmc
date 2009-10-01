#ifndef WAVWRITER_H
#define WAVWRITER_H

void wavwriter_writeheaders(FILE *f, int datasize,
                            int numchannels, int samplerate,
                            int bitspersample);

#endif /* WAVWRITER_H */

