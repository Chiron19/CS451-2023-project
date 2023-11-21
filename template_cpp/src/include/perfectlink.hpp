#pragma once

#ifdef __cplusplus

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <set>
#include <chrono>

#include "parser.hpp"
#include "udp.hpp"

std::string format_time_point(const std::chrono::system_clock::time_point& tp);

std::string format_duration(const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end);

void receiverPerfectLinks(int em_id, Parser parser);

void senderPerfectLinks(int em_id, Parser parser);

#endif