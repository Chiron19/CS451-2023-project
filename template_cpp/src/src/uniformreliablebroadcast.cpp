#include "uniformreliablebroadcast.hpp"

/*
    Get (string)mes from "mes.s" string
*/
std::string deformat_get_mes_urb(std::string & str)
{
    size_t position = str.find('.');

    if (position != std::string::npos) {
        return str.substr(0, position);
    } 
    else { // '.' not found
        try { return str; } 
        catch (const std::invalid_argument& e) { return ""; }
        catch (const std::out_of_range& e) { return ""; }
        return "";
    }
}

/*
    Get (int)s from "mes.s" string
*/
int deformat_get_s_urb(std::string & str)
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
    Get mapping value m = mapping[buffer], create one if not exist 
    buffer must be deformated
*/
int get_mapping_mes(Parser & parser, std::string & str)
{
    int m;
    if (parser.m_mapping_urb.find(str) == parser.m_mapping_urb.end()) {
        m = parser.m_mapping_urb[str] = static_cast<int>(parser.ack.size());
        parser.ack.push_back(std::set<int>());
    }
    else {
        m = parser.m_mapping_urb[str];
    }
    return m;
}

// upon event ⟨ urb, Init ⟩ do 
//  delivered := ∅;
//  pending := ∅;
//  correct := Π;
//  forall m do ack[m] := ∅;
void init_urb(Parser & parser)
{
    initPerfectLink(static_cast<int>(parser.id()), parser);
    parser.delivered_urb.clear();
    parser.pending.clear();
    parser.ack.clear();

    parser.writeConsole("init urb done");
}

// upon event ⟨ urb, Broadcast | m ⟩ do
//  pending := pending ∪ {(self, m)};
//  trigger ⟨ beb, Broadcast | [DATA, self, m] ⟩;
// Notice! buffer is a pure string without ".@" (reserved symbol for separation)
// Notice! (string)buffer is mapped to (int)m
void broadcast_urb(int em_id, Parser & parser, std::string buffer)
{
    int m = get_mapping_mes(parser, buffer);
    parser.pending.insert({em_id, m});
    buffer = buffer + "." + std::to_string(em_id); // Before check pending, buffer should be formated
    // check_pending_urb(em_id, parser, {em_id, buffer}, em_id, m);
    parser.writeConsole("%d->A %s send_beb m=%d", em_id, buffer.c_str(), m);
    for (auto &host : parser.hosts()) {
        senderPerfectLinks(em_id, static_cast<int>(host.id), parser, buffer);
    }
}

// For Algorithm 3.4: All-Ack Uniform Reliable Broadcast:
// upon event ⟨ beb, Deliver | p, [DATA, s, m] ⟩ do 
//  ack[m] := ack[m] ∪ {p};
//  if (s, m) ̸∈ pending then
//      pending := pending ∪ {(s, m)};
//      trigger ⟨ beb, Broadcast | [DATA, s, m] ⟩;
void upon_event_deliver_beb_urb(int em_id, Parser &parser, message_t mes)
{
    int s = deformat_get_s_urb(mes.second); // Get src_id
    std::string buf = deformat_get_mes_urb(mes.second);
    int m = get_mapping_mes(parser, buf);

    if (m < 0) {
        std::cerr << "fail to get m from message." << std::endl;
        exit(0);
    }
    parser.ack[m].insert(mes.first); // Attention!!!  (p = sender_id, s = src_id)
    parser.writeConsole("%d->%d %s recv_beb ✓m=%d", s, em_id, mes.second.c_str(), m);

    if (parser.pending.find({s, m}) == parser.pending.end()) {
        parser.pending.insert({s, m});

        check_pending_urb(em_id, parser, mes, s, m);

        for (auto &host : parser.hosts()) {
            senderPerfectLinks(s, static_cast<int>(host.id), parser, mes.second);
        }
    }
    else {
        // ack[m] changed, check pending
        check_pending_urb(em_id, parser, mes, s, m);
    }
}

// deliver urb
void deliver_urb(int em_id, Parser & parser, message_t mes, int s, int m)
{
    // parser.delivered_urb.insert({s, m});
    parser.writeConsole("%d->%d deli_urb %s ✓", mes.first, em_id, mes.second.c_str());
    parser.writeOutputFile("d %d %s", s, deformat_get_mes_urb(mes.second).c_str());
    // upon_event_deliver_urb_fifo(em_id, parser, mes, m); //
}

// function candeliver(m) returns Boolean is 
// return #(ack[m]) > N/2;
bool candeliver_urb(int m, Parser & parser)
{
    return parser.ack[m].size() > parser.hosts().size() / 2;
}

// upon exists (s, m) ∈ pending such that candeliver(m) ∧ m ̸∈ delivered do 
// delivered := delivered ∪ {m};
// trigger ⟨ urb, Deliver | s, m ⟩;
void check_pending_urb(int em_id, Parser &parser, message_t mes, int s, int m)
{
    printf("checking {%d, %s}, ack[%d]: ", s, mes.second.c_str(), m);
    for (auto it = parser.ack[m].begin(); it != parser.ack[m].end(); ++it) {
        std::cout << *it << " ";
    }
    printf("\n");
    if (candeliver_urb(m, parser)) {
        if (parser.delivered_urb.find({s, m}) == parser.delivered_urb.end()) {
            parser.delivered_urb.insert({s, m});
            deliver_urb(em_id, parser, mes, s, m);
            parser.pending.erase({s, m});
        }
    }
}