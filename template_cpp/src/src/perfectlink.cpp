#include "perfectlink.hpp"

Udp udp;

void initPerfectLink(int em_id, Parser & parser) {
    parser.not_delivered_pl.clear();
     while (!parser.ack_queue_pl.empty()) {
        parser.ack_queue_pl.pop();
    }
    if (udp.addr.empty()) udp.init_udp(em_id, parser);

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
                parser.ack_queue_pl.push(mes); // set ack queue
                // udp.send_udp(em_id, mes.first, mes.second + "@");
                // upon_event_deliver_beb_urb(em_id, parser, mes); // trigger event urb
                // trigger event lattice
                try {
                    upon_event_deliver_beb_lattice(em_id, parser, mes); 
                } catch (const std::exception& e) {
                    // std::cout << "reach the return point\n";
                    return;
                }
            }
        }

    //     // send "ack(@)" to message already received
    //     while (parser.ack_queue_pl.size()) {
    //         message_t mes = parser.ack_queue_pl.back();
    //         parser.ack_queue_pl.pop_back();
    //         udp.send_udp(em_id, mes.first, mes.second + "@");
    //     }

        
    }
}

void resendPerfectLinks(int em_id, Parser & parser) {
    // resend all un-delivered message
    for (auto mes : parser.not_delivered_pl) {
        udp.send_udp(em_id, mes.first, mes.second);
    }
    // send ack in ack queue
    while (!parser.ack_queue_pl.empty()) {
        auto mes = parser.ack_queue_pl.front();
        udp.send_udp(em_id, mes.first, mes.second + "@");
        parser.ack_queue_pl.pop();
    }
}

/*
    sender: em_id -> dst_em_id
    sending buffer "em_id:[buffer]"
*/
void senderPerfectLinks(int em_id, int dst_em_id, Parser & parser, std::string & buffer) {
    // blocking mode
    // if (parser.not_delivered_pl.size() >= 1000) return;

    // parser.writeConsole("Start senderPerfectLinks");
    // Original: Infinite Loop (keep sending m messages)
    // Modified: Sending finite times only, expired after
    // Notice! You can NOT run this loop infinitely, which would block the thread!!! 
    // Notice! If you want it run eventually perfect link, you need to trigger the function in upper layer.
    parser.not_delivered_pl.insert({dst_em_id, buffer});
    udp.send_udp(em_id, dst_em_id, buffer);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // for (int k = 0; k < 1; k++) {
    //     if (parser.not_delivered_pl.find({dst_em_id, buffer}) == parser.not_delivered_pl.end()) break; // this message has been delivered

    //     // congestion control
    //     // std::this_thread::sleep_for(std::chrono::milliseconds(k)); 
    // }

    // parser.not_delivered_pl.erase({dst_em_id, buffer}); // delivered/expired, erase it
}