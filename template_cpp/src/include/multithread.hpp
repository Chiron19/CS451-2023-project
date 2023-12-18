#pragma once

#ifdef __cplusplus

#ifndef MULTITHREAD_H
#define MULTITHREAD_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <pthread.h>

#include "parser.hpp"
#include "hello.h"
#include "udp.hpp"
#include "perfectlink.hpp"
#include "uniformreliablebroadcast.hpp"
#include "fifobroadcast.hpp"

extern pthread_mutex_t mutex;
extern pthread_cond_t cond;

struct complex_args {
    int em_id_;
    Parser &parser_;
};

void* send_thread(void* arg);
void* recv_thread(void* arg);
void* rese_thread(void* arg);
void thread_run(int em_id, Parser &parser);

#endif
#endif