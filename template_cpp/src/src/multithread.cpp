#include "multithread.hpp"

pthread_mutex_t mutex;
pthread_cond_t cond;

void* send_thread(void* arg)
{
    struct complex_args* args = static_cast<struct complex_args*>(arg);
    int em_id = args->em_id_;
    Parser & parser = args->parser_;
    
    for (int m = 1; m <= parser.message_to_send; m++)
    {
        broadcast_fifo(em_id, parser, m);
    }

    int *result = static_cast<int*>(malloc(sizeof(int)));
    *result = 0;
    return result;
}

void* recv_thread(void* arg)
{
    struct complex_args* args = static_cast<struct complex_args*>(arg);
    int em_id = args->em_id_;
    Parser & parser = args->parser_;
    
    receiverPerfectLinks(em_id, parser);

    int *result = static_cast<int*>(malloc(sizeof(int)));
    *result = 0;
    return result;
}

void thread_run(int em_id, Parser &parser)
{
    pthread_t sendThread, recvThread;
    void* sendThread_return;
    void* recvThread_return;

    // Initialize the mutex for thread synchronization
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);

    struct complex_args arg = {em_id, parser};

    while (1) {
        pthread_create(&sendThread, nullptr, send_thread, *arg);
        parser.writeConsole("pthread_create sendThread");

        pthread_create(&recvThread, nullptr, recv_thread, *arg);
        parser.writeConsole("pthread_create recvThread");

        // Wait for the threads to finish (you can implement a termination condition)
        pthread_join(sendThread, &sendThread_return);

        pthread_join(recvThread, &recvThread_return);
        
        // int result = *(int *)sendThread_return;
    }

    // Cleanup
    pthread_mutex_destroy(&mutex);
}