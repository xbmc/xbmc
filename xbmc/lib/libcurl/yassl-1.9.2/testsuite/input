// testsuite.cpp

#include "test.hpp"
#include "md5.hpp"

typedef unsigned char byte;

void taocrypt_test(void*);
void file_test(char*, byte*);

void client_test(void*);
void echoclient_test(void*);

THREAD_RETURN YASSL_API server_test(void*);
THREAD_RETURN YASSL_API echoserver_test(void*);

int main(int argc, char** argv)
{
    func_args args(argc, argv);
    func_args server_args(args);

    // *** Crypto Test ***
    taocrypt_test(&args);
    assert(args.return_code == 0);
    
    
    // *** Simple yaSSL client server test ***
    THREAD_TYPE thread;

    start_thread(server_test, &server_args, &thread);
    client_test(&args);

    assert(args.return_code == 0);
    join_thread(thread);
    assert(server_args.return_code == 0);
    

    // *** Echo input yaSSL client server test ***
    start_thread(echoserver_test, &server_args, &thread);
    func_args echo_args;

            // setup args
    echo_args.argc = 3;
    echo_args.argv = new char*[echo_args.argc];
    for (int i = 0; i < echo_args.argc; i++)
        echo_args.argv[i] = new char[32];
   
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
    join_thread(thread);
    assert(server_args.return_code == 0);


            // input output compare
    byte input[TaoCrypt::MD5::DIGEST_SIZE];
    byte output[TaoCrypt::MD5::DIGEST_SIZE];
    file_test("input", input);
    file_test("output", output);
    assert(memcmp(input, output, sizeof(input)) == 0);

    printf("\nAll tests passed!\n");

    // cleanup
    for (int j = echo_args.argc; j >= 0; j--)
        delete[] echo_args.argv[j];
    delete[] echo_args.argv;

    return 0;
}



void start_thread(THREAD_FUNC fun, func_args* args, THREAD_TYPE* thread)
{
#ifdef _WIN32
    *thread = _beginthreadex(0, 0, fun, args, 0, 0);
#else
    pthread_create(thread, 0, fun, args);
#endif
}


void join_thread(THREAD_TYPE thread)
{
#ifdef _WIN32
    int res = WaitForSingleObject(reinterpret_cast<HANDLE>(thread), INFINITE);
    assert(res == WAIT_OBJECT_0);
    res = CloseHandle(reinterpret_cast<HANDLE>(thread));
    assert(res);
#else
    pthread_join(thread, 0);
#endif
}
