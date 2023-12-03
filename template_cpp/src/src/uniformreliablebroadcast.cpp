#include "uniformreliablebroadcast.hpp"

// upon event ⟨ urb, Init ⟩ do 
//  delivered := ∅;
//  pending := ∅;
//  correct := Π;
//  forall m do ack[m] := ∅;
void init_urb(Parser & parser)
{
    initPerfectLink(static_cast<int>(parser.id()), parser);
    parser.delivered_urb.assign(parser.message_to_send + 1, 0);
    parser.pending.clear();
    parser.faulty.assign(parser.hosts().size(), 0);
    parser.ack.assign(parser.message_to_send + 1, std::set<int>());

    parser.writeConsole("init urb done");
}

// upon event ⟨ urb, Broadcast | m ⟩ do
//  pending := pending ∪ {(self, m)};
//  trigger ⟨ beb, Broadcast | [DATA, self, m] ⟩;
void broadcast_urb(int em_id, Parser & parser, std::string buffer)
{
    parser.pending.insert({em_id, buffer});
    check_pending_urb(em_id, parser);
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
    int m = deformat_get_m_fifo(mes.second);
    if (m < 0) {
        std::cerr << "fail to get m from message." << std::endl;
        exit(0);
    }
    parser.ack[m].insert(mes.first);
    parser.writeConsole("%d->%d ✓%s m=%d", mes.first, em_id, mes.second.c_str(), m);
    if (parser.pending.find(mes) == parser.pending.end()) {
        parser.pending.insert(mes);

        check_pending_urb(em_id, parser);

        for (auto &host : parser.hosts()) {
            senderPerfectLinks(mes.first, static_cast<int>(host.id), parser, mes.second);
        }
    }
}

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
void deliver_urb(int em_id, Parser & parser, message_t mes, int m)
{
    parser.writeConsole("%d->%d deliver_urb %s ✓", m, em_id, mes.second.c_str());
    int s = mes.first;
    if (!parser.delivered_fifo[m]) {
        std::vector<int> m_past = deformat_get_m_past_fifo(mes.second);
        for (auto n: m_past) {
            if (!parser.delivered_fifo[n]) {
                deliver_fifo(s, parser, n);
                parser.delivered_fifo[n] = 1;
                
                message_t m1 = {s, std::to_string(n)};
                auto it = std::find(parser.past_fifo.begin(), parser.past_fifo.end(), m1);
                if (it == parser.past_fifo.end()) {
                    // (s, n) not found in past
                    parser.past_fifo.push_back(m1);
                }
            }
        }

        deliver_fifo(em_id, parser, m);
        parser.delivered_fifo[m] = 1;

        message_t m2 = {em_id, std::to_string(m)};
        auto it = std::find(parser.past_fifo.begin(), parser.past_fifo.end(), m2);
        if (it == parser.past_fifo.end()) {
            // (em_id, m) not found in past
            parser.past_fifo.push_back(m2);
        }
    }
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
void check_pending_urb(int em_id, Parser &parser)
{
    for (auto const &itr: parser.pending)
    {
        int m = deformat_get_m_fifo(const_cast<std::string&>(itr.second));
        if (candeliver_urb(m, parser) && !parser.delivered_urb[m]) {
            parser.delivered_urb[m] = 1;
            deliver_urb(em_id, parser, itr, m);
            parser.pending.erase(itr);
        }
    }
}