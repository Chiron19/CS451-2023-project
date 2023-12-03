#include "perfectlink.hpp"

Udp udp;

void initPerfectLink(int em_id, Parser & parser) {
    udp.init_udp(em_id, parser);

    parser.writeConsole("init perfectlink done");
}

void receiverPerfectLinks(int em_id, Parser & parser) {
    parser.writeConsole("Start receiverPerfectLinks");
    for (;;) {
        message_t mes = udp.receive_udp();
        // parser.writeConsole("recv fr: %d %s", mes.first, mes.second.c_str());
        if (mes.first > 0) {
            // Searching in the set
            if (parser.delivered_pl.find(mes) == parser.delivered_pl.end()) {
                // Not found: insert to the set 
                parser.delivered_pl.insert(mes);
                parser.writeConsole("deli_pl: %d %s", mes.first, mes.second.c_str());

                upon_event_deliver_beb_urb(em_id, parser, mes);
            }
        }
    }
}

/*
    sender: em_id -> parser.recv_em_id
    sending m messages
    message 
*/
void senderPerfectLinks(int src_em_id, int dst_em_id, Parser & parser, std::string & buffer) {
    parser.writeConsole("Start senderPerfectLinks");
    // Original: Infinite Loop (keep sending m messages)
    // Modified: Sending finite times only
    parser.writeConsole("%d->%d %s (x10)",  src_em_id, dst_em_id, buffer.c_str());
    for (int k = 0; k < 10; k++) {
        udp.send_udp(dst_em_id, std::to_string(src_em_id) + ":" + buffer);
    }
}