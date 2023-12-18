#include "multithread.hpp"

pthread_mutex_t mutex;
pthread_cond_t cond;

void* send_thread(void* arg)
{
    struct complex_args* args = static_cast<struct complex_args*>(arg);
    int em_id = args->em_id_;
    Parser & parser = args->parser_;
    
    std::cout << "====================== round_num " << parser.round_num << std::endl;
    std::cout << "parser.active_lattice " << parser.active_lattice << std::endl;
    std::cout << "parser.ack_count_lattice " << parser.ack_count_lattice.size() << std::endl;
    std::cout << "parser.nack_count_lattice " << parser.nack_count_lattice.size() << std::endl;
    std::cout << "parser.active_proposal_number_lattice " << parser.active_proposal_number_lattice << std::endl;
    std::cout << "parser.proposed_value_lattice " << parser.proposed_value_lattice.size() << std::endl;

    /* for acceptor */
    std::cout << "parser.accepted_value_lattice " << parser.accepted_value_lattice.size() << std::endl;

    /* for each round */
    std::cout << "parser.fin_lattice " << parser.fin_lattice.size() << std::endl;      

        // propose once, wait until decided
        propose_lattice(em_id, parser, parser.proposals_lattice[parser.round_num]);
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
        } while (parser.active_lattice);

        // broadcast finish message
        std::string buf = format_finish_lattice(parser.round_num, em_id);
        format_round_wrapper_lattice(buf, parser.round_num);
        for (auto &host : parser.hosts()) {
            senderPerfectLinks(em_id, static_cast<int>(host.id), parser, buf);
        }
        // do {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // } while (parser.fin_lattice.size() <= parser.hosts().size() / 2);

        parser.writeConsole("round %d: %s", parser.round_num, format_plaintext_lattice(parser.fin_lattice).c_str());    

    // // Testing with urb
    // for (int k = 1;; k = 0)
    // {
    //     for (int m = 1; m <= parser.message_to_send; m++)
    //     {
    //         broadcast_urb(em_id, parser, std::to_string(m));
    //     }
    // }

    int *result = static_cast<int*>(malloc(sizeof(int)));
    *result = 0;
    return result;
}

void* recv_thread(void* arg)
{
    struct complex_args* args = static_cast<struct complex_args*>(arg);
    int em_id = args->em_id_;
    Parser & parser = args->parser_;
    
    for (;;)
    {
        receiverPerfectLinks(em_id, parser);
        if (parser.fin_lattice.size() > parser.hosts().size() / 2) break;
        // initPerfectLink(em_id, parser);
    }

    int *result = static_cast<int*>(malloc(sizeof(int)));
    *result = 0;
    return result;
}

void* rese_thread(void* arg)
{
    struct complex_args* args = static_cast<struct complex_args*>(arg);
    int em_id = args->em_id_;
    Parser & parser = args->parser_;
    
    for (;;)
    {
        resendPerfectLinks(em_id, parser);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (parser.fin_lattice.size() > parser.hosts().size() / 2) break;
        // initPerfectLink(em_id, parser);
    }

    int *result = static_cast<int*>(malloc(sizeof(int)));
    *result = 0;
    return result;
}

void thread_run(int em_id, Parser &parser)
{
    pthread_t sendThread, recvThread, reseThread;
    void* sendThread_return;
    void* recvThread_return;
    void* reseThread_return;

    // Initialize the mutex for thread synchronization
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);

    struct complex_args arg = {em_id, parser};

    while (parser.round_num < parser.message_to_send) {
        initPerfectLink(em_id, parser);
        init_lattice(em_id, parser);

        pthread_create(&sendThread, nullptr, send_thread, &arg);
        // parser.writeConsole("pthread_create sendThread");

        pthread_create(&recvThread, nullptr, recv_thread, &arg);

        pthread_create(&reseThread, nullptr, rese_thread, &arg);
        // parser.writeConsole("pthread_create recvThread");

        // Wait for the threads to finish (you can implement a termination condition)
        pthread_join(sendThread, &sendThread_return);
        parser.writeConsole("pthread_join send");

        pthread_join(recvThread, &recvThread_return);
        parser.writeConsole("pthread_join recv");

        pthread_join(reseThread, &reseThread_return);
        
        parser.round_num ++;
        // int result = *(int *)sendThread_return;
    }

    // Cleanup
    pthread_mutex_destroy(&mutex);
}