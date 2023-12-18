#include <chrono>
#include <iostream>
#include <thread>
#include <signal.h>

#include "parser.hpp"
#include "hello.h"
#include "udp.hpp"
#include "perfectlink.hpp"
#include "uniformreliablebroadcast.hpp"
#include "fifobroadcast.hpp"
#include "multithread.hpp"
#include "latticeagreement.hpp"

static void stop(int) {
  // reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // immediately stop network packet processing
  std::cout << "Immediately stopping network packet processing.\n";

  // write/flush output file if necessary
  std::cout << "Writing output.\n";

  // exit directly from signal handler
  exit(0);
}

int main(int argc, char **argv) {
  signal(SIGTERM, stop);
  signal(SIGINT, stop);

  // `true` means that a config file is required.
  // Call with `false` if no config file is necessary.
  bool requireConfig = true;

  Parser parser(argc, argv);
  parser.parse();

  // hello();
  std::cout << std::endl;

  int pid = getpid();
  int em_id = static_cast<int> (parser.id());

  std::cout << "My PID: " << pid << "\n";
  std::cout << "From a new terminal type `kill -SIGINT " << getpid() << "` or `kill -SIGTERM "
            << getpid() << "` to stop processing packets\n\n";

  std::cout << "My ID: " << em_id  << "\n\n";

  std::cout << "List of resolved hosts is:\n";
  std::cout << "==========================\n";
  auto hosts = parser.hosts();
  for (auto &host : hosts) {
    std::cout << host.id << "\n";
    std::cout << "Human-readable IP: " << host.ipReadable() << "\n";
    std::cout << "Machine-readable IP: " << host.ip << "\n";
    std::cout << "Human-readbale Port: " << host.portReadable() << "\n";
    std::cout << "Machine-readbale Port: " << host.port << "\n";
    std::cout << "\n";
  }
  std::cout << "\n";

  std::cout << "Path to output:\n";
  std::cout << "===============\n";
  std::cout << parser.outputPath() << "\n";
  parser.clearOutputFile();
  std::cout << "\n";

  const char* config_path = parser.configPath();
  std::cout << "Path to config:\n";
  std::cout << "===============\n";
  std::cout << config_path << "\n\n";

  // std::cout << "Perfect Link:\n";
  // std::cout << "===============\n";
  // std::cout << "Configure result is " << (parser.configPerfectLink(std::string(config_path)) ? "SUCCESS" : "FAIL") << std::endl;
  // std::cout << "Numbers of messages for senders: " << parser.message_to_send << std::endl;
  // std::cout << "id of the receiver: " << parser.recv_em_id << std::endl;

  // std::cout << "Uniform Reliable Broadcast:\n";
  // std::cout << "===============\n";
  // std::cout << "Configure result is " << (parser.config_fifo(std::string(config_path)) ? "SUCCESS" : "FAIL") << std::endl;
  // std::cout << "Numbers of messages to broadcast: " << parser.message_to_send << std::endl;

  std::cout << "Lattice Agreement:\n";
  std::cout << "===============\n";
  std::cout << "Configure result is " << (parser.config_lattice(std::string(config_path)) ? "SUCCESS" : "FAIL") << std::endl;
  std::cout << "Numbers of proposals: " << parser.message_to_send << std::endl;

  std::cout << "Doing some initialization...\n";
  initPerfectLink(em_id, parser);
  // init_urb(parser);
  std::cout << std::endl;

  std::cout << "Broadcasting and delivering messages...\n\n";
  thread_run(em_id, parser);
  
  std::cout << "Broadcast Finished, Sleep.\n\n";
  // After a process finishes broadcasting,
  // it waits forever for the delivery of messages.
  while (true) {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
