#include "fifobroadcast.hpp"

/*
    Format the past into "x,y,z" string (no space)
*/
std::string format_past_fifo(Parser parser)
{
    std::string res;
    for (message_t j: parser.past_fifo)
    {
        res = res + j.second + ",";
    }
    if (res.size()) res.pop_back();
    return res;
}

/*
    Get m from "x,y,z;m" string
*/
int deformat_get_m_fifo(std::string & str)
{
    size_t position = str.find(';');

    if (position != std::string::npos) {
        return std::stoi(str.substr(position + 1));
    } 
    else { // ';' not found
        try { return std::stoi(str); } 
        catch (const std::invalid_argument& e) { return -1; }
        catch (const std::out_of_range& e) { return -1; }
        return -1;
    }
}

/*
    Get a vector of x, y, z from "x,y,z;m" string
*/
std::vector<int> deformat_get_m_past_fifo(std::string & str)
{
    size_t position = str.find(';');

    if (position != std::string::npos) {
        str = str.substr(0, position);
        std::vector<int> res;
        size_t pos = 0;
        std::string token;
        while ((pos = str.find(',')) != std::string::npos) {
            token = str.substr(0, pos);
            res.push_back(std::stoi(token));
            str.erase(0, pos + 1);
        }
        return res;
    } 
    else {
        return {}; // ';' not found
    }
}

// upon event ⟨ crb, Init ⟩ do 
//  delivered := ∅;
//  past := [];
void init_fifo(int em_id, Parser parser)
{
    init_urb(parser);
    parser.delivered_fifo.assign(parser.message_to_send, 0);
    parser.past_fifo.clear();
}

// upon event ⟨ crb, Broadcast | m ⟩ do
//  trigger ⟨ rb, Broadcast | [DATA, past, m] ⟩; 
//  append(past, (self, m));
void broadcast_fifo(int em_id, Parser parser, int m)
{
    broadcast_urb(em_id, parser, format_past_fifo(parser) + ";" + std::to_string(m));
    parser.past_fifo.push_back({em_id, std::to_string(m)}); 
    std::string res = "b " + std::to_string(m);
    parser.writeConsole(res.c_str());
    parser.writeOutputFile(res.c_str());
}

// deliver fifo
void deliver_fifo(int em_id, Parser parser, int m)
{
    std::string res = "d " + std::to_string(em_id) + " " + std::to_string(m);
    parser.writeConsole(res.c_str());
    parser.writeOutputFile(res.c_str());
}

 