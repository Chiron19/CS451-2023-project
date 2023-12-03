#include "fifobroadcast.hpp"

/*
    Format the past into "x,y,z" string (no space)
*/
std::string format_past_fifo(Parser & parser)
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
void init_fifo(int em_id, Parser & parser)
{
    init_urb(parser);
    parser.delivered_fifo.assign(parser.message_to_send, 0);
    parser.past_fifo.clear();

    parser.writeConsole("init fifo done");
}

// upon event ⟨ crb, Broadcast | m ⟩ do
//  trigger ⟨ rb, Broadcast | [DATA, past, m] ⟩; 
//  append(past, (self, m));
void broadcast_fifo(int em_id, Parser & parser, int m, bool is_write_outputfile)
{
    std::string res = "b " + std::to_string(m);
    parser.writeConsole(res.c_str());
    if (is_write_outputfile) parser.writeOutputFile(res.c_str());
    
    broadcast_urb(em_id, parser, format_past_fifo(parser) + ";" + std::to_string(m));
    parser.past_fifo.insert({em_id, std::to_string(m)}); 
}

// deliver fifo
void deliver_fifo(int em_id, Parser & parser, int m)
{
    parser.delivered_fifo[m] = 1;

    std::string res = "d " + std::to_string(em_id) + " " + std::to_string(m);
    parser.writeConsole(res.c_str());
    parser.writeOutputFile(res.c_str());
}

// Algorithm 3.13: No-Waiting Causal Broadcast
// upon event ⟨ rb, Deliver | p, [DATA, mpast, m] ⟩ do 
//  if m ̸∈ delivered then
//      forall (s, n) ∈ mpast do    // by the order in the list
//          if n ̸∈ delivered then
//              trigger ⟨ crb, Deliver | s, n ⟩; 
//              delivered := delivered ∪ {n}; 
//              if (s, n) ̸∈ past then
//                  append(past, (s, n)); 
//      trigger ⟨ crb, Deliver | p, m ⟩;
//      delivered := delivered ∪ {m}; 
//      if (p, m) ̸∈ past then
//          append(past, (p, m));
void upon_event_deliver_urb_fifo(int em_id, Parser & parser, message_t mes, int m)
{
    int s = mes.first;
    if (!parser.delivered_fifo[m]) {
        std::vector<int> m_past = deformat_get_m_past_fifo(mes.second);
        for (auto n: m_past) {
            if (!parser.delivered_fifo[n]) {
                deliver_fifo(s, parser, n);
                parser.delivered_fifo[n] = 1;
                
                message_t m1 = {s, std::to_string(n)};
                parser.past_fifo.insert(m1); // insert (s, n) to the set past (if existed, nothing inserted)
            }
        }

        deliver_fifo(em_id, parser, m);
        parser.delivered_fifo[m] = 1;

        message_t m2 = {em_id, std::to_string(m)};
        parser.past_fifo.insert(m2); // insert (em_id, m) to the set past (if existed, nothing inserted)
    }
}
