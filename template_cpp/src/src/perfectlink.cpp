#include "perfectlink.hpp"

Udp udp;

void initPerfectLink(int em_id, Parser & parser) {
    parser.delivered_pl.clear();
    parser.ack_queue_pl.clear();
    udp.init_udp(em_id, parser);

    parser.writeConsole("init perfectlink done");
}

/*
    receive: {sender_id, buffer}
    Notice! sender_id != src_id (encoded in buffer)
*/
void receiverPerfectLinks(int em_id, Parser & parser) {
    parser.writeConsole("Start receiverPerfectLinks");
    for (;;) {
        message_t mes = udp.receive_udp();
        // parser.writeConsole("recv fr: %d %s", mes.first, mes.second.c_str());
        if (mes.first > 0) {
            size_t pos = mes.second.find("@"); // Check if the string contains "ack(@)"
            if (pos != std::string::npos) { // this message is an ack message
                // If "ack" is found, erase it
                mes.second.erase(pos, 1);  // Assuming "ack(@)" is 1 characters long
                if (parser.delivered_pl.find(mes) == parser.delivered_pl.end()) {
                    // Not acked before: set acked
                    parser.delivered_pl.insert(mes);
                    parser.writeConsole("%d->%d %s deli_pl ✓", em_id, mes.first, mes.second.c_str());
                }
            }
            else { // this message is not ack message
                parser.ack_queue_pl.push_back(mes); // set ack queue
                upon_event_deliver_beb_urb(em_id, parser, mes); // trigger event urb
            }
        }
    }
}

/*
    sender: em_id -> parser.recv_em_id
    sending buffer "em_id:[data](@)"
*/
void senderPerfectLinks(int em_id, int dst_em_id, Parser & parser, std::string & buffer) {
    // parser.writeConsole("Start senderPerfectLinks");
    // Original: Infinite Loop (keep sending m messages)
    // Modified: Sending finite times only
    for (int k = 0; k < 3; k++) {
        // send "ack(@)" to message already received
        while (parser.ack_queue_pl.size()) {
            message_t mes = parser.ack_queue_pl.back();
            parser.ack_queue_pl.pop_back();
            udp.send_udp(mes.first, std::to_string(em_id) + ":" + mes.second + "@");
        }

        // check before send everytime, if the message has been acked (not delivered yet)
        if (parser.delivered_pl.find({dst_em_id, buffer}) == parser.delivered_pl.end()) {
            if (!k) parser.writeConsole("%d->%d %s send_pl", em_id, dst_em_id, buffer.c_str());
            udp.send_udp(dst_em_id, std::to_string(em_id) + ":" + buffer);
        }    
        else break;
    }
}