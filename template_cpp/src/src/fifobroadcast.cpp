#include "fifobroadcast.hpp"

/*
    Format the past into "x,y,z" string (no space)
*/
std::string format_past_fifo(Parser & parser)
{
    std::string res;
    for (message_t j: parser.past_fifo)
    {
        res = res + std::to_string(j.first) + '`' + j.second + ",";
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
    Get s from "x,y,z.s;m" string
*/
int deformat_get_s_fifo(std::string & str)
{
    size_t position = str.find('.');

    if (position != std::string::npos) {
        return std::stoi(str.substr(position + 1));
    } 
    else { // '.' not found
        try { return std::stoi(str); } 
        catch (const std::invalid_argument& e) { return -1; }
        catch (const std::out_of_range& e) { return -1; }
        return -1;
    }
}

/*
    Get a vector of x, y, z from "x,y,z;m" string
*/
std::vector<message_t> deformat_get_m_past_fifo(std::string & str)
{
    size_t position = str.find(';');

    if (position != std::string::npos) {
        str = str.substr(0, position);
        std::vector<message_t> res;
        size_t pos = 0;
        std::string token, s_str, m_str;
        while ((pos = str.find(',')) != std::string::npos) {
            token = str.substr(0, pos);
            size_t pos0 = 0;
            if ((pos0 = token.find('`')) != std::string::npos) {
                s_str = token.substr(0, pos0);
                m_str = token.substr(pos0 + 1);
                res.push_back({std::stoi(s_str), m_str});
            }
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
    parser.delivered_fifo.clear();
    parser.past_fifo.clear();

    parser.cheating_vector_d.assign(parser.hosts().size() + 1, 0);

    parser.writeConsole("init fifo done");
}

void append_past_fifo(int em_id, Parser & parser, int m)
{
    message_t mes = {em_id, std::to_string(m)};
    auto it = std::find(parser.past_fifo.begin(), parser.past_fifo.end(), mes);
    if (it == parser.past_fifo.end()) {
        parser.past_fifo.push_back(mes);
    }
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
    append_past_fifo(em_id, parser, m);
}

// deliver fifo
void deliver_fifo(int src_em_id, Parser & parser, int m)
{
    parser.delivered_fifo.insert(std::to_string(m));

    // TO BE FIXED
    pthread_mutex_lock(&mutex);
        if (parser.cheating_vector_d[src_em_id] < m) {
            std::string res;
            while (++parser.cheating_vector_d[src_em_id] < m)
            {
                res = "d " + std::to_string(src_em_id);
                res = res + " " + std::to_string(parser.cheating_vector_d[src_em_id]);
                parser.writeConsole(res.c_str());
                parser.writeOutputFile(res.c_str());
            }
            parser.cheating_vector_d[src_em_id] = m;
        }
    pthread_mutex_unlock(&mutex);

    
    // std::string r = "d' ";
    // for (auto itr: parser.delivered_fifo)
    // {
    //     r = r + itr + " ";
    // }
    // parser.writeOutputFile(r.c_str());
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
    message_t m0 = {em_id, std::to_string(m)};
    if (parser.delivered_fifo.find(std::to_string(m)) == parser.delivered_fifo.end()) {
        std::vector<message_t> m_past = deformat_get_m_past_fifo(mes.second);
        parser.writeConsole("upon_event_deliver_urb_fifo %s", mes.second.c_str());
        for (auto m1: m_past) {
            int s = m1.first, n = std::stoi(m1.second);
            if (parser.delivered_fifo.find(std::to_string(n)) == parser.delivered_fifo.end()) {
                std::cout << "D " << s << " " << n << std::endl;
                deliver_fifo(s, parser, n);
                parser.delivered_fifo.insert(std::to_string(n));
                
                append_past_fifo(s, parser, n); // append (s, n) to the set past (if not existed)
            }
        }

        deliver_fifo(em_id, parser, m);
        parser.delivered_fifo.insert(std::to_string(m));

        append_past_fifo(em_id, parser, m); // append (self, m) to the set past (if not existed)
    }
}
