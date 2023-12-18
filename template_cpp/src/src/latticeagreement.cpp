#include "latticeagreement.hpp"

/*
    format the input to string buffer as "p1 p2 p3 ..." (separated by spaces)
*/
std::string format_plaintext_lattice(std::set<int> & p)
{
    std::stringstream ss;
    for (auto it = p.begin(); it != p.end(); ++it) {
        if (it != p.begin()) {
            ss << " ";
        }
        ss << *it;
    }
    return ss.str();
}

/*
    format string wrapper with round number r "......?r"
    perform on the original string
    must format before sendPerfectlink()
*/
void format_round_wrapper_lattice(std::string & str, int r)
{
    str = str + "?" + std::to_string(r);
}

/*
    format the input to string buffer as "Pp1,p2,p3/k"
*/
std::string format_proposal_buffer_lattice(std::set<int> & p, int k)
{
    std::stringstream ss;
    for (auto it = p.begin(); it != p.end(); ++it) {
        if (it != p.begin()) {
            ss << ",";
        }
        ss << *it;
    }
    ss << "/" << k;
    return "P" + ss.str();
}

/*
    format the input to string buffer as "A/k"
*/
std::string format_ack_buffer_lattice(int k) {
    return "A/" + std::to_string(k);
}

/*
    format the input to string buffer as "Np1,p2,p3/k"
*/
std::string format_nack_buffer_lattice(std::set<int> & p, int k)
{
    std::stringstream ss;
    for (auto it = p.begin(); it != p.end(); ++it) {
        if (it != p.begin()) {
            ss << ",";
        }
        ss << *it;
    }
    ss << "/" << k;
    return "N" + ss.str();
}

/*
    format the input to string buffer as "Fp/k"
*/
std::string format_finish_lattice(int p, int k) {
    return "F" + std::to_string(p) + "/" + std::to_string(k);
}

/*
    deformat from string wrapper with round number r "......?r"
    must perform before deformat_lattice()
*/
int deformat_round_wrapper_lattice(std::string & str)
{
    // Find the position of "?"
    size_t pos = str.find("?");

    if (pos != std::string::npos) {
        // Extract the number after "?"
        std::string numberStr = str.substr(pos + 1);

        try {
            // Convert the extracted string to an integer
            int roundNumber = std::stoi(numberStr);

            // Erase at and after '?' from the original string
            str.erase(pos);

            // Return the extracted round number
            return roundNumber;
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error: Invalid round number format." << std::endl;
        } catch (const std::out_of_range& e) {
            std::cerr << "Error: Round number out of range." << std::endl;
        }
    } else {
        std::cerr << "Error: Couldn't find '?' in the string." << std::endl;
    }

    // Return a default value if extraction fails
    return -1;
}

/*
    deformat string buffer as {char C, std::set<int> P, int K}
*/
std::tuple<char, std::set<int>, int> deformat_lattice(const std::string& str)
{
    char type;
    std::set<int> parameters;
    int k = 0;

    if (!str.empty()) { // Check if the string is not empty
        std::stringstream ss(str); // Use stringstream to tokenize the string
        ss >> type; // Read the first character to determine the type

        if (std::isupper(type)) { // Determine the format
            int value;
            char delimiter;
            
            if (ss >> delimiter && delimiter == '/')  ss >> k; // Read 'k' value
            else {
                ss.clear();
                ss.seekg(1); // Skip the first character
                // Read 'p' values
                while (ss >> value >> delimiter) {
                    if (delimiter == ',' || delimiter == '/') {
                        parameters.insert(value);
                    } else {
                        // Invalid delimiter
                        std::cerr << "Invalid delimiter: " << delimiter << std::endl;
                        return std::make_tuple(char{}, std::set<int>{}, int{}); // Return an empty tuple in case of error
                    }
                }
                k = value; // Set 'k' value
            }  
        } else {
            // Invalid type
            std::cerr << "Invalid proposal type: " << type << std::endl;
            return std::make_tuple(char{}, std::set<int>{}, int{}); // Return an empty tuple in case of error
        }
    }

    // std::cout << str << " " << type << " {";
    // for (auto i : parameters) {
    //     std::cout << i << " ";
    // }
    // std::cout << "} (" << k << ")" << std::endl;
    
    return std::make_tuple(type, parameters, k);
}

// Check if a ⊆ b
bool isSubset(const std::set<int>& a, const std::set<int>& b) { 
    return std::includes(b.begin(), b.end(), a.begin(), a.end());
}

// return set1 U set2
std::set<int> setUnion(const std::set<int>& set1, const std::set<int>& set2) {
    // Create a temporary set to store the result of set1 U set2
    std::set<int> result;

    // Perform the set union operation
    std::set_union(set1.begin(), set1.end(), set2.begin(), set2.end(), std::inserter(result, result.begin()));

    // Return the result
    return result;
}

void init_lattice(int em_id, Parser & parser)
{
    pthread_mutex_lock(&mutex);
    /* for proposer */
    parser.active_lattice = false;
    parser.ack_count_lattice.clear();
    parser.nack_count_lattice.clear();
    parser.active_proposal_number_lattice = 0;
    parser.proposed_value_lattice.clear();

    /* for acceptor */
    parser.accepted_value_lattice.clear();

    /* for each round */
    parser.fin_lattice.clear();
    pthread_mutex_unlock(&mutex);
}

void propose_lattice(int em_id, Parser & parser, std::set<int> & proposal_set)
{
    parser.proposed_value_lattice = proposal_set;
    parser.active_lattice = true;
    parser.active_proposal_number_lattice ++;
    parser.ack_count_lattice.clear();
    parser.nack_count_lattice.clear();

    // trigger beb.broadcast(<PROP, prop_val_i, active_prop_num_i>)
    std::string buffer = format_proposal_buffer_lattice(proposal_set, parser.active_proposal_number_lattice);
    format_round_wrapper_lattice(buffer, parser.round_num);
    // parser.writeConsole("%s send", buffer.c_str());
    parser.writeConsole("%s PROP %d", format_plaintext_lattice(parser.proposed_value_lattice).c_str(), parser.round_num);
    for (auto &host : parser.hosts()) {
        senderPerfectLinks(em_id, static_cast<int>(host.id), parser, buffer);
    }
}

void decide_lattice(int em_id, Parser & parser)
{
    std::string res = format_plaintext_lattice(parser.proposed_value_lattice);
    
    parser.writeOutputFile(res.c_str());
    parser.writeConsole("%s DECI %d", res.c_str(), parser.round_num);

    parser.fin_lattice.insert(em_id);
}

void upon_event_deliver_beb_lattice(int em_id, Parser & parser, message_t mes)
{
    int r = deformat_round_wrapper_lattice(mes.second);
    if (r != parser.round_num) throw std::runtime_error("");
    pthread_mutex_lock(&mutex);
    auto res = deformat_lattice(mes.second);
    parser.writeConsole("%s recv", mes.second.c_str());
    char type = std::get<0>(res);
    try {
        if (type == 'P') { upon_event_recv_prop_lattice(em_id, parser, std::get<1>(res), std::get<2>(res), mes.first, r); }
        else if (type == 'A') { upon_event_recv_ack_lattice(em_id, parser, std::get<2>(res), mes.first, r); }
        else if (type == 'N') { upon_event_recv_nack_lattice(em_id, parser, std::get<1>(res), std::get<2>(res), mes.first, r); }
        else if (type == 'F') { upon_event_recv_fin_lattice(em_id, parser, std::get<1>(res), std::get<2>(res)); }
    } catch (const std::exception& e) {
        pthread_mutex_unlock(&mutex);
        throw;
    }
    pthread_mutex_unlock(&mutex);
}

void upon_event_recv_fin_lattice(int em_id, Parser & parser, std::set<int> & val, int fin_em_id)
{
    if (!val.empty()) {
        int round_num = *(val.begin());
        if (parser.round_num == round_num) 
            parser.fin_lattice.insert(fin_em_id);
        // parser.writeConsole("FIN: %s", format_plaintext_lattice(parser.fin_lattice).c_str());
        if (parser.fin_lattice.size() > parser.hosts().size() / 2) throw std::runtime_error("");
    }
}

void upon_event_recv_ack_lattice(int em_id, Parser & parser, int prop_num, int sender_id, int current_round)
{
    if (current_round != parser.round_num) throw std::runtime_error("");
    if (prop_num == parser.active_proposal_number_lattice) {
        parser.ack_count_lattice.insert(sender_id);
        
        try {
            check_ack_count_lattice(em_id, parser, current_round);
            check_nack_count_lattice(em_id, parser, current_round);
        } catch (const std::exception& e) {
           throw;
        }
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
void upon_event_recv_nack_lattice(int em_id, Parser & parser, std::set<int> & val, int prop_num, int sender_id, int current_round)
{
    if (current_round != parser.round_num) throw std::runtime_error("");
    if (prop_num == parser.active_proposal_number_lattice) {
        parser.proposed_value_lattice = setUnion(parser.proposed_value_lattice, val);
        parser.nack_count_lattice.insert(sender_id);

        try {
            check_nack_count_lattice(em_id, parser, current_round);
        } catch (const std::exception& e) {
           throw;
        }
        
    }
}
#pragma GCC diagnostic pop

void upon_event_recv_prop_lattice(int em_id, Parser & parser, std::set<int> & prop_val, int prop_num, int sender_id, int current_round)
{
    if (current_round != parser.round_num) throw std::runtime_error("");
    if (isSubset(parser.accepted_value_lattice, prop_val)) {
        if (current_round == parser.round_num)
            parser.accepted_value_lattice = prop_val;
        else throw std::runtime_error("");

        // send <ACK, prop_num> to p_j
        std::string buffer = format_ack_buffer_lattice(prop_num);
        format_round_wrapper_lattice(buffer, parser.round_num);
        senderPerfectLinks(em_id, sender_id, parser, buffer);
    }
    else {
        if (current_round == parser.round_num)
            parser.accepted_value_lattice = setUnion(parser.accepted_value_lattice, prop_val);
        else throw std::runtime_error("");

        // send <NACK, accept_val, prop_num> to p_j
        std::string buffer = format_nack_buffer_lattice(parser.accepted_value_lattice, prop_num);
        format_round_wrapper_lattice(buffer, parser.round_num);
        senderPerfectLinks(em_id, sender_id, parser, buffer);
    }
}

void check_ack_count_lattice(int em_id, Parser & parser, int current_round)
{
    if (current_round != parser.round_num) throw std::runtime_error("");
    if ((parser.ack_count_lattice.size() > parser.hosts().size() / 2) && parser.active_lattice) {
        decide_lattice(em_id, parser);
        parser.active_lattice = false;
    }
}

void check_nack_count_lattice(int em_id, Parser & parser, int current_round)
{
    if (current_round != parser.round_num) throw std::runtime_error("");
    if (parser.nack_count_lattice.size() && (parser.ack_count_lattice.size() + parser.nack_count_lattice.size() > parser.hosts().size() / 2) && parser.active_lattice) {
        parser.active_proposal_number_lattice ++;
        parser.ack_count_lattice.clear();
        parser.nack_count_lattice.clear();

        // trigger beb.broadcast(<PROP, prop_val_i, active_prop_num_i>)
        std::string buffer = format_proposal_buffer_lattice(parser.proposed_value_lattice, parser.active_proposal_number_lattice);
        format_round_wrapper_lattice(buffer, parser.round_num);
        for (auto &host : parser.hosts()) {
            senderPerfectLinks(em_id, static_cast<int>(host.id), parser, buffer);
        }
    }
}