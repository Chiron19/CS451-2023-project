#pragma once

#ifdef __cplusplus

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <set>
#include <chrono>

#include "parser.hpp"
#include "udp.hpp"
#include "perfectlink.hpp"

/* 
    Reliable and Secure Distributed Programming, 2nd edition
    Christian Cachin, Rachid Guerraoui, al.

    Page 83, Algorithm 3.4: All-Ack Uniform Reliable Broadcast
    Page 85, Algorithm 3.5: Majority-Ack Uniform Reliable Broadcast
*/
void init_urb(Parser parser);

void broadcast_urb(int em_id, Parser parser, std::string buffer);

void deliver_urb(int em_id, Parser parser, message_t mes, int m);

bool candeliver_urb(int m, Parser parser);

#endif