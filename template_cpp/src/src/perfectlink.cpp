#include "perfectlink.hpp"

Udp udp;

void initPerfectLink(int em_id, Parser parser) {
    udp.init_udp(em_id, parser);
}

void receiverPerfectLinks(int em_id, Parser parser) {
    parser.writeConsole("Start receiverPerfectLinks");
    for (;;) {
        message_t mes = udp.receive_udp();
        parser.writeConsole("recv fr: %d %s", mes.first, mes.second.c_str());
        if (mes.first > 0) {
            // Searching in the set
            if (parser.delivered_pl.find(mes) == parser.delivered_pl.end()) {
                // Not found: insert to the set 
                parser.delivered_pl.insert(mes);
                parser.writeConsole("deli_pl: %d %s", mes.first, mes.second.c_str());
                
                // For Algorithm 3.4: All-Ack Uniform Reliable Broadcast:
                // upon event ⟨ beb, Deliver | p, [DATA, s, m] ⟩ do 
                //  ack[m] := ack[m] ∪ {p};
                //  if (s, m) ̸∈ pending then
                //      pending := pending ∪ {(s, m)};
                //      trigger ⟨ beb, Broadcast | [DATA, s, m] ⟩;
                int m = deformat_get_m_fifo(mes.second);
                if (m < 0) {
                    std::cerr << "fail to get m from message." << std::endl;
                    exit(0);
                }
                parser.ack[m].insert(mes.first);
                parser.writeConsole("%d->%d ✓%s m=%d", mes.first, em_id, mes.second.c_str(), m);
                if (parser.pending.find(mes) == parser.pending.end()) {
                    parser.pending.insert(mes);

// For Algorithm 3.13: No-Waiting Causal Broadcas
// upon exists (s, m) ∈ pending such that candeliver(m) ∧ m ̸∈ delivered do 
// delivered := delivered ∪ {m};
// trigger ⟨ urb, Deliver | s, m ⟩;
                    if (candeliver_urb(m, parser) && !parser.delivered_urb[m]) {
                        parser.delivered_urb[m] = 1;
                        deliver_urb(em_id, parser, mes, m);
                        parser.writeConsole("urb delivered");
                    }

                    for (auto &host : parser.hosts()) {
                        senderPerfectLinks(mes.first, static_cast<int>(host.id), parser, mes.second);
                    }
                }
                // printf("d %d %s", mes.first, mes.second.c_str());
                // std::ofstream outputFile(parser.outputPath(), std::ios::app); // Open for appending
                
                /*
                if (outputFile.is_open()) {
                    std::cout << format_time_point(now_time) << " " << format_duration(start_time, now_time) << " d " << mes.first << ' ' << mes.second << std::endl;
                    outputFile << "d " << mes.first << ' ' << mes.second << std::endl;
                    outputFile.close();
                } else {
                    std::cerr << "Failed to open the file for appending." << std::endl;
                }
                */
            }
            // else {
            //     auto now_time = std::chrono::high_resolution_clock::now(); // Record the now time
            //     std::cout << format_time_point(now_time) << " " << format_duration(start_time, now_time) << " D " << mes.first << ' ' << mes.second << std::endl;
            // }
        }
    }
}

/*
    sender: em_id -> parser.recv_em_id
    sending m messages
    message 
*/
void senderPerfectLinks(int src_em_id, int dst_em_id, Parser parser, std::string buffer) {
    parser.writeConsole("Start senderPerfectLinks");
    // Original: Infinite Loop (keep sending m messages)
    // Modified: Sending finite times only
    parser.writeConsole("%d->%d %s (x10)",  src_em_id, dst_em_id, buffer.c_str());
    for (int k = 0; k < 10; k++) {
        // for (int i = 1; i <= parser.message_to_send; i++)
        // {
            udp.send_udp(dst_em_id, std::to_string(src_em_id) + ":" + buffer);
            // parser.writeConsole("%d->%d %s",  src_em_id, dst_em_id, buffer.c_str());
            /*
            if (!k) { // broadcast message only show once
                std::ofstream outputFile(parser.outputPath(), std::ios::app); // Open for appending
                auto now_time = std::chrono::high_resolution_clock::now(); // Record the now time
                if (outputFile.is_open()) {
                    std::cout << format_time_point(now_time) << " " << format_duration(start_time, now_time) << " b " << i << std::endl;
                    outputFile << "b " << i << std::endl;
                    outputFile.close();
                } else {
                    std::cerr << "Failed to open the file for appending." << std::endl;
                }
            }
            else {
                auto now_time = std::chrono::high_resolution_clock::now(); // Record the now time
                std::cout << format_time_point(now_time) << " " << format_duration(start_time, now_time) << " B " << i << std::endl;
            }
            */
        // }
    }
}