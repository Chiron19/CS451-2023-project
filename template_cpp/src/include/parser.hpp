#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <map>

#include <algorithm>
#include <cctype>
#include <locale>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <stdarg.h>
#include <chrono>

typedef std::pair<int, std::string> message_t; // sender em_id + message
class Parser {
public:
  struct Host {
    Host() {}
    Host(size_t id, std::string &ip_or_hostname, unsigned short port)
        : id{id}, port{htons(port)} {

      if (isValidIpAddress(ip_or_hostname.c_str())) {
        ip = inet_addr(ip_or_hostname.c_str());
      } else {
        ip = ipLookup(ip_or_hostname.c_str());
      }
    }

    std::string ipReadable() const {
      in_addr tmp_ip;
      tmp_ip.s_addr = ip;
      return std::string(inet_ntoa(tmp_ip));
    }

    unsigned short portReadable() const { return ntohs(port); }

    unsigned long id;
    in_addr_t ip;
    unsigned short port;

  private:
    bool isValidIpAddress(const char *ipAddress) {
      struct sockaddr_in sa;
      int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
      return result != 0;
    }

    in_addr_t ipLookup(const char *host) {
      struct addrinfo hints, *res;
      char addrstr[128];
      void *ptr;

      memset(&hints, 0, sizeof(hints));
      hints.ai_family = PF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags |= AI_CANONNAME;

      if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        throw std::runtime_error(
            "Could not resolve host `" + std::string(host) +
            "` to IP: " + std::string(std::strerror(errno)));
      }

      while (res) {
        inet_ntop(res->ai_family, res->ai_addr->sa_data, addrstr, 128);

        switch (res->ai_family) {
        case AF_INET:
          ptr =
              &(reinterpret_cast<struct sockaddr_in *>(res->ai_addr))->sin_addr;
          inet_ntop(res->ai_family, ptr, addrstr, 128);
          return inet_addr(addrstr);
          break;
        // case AF_INET6:
        //     ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
        //     break;
        default:
          break;
        }
        res = res->ai_next;
      }

      throw std::runtime_error("No host resolves to IPv4");
    }
  };

public:
  Parser(const int argc, char const *const *argv, bool withConfig = true)
      : argc{argc}, argv{argv}, withConfig{withConfig}, parsed{false} {}

  void parse() {
    if (!parseInternal()) {
      help(argc, argv);
    }

    parsed = true;
  }

  unsigned long id() const {
    checkParsed();
    return id_;
  }

  const char *hostsPath() const {
    checkParsed();
    return hostsPath_.c_str();
  }

  const char *outputPath() const {
    checkParsed();
    return outputPath_.c_str();
  }

  const char *configPath() const {
    checkParsed();
    if (!withConfig) {
      throw std::runtime_error("Parser is configure to ignore the config path");
    }

    return configPath_.c_str();
  }

  void clearOutputFile() {
    std::ofstream outputFile(outputPath(), std::ofstream::out | std::ofstream::trunc); // open and clear the file

    if (outputFile.is_open())
    {
        std::cout << "File opened and cleared successfully\n";
        outputFile.close(); // close the file
    }
    else
    {
        std::cerr << "Error opening or clearing the file\n";
    }
  }

  int message_to_send, recv_em_id;
  std::set<message_t> not_delivered_pl; // dst_id, message_buffer
  std::queue<message_t> ack_queue_pl; 

  /*
    Perfect Links application configuration
    Input path to config file, return true if success

    The config file contains two integers m i in its first line. The integers are separated by a single space character. m defines how many messages each sender process should send. i is the index of the receiver process. The receiver process only receives messages while the sender processes only send messages. All n-1 sender processes, send m messages each. Sender processes send messages 1 to m in order.
  */
  bool configPerfectLink(const std::string& config_path) {
    std::ifstream config(config_path);

    config.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
    try {
      config >> message_to_send >> recv_em_id;
    }
    catch (const std::ifstream::failure& e) {
      std::cerr << "Exception opening/reading/closing file\n";
      return false;
    }
    
    return true;
  }

  std::set<std::string> delivered_fifo; // message_buffer (because it's broadcast, "delivered" means broadcast to all dst_id done)
  std::vector<message_t> past_fifo;   // past_fifo: a vector of all past message (s, m) in order

  std::set<std::pair<int, int>> delivered_urb;  // src_id, message_k (urb not required totality, so src_id exist)
  std::set<std::pair<int, int>> pending; // set of (src_id, message_k) pairs
  std::vector<std::set<int>> ack; // ack[k]: set of process id that has acked message k
  std::map<std::string, int> m_mapping_urb; // map mes to int k

  std::vector<int> cheating_vector_d;

  /*
    FIFO Broadcast application configuration
    Input path to config file, return true if success

    The config file contains an integer m in its first line. m defines how many messages each process should broadcast. Processes broadcast messages 1 to m in order.
  */
  bool config_fifo(const std::string& config_path) {
    std::ifstream config(config_path);

    config.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
    try {
      config >> message_to_send;
    }
    catch (const std::ifstream::failure& e) {
      std::cerr << "Exception opening/reading/closing file\n";
      return false;
    }
    
    return true;
  }

  /* variable for lattice agreement */
  bool active_lattice;
  int ack_count_lattice, nack_count_lattice, active_proposal_number_lattice;
  std::set<int> proposed_value_lattice;
  std::set<int> accepted_value_lattice;
  int vs, ds;
  std::vector<std::set<int>> proposals_lattice; // proposals_lattice[k]: a set of int in k-th proposal
  std::set<int> fin_lattice; // a set of finished process id
  int round_num = 0;

  /*
    Lattice Agreement application configuration
    Input path to config file, return true if success

    The config file consists of multiple lines.
    The first line contains three integers, p vs ds (separated by single spaces). p denotes the number of proposals for each process, vs denotes the maximum number of elements in a proposal, and ds denotes the maximum number of distinct elements across all proposals of all processes.
    The subsequent p lines contain proposals. Each proposal is a set of positive integers, written as a list of integers separated by single spaces. Every line can have up to vs integers.
  */
  bool config_lattice(const std::string& config_path) {
    std::ifstream config(config_path);

    config.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
    try {
      config >> message_to_send >> vs >> ds;
      proposals_lattice.assign(message_to_send, std::set<int>());
      std::string line;
      for (int i = 0; i < message_to_send; i++) {
        if (std::getline(config >> std::ws, line)) {
          std::stringstream ss(line);
          int num;
          while (ss >> num) {
              proposals_lattice[i].insert(num);
          }
        }
      }   
    }
    catch (const std::ifstream::failure& e) {
      std::cerr << "Exception opening/reading/closing file\n";
      return false;
    }

    for (int i = 0; i < message_to_send; i++) {
        std::cout << "parser.proposals_lattice[" << i << "].size()= " << proposals_lattice[i].size() << std::endl;
    }
    
    return true;
  }

  std::chrono::system_clock::time_point start_time = std::chrono::high_resolution_clock::now(); // Record the starting time

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

  void reset_start_time()
  {
    start_time = std::chrono::high_resolution_clock::now();
  }

  std::string format_now_time_and_duration()
  {
    auto now_time = std::chrono::high_resolution_clock::now(); // Record the now time
    return format_time_point(now_time) + " " + format_duration(start_time, now_time);
  }

  void writeConsole(const char* format, ...) {
    va_list args;
    va_start(args, format);

    int len = vsnprintf(NULL, 0, format, args);
    va_end(args);

    char* output = static_cast<char*>(malloc(len + 1));

    va_start(args, format);
    vsnprintf(output, len + 1, format, args);
    va_end(args);

    std::cout << format_now_time_and_duration() << " " << output << std::endl;

    free(output);
  }

  void writeOutputFile(const char* format, ...) {
    std::ofstream outputFile(outputPath(), std::ios::app); // Open for appending

    va_list args;
    va_start(args, format);

    int len = vsnprintf(NULL, 0, format, args);
    va_end(args);

    char* output = static_cast<char*>(malloc(len + 1));

    va_start(args, format);
    vsnprintf(output, len + 1, format, args);
    va_end(args);
    
    if (outputFile.is_open()) {
      outputFile << output << std::endl;
      outputFile.close();
    }
    else {
      std::cerr << "failed to open the file for appending." << std::endl;
    }

    free(output);
  }

  std::vector<Host> hosts() {
    std::ifstream hostsFile(hostsPath());
    std::vector<Host> hosts;

    if (!hostsFile.is_open()) {
      std::ostringstream os;
      os << "`" << hostsPath() << "` does not exist.";
      throw std::invalid_argument(os.str());
    }

    std::string line;
    int lineNum = 0;
    while (std::getline(hostsFile, line)) {
      lineNum += 1;

      std::istringstream iss(line);

      trim(line);
      if (line.empty()) {
        continue;
      }

      unsigned long id;
      std::string ip;
      unsigned short port;

      if (!(iss >> id >> ip >> port)) {
        std::ostringstream os;
        os << "Parsing for `" << hostsPath() << "` failed at line " << lineNum;
        throw std::invalid_argument(os.str());
      }

      hosts.push_back(Host(id, ip, port));
    }

    if (hosts.size() < 2UL) {
      std::ostringstream os;
      os << "`" << hostsPath() << "` must contain at least two hosts";
      throw std::invalid_argument(os.str());
    }

    auto comp = [](const Host &x, const Host &y) { return x.id < y.id; };
    auto result = std::minmax_element(hosts.begin(), hosts.end(), comp);
    size_t minID = (*result.first).id;
    size_t maxID = (*result.second).id;
    if (minID != 1UL || maxID != static_cast<unsigned long>(hosts.size())) {
      std::ostringstream os;
      os << "In `" << hostsPath()
         << "` IDs of processes have to start from 1 and be compact";
      throw std::invalid_argument(os.str());
    }

    std::sort(hosts.begin(), hosts.end(),
              [](const Host &a, const Host &b) -> bool { return a.id < b.id; });

    return hosts;
  }

private:
  bool parseInternal() {
    if (!parseID()) {
      return false;
    }

    if (!parseHostPath()) {
      return false;
    }

    if (!parseOutputPath()) {
      return false;
    }

    if (!parseConfigPath()) {
      return false;
    }

    return true;
  }

  void help(const int, char const *const *argv) {
    auto configStr = "CONFIG";
    std::cerr << "Usage: " << argv[0]
              << " --id ID --hosts HOSTS --output OUTPUT";

    if (!withConfig) {
      std::cerr << "\n";
    } else {
      std::cerr << " CONFIG\n";
    }

    exit(EXIT_FAILURE);
  }

  bool parseID() {
    if (argc < 3) {
      return false;
    }

    if (std::strcmp(argv[1], "--id") == 0) {
      if (isPositiveNumber(argv[2])) {
        try {
          id_ = std::stoul(argv[2]);
        } catch (std::invalid_argument const &e) {
          return false;
        } catch (std::out_of_range const &e) {
          return false;
        }

        return true;
      }
    }

    return false;
  }

  bool parseHostPath() {
    if (argc < 5) {
      return false;
    }

    if (std::strcmp(argv[3], "--hosts") == 0) {
      hostsPath_ = std::string(argv[4]);
      return true;
    }

    return false;
  }

  bool parseOutputPath() {
    if (argc < 7) {
      return false;
    }

    if (std::strcmp(argv[5], "--output") == 0) {
      outputPath_ = std::string(argv[6]);
      return true;
    }

    return false;
  }

  bool parseConfigPath() {
    if (!withConfig) {
      return true;
    }

    if (argc < 8) {
      return false;
    }

    configPath_ = std::string(argv[7]);
    return true;
  }

  bool isPositiveNumber(const std::string &s) const {
    return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) {
                           return !std::isdigit(c);
                         }) == s.end();
  }

  void checkParsed() const {
    if (!parsed) {
      throw std::runtime_error("Invoke parse() first");
    }
  }

  void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    [](int ch) { return !std::isspace(ch); }));
  }

  void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](int ch) { return !std::isspace(ch); })
                .base(),
            s.end());
  }

  void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
  }

private:
  const int argc;
  char const *const *argv;
  bool withConfig;

  bool parsed;

  unsigned long id_;
  std::string hostsPath_;
  std::string outputPath_;
  std::string configPath_;
};
