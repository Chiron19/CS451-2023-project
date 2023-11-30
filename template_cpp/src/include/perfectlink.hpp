#pragma once

#ifdef __cplusplus

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <set>
#include <chrono>

#include "parser.hpp"
#include "udp.hpp"
#include "uniformreliablebroadcast.hpp"
#include "fifobroadcast.hpp"

void initPerfectLink(int em_id, Parser parser);

void receiverPerfectLinks(int em_id, Parser parser);

void senderPerfectLinks(int src_em_id, int dst_em_id, Parser parser, std::string buffer);

#endif