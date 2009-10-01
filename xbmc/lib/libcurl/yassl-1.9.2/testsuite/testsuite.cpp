// testsuite.cpp

#include "test.hpp"
#include "md5.hpp"


typedef unsigned char byte;

void taocrypt_test(void*);
void file_test(const char*, byte*);

void client_test(void*);
void echoclient_test(void*);

THREAD_RETURN YASSL_API server_test(void*);
THREAD_RETURN YASSL_API echoserver_test(void*);

void wait_tcp_ready(func_args&);



int main(int argc, char** argv)
{
    func_args args(argc, argv);
    func_args server_args(argc, argv);

    // *** Crypto Test ***
    taocrypt_test(&args);
    assert(args.return_code == 0);
    
    
    // *** Simple yaSSL client server test ***
    tcp_ready ready;
    server_args.SetSignal(&ready);

    THREAD_TYPE serverThread;
    start_thread(server_test, &server_args, &serverThread);
    wait_tcp_ready(server_args);

    client_test(&args);
    assert(args.return_code == 0);
    join_thread(serverThread);
    assert(server_args.return_code == 0);
    

    // *** Echo input yaSSL client server test ***
    start_thread(echoserver_test, &server_args, &serverThread);
    wait_tcp_ready(server_args);
    func_args echo_args;

            // setup args
    const int numArgs = 3;
    echo_args.argc = numArgs;
    char* myArgv[numArgs];

    char argc0[32];
    char argc1[32];
    char argc2[32];

    myArgv[0] = argc0;
    myArgv[1] = argc1;
    myArgv[2] = argc2;

    echo_args.argv = myArgv;
   
    strcpy(echo_args.argv[0], "echoclient");
    strcpy(echo_args.argv[1], "input");
    strcpy(echo_args.argv[2], "output");
    remove("output");

            // make sure OK
    echoclient_test(&echo_args);
    assert(echo_args.return_code == 0);


    // *** Echo quit yaSSL client server test ***
    echo_args.argc = 2;
    strcpy(echo_args.argv[1], "quit");

    echoclient_test(&echo_args);
    assert(echo_args.return_code == 0);
    join_thread(serverThread);
    assert(server_args.return_code == 0);


            // input output compare
    byte input[TaoCrypt::MD5::DIGEST_SIZE];
    byte output[TaoCrypt::MD5::DIGEST_SIZE];
    file_test("input", input);
    file_test("output", output);
    assert(memcmp(input, output, sizeof(input)) == 0);

    printf("\nAll tests passed!\n");
    yaSSL_CleanUp();

    return 0;
}



void start_thread(THREAD_FUNC fun, func_args* args, THREAD_TYPE* thread)
{
#ifndef _POSIX_THREADS
    *thread = (HANDLE)_beginthreadex(0, 0, fun, args, 0, 0);
#else
    pthread_create(thread, 0, fun, args);
#endif
}


void join_thread(THREAD_TYPE thread)
{
#ifndef _POSIX_THREADS
    int res = WaitForSingleObject(thread, INFINITE);
    assert(res == WAIT_OBJECT_0);
    res = CloseHandle(thread);
    assert(res);
#else
    pthread_join(thread, 0);
#endif
}



void wait_tcp_ready(func_args& args)
{
#ifdef _POSIX_THREADS
    pthread_mutex_lock(&args.signal_->mutex_);
    
    if (!args.signal_->ready_)
        pthread_cond_wait(&args.signal_->cond_, &args.signal_->mutex_);
    args.signal_->ready_ = false; // reset

    pthread_mutex_unlock(&args.signal_->mutex_);
#endif
}


int test_openSSL_des()
{
    /* test des encrypt/decrypt */
    char data[] = "this is my data ";
    int  dataSz = (int)strlen(data);
    DES_key_schedule key[3];
    byte iv[8];
    EVP_BytesToKey(EVP_des_ede3_cbc(), EVP_md5(), NULL, (byte*)data, dataSz, 1,
                   (byte*)key, iv);

    byte cipher[16];
    DES_ede3_cbc_encrypt((byte*)data, cipher, dataSz, &key[0], &key[1],
                         &key[2], &iv, true);
    byte plain[16];
    DES_ede3_cbc_encrypt(cipher, plain, 16, &key[0], &key[1], &key[2],
                         &iv, false);
    return 0;
}
