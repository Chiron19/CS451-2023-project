#pragma once

#ifdef __cplusplus

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <memory>

#include "parser.hpp"
#include "hello.h"
#include "udp.hpp"
#include "perfectlink.hpp"
#include "uniformreliablebroadcast.hpp"
#include "fifobroadcast.hpp"

pthread_mutex_t mutex;
pthread_cond_t cond;

struct complex_args {
    int em_id;
    Parser parser;
};

void* send_thread(void* arg);
void* recv_thread(void* arg);
void thread_run(int em_id, Parser parser);

void* send_thread(void* arg) {
    struct complex_args* args = static_cast<complex_args*>(arg);
    int em_id = args->em_id;
    Parser parser = args->parser;

    int *result = static_cast<int*>(malloc(sizeof(int)));
    *result = 0;

    for (int i = 1; i <= parser.message_to_send; i++)
    {
        broadcast_urb(em_id, parser, std::to_string(i));
    }

    *result = 1;
    return result;
}

void* recv_thread(void* arg) {
    struct complex_args* args = static_cast<complex_args*>(arg);
    int em_id = args->em_id;
    Parser parser = args->parser;

    receiverPerfectLinks(em_id, parser);

    int *result = static_cast<int*>(malloc(sizeof(int)));
    *result = 1;
    return result;
}

// create and run thread for both sender and receiver
void thread_run(int em_id, Parser parser) {
    pthread_t sendThread, recvThread;
    void* sendThread_return;
    void* recvThread_return;

    // Initialize the mutex for thread synchronization
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);
    struct complex_args args = {em_id, parser};

    while (true) {
        // Create the send and receive threads
        pthread_create(&sendThread, nullptr, send_thread, &args);
        parser.writeConsole("pthread_create send_thread");

        pthread_create(&recvThread, nullptr, recv_thread, &args);
        parser.writeConsole("pthread_create recv_thread");

        // Wait for the threads to finish (you can implement a termination condition)
        pthread_join(sendThread, &sendThread_return);
        parser.writeConsole("pthread_join send_thread");

        pthread_join(recvThread, &recvThread_return);
        parser.writeConsole("pthread_join recv_thread");
    }
    // int result = *(int *)sendThread_return;

    // Cleanup
    pthread_mutex_destroy(&mutex);
}

#endif