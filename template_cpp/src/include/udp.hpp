#pragma once

#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <vector>
#include <stack>
#include <fstream>
#include <iostream>
#include <sys/socket.h>

#include "parser.hpp"

#define MAXLINE 10000

class Udp {
public:
    int em_id, send_fd, recv_fd;
    Parser parser;
    std::vector<std::pair<std::string, const short> > addr; // {ip, port}
    
    Udp(int em_id, Parser parser): em_id(em_id), parser(parser) {
        // For id == 0, put empty element
        addr.push_back({"", 0});
        for (auto &host : parser.hosts()) {
            addr.push_back(std::make_pair(host.ipReadable(), host.portReadable()));
            // std::cout << addr.back().first << " " << addr.back().second << std::endl;
        }         
        // Initialization
        setup_socket_udp(&recv_fd, 0);
        setup_socket_udp(&send_fd, 1);
        
    }

    /*
        Send UDP packet to target
     */
    void send_udp(int recv_em_id, const std::string& message) {
        struct sockaddr_in recvaddr;
        memset(&recvaddr, 0, sizeof(recvaddr));
        recvaddr.sin_family = AF_INET;
        recvaddr.sin_port = htons(addr[recv_em_id].second);
        recvaddr.sin_addr.s_addr = inet_addr(addr[recv_em_id].first.c_str());

        // std::cout << "sendto:" << recv_em_id << " " << message << std::endl;
        // std::cout << recvaddr.sin_addr.s_addr << " " << addr[recv_em_id].first << " " << recvaddr.sin_port << std::endl;

        ssize_t n = sendto(send_fd, message.c_str(), message.size(), 0, 
                reinterpret_cast<const struct sockaddr*>(&recvaddr), sizeof(recvaddr));

        if (n < 0) {
             std::cout << "SENDTO ERROR " << errno << std::endl;
        }
        // std::cout << "Send " << n << " bytes" << std::endl;
    }
    
    /*
        Returns received message or empty string if nothing was received
     */
    message_t receive_udp() {
        char buffer[MAXLINE];
        struct sockaddr_in sender_addr;
        int sender_em_id = -1;
        memset(&sender_addr, 0, sizeof(sender_addr));
        socklen_t len = sizeof(sender_addr);

        // ssize_t n = recvfrom(recv_fd, const_cast<char *>(buffer), MAXLINE, MSG_WAITALL, reinterpret_cast<struct sockaddr*>(&sender_addr), &len);

        // Non-blocking Sockets
        ssize_t n = recvfrom(recv_fd, const_cast<char *>(buffer), MAXLINE, 0, reinterpret_cast<struct sockaddr*>(&sender_addr), &len);


        if (n < 0) {
            std::cout << "RECVFROM ERROR " << errno << ": " << strerror(errno) << std::endl;
            return {-1, ""};
        }
        buffer[n] = '\0';

        // Find the send's id with the port no.
        // std::cout << inet_ntoa(sender_addr.sin_addr) << " " << ntohs(sender_addr.sin_port) << std::endl;
        for (unsigned long i = 1; i < addr.size(); ++i) {
            if (htons(addr[i].second) == sender_addr.sin_port) {
                sender_em_id = static_cast<int> (i);
                break;
            }
        }

        if (sender_em_id == -1) {
            std::cout << "ERROR - SENDER DOESNT EXIST" << std::endl;
            return {-1, ""};
        }

        // printf("Received packet from proc %d (%s), message: %s\n", sender_em_id, addr[sender_em_id].first.c_str(), buffer);
        return {sender_em_id, buffer};
    }

private:
    // type: 0 recv, 1 send
    void setup_socket_udp(int* __fd, int type) {
        // SOCK_DGRAM for UDP socket       
        if ((*__fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            printf("Socket creation failed...\n");
            exit(0);
        }
        
        // socket configuration: incoming at assigned port
        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(addr[em_id].second);

        if (type) return;
        // bind socket with receiver's file descipter
        if (bind(*__fd, reinterpret_cast<const struct sockaddr *>(&servaddr), 
                sizeof(servaddr)) < 0) {
            printf("Socket binding failed...\n");
            exit(0);
        }
    }
};
