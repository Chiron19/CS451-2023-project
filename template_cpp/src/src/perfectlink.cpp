#include "perfectlink.hpp"

/* Format Time point into "HH:MM:SS:fff" */ 
std::string format_time_point(const std::chrono::system_clock::time_point& tp) {
    // Extract the time since epoch (in seconds) from the time_point
    time_t time_since_epoch = std::chrono::system_clock::to_time_t(tp);

    // Extract the milliseconds from the time_point
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;

    // Convert the time since epoch to a tm struct for local time
    struct tm local_time;
    localtime_r(&time_since_epoch, &local_time);

    char buffer[16]; // HH:MM:SS.fff + null terminator
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", local_time.tm_hour, local_time.tm_min, local_time.tm_sec, static_cast<int>(ms.count()));

    return buffer;
}

/* Format Time Duration into "MM:ss:fff" */ 
std::string format_duration(const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(ms) % 100;
    ms -= minutes;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(ms) % 100;
    ms -= seconds;
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(ms) % 1000;

    char buffer[16]; // MM:SS.fff + null terminator
    snprintf(buffer, sizeof(buffer), "%02ld:%02ld.%03ld",
    minutes.count(), seconds.count(), millisecs.count());

    return buffer;
}

void receiverPerfectLinks(int em_id, Parser parser) {
    Udp udp(em_id, parser);
    std::set<message_t> delivered;
    auto start_time = std::chrono::high_resolution_clock::now(); // Record the starting time
    std::cout << format_time_point(start_time) << " " << format_duration(start_time, start_time) << " Start" << std::endl;
    for (;;) {
        message_t mes = udp.receive_udp();
        if (mes.first > 0) {
            // Searching in the set
            if (auto search = delivered.find(mes); search == delivered.end()) {
                // Not found: insert to the set 
                delivered.insert(mes);
                // printf("d %d %s", mes.first, mes.second.c_str());
                std::ofstream outputFile(parser.outputPath(), std::ios::app); // Open for appending
                auto now_time = std::chrono::high_resolution_clock::now(); // Record the now time
                if (outputFile.is_open()) {
                    std::cout << format_time_point(now_time) << " " << format_duration(start_time, now_time) << " d " << mes.first << ' ' << mes.second << std::endl;
                    outputFile << "d " << mes.first << ' ' << mes.second << std::endl;
                    outputFile.close();
                } else {
                    std::cerr << "Failed to open the file for appending." << std::endl;
                }
            }
            // else {
            //     auto now_time = std::chrono::high_resolution_clock::now(); // Record the now time
            //     std::cout << format_time_point(now_time) << " " << format_duration(start_time, now_time) << " D " << mes.first << ' ' << mes.second << std::endl;
            // }
        }
    }
}

void senderPerfectLinks(int em_id, Parser parser) {
    Udp udp(em_id, parser);
    auto start_time = std::chrono::high_resolution_clock::now(); // Record the starting time
    std::cout << format_time_point(start_time) << " " << format_duration(start_time, start_time) << " Start" << std::endl;
    for (bool k = 0;; k = 1) {
        for (int i = 1; i <= parser.message_to_send; i++)
        {
            udp.send_udp(parser.recv_em_id, std::to_string(i)); 
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
        }
    }
}