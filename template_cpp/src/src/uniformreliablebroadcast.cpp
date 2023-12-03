#include "uniformreliablebroadcast.hpp"

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
    parser.ack.assign(parser.message_to_send + 1, std::set<int>());

    parser.writeConsole("init urb done");
}

// upon event ⟨ urb, Broadcast | m ⟩ do
//  pending := pending ∪ {(self, m)};
//  trigger ⟨ beb, Broadcast | [DATA, self, m] ⟩;
void broadcast_urb(int em_id, Parser & parser, std::string buffer)
{
    int m = deformat_get_m_fifo(buffer);
    parser.pending.insert({em_id, std::to_string(m)});
    check_pending_urb(em_id, parser, {em_id, buffer}, em_id, m);
    for (auto &host : parser.hosts()) {
        std::string str = buffer + "." + std::to_string(em_id);
        senderPerfectLinks(em_id, static_cast<int>(host.id), parser, str);
    }
}

// For Algorithm 3.4: All-Ack Uniform Reliable Broadcast:
// upon event ⟨ beb, Deliver | p, [DATA, s, m] ⟩ do 
//  ack[m] := ack[m] ∪ {p};
//  if (s, m) ̸∈ pending then
//      pending := pending ∪ {(s, m)};
//      trigger ⟨ beb, Broadcast | [DATA, s, m] ⟩;
void upon_event_deliver_beb_urb(int em_id, Parser &parser, message_t mes, int s, int m)
{
    if (m < 0) {
        std::cerr << "fail to get m from message." << std::endl;
        exit(0);
    }
    parser.ack[m].insert(mes.first);
    parser.writeConsole("%d->%d %s recv_beb ✓m=%d", mes.first, em_id, mes.second.c_str(), m);

    message_t m0 = {s, std::to_string(m)};
    if (parser.pending.find(m0) == parser.pending.end()) {
        parser.pending.insert(m0);

        check_pending_urb(em_id, parser, mes, s, m);

        for (auto &host : parser.hosts()) {
            senderPerfectLinks(mes.first, static_cast<int>(host.id), parser, mes.second);
        }
    }
}

// deliver urb
void deliver_urb(int em_id, Parser & parser, message_t mes, int s, int m)
{
    parser.delivered_urb.insert({s, std::to_string(m)});
    // parser.writeConsole("%d->%d deliver_urb %s ✓", m, em_id, mes.second.c_str());

    upon_event_deliver_urb_fifo(em_id, parser, mes, m);
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
    if (candeliver_urb(m, parser)) {
        message_t m0 = {s, std::to_string(m)};
        if (parser.delivered_urb.find(m0) == parser.delivered_urb.end()) {
            parser.delivered_urb.insert(m0);
            deliver_urb(em_id, parser, mes, s, m);
            parser.pending.erase(m0);
        }
    }
}