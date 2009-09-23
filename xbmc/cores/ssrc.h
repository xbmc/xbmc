//------------------------------------------------------------------------------------
// Sample frequency change class, based on original SSRC by Naoki Shibata
// Released under LGPL
// Updated to C++ class by Spoon (www.dbpoweramp.com) March 2002 dbpoweramp@dbpoweramp.com
//
// Updated to work with XBMC March 2004 by JMarshall
//
//
// Example (NB wfxInformat should be already filled in with the input data type):
//
// Cssrc SampleRateChange;
// SampleRateChange.InitConverter(&wfxInformat, 48000); // convert to 48KHz
// while (ReadChunksOfInputFile)
// {
//    int DataSize = InputJustRead;
//    IsEOF = false;
//    char *pConvData = SampleRateChange.ConvertSomeData(InData, DataSize, IsEOF);
//    if (pConvData)
//    {
//        // Data is in pConvData and is DataSize bytes long
//    delete pConvData;
//    }
// }
//-------------------------------------------------------------------------------------
#ifndef CssrcH
#define CssrcH

//#include <mmsystem.h>

//#include "clsDataStream.h"

#ifndef HIGH_PREC
typedef float REAL;
#define AA 96
 #define DF 8000
 #define FFTFIRLEN 1024;
#else
typedef double REAL;
#define AA 120
 #define DF 100
 #define FFTFIRLEN 16384;
#endif
#ifndef M_PI
 #define M_PI 3.1415926535897932384626433832795028842
#endif
#define RANDBUFLEN 65536
#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))
#define POOLSIZE 97

#ifndef RAND_MAX
#define RAND_MAX 0x7FFF
#endif


const int scoeffreq[] = {0, 48000, 44100, 37800, 32000, 22050, 48000, 44100};
const int scoeflen[] = {1, 16, 20, 16, 16, 15, 16, 15};
const int samp[] = {8, 18, 27, 8, 8, 8, 10, 9};
const double shapercoefs[8][21] =
  {
    { -1}
    ,  /* triangular dither */

    { -2.8720729351043701172, 5.0413231849670410156, -6.2442994117736816406, 5.8483986854553222656,
      -3.7067542076110839844, 1.0495119094848632812, 1.1830236911773681641, -2.1126792430877685547,
      1.9094531536102294922, -0.99913084506988525391, 0.17090806365013122559, 0.32615602016448974609,
      -0.39127644896507263184, 0.26876461505889892578, -0.097676105797290802002, 0.023473845794796943665,
    },  /* 48k, N=16, amp=18 */

    { -2.6773197650909423828, 4.8308925628662109375, -6.570110321044921875, 7.4572014808654785156,
      -6.7263274192810058594, 4.8481650352478027344, -2.0412089824676513672, -0.7006359100341796875,
      2.9537565708160400391, -4.0800385475158691406, 4.1845216751098632812, -3.3311812877655029297,
      2.1179926395416259766, -0.879302978515625, 0.031759146600961685181, 0.42382788658142089844,
      -0.47882103919982910156, 0.35490813851356506348, -0.17496839165687561035, 0.060908168554306030273,
    },  /* 44.1k, N=20, amp=27 */

    { -1.6335992813110351562, 2.2615492343902587891, -2.4077029228210449219, 2.6341717243194580078,
      -2.1440362930297851562, 1.8153258562088012695, -1.0816224813461303711, 0.70302653312683105469,
      -0.15991993248462677002, -0.041549518704414367676, 0.29416576027870178223, -0.2518316805362701416,
      0.27766478061676025391, -0.15785403549671173096, 0.10165894031524658203, -0.016833892092108726501,
    },  /* 37.8k, N=16 */

    { -0.82901298999786376953, 0.98922657966613769531, -0.59825712442398071289, 1.0028809309005737305,
      -0.59938216209411621094, 0.79502451419830322266, -0.42723315954208374023, 0.54492527246475219727,
      -0.30792605876922607422, 0.36871799826622009277, -0.18792048096656799316, 0.2261127084493637085,
      -0.10573341697454452515, 0.11435490846633911133, -0.038800679147243499756, 0.040842197835445404053,
    },  /* 32k, N=16 */

    { -0.065229974687099456787, 0.54981261491775512695, 0.40278548002243041992, 0.31783768534660339355,
      0.28201797604560852051, 0.16985194385051727295, 0.15433363616466522217, 0.12507140636444091797,
      0.08903945237398147583, 0.064410120248794555664, 0.047146003693342208862, 0.032805237919092178345,
      0.028495194390416145325, 0.011695005930960178375, 0.011831838637590408325,
    },  /* 22.05k, N=15 */

    { -2.3925774097442626953, 3.4350297451019287109, -3.1853709220886230469, 1.8117271661758422852,
      0.20124770700931549072, -1.4759907722473144531, 1.7210904359817504883, -0.97746700048446655273,
      0.13790138065814971924, 0.38185903429985046387, -0.27421241998672485352, -0.066584214568138122559,
      0.35223302245140075684, -0.37672343850135803223, 0.23964276909828186035, -0.068674825131893157959,
    },  /* 48k, N=16, amp=10 */

    { -2.0833916664123535156, 3.0418450832366943359, -3.2047898769378662109, 2.7571926116943359375,
      -1.4978630542755126953, 0.3427594602108001709, 0.71733748912811279297, -1.0737057924270629883,
      1.0225815773010253906, -0.56649994850158691406, 0.20968692004680633545, 0.065378531813621520996,
      -0.10322438180446624756, 0.067442022264003753662, 0.00495197344571352005,
    },  /* 44.1k, N=15, amp=9 */
  };

class Cssrc
{
public:
  Cssrc(void);
  ~Cssrc();

  //---------------------------------------------------------------------------
  // Inits Freq Converter, returns false if cannot do
  //---------------------------------------------------------------------------
  bool InitConverter(int OldFreq, int OldBPS, int Channels, int NewFreq, int NewBPS, int OutputBufferSize);

  //---------------------------------------------------------------------------
  // returns the input bitrate that we are using (in bits per second)
  //---------------------------------------------------------------------------
  int GetInputBitrate();

  //---------------------------------------------------------------------------
  // Deinitializes everything, cleaning up any buffers that exist
  //---------------------------------------------------------------------------
  void DeInitialize();

  //---------------------------------------------------------------------------
  // Get Resampled data out of the buffers
  // returns true if data was got, returns false if there is no data ready
  //---------------------------------------------------------------------------
  bool GetData(unsigned char *pOutData);

  //---------------------------------------------------------------------------
  // Put up to iSize bytes of data into our resampler
  // returns the amount of data read in.
  // if there is not enough data, it returns -1
  // if we first need to do a GetData() it returns 0
  //---------------------------------------------------------------------------
  int PutData(unsigned char *pInData, int iSize);

  //---------------------------------------------------------------------------
  // Put up to iSize samples of **FLOAT** data into our resampler
  // returns the amount of data read in.
  // if there is not enough data, it returns -1
  // if we first need to do a GetData() it returns 0
  //---------------------------------------------------------------------------
  int PutFloatData(float *pInData, int numSamples);

  //---------------------------------------------------------------------------
  // returns the amount of data (or samples) that the resampler will take in
  // in one run of PutData()
  // if we first need to do a GetData() it returns 0.
  // if we can't take any data (eg downsampling, which is current disabled) it
  // returns -1
  //---------------------------------------------------------------------------
  int GetInputSize();
  int GetInputSamples();

  int GetMaxInputSize() { return m_iMaxInputSize;};
  //---------------------------------------------------------------------------
  // Converts some data-
  // DataSize is adjusted
  // returns a new buffer (could be NULL), *** needs to be DELETED
  // IsEOF is set to true when the last chunk of data is supplied in
  //---------------------------------------------------------------------------
  // NOT USED FOR XBMC!
  // char *ConvertSomeData(char *InData, int &DataSize, bool IsEOF);

private:
  // clsDataStream DataStream;

  int m_iMaxInputSize;    // Total amount of data we take in at once
  // The temporary output buffer
  int m_iOutputBufferSize;   // Amount to output to next filter in the chain
  int m_iResampleBufferSize;   // Total amount of room in the buffer
  int m_iResampleBufferPos;   // Where we are in the buffer
  unsigned char *m_pResampleBuffer; // The buffer

  double **shapebuf;
  int shaper_type, shaper_len, shaper_clipmin, shaper_clipmax;
  int *randbuf, randptr;
  bool UpSampling;
  bool DownSampling;
  int frqgcd, nch, sfrq, bps, dfrq, dbps, osf, fs1, fs2;
  int n1, n1x, n1y, n2, n2b, n2x, n2y, n1b;
  int filter2len; /* stage 2 filter length */
  int filter1len; /* stage 1 filter length */
  int spcount;
  double peak;
  int *fft_ip;
  REAL *fft_w;
  int *f1order, *f1inc;
  int *f2order, *f2inc;
  unsigned char *rawinbuf, *rawoutbuf;
  REAL *inbuf, *outbuf;
  REAL **buf1, **buf2;
  REAL **stage1US, *stage2US;
  REAL *stage1DS, **stage2DS;

  int n2b2;
  int delay;
  int rp;
  int ds;
  int nsmplwrt1;
  int nsmplwrt2;
  int s1p;
  int init, ending;
  unsigned int sumread, sumwrite;
  int osc;
  REAL *ip, *ip_backup;
  int s1p_backup, osc_backup;
  int ch, p;
  int inbuflen;

  int n1b2;
  int rps;
  int rp2;
  int s2p;
  REAL *bp;
  int rps_backup, s2p_backup;
  int k;
  REAL *op;



  //---------------------------------------------------------------------------
  // Inits Filter Stage 1, returns false if error
  //---------------------------------------------------------------------------
  bool InitFilters(void);

  //---------------------------------------------------------------------------
  // Upsamples a buffer full of rawindata
  // returns the datalength
  //---------------------------------------------------------------------------
  int UpSampleRawIn(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int toberead2, int nsmplread);

  //---------------------------------------------------------------------------
  // Upsamples a buffer full of *FLOAT* rawindata
  // returns the datalength
  //---------------------------------------------------------------------------
  int UpSampleFloatIn(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int toberead2, int nsmplread);

  //---------------------------------------------------------------------------
  // Common routine for above upsampling routines
  //---------------------------------------------------------------------------
  int UpSampleCommon(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int toberead2, int nsmplread);

  //---------------------------------------------------------------------------
  // Downsamples a buffer full of rawindata
  // returns the datalength
  //---------------------------------------------------------------------------
  int DownSampleRawIn(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int nsmplread);

  //---------------------------------------------------------------------------
  // Downsamples a buffer full of *FLOAT* rawindata
  // returns the datalength
  //---------------------------------------------------------------------------
  int DownSampleFloatIn(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int nsmplread);

  //---------------------------------------------------------------------------
  // Common routine for above downsampling routines
  //---------------------------------------------------------------------------
  int DownSampleCommon(unsigned char * *pRetDataPtr, bool IsEof, int toberead, int nsmplread);

  //----------------------------------------------------
  double alpha(double a);
  double dbesi0(double x);
  double win(double n, int len, double alp, double iza);
  double sinc(double x);
  double hn_lpf(int n, double lpf, double fs);
  int gcd(int x, int y);
  int extract_int(unsigned char *buf);
  short extract_short(unsigned char *buf);
  void bury_int(unsigned char *buf, int i);
  void bury_short(unsigned char *buf, short s);
  //----------------------------------------------------
  void dstsub(int n, REAL *a, int nc, REAL *c);
  void dctsub(int n, REAL *a, int nc, REAL *c);
  void rftbsub(int n, REAL *a, int nc, REAL *c);
  void rftfsub(int n, REAL *a, int nc, REAL *c);
  void cftx020(REAL *a);
  void cftb040(REAL *a);
  void cftf040(REAL *a);
  void cftf082(REAL *a, REAL *w);
  void cftf081(REAL *a, REAL *w);
  void cftf162(REAL *a, REAL *w);
  void cftf161(REAL *a, REAL *w);
  void cftfx42(int n, REAL *a, int nw, REAL *w);
  void cftfx41(int n, REAL *a, int nw, REAL *w);
  void cftmdl2(int n, REAL *a, REAL *w);
  void cftmdl1(int n, REAL *a, REAL *w);
  void cftexp2(int n, REAL *a, int nw, REAL *w);
  void cftexp1(int n, REAL *a, int nw, REAL *w);
  void cftrec2(int n, REAL *a, int nw, REAL *w);
  void cftrec1(int n, REAL *a, int nw, REAL *w);
  void cftb1st(int n, REAL *a, REAL *w);
  void cftf1st(int n, REAL *a, REAL *w);
  void bitrv208neg(REAL *a);
  void bitrv208(REAL *a);
  void bitrv216neg(REAL *a);
  void bitrv216(REAL *a);
  void bitrv2conj(int n, int *ip, REAL *a);
  void bitrv2(int n, int *ip, REAL *a);
  void cftbsub(int n, REAL *a, int *ip, int nw, REAL *w);
  void cftfsub(int n, REAL *a, int *ip, int nw, REAL *w);
  void makect(int nc, int *ip, REAL *c);
  void makewt(int nw, int *ip, REAL *w);
  void dfst(int n, REAL *a, REAL *t, int *ip, REAL *w);
  void dfct(int n, REAL *a, REAL *t, int *ip, REAL *w);
  void ddst(int n, int isgn, REAL *a, int *ip, REAL *w);
  void ddct(int n, int isgn, REAL *a, int *ip, REAL *w);
  void rdft(int n, int isgn, REAL *a, int *ip, REAL *w);
  void cdft(int n, int isgn, REAL *a, int *ip, REAL *w);
};

#endif
