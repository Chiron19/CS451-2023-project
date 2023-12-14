#include "multithread.hpp"

pthread_mutex_t mutex;
pthread_cond_t cond;

void* send_thread(void* arg)
{
    struct complex_args* args = static_cast<struct complex_args*>(arg);
    int em_id = args->em_id_;
    Parser & parser = args->parser_;
    
    // for (int i = 0; i < parser.message_to_send; i++)
    // {
    //     init_lattice(em_id, parser);
    //     propose_lattice(em_id, parser, parser.proposals_lattice[i]);
    //     for (;;) {
    //         if (!parser.active_lattice) {
    //             std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //             break;
    //         }
    //     }
    // }

    // Testing with urb
    for (int k = 1;; k = 0)
    {
        for (int m = 1; m <= parser.message_to_send; m++)
        {
            broadcast_urb(em_id, parser, std::to_string(m));
        }
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

void* ack_thread(void* arg)
{
    struct complex_args* args = static_cast<struct complex_args*>(arg);
    int em_id = args->em_id_;
    Parser & parser = args->parser_;
    
    ackPerfectLinks(em_id, parser);

    int *result = static_cast<int*>(malloc(sizeof(int)));
    *result = 0;
    return result;
}

void thread_run(int em_id, Parser &parser)
{
    pthread_t sendThread, recvThread, ackThread;
    void* sendThread_return;
    void* recvThread_return;
    void* ackThread_return;

    // Initialize the mutex for thread synchronization
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);

    struct complex_args arg = {em_id, parser};

    while (1) {
        pthread_create(&sendThread, nullptr, send_thread, &arg);
        // parser.writeConsole("pthread_create sendThread");

        pthread_create(&recvThread, nullptr, recv_thread, &arg);

        pthread_create(&ackThread, nullptr, ack_thread, &arg);
        // parser.writeConsole("pthread_create recvThread");

        // Wait for the threads to finish (you can implement a termination condition)
        pthread_join(sendThread, &sendThread_return);

        pthread_join(recvThread, &recvThread_return);

        pthread_join(ackThread, &ackThread_return);
        
        // int result = *(int *)sendThread_return;
    }

    // Cleanup
    pthread_mutex_destroy(&mutex);
}