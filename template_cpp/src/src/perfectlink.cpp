#include "perfectlink.hpp"

Udp udp;

void initPerfectLink(int em_id, Parser & parser) {
    parser.not_delivered_pl.clear();
    parser.ack_queue_pl.clear();
    udp.init_udp(em_id, parser);

    parser.writeConsole("init perfectlink done");
}

/*
    receive: {sender_id, buffer}
    Notice! sender_id != src_id (may encoded in buffer)
*/
void receiverPerfectLinks(int em_id, Parser & parser) {
    // parser.writeConsole("Start receiverPerfectLinks");
    for (;;) {

        message_t mes = udp.receive_udp();
        // parser.writeConsole("recv fr: %d %s", mes.first, mes.second.c_str());
        if (mes.first > 0) {
            size_t pos = mes.second.find("@"); // Check if the string contains "ack(@)"
            if (pos != std::string::npos) { // this message is an ack message
                mes.second.erase(pos, 1);  // Assuming "ack(@)" is 1 characters long, erase it
                parser.not_delivered_pl.erase(mes);// Not acked before: set acked
                // parser.writeConsole("%d->%d %s deli_pl ✓", em_id, mes.first, mes.second.c_str());
            }
            else { // this message is not ack message
                // parser.ack_queue_pl.push_back(mes); // set ack queue
                udp.send_udp(em_id, mes.first, mes.second + "@");
                upon_event_deliver_beb_urb(em_id, parser, mes); // trigger event urb
                // upon_event_deliver_beb_lattice(em_id, parser, mes); // trigger event lattice
            }
        }
    }
}

/*
    sender: em_id -> dst_em_id
    ack buffer "em_id:[buffer]@"
*/
void ackPerfectLinks(int em_id, Parser & parser) {
    // parser.writeConsole("Start ackPerfectLinks");
    // for (;;) {
    //     // send "ack(@)" to message already received
    //     while (parser.ack_queue_pl.size()) {
    //         message_t mes = parser.ack_queue_pl.back();
    //         parser.ack_queue_pl.pop_back();
    //         udp.send_udp(em_id, mes.first, mes.second + "@");
    //     }
    // }
}

/*
    sender: em_id -> dst_em_id
    sending buffer "em_id:[buffer]"
*/
void senderPerfectLinks(int em_id, int dst_em_id, Parser & parser, std::string & buffer) {
    // parser.writeConsole("Start senderPerfectLinks");
    // Original: Infinite Loop (keep sending m messages)
    // Modified: Sending finite times only, expired after
    // Notice! You can NOT run this loop infinitely, which would block the thread!!! 
    // Notice! If you want it run eventually perfect link, you need to trigger the function in upper layer.
    parser.not_delivered_pl.insert({dst_em_id, buffer});
    for (int k = 0; k < 1; k++) {
        if (parser.not_delivered_pl.find({dst_em_id, buffer}) == parser.not_delivered_pl.end()) break; // this message has been delivered

        // resend all un-delivered message
        for (auto mes : parser.not_delivered_pl) {
            udp.send_udp(em_id, mes.first, mes.second);
        }

        // congestion control
        // std::this_thread::sleep_for(std::chrono::milliseconds(k)); 
    }

    // parser.not_delivered_pl.erase({dst_em_id, buffer}); // delivered/expired, erase it
}