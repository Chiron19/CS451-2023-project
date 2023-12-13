#include "latticeagreement.hpp"

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

        if (type == 'P' || type == 'A' || type == 'N') { // Determine the format
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

// Check if a is a subset of b
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
    /* for proposer */
    parser.active_lattice = false;
    parser.ack_count_lattice = 0;
    parser.nack_count_lattice = 0;
    parser.active_proposal_number_lattice = 0;
    parser.proposed_value_lattice.clear();

    /* for acceptor */
    parser.accepted_value_lattice.clear();
}

void propose_lattice(int em_id, Parser & parser, std::set<int> & proposal_set)
{
    parser.proposed_value_lattice = proposal_set;
    parser.active_lattice = true;
    parser.active_proposal_number_lattice ++;
    parser.ack_count_lattice = parser.nack_count_lattice = 0;

    std::string buffer = format_proposal_buffer_lattice(proposal_set, parser.active_proposal_number_lattice);
    for (auto &host : parser.hosts()) {
        senderPerfectLinks(em_id, static_cast<int>(host.id), parser, buffer);
    }
}

void decide_lattice(int em_id, Parser & parser)
{
    std::string res;
    for (auto it : parser.proposed_value_lattice) {
       res = res + std::to_string(it) + " ";
    }
    
    parser.writeOutputFile(res.c_str());
}

void upon_event_deliver_beb_lattice(int em_id, Parser & parser, message_t mes)
{
    auto res = deformat_lattice(mes.second);
    if (std::get<0>(res) == 'P') { upon_event_recv_prop_lattice(em_id, parser, std::get<1>(res), std::get<2>(res), mes.first); }
    else if (std::get<0>(res) == 'A') { upon_event_recv_ack_lattice(em_id, parser, std::get<2>(res)); }
    else if (std::get<0>(res) == 'N') { upon_event_recv_nack_lattice(em_id, parser, std::get<1>(res), std::get<2>(res)); }
}

void upon_event_recv_ack_lattice(int em_id, Parser & parser, int prop_num)
{
    if (prop_num == parser.active_proposal_number_lattice) {
        parser.ack_count_lattice ++;

        check_ack_count_lattice(em_id, parser);
        check_nack_count_lattice(em_id, parser);
    }
}

void upon_event_recv_nack_lattice(int em_id, Parser & parser, std::set<int> & val, int prop_num)
{
    if (prop_num == parser.active_proposal_number_lattice) {
        parser.proposed_value_lattice = setUnion(parser.proposed_value_lattice, val);
        parser.nack_count_lattice ++;

        check_nack_count_lattice(em_id, parser);
    }
}

void upon_event_recv_prop_lattice(int em_id, Parser & parser, std::set<int> & prop_val, int prop_num, int sender_id)
{
    if (isSubset(parser.accepted_value_lattice, prop_val)) {
        parser.accepted_value_lattice = prop_val;
        std::string buffer = format_ack_buffer_lattice(prop_num);
        senderPerfectLinks(em_id, sender_id, parser, buffer);
    }
    else {
        parser.accepted_value_lattice = setUnion(parser.accepted_value_lattice, prop_val);
        std::string buffer = format_nack_buffer_lattice(parser.accepted_value_lattice, prop_num);
        senderPerfectLinks(em_id, sender_id, parser, buffer);
    }
}

void check_ack_count_lattice(int em_id, Parser & parser)
{
    if ((parser.ack_count_lattice > static_cast<int>(parser.hosts().size() / 2)) && parser.active_lattice) {
        decide_lattice(em_id, parser);
        parser.active_lattice = false;
    }
}

void check_nack_count_lattice(int em_id, Parser & parser)
{
    if (parser.nack_count_lattice > 0 && (parser.ack_count_lattice + parser.nack_count_lattice > static_cast<int>(parser.hosts().size() / 2)) && parser.active_lattice) {
        parser.active_proposal_number_lattice ++;
        parser.ack_count_lattice = parser.nack_count_lattice = 0;

        std::string buffer = format_proposal_buffer_lattice(parser.proposed_value_lattice, parser.active_proposal_number_lattice);
        for (auto &host : parser.hosts()) {
            senderPerfectLinks(em_id, static_cast<int>(host.id), parser, buffer);
        }
    }
}