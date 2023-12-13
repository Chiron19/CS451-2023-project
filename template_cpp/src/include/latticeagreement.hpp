#pragma once

#ifdef __cplusplus

#ifndef LATTICEAGREEMENT_H
#define LATTICEAGREEMENT_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <set>
#include <tuple>
#include <algorithm>
#include <chrono>
#include <exception>

#include "parser.hpp"
#include "udp.hpp"
#include "perfectlink.hpp"

/* 
    Lattice Agreement
*/

std::string format_proposal_buffer_lattice(std::set<int>& p, int k);
std::string format_ack_buffer_lattice(int k);
std::string format_nack_buffer_lattice(std::set<int>& p, int k);

std::tuple<char, std::set<int>, int> deformat_lattice(const std::string& str);

bool isSubset(const std::set<int>& a, const std::set<int>& b);
std::set<int> setUnion(const std::set<int>& set1, const std::set<int>& set2);

void init_lattice(int em_id, Parser& parser);
void propose_lattice(int em_id, Parser& parser, std::set<int>& proposal_set);
void decide_lattice(int em_id, Parser& parser);

void upon_event_deliver_beb_lattice(int em_id, Parser& parser, message_t mes);
void upon_event_recv_ack_lattice(int em_id, Parser& parser, int prop_num);
void upon_event_recv_nack_lattice(int em_id, Parser& parser, std::set<int>& val, int prop_num);
void upon_event_recv_prop_lattice(int em_id, Parser& parser, std::set<int>& prop_val, int prop_num, int sender_id);

void check_ack_count_lattice(int em_id, Parser& parser);
void check_nack_count_lattice(int em_id, Parser& parser);

#endif
#endif