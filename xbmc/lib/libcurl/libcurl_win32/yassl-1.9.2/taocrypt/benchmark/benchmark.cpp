// benchmark.cpp
// TaoCrypt benchmark

#include <string.h>
#include <stdio.h>

#include "runtime.hpp"
#include "des.hpp"
#include "aes.hpp"
#include "twofish.hpp"
#include "blowfish.hpp"
#include "arc4.hpp"
#include "md5.hpp"
#include "sha.hpp"
#include "ripemd.hpp"
#include "rsa.hpp"
#include "dh.hpp"
#include "dsa.hpp"


using namespace TaoCrypt;

void bench_aes(bool show);
void bench_des();
void bench_blowfish();
void bench_twofish();
void bench_arc4();

void bench_md5();
void bench_sha();
void bench_ripemd();

void bench_rsa();
void bench_dh();
void bench_dsa();

double current_time();




int main(int argc, char** argv)
{
    bench_aes(false);
    bench_aes(true);
    bench_blowfish();
    bench_twofish();
    bench_arc4();
    bench_des();
    
    printf("\n");

    bench_md5();
    bench_sha();
    bench_ripemd();

    printf("\n");
    
    bench_rsa();
    bench_dh();
    bench_dsa();

    return 0;
}

const int megs = 5;  // how much to test

const byte key[] = 
{
    0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
    0xfe,0xde,0xba,0x98,0x76,0x54,0x32,0x10,
    0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67
};

const byte iv[] = 
{
    0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef,
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81
    
};


byte plain [1024*1024];
byte cipher[1024*1024];


void bench_des()
{
    DES_EDE3_CBC_Encryption enc;
    enc.SetKey(key, 16, iv);

    double start = current_time();

    for(int i = 0; i < megs; i++)
        enc.Process(plain, cipher, sizeof(plain));

    double total = current_time() - start;

    double persec = 1 / total * megs;

    printf("3DES     %d megs took %5.3f seconds, %6.2f MB/s\n", megs, total,
                                                             persec);
}


void bench_aes(bool show)
{
    AES_CBC_Encryption enc;
    enc.SetKey(key, 16, iv);

    double start = current_time();
 
    for(int i = 0; i < megs; i++)
        enc.Process(plain, cipher, sizeof(plain));

    double total = current_time() - start;

    double persec = 1 / total * megs;

    if (show)
        printf("AES      %d megs took %5.3f seconds, %6.2f MB/s\n", megs, total,
                                                                 persec);
}


void bench_twofish()
{
    Twofish_CBC_Encryption enc;
    enc.SetKey(key, 16, iv);

    double start = current_time();

    for(int i = 0; i < megs; i++)
        enc.Process(plain, cipher, sizeof(plain));

    double total = current_time() - start;

    double persec = 1 / total * megs;

    printf("Twofish  %d megs took %5.3f seconds, %6.2f MB/s\n", megs, total,
                                                            persec);

}


void bench_blowfish()
{
    Blowfish_CBC_Encryption enc;
    enc.SetKey(key, 16, iv);

    double start = current_time();

    for(int i = 0; i < megs; i++)
        enc.Process(plain, cipher, sizeof(plain));

    double total = current_time() - start;

    double persec = 1 / total * megs;

    printf("Blowfish %d megs took %5.3f seconds, %6.2f MB/s\n", megs, total,
                                                             persec);
}


void bench_arc4()
{
    ARC4 enc;
    enc.SetKey(key, 16);

    double start = current_time();

    for(int i = 0; i < megs; i++)
        enc.Process(cipher, plain, sizeof(plain));

    double total = current_time() - start;

    double persec = 1 / total * megs;

    printf("ARC4     %d megs took %5.3f seconds, %6.2f MB/s\n", megs, total,
                                                             persec);
}


void bench_md5()
{
    MD5 hash;
    byte digest[MD5::DIGEST_SIZE];

    double start = current_time();

    
    for(int i = 0; i < megs; i++)
        hash.Update(plain, sizeof(plain));
   
    hash.Final(digest);

    double total = current_time() - start;

    double persec = 1 / total * megs;

    printf("MD5      %d megs took %5.3f seconds, %6.2f MB/s\n", megs, total,
                                                             persec);
}


void bench_sha()
{
    SHA hash;
    byte digest[SHA::DIGEST_SIZE];

    double start = current_time();

    
    for(int i = 0; i < megs; i++)
        hash.Update(plain, sizeof(plain));
   
    hash.Final(digest);

    /*
    for(int i = 0; i < megs; i++)
        hash.AsmTransform(plain, 16384);
    */


    double total = current_time() - start;

    double persec = 1 / total * megs;

    printf("SHA      %d megs took %5.3f seconds, %6.2f MB/s\n", megs, total,
                                                             persec);
}


void bench_ripemd()
{
    RIPEMD160 hash;
    byte digest[RIPEMD160::DIGEST_SIZE];

    double start = current_time();

    
    for(int i = 0; i < megs; i++)
        hash.Update(plain, sizeof(plain));
   
    hash.Final(digest);

    double total = current_time() - start;

    double persec = 1 / total * megs;

    printf("RIPEMD   %d megs took %5.3f seconds, %6.2f MB/s\n", megs, total,
                                                             persec);
}

RandomNumberGenerator rng;

void bench_rsa()
{
    const int times = 100;

    Source source;
    FileSource("./rsa1024.der", source);

    if (source.size() == 0) {
        printf("can't find ./rsa1024.der\n");
        return;
    }
    RSA_PrivateKey priv(source);
    RSAES_Encryptor enc(priv);

    byte      message[] = "Everyone gets Friday off.";
    byte      cipher[128];  // for 1024 bit
    byte      plain[128];   // for 1024 bit
    const int len = (word32)strlen((char*)message);
    
    int i;    
    double start = current_time();

    for (i = 0; i < times; i++)
        enc.Encrypt(message, len, cipher, rng);

    double total = current_time() - start;
    double each  = total / times;   // per second
    double milliEach = each * 1000; // milliseconds

    printf("RSA 1024 encryption took %6.2f milliseconds, avg over %d" 
           " iterations\n", milliEach, times);

    RSAES_Decryptor dec(priv);

    start = current_time();

    for (i = 0; i < times; i++)
        dec.Decrypt(cipher, 128, plain, rng);

    total = current_time() - start;
    each  = total / times;   // per second
    milliEach = each * 1000; // milliseconds

    printf("RSA 1024 decryption took %6.2f milliseconds, avg over %d" 
           " iterations\n", milliEach, times);
}


void bench_dh()
{
    const int times = 100;

    Source source;
    FileSource("./dh1024.der", source);

    if (source.size() == 0) {
        printf("can't find ./dh1024.der\n");
        return;
    }
    DH dh(source);

    byte      pub[128];    // for 1024 bit
    byte      priv[128];   // for 1024 bit
    
    int i;    
    double start = current_time();

    for (i = 0; i < times; i++)
        dh.GenerateKeyPair(rng, priv, pub);

    double total = current_time() - start;
    double each  = total / times;   // per second
    double milliEach = each * 1000; // milliseconds

    printf("DH  1024 key generation  %6.2f milliseconds, avg over %d" 
           " iterations\n", milliEach, times);

    DH dh2(dh); 
    byte      pub2[128];    // for 1024 bit
    byte      priv2[128];   // for 1024 bit
    dh2.GenerateKeyPair(rng, priv2, pub2);
    unsigned char key[256];

    start = current_time();

    for (i = 0; i < times; i++)
        dh.Agree(key, priv, pub2);

    total = current_time() - start;
    each  = total / times;      // per second
    milliEach = each * 1000;   //  in milliseconds

    printf("DH  1024 key agreement   %6.2f milliseconds, avg over %d"
           " iterations\n", milliEach, times);
}

void bench_dsa()
{
    const int times = 100;

    Source source;
    FileSource("./dsa1024.der", source);

    if (source.size() == 0) {
        printf("can't find ./dsa1024.der\n");
        return;
    }

    DSA_PrivateKey key(source);
    DSA_Signer signer(key);

    SHA sha;
    byte digest[SHA::DIGEST_SIZE];
    byte signature[40];
    const char msg[] = "this is the message";
    sha.Update((byte*)msg, sizeof(msg));
    sha.Final(digest);
    
    int i;    
    double start = current_time();

    for (i = 0; i < times; i++)
        signer.Sign(digest, signature, rng); 

    double total = current_time() - start;
    double each  = total / times;   // per second
    double milliEach = each * 1000; // milliseconds

    printf("DSA 1024 sign   took     %6.2f milliseconds, avg over %d" 
           " iterations\n", milliEach, times);

    DSA_Verifier verifier(key);

    start = current_time();

    for (i = 0; i < times; i++)
        verifier.Verify(digest, signature); 

    total = current_time() - start;
    each  = total / times;      // per second
    milliEach = each * 1000;   //  in milliseconds

    printf("DSA 1024 verify took     %6.2f milliseconds, avg over %d"
           " iterations\n", milliEach, times);
}



#ifdef _WIN32

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    double current_time()
    {
        static bool          init(false);
        static LARGE_INTEGER freq;
    
        if (!init) {
            QueryPerformanceFrequency(&freq);
            init = true;
        }

        LARGE_INTEGER count;
        QueryPerformanceCounter(&count);

        return static_cast<double>(count.QuadPart) / freq.QuadPart;
    }

#else

    #include <sys/time.h>

    double current_time()
    {
        struct timeval tv;
        gettimeofday(&tv, 0);

        return static_cast<double>(tv.tv_sec) 
             + static_cast<double>(tv.tv_usec) / 1000000;
    }

#endif // _WIN32
