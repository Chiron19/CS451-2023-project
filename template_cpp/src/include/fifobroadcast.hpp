#pragma once

#ifdef __cplusplus

#ifndef FIFOBROADCAST_H
#define FIFOBROADCAST_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <set>
#include <chrono>

#include "parser.hpp"
#include "udp.hpp"
#include "perfectlink.hpp"
#include "uniformreliablebroadcast.hpp"

/* 
    Introduction to Reliable and Secure Distributed Programming, 2nd edition
    Christian Cachin, Rachid Guerraoui, al.

    Page 105, Algorithm 3.13: No-Waiting Causal Broadcast
*/
std::string format_past_fifo(Parser &parser);

int deformat_get_m_fifo(std::string & str);

std::vector<int> deformat_get_m_past_fifo(std::string & str);

void init_fifo(int em_id, Parser &parser);

void broadcast_fifo(int em_id, Parser &parser, int m, bool is_write_outputfile);

void deliver_fifo(int em_id, Parser &parser, int m);

void upon_event_deliver_urb_fifo(int em_id, Parser & parser, message_t mes, int m);

#endif
#endif