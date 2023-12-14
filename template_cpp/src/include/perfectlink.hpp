#pragma once

#ifdef __cplusplus

#ifndef PERFECTLINK_H
#define PERFECTLINK_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <set>
#include <thread>
#include <chrono>

#include "parser.hpp"
#include "udp.hpp"
#include "uniformreliablebroadcast.hpp"
#include "fifobroadcast.hpp"
#include "multithread.hpp"
#include "latticeagreement.hpp"

/* 
    Introduction to Reliable and Secure Distributed Programming, 2nd edition
    Christian Cachin, Rachid Guerraoui, al.

    Page 36, Algorithm 2.1: Retransmit Forever
    Page 38, Algorithm 2.2: Eliminate Duplicates
    Page 41, Algorithm 2.3: Log Delivered
*/
void initPerfectLink(int em_id, Parser &parser);

void receiverPerfectLinks(int em_id, Parser &parser);

void senderPerfectLinks(int src_em_id, int dst_em_id, Parser &parser, std::string & buffer);

void ackPerfectLinks(int em_id, Parser & parser);

#endif
#endif