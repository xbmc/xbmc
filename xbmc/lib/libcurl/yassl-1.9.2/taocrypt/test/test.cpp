// test.cpp
// test taocrypt functionality

#include <string.h>
#include <stdio.h>

#include "runtime.hpp"
#include "sha.hpp"
#include "md5.hpp"
#include "md2.hpp"
#include "md4.hpp"
#include "ripemd.hpp"
#include "hmac.hpp"
#include "arc4.hpp"
#include "des.hpp"
#include "rsa.hpp"
#include "dsa.hpp"
#include "aes.hpp"
#include "twofish.hpp"
#include "blowfish.hpp"
#include "asn.hpp"
#include "dh.hpp"
#include "coding.hpp"
#include "random.hpp"
#include "pwdbased.hpp"



using TaoCrypt::byte;
using TaoCrypt::word32;
using TaoCrypt::SHA;
using TaoCrypt::SHA256;
using TaoCrypt::SHA224;
#ifdef WORD64_AVAILABLE
    using TaoCrypt::SHA512;
    using TaoCrypt::SHA384;
#endif
using TaoCrypt::MD5;
using TaoCrypt::MD2;
using TaoCrypt::MD4;
using TaoCrypt::RIPEMD160;
using TaoCrypt::HMAC;
using TaoCrypt::ARC4;
using TaoCrypt::DES_EDE3_CBC_Encryption;
using TaoCrypt::DES_EDE3_CBC_Decryption;
using TaoCrypt::DES_CBC_Encryption;
using TaoCrypt::DES_CBC_Decryption;
using TaoCrypt::DES_ECB_Encryption;
using TaoCrypt::DES_ECB_Decryption;
using TaoCrypt::AES_CBC_Encryption;
using TaoCrypt::AES_CBC_Decryption;
using TaoCrypt::AES_ECB_Encryption;
using TaoCrypt::AES_ECB_Decryption;
using TaoCrypt::Twofish_CBC_Encryption;
using TaoCrypt::Twofish_CBC_Decryption;
using TaoCrypt::Twofish_ECB_Encryption;
using TaoCrypt::Twofish_ECB_Decryption;
using TaoCrypt::Blowfish_CBC_Encryption;
using TaoCrypt::Blowfish_CBC_Decryption;
using TaoCrypt::Blowfish_ECB_Encryption;
using TaoCrypt::Blowfish_ECB_Decryption;
using TaoCrypt::RSA_PrivateKey;
using TaoCrypt::RSA_PublicKey;
using TaoCrypt::DSA_PrivateKey;
using TaoCrypt::DSA_PublicKey;
using TaoCrypt::DSA_Signer;
using TaoCrypt::DSA_Verifier;
using TaoCrypt::RSAES_Encryptor;
using TaoCrypt::RSAES_Decryptor;
using TaoCrypt::Source;
using TaoCrypt::FileSource;
using TaoCrypt::FileSource;
using TaoCrypt::HexDecoder;
using TaoCrypt::HexEncoder;
using TaoCrypt::Base64Decoder;
using TaoCrypt::Base64Encoder;
using TaoCrypt::CertDecoder;
using TaoCrypt::DH;
using TaoCrypt::EncodeDSA_Signature;
using TaoCrypt::DecodeDSA_Signature;
using TaoCrypt::PBKDF2_HMAC;
using TaoCrypt::tcArrayDelete;
using TaoCrypt::GetCert;
using TaoCrypt::GetPKCS_Cert;


struct testVector {
    byte*  input_;
    byte*  output_; 
    word32 inLen_;
    word32 outLen_;

    testVector(const char* in, const char* out) : input_((byte*)in),
               output_((byte*)out), inLen_((word32)strlen(in)),
               outLen_((word32)strlen(out)) {}
};

int  sha_test();
int  sha256_test();
#ifdef WORD64_AVAILABLE
    int  sha512_test();
    int  sha384_test();
#endif
int  sha224_test();
int  md5_test();
int  md2_test();
int  md4_test();
int  ripemd_test();
int  hmac_test();
int  arc4_test();
int  des_test();
int  aes_test();
int  twofish_test();
int  blowfish_test();
int  rsa_test();
int  dsa_test();
int  dh_test();
int  pwdbased_test();
int  pkcs12_test();

TaoCrypt::RandomNumberGenerator rng;


void err_sys(const char* msg, int es)
{
    printf("%s\n", msg);
    exit(es);    
}

// func_args from test.hpp, so don't have to pull in other junk
struct func_args {
    int    argc;
    char** argv;
    int    return_code;
};


/* 
   DES, AES, Blowfish, and Twofish need aligned (4 byte) input/output for
   processing, can turn this off by setting gpBlock(assumeAligned = false)
   but would hurt performance.  yaSSL always uses dynamic memory so we have
   at least 8 byte alignment.  This test tried to force alignment for stack
   variables (for convenience) but some compiler versions and optimizations
   seemed to be off.  So we have msgTmp variable which we copy into dynamic
   memory at runtime to ensure proper alignment, along with plain/cipher.
   Whew!
*/
const byte msgTmp[] = { // "now is the time for all " w/o trailing 0
    0x6e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,
    0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
    0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20
};

byte* msg    = 0;   // for block cipher input
byte* plain  = 0;   // for cipher decrypt comparison 
byte* cipher = 0;   // block output


void taocrypt_test(void* args)
{
    ((func_args*)args)->return_code = -1; // error state

    msg    = NEW_TC byte[24];
    plain  = NEW_TC byte[24];
    cipher = NEW_TC byte[24];

    memcpy(msg, msgTmp, 24);

    int ret = 0;
    if ( (ret = sha_test()) ) 
        err_sys("SHA      test failed!\n", ret);
    else
        printf( "SHA      test passed!\n");

    if ( (ret = sha256_test()) ) 
        err_sys("SHA-256  test failed!\n", ret);
    else
        printf( "SHA-256  test passed!\n");

    if ( (ret = sha224_test()) ) 
        err_sys("SHA-224  test failed!\n", ret);
    else
        printf( "SHA-224  test passed!\n");

#ifdef WORD64_AVAILABLE

    if ( (ret = sha512_test()) ) 
        err_sys("SHA-512  test failed!\n", ret);
    else
        printf( "SHA-512  test passed!\n");

    if ( (ret = sha384_test()) ) 
        err_sys("SHA-384  test failed!\n", ret);
    else
        printf( "SHA-384  test passed!\n");

#endif

    if ( (ret = md5_test()) ) 
        err_sys("MD5      test failed!\n", ret);
    else
        printf( "MD5      test passed!\n");

    if ( (ret = md2_test()) ) 
        err_sys("MD2      test failed!\n", ret);
    else
        printf( "MD2      test passed!\n");

    if ( (ret = md4_test()) ) 
        err_sys("MD4      test failed!\n", ret);
    else
        printf( "MD4      test passed!\n");

    if ( (ret = ripemd_test()) )
        err_sys("RIPEMD   test failed!\n", ret);
    else
        printf( "RIPEMD   test passed!\n");

    if ( ( ret = hmac_test()) )
        err_sys("HMAC     test failed!\n", ret);
    else
        printf( "HMAC     test passed!\n");

    if ( (ret = arc4_test()) )
        err_sys("ARC4     test failed!\n", ret);
    else
        printf( "ARC4     test passed!\n");

    if ( (ret = des_test()) )
        err_sys("DES      test failed!\n", ret);
    else
        printf( "DES      test passed!\n");

    if ( (ret = aes_test()) )
        err_sys("AES      test failed!\n", ret);
    else
        printf( "AES      test passed!\n");

    if ( (ret = twofish_test()) )
        err_sys("Twofish  test failed!\n", ret);
    else
        printf( "Twofish  test passed!\n");

    if ( (ret = blowfish_test()) )
        err_sys("Blowfish test failed!\n", ret);
    else
        printf( "Blowfish test passed!\n");

    if ( (ret = rsa_test()) )
        err_sys("RSA      test failed!\n", ret);
    else
        printf( "RSA      test passed!\n");

    if ( (ret = dh_test()) )
        err_sys("DH       test failed!\n", ret);
    else
        printf( "DH       test passed!\n");

    if ( (ret = dsa_test()) )
        err_sys("DSA      test failed!\n", ret);
    else
        printf( "DSA      test passed!\n");

    if ( (ret = pwdbased_test()) )
        err_sys("PBKDF2   test failed!\n", ret);
    else
        printf( "PBKDF2   test passed!\n");

    /* not ready yet
    if ( (ret = pkcs12_test()) )
        err_sys("PKCS12   test failed!\n", ret);
    else
        printf( "PKCS12   test passed!\n");
    */

    tcArrayDelete(cipher);
    tcArrayDelete(plain);
    tcArrayDelete(msg);

    ((func_args*)args)->return_code = ret;
}


// so overall tests can pull in test function 
#ifndef NO_MAIN_DRIVER

    int main(int argc, char** argv)
    {
        func_args args;

        args.argc = argc;
        args.argv = argv;

        taocrypt_test(&args);
        TaoCrypt::CleanUp();

        return args.return_code;
    }

#endif // NO_MAIN_DRIVER


void file_test(const char* file, byte* check)
{
    FILE* f;
    int i = 0;
    MD5    md5;
    byte   buf[1024];
    byte   md5sum[MD5::DIGEST_SIZE];
    
    if( !( f = fopen( file, "rb" ) )) {
        printf("Can't open %s\n", file);
        return;
    }
    while( ( i = (int)fread(buf, 1, sizeof(buf), f )) > 0 )
        md5.Update(buf, i);
    
    md5.Final(md5sum);
    memcpy(check, md5sum, sizeof(md5sum));

    for(int j = 0; j < MD5::DIGEST_SIZE; ++j ) 
        printf( "%02x", md5sum[j] );
   
    printf("  %s\n", file);

    fclose(f);
}


int sha_test()
{
    SHA  sha;
    byte hash[SHA::DIGEST_SIZE];

    testVector test_sha[] =
    {
        testVector("abc", 
                 "\xA9\x99\x3E\x36\x47\x06\x81\x6A\xBA\x3E\x25\x71\x78\x50\xC2"
                 "\x6C\x9C\xD0\xD8\x9D"),
        testVector("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                 "\x84\x98\x3E\x44\x1C\x3B\xD2\x6E\xBA\xAE\x4A\xA1\xF9\x51\x29"
                 "\xE5\xE5\x46\x70\xF1"),
        testVector("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaa", 
                 "\x00\x98\xBA\x82\x4B\x5C\x16\x42\x7B\xD7\xA1\x12\x2A\x5A\x44"
                 "\x2A\x25\xEC\x64\x4D"),
        testVector("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaa",
                 "\xAD\x5B\x3F\xDB\xCB\x52\x67\x78\xC2\x83\x9D\x2F\x15\x1E\xA7"
                 "\x53\x99\x5E\x26\xA0")  
    };

    int times( sizeof(test_sha) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        sha.Update(test_sha[i].input_, test_sha[i].inLen_);
        sha.Final(hash);

        if (memcmp(hash, test_sha[i].output_, SHA::DIGEST_SIZE) != 0)
            return -1 - i;
    }

    return 0;
}


int sha256_test()
{
    SHA256 sha;
    byte   hash[SHA256::DIGEST_SIZE];

    testVector test_sha[] =
    {
        testVector("abc",
                 "\xBA\x78\x16\xBF\x8F\x01\xCF\xEA\x41\x41\x40\xDE\x5D\xAE\x22"
                 "\x23\xB0\x03\x61\xA3\x96\x17\x7A\x9C\xB4\x10\xFF\x61\xF2\x00"
                 "\x15\xAD"),
        testVector("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                 "\x24\x8D\x6A\x61\xD2\x06\x38\xB8\xE5\xC0\x26\x93\x0C\x3E\x60"
                 "\x39\xA3\x3C\xE4\x59\x64\xFF\x21\x67\xF6\xEC\xED\xD4\x19\xDB"
                 "\x06\xC1")
    };

    int times( sizeof(test_sha) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        sha.Update(test_sha[i].input_, test_sha[i].inLen_);
        sha.Final(hash);

        if (memcmp(hash, test_sha[i].output_, SHA256::DIGEST_SIZE) != 0)
            return -1 - i;
    }

    return 0;
}


#ifdef WORD64_AVAILABLE

int sha512_test()
{
    SHA512 sha;
    byte   hash[SHA512::DIGEST_SIZE];

    testVector test_sha[] =
    {
        testVector("abc",
                 "\xdd\xaf\x35\xa1\x93\x61\x7a\xba\xcc\x41\x73\x49\xae\x20\x41"
                 "\x31\x12\xe6\xfa\x4e\x89\xa9\x7e\xa2\x0a\x9e\xee\xe6\x4b\x55"
                 "\xd3\x9a\x21\x92\x99\x2a\x27\x4f\xc1\xa8\x36\xba\x3c\x23\xa3"
                 "\xfe\xeb\xbd\x45\x4d\x44\x23\x64\x3c\xe8\x0e\x2a\x9a\xc9\x4f"
                 "\xa5\x4c\xa4\x9f"),
        testVector("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhi"
                   "jklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 
                 "\x8e\x95\x9b\x75\xda\xe3\x13\xda\x8c\xf4\xf7\x28\x14\xfc\x14"
                 "\x3f\x8f\x77\x79\xc6\xeb\x9f\x7f\xa1\x72\x99\xae\xad\xb6\x88"
                 "\x90\x18\x50\x1d\x28\x9e\x49\x00\xf7\xe4\x33\x1b\x99\xde\xc4"
                 "\xb5\x43\x3a\xc7\xd3\x29\xee\xb6\xdd\x26\x54\x5e\x96\xe5\x5b"
                 "\x87\x4b\xe9\x09")
    };

    int times( sizeof(test_sha) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        sha.Update(test_sha[i].input_, test_sha[i].inLen_);
        sha.Final(hash);

        if (memcmp(hash, test_sha[i].output_, SHA512::DIGEST_SIZE) != 0)
            return -1 - i;
    }

    return 0;
}


int sha384_test()
{
    SHA384 sha;
    byte   hash[SHA384::DIGEST_SIZE];

    testVector test_sha[] =
    {
        testVector("abc",
                 "\xcb\x00\x75\x3f\x45\xa3\x5e\x8b\xb5\xa0\x3d\x69\x9a\xc6\x50"
                 "\x07\x27\x2c\x32\xab\x0e\xde\xd1\x63\x1a\x8b\x60\x5a\x43\xff"
                 "\x5b\xed\x80\x86\x07\x2b\xa1\xe7\xcc\x23\x58\xba\xec\xa1\x34"
                 "\xc8\x25\xa7"),
        testVector("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhi"
                   "jklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 
                 "\x09\x33\x0c\x33\xf7\x11\x47\xe8\x3d\x19\x2f\xc7\x82\xcd\x1b"
                 "\x47\x53\x11\x1b\x17\x3b\x3b\x05\xd2\x2f\xa0\x80\x86\xe3\xb0"
                 "\xf7\x12\xfc\xc7\xc7\x1a\x55\x7e\x2d\xb9\x66\xc3\xe9\xfa\x91"
                 "\x74\x60\x39")
    };

    int times( sizeof(test_sha) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        sha.Update(test_sha[i].input_, test_sha[i].inLen_);
        sha.Final(hash);

        if (memcmp(hash, test_sha[i].output_, SHA384::DIGEST_SIZE) != 0)
            return -1 - i;
    }

    return 0;
}

#endif // WORD64_AVAILABLE


int sha224_test()
{
    SHA224 sha;
    byte   hash[SHA224::DIGEST_SIZE];

    testVector test_sha[] =
    {
        testVector("abc",
                 "\x23\x09\x7d\x22\x34\x05\xd8\x22\x86\x42\xa4\x77\xbd\xa2\x55"
                 "\xb3\x2a\xad\xbc\xe4\xbd\xa0\xb3\xf7\xe3\x6c\x9d\xa7"),
        testVector("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                 "\x75\x38\x8b\x16\x51\x27\x76\xcc\x5d\xba\x5d\xa1\xfd\x89\x01"
                 "\x50\xb0\xc6\x45\x5c\xb4\xf5\x8b\x19\x52\x52\x25\x25")
    };

    int times( sizeof(test_sha) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        sha.Update(test_sha[i].input_, test_sha[i].inLen_);
        sha.Final(hash);

        if (memcmp(hash, test_sha[i].output_, SHA224::DIGEST_SIZE) != 0)
            return -1 - i;
    }

    return 0;
}


int md5_test()
{
    MD5  md5;
    byte hash[MD5::DIGEST_SIZE];

    testVector test_md5[] =
    {
        testVector("abc", 
                 "\x90\x01\x50\x98\x3c\xd2\x4f\xb0\xd6\x96\x3f\x7d\x28\xe1\x7f"
                 "\x72"),
        testVector("message digest", 
                 "\xf9\x6b\x69\x7d\x7c\xb7\x93\x8d\x52\x5a\x2f\x31\xaa\xf1\x61"
                 "\xd0"),
        testVector("abcdefghijklmnopqrstuvwxyz",
                 "\xc3\xfc\xd3\xd7\x61\x92\xe4\x00\x7d\xfb\x49\x6c\xca\x67\xe1"
                 "\x3b"),
        testVector("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345"
                 "6789",
                 "\xd1\x74\xab\x98\xd2\x77\xd9\xf5\xa5\x61\x1c\x2c\x9f\x41\x9d"
                 "\x9f"),
        testVector("1234567890123456789012345678901234567890123456789012345678"
                 "9012345678901234567890",
                 "\x57\xed\xf4\xa2\x2b\xe3\xc9\x55\xac\x49\xda\x2e\x21\x07\xb6"
                 "\x7a")
    };

    int times( sizeof(test_md5) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        md5.Update(test_md5[i].input_, test_md5[i].inLen_);
        md5.Final(hash);

        if (memcmp(hash, test_md5[i].output_, MD5::DIGEST_SIZE) != 0)
            return -5 - i;
    }

    return 0;
}


int md4_test()
{
    MD4  md4;
    byte hash[MD4::DIGEST_SIZE];

    testVector test_md4[] =
    {
        testVector("",
                 "\x31\xd6\xcf\xe0\xd1\x6a\xe9\x31\xb7\x3c\x59\xd7\xe0\xc0\x89"
                 "\xc0"),
        testVector("a",
                 "\xbd\xe5\x2c\xb3\x1d\xe3\x3e\x46\x24\x5e\x05\xfb\xdb\xd6\xfb"
                 "\x24"),
        testVector("abc", 
                 "\xa4\x48\x01\x7a\xaf\x21\xd8\x52\x5f\xc1\x0a\xe8\x7a\xa6\x72"
                 "\x9d"),
        testVector("message digest", 
                 "\xd9\x13\x0a\x81\x64\x54\x9f\xe8\x18\x87\x48\x06\xe1\xc7\x01"
                 "\x4b"),
        testVector("abcdefghijklmnopqrstuvwxyz",
                 "\xd7\x9e\x1c\x30\x8a\xa5\xbb\xcd\xee\xa8\xed\x63\xdf\x41\x2d"
                 "\xa9"),
        testVector("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345"
                 "6789",
                 "\x04\x3f\x85\x82\xf2\x41\xdb\x35\x1c\xe6\x27\xe1\x53\xe7\xf0"
                 "\xe4"),
        testVector("1234567890123456789012345678901234567890123456789012345678"
                 "9012345678901234567890",
                 "\xe3\x3b\x4d\xdc\x9c\x38\xf2\x19\x9c\x3e\x7b\x16\x4f\xcc\x05"
                 "\x36")
    };

    int times( sizeof(test_md4) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        md4.Update(test_md4[i].input_, test_md4[i].inLen_);
        md4.Final(hash);

        if (memcmp(hash, test_md4[i].output_, MD4::DIGEST_SIZE) != 0)
            return -5 - i;
    }

    return 0;
}


int md2_test()
{
    MD2  md5;
    byte hash[MD2::DIGEST_SIZE];

    testVector test_md2[] =
    {
        testVector("",
                   "\x83\x50\xe5\xa3\xe2\x4c\x15\x3d\xf2\x27\x5c\x9f\x80\x69"
                   "\x27\x73"),
        testVector("a",
                   "\x32\xec\x01\xec\x4a\x6d\xac\x72\xc0\xab\x96\xfb\x34\xc0"
                   "\xb5\xd1"),
        testVector("abc",
                   "\xda\x85\x3b\x0d\x3f\x88\xd9\x9b\x30\x28\x3a\x69\xe6\xde"
                   "\xd6\xbb"),
        testVector("message digest",
                   "\xab\x4f\x49\x6b\xfb\x2a\x53\x0b\x21\x9f\xf3\x30\x31\xfe"
                   "\x06\xb0"),
        testVector("abcdefghijklmnopqrstuvwxyz",
                   "\x4e\x8d\xdf\xf3\x65\x02\x92\xab\x5a\x41\x08\xc3\xaa\x47"
                   "\x94\x0b"),
        testVector("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                   "0123456789",
                   "\xda\x33\xde\xf2\xa4\x2d\xf1\x39\x75\x35\x28\x46\xc3\x03"
                   "\x38\xcd"),
        testVector("12345678901234567890123456789012345678901234567890123456"
                   "789012345678901234567890",
                   "\xd5\x97\x6f\x79\xd8\x3d\x3a\x0d\xc9\x80\x6c\x3c\x66\xf3"
                   "\xef\xd8")
    };

    int times( sizeof(test_md2) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        md5.Update(test_md2[i].input_, test_md2[i].inLen_);
        md5.Final(hash);

        if (memcmp(hash, test_md2[i].output_, MD2::DIGEST_SIZE) != 0)
            return -10 - i;
    }

    return 0;
}


int ripemd_test()
{
    RIPEMD160  ripe160;
    byte hash[RIPEMD160::DIGEST_SIZE];

    testVector test_ripemd[] =
    {
        testVector("",
                   "\x9c\x11\x85\xa5\xc5\xe9\xfc\x54\x61\x28\x08\x97\x7e\xe8"
                   "\xf5\x48\xb2\x25\x8d\x31"),
        testVector("a",
                   "\x0b\xdc\x9d\x2d\x25\x6b\x3e\xe9\xda\xae\x34\x7b\xe6\xf4"
                   "\xdc\x83\x5a\x46\x7f\xfe"),
        testVector("abc",
                   "\x8e\xb2\x08\xf7\xe0\x5d\x98\x7a\x9b\x04\x4a\x8e\x98\xc6"
                   "\xb0\x87\xf1\x5a\x0b\xfc"),
        testVector("message digest",
                   "\x5d\x06\x89\xef\x49\xd2\xfa\xe5\x72\xb8\x81\xb1\x23\xa8"
                   "\x5f\xfa\x21\x59\x5f\x36"),
        testVector("abcdefghijklmnopqrstuvwxyz",
                   "\xf7\x1c\x27\x10\x9c\x69\x2c\x1b\x56\xbb\xdc\xeb\x5b\x9d"
                   "\x28\x65\xb3\x70\x8d\xbc"),
        testVector("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                   "\x12\xa0\x53\x38\x4a\x9c\x0c\x88\xe4\x05\xa0\x6c\x27\xdc"
                   "\xf4\x9a\xda\x62\xeb\x2b"),
        testVector("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123"
                   "456789",
                   "\xb0\xe2\x0b\x6e\x31\x16\x64\x02\x86\xed\x3a\x87\xa5\x71"
                   "\x30\x79\xb2\x1f\x51\x89"),
        testVector("12345678901234567890123456789012345678901234567890123456"
                   "789012345678901234567890",
                   "\x9b\x75\x2e\x45\x57\x3d\x4b\x39\xf4\xdb\xd3\x32\x3c\xab"
                   "\x82\xbf\x63\x32\x6b\xfb"),
    };

    int times( sizeof(test_ripemd) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        ripe160.Update(test_ripemd[i].input_, test_ripemd[i].inLen_);
        ripe160.Final(hash);

        if (memcmp(hash, test_ripemd[i].output_, RIPEMD160::DIGEST_SIZE) != 0)
            return -100 - i;
    }

    return 0;
}


int hmac_test()
{
    HMAC<MD5> hmacMD5;
    byte hash[MD5::DIGEST_SIZE];

    const char* keys[]=
    {
        "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b",
        "Jefe",
        "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
    };

    testVector test_hmacMD5[] = 
    {
        testVector("Hi There",
                 "\x92\x94\x72\x7a\x36\x38\xbb\x1c\x13\xf4\x8e\xf8\x15\x8b\xfc"
                 "\x9d"),
        testVector("what do ya want for nothing?",
                 "\x75\x0c\x78\x3e\x6a\xb0\xb5\x03\xea\xa8\x6e\x31\x0a\x5d\xb7"
                 "\x38"),
        testVector("\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
                 "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
                 "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
                 "\xDD\xDD\xDD\xDD\xDD\xDD",
                 "\x56\xbe\x34\x52\x1d\x14\x4c\x88\xdb\xb8\xc7\x33\xf0\xe8\xb3"
                 "\xf6")
    };

    int times( sizeof(test_hmacMD5) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        hmacMD5.SetKey((byte*)keys[i], (word32)strlen(keys[i]));
        hmacMD5.Update(test_hmacMD5[i].input_, test_hmacMD5[i].inLen_);
        hmacMD5.Final(hash);

        if (memcmp(hash, test_hmacMD5[i].output_, MD5::DIGEST_SIZE) != 0)
            return -20 - i;
    }

    return 0;
}


int arc4_test()
{
    byte cipher[16];
    byte plain[16];

    const char* keys[] = 
    {           
        "\x01\x23\x45\x67\x89\xab\xcd\xef",
        "\x01\x23\x45\x67\x89\xab\xcd\xef",
        "\x00\x00\x00\x00\x00\x00\x00\x00",
        "\xef\x01\x23\x45"
    };

    testVector test_arc4[] =
    {
        testVector("\x01\x23\x45\x67\x89\xab\xcd\xef",
                   "\x75\xb7\x87\x80\x99\xe0\xc5\x96"),
        testVector("\x00\x00\x00\x00\x00\x00\x00\x00",
                   "\x74\x94\xc2\xe7\x10\x4b\x08\x79"),
        testVector("\x00\x00\x00\x00\x00\x00\x00\x00",
                   "\xde\x18\x89\x41\xa3\x37\x5d\x3a"),
        testVector("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
                   "\xd6\xa1\x41\xa7\xec\x3c\x38\xdf\xbd\x61")
    };


    int times( sizeof(test_arc4) / sizeof(testVector) );
    for (int i = 0; i < times; ++i) {
        ARC4::Encryption enc;
        ARC4::Decryption dec;

        enc.SetKey((byte*)keys[i], (word32)strlen(keys[i]));
        dec.SetKey((byte*)keys[i], (word32)strlen(keys[i]));

        enc.Process(cipher, test_arc4[i].input_, test_arc4[i].outLen_);
        dec.Process(plain,  cipher, test_arc4[i].outLen_);

        if (memcmp(plain, test_arc4[i].input_, test_arc4[i].outLen_))
            return -30 - i;

        if (memcmp(cipher, test_arc4[i].output_, test_arc4[i].outLen_))
            return -40 - i;
    }

    return 0;
}


int des_test()
{
    //ECB mode
    DES_ECB_Encryption enc;
    DES_ECB_Decryption dec;

    const int sz = TaoCrypt::DES_BLOCK_SIZE * 3;
    const byte key[] = { 0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef };
    const byte iv[] =  { 0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef };

    enc.SetKey(key, sizeof(key));
    enc.Process(cipher, msg, sz);
    dec.SetKey(key, sizeof(key));
    dec.Process(plain, cipher, sz);

    if (memcmp(plain, msg, sz))
        return -50;

    const byte verify1[] = 
    {
        0xf9,0x99,0xb8,0x8e,0xaf,0xea,0x71,0x53,
        0x6a,0x27,0x17,0x87,0xab,0x88,0x83,0xf9,
        0x89,0x3d,0x51,0xec,0x4b,0x56,0x3b,0x53
    };

    if (memcmp(cipher, verify1, sz))
        return -51;

    // CBC mode
    DES_CBC_Encryption enc2;
    DES_CBC_Decryption dec2;

    enc2.SetKey(key, sizeof(key), iv);
    enc2.Process(cipher, msg, sz);
    dec2.SetKey(key, sizeof(key), iv);
    dec2.Process(plain, cipher, sz);

    if (memcmp(plain, msg, sz))
        return -52;

    const byte verify2[] = 
    {
        0x8b,0x7c,0x52,0xb0,0x01,0x2b,0x6c,0xb8,
        0x4f,0x0f,0xeb,0xf3,0xfb,0x5f,0x86,0x73,
        0x15,0x85,0xb3,0x22,0x4b,0x86,0x2b,0x4b
    };

    if (memcmp(cipher, verify2, sz))
        return -53;

    // EDE3 CBC mode
    DES_EDE3_CBC_Encryption enc3;
    DES_EDE3_CBC_Decryption dec3;

    const byte key3[] = 
    {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
        0xfe,0xde,0xba,0x98,0x76,0x54,0x32,0x10,
        0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67
    };
    const byte iv3[] = 
    {
        0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef,
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
        0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81
        
    };

    enc3.SetKey(key3, sizeof(key3), iv3);
    enc3.Process(cipher, msg, sz);
    dec3.SetKey(key3, sizeof(key3), iv3);
    dec3.Process(plain, cipher, sz);

    if (memcmp(plain, msg, sz))
        return -54;

    const byte verify3[] = 
    {
        0x08,0x8a,0xae,0xe6,0x9a,0xa9,0xc1,0x13,
        0x93,0x7d,0xf7,0x3a,0x11,0x56,0x66,0xb3,
        0x18,0xbc,0xbb,0x6d,0xd2,0xb1,0x16,0xda
    };

    if (memcmp(cipher, verify3, sz))
        return -55;

    return 0;
}


int aes_test()
{
    AES_CBC_Encryption enc;
    AES_CBC_Decryption dec;
    const int bs(TaoCrypt::AES::BLOCK_SIZE);

    byte key[] = "0123456789abcdef   ";  // align
    byte iv[]  = "1234567890abcdef   ";  // align

    enc.SetKey(key, bs, iv);
    dec.SetKey(key, bs, iv);

    enc.Process(cipher, msg, bs);
    dec.Process(plain, cipher, bs);

    if (memcmp(plain, msg, bs))
        return -60;

    const byte verify[] = 
    {
        0x95,0x94,0x92,0x57,0x5f,0x42,0x81,0x53,
        0x2c,0xcc,0x9d,0x46,0x77,0xa2,0x33,0xcb
    };

    if (memcmp(cipher, verify, bs))
        return -61;

    AES_ECB_Encryption enc2;
    AES_ECB_Decryption dec2;

    enc2.SetKey(key, bs, iv);
    dec2.SetKey(key, bs, iv);

    enc2.Process(cipher, msg, bs);
    dec2.Process(plain, cipher, bs);

    if (memcmp(plain, msg, bs))
        return -62;

    const byte verify2[] = 
    {
        0xd0,0xc9,0xd9,0xc9,0x40,0xe8,0x97,0xb6,
        0xc8,0x8c,0x33,0x3b,0xb5,0x8f,0x85,0xd1
    };

    if (memcmp(cipher, verify2, bs))
        return -63;

    return 0;
}


int twofish_test()
{
    Twofish_CBC_Encryption enc;
    Twofish_CBC_Decryption dec;
    const int bs(TaoCrypt::Twofish::BLOCK_SIZE);

    byte key[] = "0123456789abcdef   ";  // align
    byte iv[]  = "1234567890abcdef   ";  // align

    enc.SetKey(key, bs, iv);
    dec.SetKey(key, bs, iv);

    enc.Process(cipher, msg, bs);
    dec.Process(plain, cipher, bs);

    if (memcmp(plain, msg, bs))
        return -60;

    const byte verify[] = 
    {
        0xD2,0xD7,0x47,0x47,0x4A,0x65,0x4E,0x16,
        0x21,0x03,0x58,0x79,0x5F,0x02,0x27,0x2C
    };

    if (memcmp(cipher, verify, bs))
        return -61;

    Twofish_ECB_Encryption enc2;
    Twofish_ECB_Decryption dec2;

    enc2.SetKey(key, bs, iv);
    dec2.SetKey(key, bs, iv);

    enc2.Process(cipher, msg, bs);
    dec2.Process(plain, cipher, bs);

    if (memcmp(plain, msg, bs))
        return -62;

    const byte verify2[] = 
    {
        0x3B,0x6C,0x63,0x10,0x34,0xAB,0xB2,0x87,
        0xC4,0xCD,0x6B,0x91,0x14,0xC5,0x3A,0x09
    };

    if (memcmp(cipher, verify2, bs))
        return -63;

    return 0;
}


int blowfish_test()
{
    Blowfish_CBC_Encryption enc;
    Blowfish_CBC_Decryption dec;
    const int bs(TaoCrypt::Blowfish::BLOCK_SIZE);

    byte key[] = "0123456789abcdef   ";  // align
    byte iv[]  = "1234567890abcdef   ";  // align

    enc.SetKey(key, 16, iv);
    dec.SetKey(key, 16, iv);

    enc.Process(cipher, msg, bs * 2);
    dec.Process(plain, cipher, bs * 2);

    if (memcmp(plain, msg, bs))
        return -60;

    const byte verify[] = 
    {
        0x0E,0x26,0xAA,0x29,0x11,0x25,0xAB,0xB5,
        0xBC,0xD9,0x08,0xC4,0x94,0x6C,0x89,0xA3
    };

    if (memcmp(cipher, verify, bs))
        return -61;

    Blowfish_ECB_Encryption enc2;
    Blowfish_ECB_Decryption dec2;

    enc2.SetKey(key, 16, iv);
    dec2.SetKey(key, 16, iv);

    enc2.Process(cipher, msg, bs * 2);
    dec2.Process(plain, cipher, bs * 2);

    if (memcmp(plain, msg, bs))
        return -62;

    const byte verify2[] = 
    {
        0xE7,0x42,0xB9,0x37,0xC8,0x7D,0x93,0xCA,
        0x8F,0xCE,0x39,0x32,0xDE,0xD7,0xBC,0x5B
    };

    if (memcmp(cipher, verify2, bs))
        return -63;

    return 0;
}


int rsa_test()
{
    Source source;
    FileSource("../certs/client-key.der", source);
    if (source.size() == 0) {
        FileSource("../../certs/client-key.der", source);  // for testsuite
        if (source.size() == 0) {
            FileSource("../../../certs/client-key.der", source); // Debug dir
            if (source.size() == 0)
                err_sys("where's your certs dir?", -79);
        }
    }
    RSA_PrivateKey priv(source);

    RSAES_Encryptor enc(priv);
    byte message[] = "Everyone gets Friday off.";
    const word32 len = (word32)strlen((char*)message);
    byte cipher[64];
    enc.Encrypt(message, len, cipher, rng);

    RSAES_Decryptor dec(priv);
    byte plain[64];
    dec.Decrypt(cipher, sizeof(plain), plain, rng);

    if (memcmp(plain, message, len))
        return -70;

    dec.SSL_Sign(message, len, cipher, rng);
    if (!enc.SSL_Verify(message, len, cipher))
        return -71;


    // test decode   
    Source source2;
    FileSource("../certs/client-cert.der", source2);
    if (source2.size() == 0) {
        FileSource("../../certs/client-cert.der", source2);  // for testsuite
        if (source2.size() == 0) {
            FileSource("../../../certs/client-cert.der", source2); // Debug dir
            if (source2.size() == 0)
                err_sys("where's your certs dir?", -79);
        }
    }
    CertDecoder cd(source2, true, 0, false, CertDecoder::CA);
    if (cd.GetError().What())
        err_sys("cert error", -80);
    Source source3(cd.GetPublicKey().GetKey(), cd.GetPublicKey().size());
    RSA_PublicKey pub(source3);
 
    return 0;
}


int dh_test()
{
    Source source;
    FileSource("../certs/dh1024.dat", source);
    if (source.size() == 0) {
        FileSource("../../certs/dh1024.dat", source);  // for testsuite
        if (source.size() == 0) {
            FileSource("../../../certs/dh1024.dat", source); // win32 Debug dir
            if (source.size() == 0)
                err_sys("where's your certs dir?", -79);
        }
    }
    HexDecoder hDec(source);

    DH dh(source);

    byte pub[128];
    byte priv[128];
    byte agree[128];
    byte pub2[128];
    byte priv2[128];
    byte agree2[128];

    DH dh2(dh);

    dh.GenerateKeyPair(rng, priv, pub);
    dh2.GenerateKeyPair(rng, priv2, pub2);
    dh.Agree(agree, priv, pub2); 
    dh2.Agree(agree2, priv2, pub);

    
    if ( memcmp(agree, agree2, dh.GetByteLength()) )
        return -80;

    return 0;
}


int dsa_test()
{
    Source source;
    FileSource("../certs/dsa512.der", source);
    if (source.size() == 0) {
        FileSource("../../certs/dsa512.der", source);  // for testsuite
        if (source.size() == 0) {
            FileSource("../../../certs/dsa512.der", source); // win32 Debug dir
            if (source.size() == 0)
                err_sys("where's your certs dir?", -89);
        }
    }

    const char msg[] = "this is the message";
    byte signature[40];

    DSA_PrivateKey priv(source);
    DSA_Signer signer(priv);

    SHA sha;
    byte digest[SHA::DIGEST_SIZE];
    sha.Update((byte*)msg, sizeof(msg));
    sha.Final(digest);

    signer.Sign(digest, signature, rng);

    byte encoded[sizeof(signature) + 6];
    byte decoded[40];

    word32 encSz = EncodeDSA_Signature(signer.GetR(), signer.GetS(), encoded);
    DecodeDSA_Signature(decoded, encoded, encSz);

    DSA_PublicKey pub(priv);
    DSA_Verifier verifier(pub);

    if (!verifier.Verify(digest, decoded))
        return -90;

    return 0;
}


int pwdbased_test()
{
    PBKDF2_HMAC<SHA> pb;

    byte derived[32];
    const byte pwd1[] = "password   ";  // align
    const byte salt[]  = { 0x12, 0x34, 0x56, 0x78, 0x78, 0x56, 0x34, 0x12 };
    
    pb.DeriveKey(derived, 8, pwd1, 8, salt, sizeof(salt), 5);

    const byte verify1[] = { 0xD1, 0xDA, 0xA7, 0x86, 0x15, 0xF2, 0x87, 0xE6 };

    if ( memcmp(derived, verify1, sizeof(verify1)) )
        return -101;


    const byte pwd2[] = "All n-entities must communicate with other n-entities"
                        " via n-1 entiteeheehees   ";  // align

    pb.DeriveKey(derived, 24, pwd2, 76, salt, sizeof(salt), 500);

    const byte verify2[] = { 0x6A, 0x89, 0x70, 0xBF, 0x68, 0xC9, 0x2C, 0xAE,
                             0xA8, 0x4A, 0x8D, 0xF2, 0x85, 0x10, 0x85, 0x86,
                             0x07, 0x12, 0x63, 0x80, 0xCC, 0x47, 0xAB, 0x2D
    };

    if ( memcmp(derived, verify2, sizeof(verify2)) )
        return -102;

    return 0;
}


int pkcs12_test()
{
    Source cert;
    FileSource("../certs/server-cert.pem", cert);
    if (cert.size() == 0) {
        FileSource("../../certs/server-cert.pem", cert);  // for testsuite
        if (cert.size() == 0) {
            FileSource("../../../certs/server-cert.pem", cert); // Debug dir
            if (cert.size() == 0)
                err_sys("where's your certs dir?", -109);
        }
    }

    if (GetCert(cert) != 0)
        return -110;

    Source source;
    FileSource("../certs/server.p12", source);
    if (source.size() == 0) {
        FileSource("../../certs/server.p12", source);  // for testsuite
        if (source.size() == 0) {
            FileSource("../../../certs/server.p12", source); // Debug dir
            if (source.size() == 0)
                err_sys("where's your certs dir?", -111);
        }
    }

    if (GetPKCS_Cert("password", source) != 0)
        return -112;

    return 0;
}

